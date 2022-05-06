#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent,
                       ptr_SendCommand ptr_sendCmd,
                       ptr_RegisterCallback ptr_reCb,
                       ptr_CloseInterface ptr_closeIf):
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // set mainwindow
    setWindowTitle("Olympus IX83 Control");
    ui->actionStay_on_Top->setChecked(true);
    on_actionStay_on_Top_triggered(true);

    // set menu font
    QFont menuFt("Microsoft YaHei",9);
    menuBar()->setFont(menuFt);

    // create the settings window
    setWin = new SetWindow();
    connect(setWin, SIGNAL(sendFSPD(QString)), this, SLOT(receiveFSPD(QString)));
    connect(setWin, SIGNAL(sendSliderSettings(int,int)), this, SLOT(receiveSliderSettings(int,int)));
    connect(this, SIGNAL(sendSliderMinMax(int,int)), setWin, SLOT(receiveSliderMinMax(int,int)));

    // set clickable label
    kLabel *KLAB = new kLabel("K-LAB", this);
    QFont ft("Fredericka the Great", 16);
    KLAB->setFont(ft);
    KLAB->setCursor(Qt::PointingHandCursor);
    ui->verticalLayout_4->insertWidget(0, KLAB, 0, Qt::AlignCenter | Qt::AlignVCenter);

    // pass parameters
    this->pInterface = nullptr;
    this->ptr_closeIf = ptr_closeIf;

    // create a command thread
    cmdTh = new CMDThread(nullptr, ptr_sendCmd, ptr_reCb);

    // emit signal to cmd thread
    connect(this, SIGNAL(sendCmdOnce()),
            cmdTh, SLOT(sendCmdOnce()), Qt::QueuedConnection);
    connect(this, SIGNAL(keepSendingCmd()),
            cmdTh, SLOT(keepSendingCmd()), Qt::QueuedConnection);

    // receive response and notify from cmd thread
    connect(cmdTh, SIGNAL(sendRsp(QString)),
            this, SLOT(receiveRsp(QString)), Qt::QueuedConnection);
    connect(cmdTh, SIGNAL(sendNotify(QString)),
            this, SLOT(receiveNotify(QString)), Qt::QueuedConnection);
    connect(cmdTh, SIGNAL(sendImaging(int)),
            this, SLOT(receiveImaging(int)), Qt::QueuedConnection);

    // to draw QMessageBox for non-GUI thread
    connect(this, SIGNAL(sendRegister()), cmdTh, SLOT(receiveRegister()));
    connect(cmdTh, SIGNAL(sendRegisterResult(bool)), this, SLOT(receiveRegisterResult(bool)));
    connect(cmdTh, SIGNAL(sendEmergencyQuit()), this, SLOT(receiveEmergencyQuit()));

    // disable all the controls
    ui->lineCmd->setVisible(0);
    ui->lineRsp->setVisible(0);
    ctlSettings(false);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::startCmdThread()
{
    cmdTh->quitSymbol = false;
    cmdTh->start();
    // according to the connected parts, run the initialization sequence
    enqueueCmd("U?");
    emit keepSendingCmd();
}

void MainWindow::quitProgram(bool login)
{
    ui->syncBox->setChecked(false);
    cmdTh->quitSymbol = true;
    cmdTh->cmdFIFO.clear();

    if (login) // depends on login or not
    {
        enqueueCmd("L 0,0");
        emit sendCmdOnce();
    }
    else
        receiveRsp("quit");
}

// applicatioin exit inquiry
void MainWindow::closeEvent (QCloseEvent *e)
{
    if(quitSymbol)
    {
        cmdTh->quit();
        cmdTh->wait();
        e->accept();
        qApp->quit();
    }
    else
    {
        if( QMessageBox::question(this,
                                  "Quit",
                                  "Are you sure to quit this application?",
                                  QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes )
                == QMessageBox::Yes)
        {
            quitProgram(true);
            ui->statusbar->showMessage("Logging out from microscope, which will be fast.");
        }
        e->ignore();
    }
}

