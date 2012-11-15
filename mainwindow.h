#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QX11EmbedContainer>
#include "qtab.h"
#include <iostream>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    State &state;
    QWidget *activeWindow;
    QWidget *adopt(QWidget *);
    void split(Qt::Orientation orientation);
private slots:
    void startApp();
    void splitVertical();
    void splitHorizontal();
    void isEmbedded();
public:
    bool eventFilter(QObject *, QEvent *);
    explicit MainWindow(State &state, QWidget *parent = 0);
    ~MainWindow();
    
private:
    Ui::MainWindow *ui;
};

class Process : public QX11EmbedContainer {
    Q_OBJECT
public:
    Process(QWidget *parent) : QX11EmbedContainer(parent) {
        setAttribute(Qt::WA_DeleteOnClose);
        connect(this, SIGNAL(clientClosed()), this, SLOT(close()));
    }
    ~Process() { std::clog << "deleting container widget\n"; }
};
#endif // MAINWINDOW_H
