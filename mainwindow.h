#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QLabel>
#include <QMessageBox>
#include "cmdthread.h"
#include "ifselection.h"

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

    int escapeDist = 3000;
    bool logStatus = false;
    int currObj = 0;  // range: 0-5
    int focusNearLimit[6] = {1050000, 1050000, 1050000, 1050000, 1050000}; // unit: 0.01um
    double beforeEscape; // unit: 1um
    double currZ; // unit: 1um

    void *pInterface;
    ptr_CloseInterface ptr_closeIf;

private:
    void ctlSettings(bool);
    void processCallback(QString);
    void focusMove();

signals:
    void sendRegister();
    void sendCmd(QString cmd);

private slots:
    void receivePointer(void*);
    void receiveQuit(bool);
    void receiveRegisterResult(bool);
    void receiveRsp(QString rsp);

    void on_switchObjBtn_clicked();

    void on_wfBtn_clicked();
    void on_conBtn_clicked();

    void on_fineBtn_clicked();
    void on_escapeBtn_clicked();

    void on_sliderSetBtn_clicked();
    void on_speedSetBtn_clicked();

    void on_zSlider_sliderReleased();

    void on_zValue_valueChanged(double value);

    void on_pushButton_clicked();

private:
    Ui::MainWindow *ui;
    CMDThread *thread;
};

#endif // MAINWINDOW_H
