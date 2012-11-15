#include "json.h"
#include <QMessageBox>
#include <QtGui/QApplication>
#include <QAction>
#include <QtGui/QMessageBox>
#include "mainwindow.h"
#include "qtab.h"
#include <fstream>
#include <functional>

using namespace std::placeholders;
using namespace std;

class MyApp : public QApplication {
public:
    bool x11EventFilter(XEvent *ev);
    MyApp(int &argc, char **argv) : QApplication(argc, argv) {}
};

static App *
parseApp(istream &l)
{
    App *app = new App();
    JSON::parseObject(l, [&l, app] (istream &l, std::string idx) -> void {
        std::clog << "parse field " << idx << " in app " << app << std::endl;
        if (idx == "executable")
            app->executable = JSON::parseString(l);
        else if (idx == "args")
            JSON::parseArray(l, [app] (istream &l) -> void { app->args.push_back(JSON::parseString(l)); });
        else if (idx == "name")
            app->name = JSON::parseString(l);
        else {
            QMessageBox::warning(0, QObject::tr("unknown field in ap"), QString(idx.c_str()), QMessageBox::Ok);
            JSON::parseValue(l);
        }
    });
    app->init();
    return app;
}

void
parseState(istream &l, State &state, std::string idx)
{
    if (idx == "apps")
        JSON::parseArray(l, [&state] (istream &l) -> void { state.apps.push_back(parseApp(l)); });
}

int main(int argc, char *argv[])
{
    MyApp a(argc, argv);
    State state;
    fstream in;
    in.open("/home/peadar/.qtabrc", ios_base::in);
    try {
        JSON::parseObject(in, bind(parseState, _1, std::ref(state), _2));
    }
    catch (const exception &ex) {
        QMessageBox::warning(0, QObject::tr("Failed to read configuration file"), ex.what(), QMessageBox::Ok);
    }
    std::clog << "have " << state.apps.size() << " apps" << std::endl;
    MainWindow w(state);
    w.show();
    return a.exec();
}

void
App::init()
{
    std::clog << " init app " << name << std::endl;
    startAction = new QAction(QString::fromStdString(name), 0);
    connect(startAction, SIGNAL(triggered()), this, SIGNAL(start()));
}

#include <X11/Xlib.h>
bool
MyApp::x11EventFilter(XEvent *)
{
    // clog << ev->type << "/" << ev->xany.window << "\n";
    return false;
}
