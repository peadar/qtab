#ifndef QTAB_H
#define QTAB_H

#include <vector>
#include <QAction>

class App : public QObject {

    Q_OBJECT
public:

    QAction *startAction;
    std::string executable;
    std::string name;
    std::vector<std::string> args;
    App(QObject *parent_ = 0) : QObject(parent_), startAction() { }
    ~App() { delete startAction; }
    void init();
signals:
    void start();
};

struct State {
    typedef std::vector<App *> Apps;
    Apps apps;
};

#endif // QTAB_H
