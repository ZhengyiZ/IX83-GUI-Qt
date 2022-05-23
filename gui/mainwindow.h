#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QUrl>
#include "base/cmdthread.h"
#include "base/klabel.h"
#include "setwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr,
               ptr_SendCommand ptr_sendCmd = nullptr,
               ptr_RegisterCallback ptr_reCb = nullptr,
               ptr_CloseInterface ptr_closeIf = nullptr);
    ~MainWindow();

    void startCmdThread();
    bool quitSymbol = false;
    bool initSymbol = true;
    bool advCmd = false;
    bool EN5symbol = false;

    bool deckSymbol = false;
    int deckNum = 1;

    int escapeDist = 0;
    int currObj = 0;  // range: 0-5
    int currDeckUnit = 0;  // range: 0-7
    int focusNearLimit[6] = {1050000, 1050000, 1050000, 1050000, 1050000}; // unit: 0.01 um
    double beforeEscape; // unit: 1 um
    double currZ; // unit: 1 um
    bool syncBeforeLock = false;

    Qt::WindowFlags windowFlags = Qt::Window|Qt::WindowTitleHint|
            Qt::WindowMinimizeButtonHint|Qt::WindowCloseButtonHint;

    void *pInterface;
    ptr_CloseInterface ptr_closeIf;

private:
    void ctlSettings(bool, bool);
    void initSequence();
    void quitProgram(bool);
    void closeEvent(QCloseEvent *event);

    void enqueueCmd(QString cmd);
    void insertCmd(QString);

    void syncWithTPC(QString);
    void processNotify(QString);
    void processCallback(QString);

    void focusMove(double);
    void stepSet(int);

signals:
    void sendRegister();
    void sendCmdOnce();
    void keepSendingCmd();
    void sendSliderMinMax(int,int);

private slots:
    // with if dialog
    void receivePointer(void*);
    void receiveQuitFromDialog(bool);

    // with cmd thread
    void receiveEmergencyQuit();
    void receiveRegisterResult(bool);
    void receiveRsp(QString rsp);
    void receiveNotify(QString notify);
    void receiveImaging(int mode);

    // with settings window
    void receiveFSPD(QString);
    void receiveSliderSettings(int,int);

    // Menu Bar
    void on_actionFocus_triggered();
    void on_actionStay_on_Top_triggered(bool checked);
    void on_actionLED_triggered(bool checked);
    void on_actionReset_winpos_triggered();

    void on_actionAbout_triggered();
    void on_actionAboutQt_triggered();
    void on_actionAbout_IX83_triggered();
    void on_actionWinPos_triggered();

    void kLabelClicked();

    // Controls
    void on_objSelection_currentIndexChanged(int index);

    void on_deckSelection_currentIndexChanged(int index);
    void on_shutterBox_clicked();
    void on_syncBox_stateChanged(int arg1);

    void on_lockBtn_clicked();
    void on_zSlider_sliderReleased();
    void on_zValue_valueChanged(double value);
    void on_coarseBox_clicked();
    void on_fineBox_clicked();
    void on_escapeBtn_clicked();

    void on_lineCmd_returnPressed();
    void on_cmdBtn_clicked();

    void on_eye100_clicked();
    void on_eye50_clicked();
    void on_eye0_clicked();

private:
    Ui::MainWindow *ui;
    CMDThread *cmdTh;
    SetWindow *setWin;
};

#endif // MAINWINDOW_H
