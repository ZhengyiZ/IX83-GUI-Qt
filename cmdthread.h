#ifndef CMDTHREAD_H
#define CMDTHREAD_H

#include <QThread>
#include <QQueue>
#include <QDebug>
#include <QStandardPaths>
#include <QFile>
#include <QEventLoop>
#include <QTimer>
#include "DLL.h"

// for friend functions called by Olympus DLL
extern bool emergencyStop;

class CMDThread : public QThread
{
    Q_OBJECT
public:
    explicit CMDThread(QObject *parent = nullptr,
                       ptr_SendCommand ptr_sendCmd = nullptr,
                       ptr_RegisterCallback ptr_reCb = nullptr);

    ~CMDThread();

    void *pInterface;
    ptr_SendCommand ptr_sendCmd;
    ptr_RegisterCallback ptr_reCb;
    MDK_MSL_CMD	m_Cmd;

    bool quitSymbol = false;
    bool busy = false;
    bool syncSymbol = false;

    QString userDir;

    QQueue<QString> cmdFIFO;

    friend int CALLBACK CommandCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);
    friend int CALLBACK NotifyCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);
    friend int CALLBACK ErrorCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);

protected:
    void run();
    bool sendCmdToSDK(QString cmd);

private slots:
    void receiveRegister();
    void keepSendingCmd();
    void sendCmdOnce();

signals:
    void sendRegisterResult(bool result);
    void sendNotify(QString);
    void sendRsp(QString rsp);
    void sendEmergencyQuit();
    void sendImaging(int mode);

private:

};

#endif // CMDTHREAD_H
