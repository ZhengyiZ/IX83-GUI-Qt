#ifndef CMDTHREAD_H
#define CMDTHREAD_H

#include <QThread>
#include <QQueue>
#include <QEventLoop>
#include <QTimer>
#include <cmd.h>
#include <DLL.h>

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

    bool quitCmd = false;
    bool busy = false;

    QQueue<QString> cmdFIFO;

    friend int CALLBACK CommandCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);
    friend int CALLBACK NotifyCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);
    friend int CALLBACK ErrorCallback(ULONG MsgId, ULONG wParam, ULONG lParam, PVOID pv, PVOID pContext, PVOID pCaller);

protected:
    void run();
    bool sendStrCmd(QString cmd);
    void sendCmdFIFO();

private slots:
    void receiveRegister();
    void receiveCmd();

signals:
    void sendRegisterResult(bool result);
    void sendRsp(QString rsp);

private:

};

#endif // CMDTHREAD_H
