#include "cmdthread.h"
#include <QDebug>

CMDThread::CMDThread(QObject *parent,
                     ptr_SendCommand ptr_sendCmd,
                     ptr_RegisterCallback ptr_reCb) :
    QThread(parent)
{
    this->ptr_sendCmd = ptr_sendCmd;
    this->ptr_reCb = ptr_reCb;
}

//	COMMAND: call back entry from SDK port manager
int CALLBACK CommandCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                             PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);

    CMDThread *thread = (CMDThread *)pContext;

    thread->busy = false;

    // extract string from struct
    QString rsp = QString(QLatin1String((char *)thread->m_Cmd.m_Rsp));
    rsp.remove("\r\n", Qt::CaseInsensitive);
    qDebug() << " > " << rsp << Qt::endl;
    emit thread->sendRsp(rsp);

    return 0;
}

//	NOTIFICATION: call back entry from SDK port manager
int	CALLBACK NotifyCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                            PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);

    char *notify = (char *)pv;
    CMDThread *thread = (CMDThread *)pContext;

    // extract string from struct
    QString rsp(notify);
    rsp.remove("\r\n", Qt::CaseInsensitive);
    qDebug() << "Notify > " << rsp << Qt::endl;
    emit thread->sendRsp(rsp);

    return 0;

}

//	ERROR NOTIFICATON: call back entry from SDK port manager
int	CALLBACK ErrorCallback(ULONG MsgId, ULONG wParam, ULONG lParam,
                           PVOID pv, PVOID pContext, PVOID pCaller)
{
    UNREFERENCED_PARAMETER(MsgId);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(pContext);
    UNREFERENCED_PARAMETER(pCaller);
    UNREFERENCED_PARAMETER(pv);

    return 0;
}

// send command function
bool CMDThread::sendStrCmd(QString cmd)
{
    qDebug() << " < " << cmd << Qt::endl;

    // command initiate
    int	len;
    memset(&m_Cmd, 0x00, sizeof(MDK_MSL_CMD));

    // add a new line to command string
    cmd.append("\r\n");
    len = cmd.length();

    // QString to BYTE( unsigned char* )
    QByteArray tmp = cmd.toLatin1();

    // write to memory
    memcpy(m_Cmd.m_Cmd, tmp.data(), len);

    // set parameters for command
    // copy from sample source code
    m_Cmd.m_CmdSize		= 0;
    m_Cmd.m_Callback	= (void *)CommandCallback;
    m_Cmd.m_Context		= NULL;		// this pointer passed by pv
    m_Cmd.m_Timeout		= 10000;	// (ms)
    m_Cmd.m_Sync		= FALSE;
    m_Cmd.m_Command		= TRUE;		// TRUE: Command , FALSE: it means QUERY form ('?').

    // send command
    bool result = this->ptr_sendCmd(this->pInterface, &m_Cmd);

    return result;
}

void CMDThread::run()
{
    // register callback functions
    bool result = this->ptr_reCb(this->pInterface, CommandCallback,
                                 NotifyCallback, ErrorCallback, this);
//    result = true; //cheat code
    emit sendRegisterResult(result);
    exec();
}

CMDThread::~CMDThread()
{

}

void CMDThread::receiveCmd()
{
    while(!cmdFIFO.isEmpty())
    {
        if (!busy)
        {
            sendStrCmd(cmdFIFO.dequeue());
            busy = true;
        }
        else
        {
            // wait without blocking the thread
            QEventLoop loop;
            QTimer::singleShot(100, &loop, SLOT(quit()));  // time unit: msec
            loop.exec();
        }
    }
    return;
}

void CMDThread::receiveRegister()
{
    // register callback functions
    bool result = this->ptr_reCb(this->pInterface, CommandCallback,
                                 NotifyCallback, ErrorCallback, this);
    emit sendRegisterResult(result);
}
