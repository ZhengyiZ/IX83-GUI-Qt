#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QMessageBox>
#include "cmdthread.h"
#include "ifselection.h"
#include "verlabel.h"
#include "pollthread.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

extern Ui::MainWindow *uiself;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr,
               ptr_SendCommand ptr_sendCmd = nullptr,
               ptr_RegisterCallback ptr_reCb = nullptr,
               ptr_CloseInterface ptr_closeIf = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
    bool quitSymbol = false;
    bool initSymbol = true;
    bool firstImaging = true;
    bool advCmd = false;

    int escapeDist = 3000;
    int currObj = 0;  // range: 0-5
    int focusNearLimit[6] = {1050000, 1050000, 1050000, 1050000, 1050000}; // unit: 0.01 um
    double beforeEscape; // unit: 1 um
    double currZ; // unit: 1 um

    void *pInterface;
    ptr_CloseInterface ptr_closeIf;

private:
    void ctlSettings(bool);
    bool sendCmd(QString cmd);
    void processCallback(QString);
    bool focusMove(double);
    void insertCmdFIFO(QString);
    void quitProgram(bool);

signals:
    void sendRegister();
    void sendCmdSignal();
    void sendPolling();

private slots:
    void receivePointer(void*);
    void receiveQuitFromIFDialog(bool);
    void receiveRegisterResult(bool);
    void receiveRsp(QString rsp);
    void receiveImaging(int mode);
    void receiveEmergencyQuit();

    void on_switchObjBtn_clicked();

    void on_wfBtn_clicked();
    void on_conBtn_clicked();

    void on_fineBtn_clicked();
    void on_roughBtn_clicked();
    void on_syncBtn_clicked();
    void on_escapeBtn_clicked();

    void on_zSlider_sliderReleased();
    void on_zValue_valueChanged(double value);

    void on_maxLine_returnPressed();
    void on_sliderSetBtn_clicked();
    void on_speedSetBtn_clicked();

    void on_lineCmd_returnPressed();
    void on_cmdBtn_clicked();

    void on_eye100_clicked();
    void on_eye50_clicked();
    void on_eye0_clicked();

private:
    Ui::MainWindow *ui;
    CMDThread *cmdTh;
    pollThread *pollTh;
};

#endif // MAINWINDOW_H
