#ifndef POLLTHREAD_H
#define POLLTHREAD_H

#include <QThread>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QEventLoop>
#include <QStandardPaths>

class pollThread : public QThread
{
    Q_OBJECT
public:
    explicit pollThread(QObject *parent = nullptr);

    ~pollThread();

    QString userDir;
    bool quitSymbol = false;

protected:
    void run();

signals:
    void sendImaging(int mode);

private slots:
    void polling();

};

#endif // POLLTHREAD_H
