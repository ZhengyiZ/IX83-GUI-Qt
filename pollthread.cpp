#include "pollthread.h"

pollThread::pollThread(QObject *parent)
{

}

void pollThread::run()
{
    userDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    exec();
}

pollThread::~pollThread()
{

}

void pollThread::polling()
{
//    int count = 0;
    while(!quitSymbol)
    {
//        count++;
        QFile wf(userDir + "/.MINFLUX/wf.txt");
        QFile con(userDir + "/.MINFLUX/conf.txt");
        if (wf.exists())
        {
            qDebug() << "pollThread > wf" << Qt::endl;
            wf.remove();
            emit sendImaging(0);
        }
        else if (con.exists())
        {
            qDebug() << "pollThread > conf" << Qt::endl;
            con.remove();
            emit sendImaging(1);
        }
//        else
//            qDebug() << "pollThread > no file exist" << count << Qt::endl;
        QEventLoop loop2;
        QTimer::singleShot(500, &loop2, SLOT(quit()));
        loop2.exec();
    }
}