// WARNING: may cause stack overflow or other problems
// insert cmd FIFO only if necessary
void MainWindow::insertCmd(QString cmd)
{
    cmdTh->quitSymbol = true;
    QQueue tmp = cmdTh->cmdFIFO;
    cmdTh->cmdFIFO.clear();
    cmdTh->cmdFIFO.enqueue(cmd);
    while (!tmp.isEmpty())
        cmdTh->cmdFIFO.enqueue(tmp.dequeue());
    cmdTh->quitSymbol = false;
    Sleep(200);
    emit keepSendingCmd();
}

// enqueue cmd FIFO, recommended
void MainWindow::enqueueCmd(QString cmd)
{
    cmdTh->cmdFIFO.enqueue(cmd);
    ui->lineCmd->setText(cmd);
    ui->lineRsp->setText("");
}

// set all the controls, like EN5 0
void MainWindow::ctlSettings(bool a)
{
    ui->objSelection->setEnabled(a);

    ui->eyeBox->setEnabled(a);

    ui->cmdBtn->setEnabled(a);
    ui->lineCmd->setEnabled(a);
    ui->lineRsp->setEnabled(a);

    ui->zValue->setEnabled(a);
    ui->zSlider->setEnabled(a);
    ui->coarseBox->setEnabled(a);
    ui->fineBox->setEnabled(a);
    ui->escapeBtn->setEnabled(a);

    if (!deckSymbol)
        a = false;

    ui->deckSelection->setEnabled(a);
    ui->syncBox->setEnabled(a);
    ui->shutterBox->setEnabled(a);
}

void MainWindow::initSequence()
{
    ui->statusbar->showMessage("Executing initialization sequence, please wait a few seconds.");

    // 1. log into remote mode
    enqueueCmd("L 1,0");
    enqueueCmd("DG 1,1"); // open dialog on TPC

    // 2. get objective lenses information
    for (int i=1; i<7; i++)
        enqueueCmd("GOB " + QString::number(i));

    // 3. get deck information
    if (deckSymbol)
        for (int i=1; i<9; i++)
            enqueueCmd("GMU" + QString::number(deckNum) + " " + QString::number(i));

    // 4. check focus near limit
    enqueueCmd("NL?");

    // 5. check the escape distance
    enqueueCmd("ESC2?");

    // 6. enable TPC button
    enqueueCmd("SKD 151,1");  // objective
    enqueueCmd("SKD 152,1");
    enqueueCmd("SKD 153,1");
    enqueueCmd("SKD 154,1");
    enqueueCmd("SKD 155,1");
    enqueueCmd("SKD 156,1");
    if (deckSymbol)
    {
        if (deckNum == 1)
        {
            enqueueCmd("SKD 261,1");  // deck 1
            enqueueCmd("SKD 266,1");
            enqueueCmd("SKD 267,1");
            enqueueCmd("SKD 268,1");
            enqueueCmd("SKD 269,1");
            enqueueCmd("SKD 270,1");
            enqueueCmd("SKD 271,1");
            enqueueCmd("SKD 272,1");
            enqueueCmd("SKD 273,1");
            enqueueCmd("ESH1?");
        }
        else
        {
            enqueueCmd("SKD 301,1");  // deck 2
            enqueueCmd("SKD 306,1");
            enqueueCmd("SKD 307,1");
            enqueueCmd("SKD 308,1");
            enqueueCmd("SKD 309,1");
            enqueueCmd("SKD 310,1");
            enqueueCmd("SKD 311,1");
            enqueueCmd("SKD 312,1");
            enqueueCmd("SKD 313,1");
            enqueueCmd("ESH2?");
        }
    }
    enqueueCmd("SKD 354,1"); // escape

    // 7. check focus position, current obj and eyepiece ratio
    enqueueCmd("FP?");
    enqueueCmd("OB?");
    enqueueCmd("BIL?");

    // 8. active notifications
    enqueueCmd("N 1");
    enqueueCmd("O 1");
    enqueueCmd("SK 1");
    enqueueCmd("NOB 1");
    enqueueCmd("NFP 1");

    // 9. into operation mode
    enqueueCmd("EN5 1");
    enqueueCmd("DG 0,1"); // hide dialog
    enqueueCmd("OPE 0");

    // 10. set imaging mode to wide field
//    if (deckSymbol)
//        if (deckNum == 2)
//        {
//            enqueueCmd("MU2 5");
//            enqueueCmd("ESH2 0");
//        }
}

