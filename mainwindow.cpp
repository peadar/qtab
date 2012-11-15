#include <errno.h>
#include <stdio.h>
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "x11helper.h"
#include "json.h"
#include <QKeyEvent>
#include <QMessageBox>
#include <QShortcut>
#include <QEvent>
#include <QX11Info>

bool
MainWindow::eventFilter(QObject *object, QEvent *event)
{

    switch (event->type()) {
    case QEvent::FocusIn: {
        std::clog << "focus in event on " << object << "\n";
        QWidget *widget = qobject_cast<QWidget *>(object);
        activeWindow = widget;
        break;
    }
    case QEvent::KeyPress: {
        QKeyEvent *ev = static_cast<QKeyEvent *>(event);
        std::clog << "keypress event on " << object << ": " << ev->key() << "\n";
        return false;
    }

    case QEvent::Shortcut: {
        QShortcutEvent *ev = static_cast<QShortcutEvent *>(event);
        std::clog << "shortcut " << ev->shortcutId() << "\n";
    }
    default:
        return false;
    }
    return false;
}

QWidget *
MainWindow::adopt(QWidget *widget)
{
    const char *colors[] = {
        "red",
        "green",
        "blue",
        "cyan",
        "magenta",
        "yellow",
        "black",
        "white"
    };
    QString style = "QWidget { background-color: ";
    const char *color = colors[random() % (sizeof colors / sizeof colors[0])];
    style += color;
    style += "; }";

    widget->installEventFilter(this);
    widget->setStyleSheet(style);
    widget->setLayout(new QBoxLayout(QBoxLayout::TopToBottom));
    widget->setFocusPolicy(Qt::ClickFocus);
    std::clog << "adopted window " << widget << " with color " << color << "\n";
    return widget;
}

MainWindow::MainWindow(State &s_, QWidget *parent) :
    QMainWindow(parent),
    state(s_),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);   
    adopt(ui->appCanvas);
    installEventFilter(this);

    X11Helper::doit(QX11Info::display(), ui->appCanvas->winId());

    activeWindow = ui->appCanvas;
    QMenu *appsMenu = ui->menuApps;
    for (std::vector<App *>::iterator apps = state.apps.begin(); apps != state.apps.end(); ++apps)
        appsMenu->addAction((*apps)->startAction);
    // Add listeners for each app.
    for (State::Apps::iterator it = state.apps.begin(); it != state.apps.end(); ++it)
        connect(*it, SIGNAL(start()), this, SLOT(startApp()));

    connect(ui->actionSplit_Vertically, SIGNAL(triggered()), this, SLOT(splitVertical()));
    connect(ui->actionSplit_Horizontally, SIGNAL(triggered()), this, SLOT(splitHorizontal()));
}

void
MainWindow::splitVertical()
{
    split(Qt::Vertical);
}

void
MainWindow::splitHorizontal()
{
    split(Qt::Horizontal);
}

void
MainWindow::split(Qt::Orientation orientation)
{
    QSplitter *splitter = new QSplitter(orientation);
    splitter->addWidget(adopt(new QWidget()));
    splitter->addWidget(adopt(new QWidget()));

    activeWindow->layout()->addWidget(splitter);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void
MainWindow::isEmbedded()
{
    std::clog << "embedded window in " << sender() << "\n";
}

void
MainWindow::startApp()
{
    App *app = qobject_cast<App *>(sender());
    std::clog << app;
    QX11EmbedContainer *container = new Process(0);

    activeWindow->layout()->addWidget(container);
    adopt(container);
    connect(container, SIGNAL(clientIsEmbedded()), SLOT(isEmbedded()));
    container->show();



    const char *args[app->args.size() + 1];
    char buf[64];
    size_t i;
    for (i = 0; i < app->args.size(); ++i) {
        const char *p = (char *)app->args[i].c_str();
        if (strcmp(p, "%winid%") == 0) {
            snprintf(buf, sizeof buf, "%lu", container->winId());
            p = buf;
        }
        args[i] = p;
    }
    args[i] = 0;

    std::clog << "forking...\n";
    switch (fork()) {
    case 0:
        execv(app->executable.c_str(), (char *const *) args);
        _exit(-1);
    default:
        break;
    case -1:
        QMessageBox::warning(this, tr("Cannot launch Application"), tr("fork failed") + QString::fromAscii(strerror(errno)));
        break;
    }
    activeWindow->layout()->update();
}