// receive slider settings from the settings windows
void MainWindow::receiveSliderSettings(int min, int max)
{
    focusNearLimit[currObj] = max*100;
    QString cmd = "NL ";
    for(int i=0; i<6; i++)
    {
        cmd.append(QString::number(focusNearLimit[i]));
        if (i != 5)
            cmd.append(",");
    }

    ui->statusbar->showMessage("Setting focus near limit...", 3000);
    enqueueCmd(cmd);

    ui->zSlider->setMaximum(max);
    ui->zValue->setMaximum((double)max);
    ui->zSlider->setMinimum(min);
    ui->zValue->setMinimum((double)min);
}

// receive FSPD from the settings windows
void MainWindow::receiveFSPD(QString FSPDcmd)
{
    ui->statusbar->showMessage("Setting focus unit speeds...", 3000);
    enqueueCmd(FSPDcmd);
}

void MainWindow::receiveQuitFromDialog(bool symbol)
{
    this->quitSymbol = symbol;
    return;
}

// receive the pointer of interface address
void MainWindow::receivePointer(void *pInterface_new)
{
    pInterface = pInterface_new;
    cmdTh->pInterface = pInterface_new;
}

// receive register result from cmd thread
void MainWindow::receiveRegisterResult(bool result)
{
    if (!result)
        {
            QMessageBox::StandardButton result =
                    QMessageBox::critical(NULL, "Failed to Register Callback",
                                          "Unkown reason!\n"
                                          "Maybe there is no Interface connected.",
                                          QMessageBox::Retry|QMessageBox::Cancel, QMessageBox::Retry);
            switch (result)
            {
            case QMessageBox::Cancel:
                quitProgram(false);
                return;
            default:
                emit sendRegister();
            }
        }
}

// receive emergency quit from cmd thread
// in case that TPC is not powered on
void MainWindow::receiveEmergencyQuit()
{
    if( QMessageBox::critical(this, "TPC connection error",
                             "TPC should be powered on before running this program.\n"
                             "Please check the power of TPC and the connection between CBH and TPC.",
                             QMessageBox::Retry|QMessageBox::Cancel, QMessageBox::Cancel)
            == QMessageBox::Retry)
        insertCmd("U?");
    else
        quitProgram(false);
}

// receive imaging mode from cmd thread
// to sync with external programs, such as LabVIEW
void MainWindow::receiveImaging(int mode)
{
    switch(mode)
    {
    case 0:
        if (ui->deckSelection->currentIndex() != 4 && ui->shutterBox->isChecked())
        {
            ui->statusbar->showMessage("Switching imaging mode to Wide Field...", 3000);
            enqueueCmd("EN5 0");
            EN5symbol = false;
            enqueueCmd("MU2 5");
            enqueueCmd("ESH2 0");
            enqueueCmd("EN5 1");
        }
        else if (ui->deckSelection->currentIndex() != 4)
            ui->deckSelection->setCurrentIndex(4); // move to 5
        else if (ui->shutterBox->isChecked())
        {
            ui->shutterBox->setChecked(false); // open shutter
            on_shutterBox_clicked();
        }
        break;
    case 1:
        if (ui->deckSelection->currentIndex() != 3 && !ui->shutterBox->isChecked())
        {
            ui->statusbar->showMessage("Switching imaging mode to Confocal...", 3000);
            enqueueCmd("EN5 0");
            EN5symbol = false;
            enqueueCmd("MU2 4");
            enqueueCmd("ESH2 1");
            enqueueCmd("EN5 1");
        }
        else if (ui->deckSelection->currentIndex() != 3)
            ui->deckSelection->setCurrentIndex(3); // move to 4
        else if (!ui->shutterBox->isChecked())
        {
            ui->shutterBox->setChecked(true); // close shutter
            on_shutterBox_clicked();
        }
        break;
    }
}

// receive response from cmd thread
void MainWindow::receiveRsp(QString rsp)
{

    ui->lineRsp->setText(rsp);
    processCallback(rsp);

    if (cmdTh->quitSymbol)
    {
        // close interface
        this->ptr_closeIf(pInterface);
        quitSymbol = true;
        qApp->quit(); // closeEvent
        return;
    }

}

// receive notify from cmd thread
void MainWindow::receiveNotify(QString notify)
{
    if (notify.contains("SK ", Qt::CaseSensitive))
        syncWithTPC(notify);
    else
        processNotify(notify);
}

// process callbacks
void MainWindow::processCallback(QString str)
{
    // objective lens information
    if (str.contains("GOB", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get objective lens information.", 3000);
        else
        {
            QString indexStr = str.section(",",0,0).right(1);
            QString nameStr = str.section(",",1,1);
            QString NAStr = str.section(",",2,2);

            QString magStr = str.section(",",3,3);
            int mediumIndex = str.section(",",4,4).toInt();
            QString mediumStr = "Unknown";
            switch(mediumIndex)
            {
            case 1:
                mediumStr = "Dry";
                break;
            case 2:
                mediumStr = "Water";
                break;
            case 3:
                mediumStr = "Oil";
                break;
            case 4:
                mediumStr = "Sili-Oil";
                break;
            }

            QString objInfo;
            if( nameStr.contains("NONE", Qt::CaseInsensitive) )
                objInfo = indexStr + ": NONE";
            else
                objInfo = indexStr + ": " + magStr + "X NA=" + NAStr + " " + mediumStr;
            ui->objSelection->blockSignals(true);
            ui->objSelection->addItem(objInfo);
            ui->objSelection->blockSignals(false);
        }
    }

    // deck unit information
    else if (str.contains("GMU", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get deck units information.", 3000);
        else
        {
            QString indexStr = str.section(",",0,0).right(1);
            QString nameStr = str.section(",",3,3);

            QString deckUnitInfo;
            if( nameStr.contains("NONE", Qt::CaseInsensitive) )
                deckUnitInfo = indexStr + ": NONE";
            else
                deckUnitInfo = indexStr + ": " + nameStr;
            ui->deckSelection->blockSignals(true);
            ui->deckSelection->addItem(deckUnitInfo);
            ui->deckSelection->blockSignals(false);
        }

    }

    // switch objective lens
    else if (str.contains("OBSEQ", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch objective lens.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Switching objective lens complete.", 3000);
    }

    else if (str.contains("OB", Qt::CaseSensitive))
    {
        if (str.contains("+"))
        {
            // in case "NOB +"
        }
        else if (str.contains(" "))
        {
            currObj = str.right(1).toInt()-1;
            QString after = QString::number(currObj+1);
            ui->objSelection->blockSignals(true);
            ui->objSelection->setCurrentIndex(currObj);
            ui->objSelection->blockSignals(false);

            enqueueCmd("SKD 15" + after + ",2");

            double max = focusNearLimit[currObj]/100;
            ui->zValue->blockSignals(true);
            ui->zValue->setMaximum(max);
            ui->zValue->blockSignals(false);

            ui->zSlider->setMaximum((int)max);
        }
    }

    // operation mode
    else if (str.contains("OPE", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to change idle/setting mode.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
        {
            if (initSymbol)
            {
                ui->statusbar->showMessage("Initialization complete, ready to work.", 3000);
                initSymbol = false;
                ctlSettings(true);
//                ui->syncBox->setChecked(true);
            }
        }
    }

    // eyepiece ratio
    else if (str.contains("BIL"))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch eyepiece ratio.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Switching eyepiece ratio complete.", 3000);
        else if (str.contains(" ", Qt::CaseSensitive))
        {
            switch (str.right(1).toInt())
            {
            case 1:
                ui->eye0->setChecked(true);
                break;
            case 2:
                ui->eye50->setChecked(true);
                break;
            case 3:
                ui->eye100->setChecked(true);
                break;
            }
        }
    }

    // epi shutter
    else if (str.contains("ESH", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch shutter.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Switching shutter complete.", 3000);
        else if (str.contains(" ", Qt::CaseSensitive))
        {
            int state = str.right(1).toInt();
            if (state == 0)
            {
                ui->shutterBox->setChecked(false);
                if (deckNum == 1)
                    enqueueCmd("SKD 261,1");
                else
                    enqueueCmd("SKD 301,1");
            }
            else
            {
                ui->shutterBox->setChecked(true);
                if (deckNum == 1)
                    enqueueCmd("SKD 261,2");
                else
                    enqueueCmd("SKD 301,2");
            }
        }
    }

    // focus near limit
    else if (str.contains("NL", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get focus near limit.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Setting focus near limit complete.", 3000);
        else
        {
            str.replace("NL ", "");
            for(int i=0; i<6; i++)
                focusNearLimit[i] = str.section(",",i,i).toInt();

            double max = focusNearLimit[currObj]/100;

            ui->zValue->blockSignals(true);
            ui->zValue->setMaximum(max);
            ui->zValue->blockSignals(false);

            ui->zSlider->setMaximum((int)max);
        }
    }

    // move focus position
    else if (str.contains("FG", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to move focus.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Moving focus complete.", 3000);
    }

    else if (str.contains("FP", Qt::CaseSensitive))
    {
        if (str.contains(" ", Qt::CaseSensitive))
        {
            str.remove("FP ");
            currZ = str.toDouble() / 100;
            ui->zSlider->setValue((int)currZ);

            ui->zValue->blockSignals(true);
            ui->zValue->setValue(currZ);
            ui->zValue->blockSignals(false);
        }
    }

    // focusing unit speed
    else if (str.contains("FSPD", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to set focusing unit speed.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Setting focusing unit speed complete.", 3000);
    }

    else if (str.contains("ESC2", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get escape distance.", 3000);
        else if (str.contains(" ", Qt::CaseSensitive))
            escapeDist = str.right(1).toInt() * 1000;
    }

    // log in
    else if (str.contains("L", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
        {
            if (initSymbol)
            {
                if( QMessageBox::critical(this, "Login error",
                                         "Error code: " + str + "\n" +
                                         "TPC should be at the MENU screen with a Start Operation button.\n"
                                         "Please set TPC to the MENU screen, then press Retry.",
                                         QMessageBox::Retry|QMessageBox::Cancel, QMessageBox::Cancel)
                        == QMessageBox::Retry)
                    insertCmd("L 1,0");
                else
                    quitProgram(false);
            }
            else
                ui->statusbar->showMessage("Failed to log in.", 3000);
        }
    }

    // get the connected units
    else if (str.contains("U ", Qt::CaseSensitive))
    {
        if ( str.contains("RFACA.1", Qt::CaseSensitive) &&
                str.contains("RFACA.2", Qt::CaseSensitive) )
            insertCmd("MDC?");
        else if (str.contains("RFACA.1", Qt::CaseSensitive))
        {
            deckNum = 1;
            ui->deckLabel->setText("Deck 1");
            deckSymbol = true;
            initSequence();
        }
        else if (str.contains("RFACA.2", Qt::CaseSensitive))
        {
            deckNum = 2;
            ui->deckLabel->setText("Deck 2");
            deckSymbol = true;
            initSequence();
        }
    }

    // get the main deck
    else if (str.contains("MDC", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->deckLabel->setText("Deck Unavailable");
        else
        {
            deckNum = str.right(1).toInt();
            deckSymbol = true;
            if (deckNum == 1)
                ui->deckLabel->setText("Deck 1");
            else if (deckNum == 2)
                ui->deckLabel->setText("Deck 2");
            initSequence();
        }
    }

    // en/disable TPC operation
    else if (str.contains("EN5 +", Qt::CaseSensitive))
    {
        ctlSettings(EN5symbol);
        EN5symbol = !EN5symbol;
    }
}

// process the operation on TPC
void MainWindow::syncWithTPC(QString notify)
{
    // objective
    if (notify.contains("SK 151,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(0);
    else if (notify.contains("SK 152,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(1);
    else if (notify.contains("SK 153,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(2);
    else if (notify.contains("SK 154,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(3);
    else if (notify.contains("SK 155,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(4);
    else if (notify.contains("SK 156,1", Qt::CaseSensitive))
        ui->objSelection->setCurrentIndex(5);

    // shutter
    else if (notify.contains("SK 261,1", Qt::CaseSensitive))
    {
        if (ui->shutterBox->isChecked())
            ui->shutterBox->setChecked(false);
        else
            ui->shutterBox->setChecked(true);
        on_shutterBox_clicked();
    }
    else if (notify.contains("SK 301,1", Qt::CaseSensitive))
    {
        if (ui->shutterBox->isChecked())
            ui->shutterBox->setChecked(false);
        else
            ui->shutterBox->setChecked(true);
        on_shutterBox_clicked();
    }

    // deck unit
    else if (notify.contains("SK 266,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(0);
    else if (notify.contains("SK 267,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(1);
    else if (notify.contains("SK 268,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(2);
    else if (notify.contains("SK 269,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(3);
    else if (notify.contains("SK 270,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(4);
    else if (notify.contains("SK 271,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(5);
    else if (notify.contains("SK 272,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(6);
    else if (notify.contains("SK 273,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(7);
    else if (notify.contains("SK 306,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(0);
    else if (notify.contains("SK 307,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(1);
    else if (notify.contains("SK 308,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(2);
    else if (notify.contains("SK 309,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(3);
    else if (notify.contains("SK 310,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(4);
    else if (notify.contains("SK 311,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(5);
    else if (notify.contains("SK 312,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(6);
    else if (notify.contains("SK 313,1", Qt::CaseSensitive))
        ui->deckSelection->setCurrentIndex(7);

    // escape
    else if (notify.contains("SK 354,1", Qt::CaseSensitive))
    {
        if (ui->escapeBtn->isChecked())
            ui->escapeBtn->setChecked(false);
        else
            ui->escapeBtn->setChecked(true);
        on_escapeBtn_clicked();
    }
}

// sync controls with the auto notification
void MainWindow::processNotify(QString notify)
{
    // objective
    if (notify.contains("NOB ", Qt::CaseSensitive))
    {
        QString before = QString::number(currObj+1);
        currObj = notify.right(1).toInt()-1;
        QString after = QString::number(currObj+1);
        ui->objSelection->blockSignals(true);
        ui->objSelection->setCurrentIndex(currObj);
        ui->objSelection->blockSignals(false);

        enqueueCmd("SKD 15" + before + ",1");
        enqueueCmd("SKD 15" + after + ",2");

        double max = focusNearLimit[currObj]/100;
        ui->zValue->blockSignals(true);
        ui->zValue->setMaximum(max);
        ui->zValue->blockSignals(false);

        ui->zSlider->setMaximum((int)max);
    }

    // focus position
    else if (notify.contains("NFP ", Qt::CaseSensitive))
    {
        notify.remove("NFP ");
        currZ = notify.toDouble() / 100;
        ui->zSlider->setValue((int)currZ);

        ui->zValue->blockSignals(true);
        ui->zValue->setValue(currZ);
        ui->zValue->blockSignals(false);
    }

    // eyepiece ratio
    else if (notify.contains("NBIL ", Qt::CaseSensitive))
    {
        switch (notify.right(1).toInt())
        {
        case 1:
            ui->eye0->setChecked(true);
            break;
        case 2:
            ui->eye50->setChecked(true);
            break;
        case 3:
            ui->eye100->setChecked(true);
            break;
        }
    }

    // unit of deck
    else if (notify.contains("NMU", Qt::CaseSensitive))
    {
        int before = currDeckUnit+1;
        currDeckUnit = notify.right(1).toInt()-1;
        int after = currDeckUnit+1;
        ui->deckSelection->blockSignals(true);
        ui->deckSelection->setCurrentIndex(currDeckUnit);
        ui->deckSelection->blockSignals(false);

        if (deckNum == 1)
        {
            enqueueCmd("SKD " + QString::number(before+265) + ",1");
            enqueueCmd("SKD " + QString::number(after+265) + ",2");
        }
        else
        {
            enqueueCmd("SKD " + QString::number(before+305) + ",1");
            enqueueCmd("SKD " + QString::number(after+305) + ",2");
        }
    }

    // shutter
    else if (notify.contains("NESH", Qt::CaseSensitive))
    {
        int state = notify.right(1).toInt();
        if (state == 0)
        {
            ui->shutterBox->setChecked(false);
            if (deckNum == 1)
                enqueueCmd("SKD 261,1");
            else
                enqueueCmd("SKD 301,1");
        }
        else
        {
            ui->shutterBox->setChecked(true);
            if (deckNum == 1)
                enqueueCmd("SKD 261,2");
            else
                enqueueCmd("SKD 301,2");
        }
    }

    // LED
    else if (notify.contains("NDISP", Qt::CaseSensitive))
    {
        int state = notify.right(3).left(1).toInt();
        if (state == 1)
            ui->actionLED->setChecked(true);
        else
            ui->actionLED->setChecked(false);
    }
}

void MainWindow::focusMove(double target)
{
    QString cmd = "FG ";
    cmd.append(QString::number(target*100, 10, 0));
    enqueueCmd(cmd);
    ui->statusbar->showMessage("Moving Focus...", 3000);
    currZ = target;
}

void MainWindow::on_zSlider_sliderReleased()
{
    if (ui->escapeBtn->isChecked())
    {
        ui->escapeBtn->setChecked(false);
        enqueueCmd("SKD 354,1");
    }

    double target = (double)ui->zSlider->value();

    focusMove(target);
    ui->zValue->setValue(currZ);
    ui->zSlider->setValue((int)currZ);

    return;
}

void MainWindow::on_zValue_valueChanged(double value)
{

    if (ui->escapeBtn->isChecked())
    {
        ui->escapeBtn->setChecked(false);
        enqueueCmd("SKD 354,1");
    }

    if (currZ != value)
        focusMove(value);

}

void MainWindow::on_escapeBtn_clicked()
{
    if (ui->escapeBtn->isChecked())
    {
        // from Unchecked to Checked
        beforeEscape = currZ;
        double target = 0;
        if (currZ - escapeDist >= 0)
            target = currZ - escapeDist;

        focusMove(target);
        ui->escapeBtn->setChecked(true);
        enqueueCmd("SKD 354,2");
    }
    else
    {
        // from Checked to Unchecked
        focusMove(beforeEscape);
        ui->escapeBtn->setChecked(false);
        enqueueCmd("SKD 354,1");
    }
}

void MainWindow::on_cmdBtn_clicked()
{
    if(advCmd == false)
    {
        if( QMessageBox::warning(this, "Advanced Command Mode",
                                 "This mode will allow you to view all commands and replies,"
                                 " and send custom commands (press the enter key).\n"
                                 "Custom commands may damage the frame and cause software errors.\n"
                                 "See manual for details of commands.\n"
                                 "Continue?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)
                == QMessageBox::Yes)
        {
            advCmd = true;
            ui->lineCmd->setVisible(1);
            ui->lineRsp->setVisible(1);
            ui->cmdBtn->setDescription("Hide");
        }
    }
    else
    {
        if(ui->lineCmd->isVisible())
        {
            ui->lineCmd->setVisible(0);
            ui->lineRsp->setVisible(0);
            ui->cmdBtn->setDescription("Show");
        }
        else
        {
            ui->lineCmd->setVisible(1);
            ui->lineRsp->setVisible(1);
            ui->cmdBtn->setDescription("Hide");
        }
    }
}

void MainWindow::on_lineCmd_returnPressed()
{
    enqueueCmd(ui->lineCmd->text());
    ui->statusbar->showMessage("Success to send command.", 3000);
}

void MainWindow::on_eye100_clicked()
{
    ui->statusbar->showMessage("Switching eyepiece ratio...", 3000);
    enqueueCmd("BIL 3");
}

void MainWindow::on_eye50_clicked()
{
    ui->statusbar->showMessage("Switching eyepiece ratio...", 3000);
    enqueueCmd("BIL 2");
}

void MainWindow::on_eye0_clicked()
{
    ui->statusbar->showMessage("Switching eyepiece ratio...", 3000);
    enqueueCmd("BIL 1");
}

// set step of zValue double spin box
void MainWindow::stepSet(int mode)
{
    if (mode == 1) // signal from fine box
    {
        if (ui->fineBox->isChecked()) // into fine
        {
            ui->zValue->setSingleStep(0.01);
            ui->coarseBox->setChecked(false);
            if (ui->zValue->value() != currZ)
                focusMove(ui->zValue->value());
            return;
        }
    }
    else if (mode == 2) // signal from coarse box
    {
        if (ui->coarseBox->isChecked()) // into coarse
        {
            ui->zValue->setSingleStep(1);
            ui->fineBox->setChecked(false);
            if (ui->zValue->value() != currZ)
                focusMove(ui->zValue->value());
            return;
        }
    }
    // into normal
    ui->zValue->setSingleStep(0.1);
    if (ui->zValue->value() != currZ)
        focusMove(ui->zValue->value());
    return;
}

void MainWindow::on_coarseBox_clicked()
{
    stepSet(2);
}

void MainWindow::on_fineBox_clicked()
{
    stepSet(1);
}

void MainWindow::on_objSelection_currentIndexChanged(int index)
{
    QString objIndex = QString::number(index+1);
    ui->statusbar->showMessage("Switching objective lens...", 3000);
    enqueueCmd("EN5 0");
    EN5symbol = false;
    enqueueCmd("OBSEQ " + objIndex);
    enqueueCmd("NL?");   // query the near focus limit
    enqueueCmd("EN5 1");
}

void MainWindow::on_deckSelection_currentIndexChanged(int index)
{
    QString deckUnitIndex = QString::number(index+1);
    QString deckIndex = QString::number(deckNum);
    ui->statusbar->showMessage("Switching deck mirror units...", 3000);
    enqueueCmd("EN5 0");
    EN5symbol = false;
    enqueueCmd("MU" + deckIndex + " " + deckUnitIndex);
    enqueueCmd("EN5 1");
}

void MainWindow::on_shutterBox_clicked()
{
   QString deckIndex = QString::number(deckNum);
   enqueueCmd("EN5 0");
   EN5symbol = false;
   if (!ui->shutterBox->isChecked())
   {
       ui->statusbar->showMessage("Opening shutter...", 3000);
       enqueueCmd("ESH" + deckIndex + " 0"); // open shutter
   }
   else
   {
       ui->statusbar->showMessage("Closing shutter...", 3000);
       enqueueCmd("ESH" + deckIndex + " 1"); // close shutter
   }
   enqueueCmd("EN5 1");
}

void MainWindow::on_syncBox_stateChanged(int state)
{
    if (state == 2)
        cmdTh->syncSymbol = true;
    if (state == 0)
        cmdTh->syncSymbol = false;
}

void MainWindow::kLabelClicked()
{
    QMessageBox::about(this, "The Kuang Lab",
                       "<a href='https://person.zju.edu.cn/cfkuang/'>The Kuang Lab</a> builts super-resolution imaging systems to study biological and chemical processes at the nanoscale.<br>"
                       "Our technology leverages innovations in applied optics, signal and image processing, and design optimization.");
}

// open settings window
void MainWindow::on_actionFocus_triggered()
{
    if (ui->zValue->isEnabled())
    {
        setWin->setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
        emit sendSliderMinMax(ui->zSlider->minimum(), ui->zSlider->maximum());
        setWin->move(x() + width()/2 - setWin->width()/2,
                     y() + height()/2 - setWin->height()/2);
        setWin->show();
    }
    else
        QMessageBox::information(this, "Error",
                                 "Please try again later.", QMessageBox::Ok);
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about(this, "About this program",
                       "Version: 2.5.1<br>"
                       "Author: Zhengyi Zhan<br><br>"
                       "Download Update: <a href='https://github.com/ZhengyiZ/IX83-GUI-Qt/releases'>GitHub Releases</a><br>"
                       "Bug report: <a href='https://github.com/ZhengyiZ/IX83-GUI-Qt/issues'>GitHub Issues</a><br>");
}

void MainWindow::on_actionAboutQt_triggered()
{
    QMessageBox::aboutQt(this);
}

void MainWindow::on_actionStay_on_Top_triggered(bool checked)
{
    if (checked)
        setWindowFlags(windowFlags|Qt::WindowStaysOnTopHint);
    else
        setWindowFlags(windowFlags);
    show();
}


void MainWindow::on_actionLED_triggered(bool checked)
{
    if (checked)
    {
        enqueueCmd("IXIL 1");
        enqueueCmd("TPIL 1");
    }
    else
    {
        enqueueCmd("IXIL 0");
        enqueueCmd("TPIL 0");
    }
}

void MainWindow::on_actionAbout_IX83_triggered()
{
    QDesktopServices::openUrl(QUrl("https://www.olympus-lifescience.com/zh/microscopes/inverted/ixplore-pro/"));
}
