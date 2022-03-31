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
    this->setWindowTitle("Olympus IX83 Control");
    this->setWindowFlags(Qt::WindowStaysOnTopHint);

    // pass parameters
    this->pInterface = nullptr;
    this->ptr_closeIf = ptr_closeIf;

    // create a new wait sub thread
    thread = new CMDThread(nullptr, ptr_sendCmd, ptr_reCb);

    // emit command to thread
    connect(this, SIGNAL(sendCmdSignal()),
            thread, SLOT(receiveCmd()), Qt::QueuedConnection);

    // receive response from thread
    connect(thread, SIGNAL(sendRsp(QString)),
            this, SLOT(receiveRsp(QString)), Qt::QueuedConnection);

    // in order to solve the problem that thread cannot draw QMessageBox
    connect(this, SIGNAL(sendRegister()), thread, SLOT(receiveRegister()));
    connect(thread, SIGNAL(sendRegisterResult(bool)), this, SLOT(receiveRegisterResult(bool)));

    // init settings
    ui->minLine->setValidator(new QIntValidator(0,10500,this));
    ui->maxLine->setValidator(new QIntValidator(0,10500,this));
    ui->initLine->setValidator(new QIntValidator(1,700,this));
    ui->constantLine->setValidator(new QIntValidator(1,3000,this));
    ui->finalLine->setValidator(new QIntValidator(1,1000,this));
    ui->toolBox->setCurrentIndex(0);

    ui->lineCmd->setVisible(0);
    ui->lineRsp->setVisible(0);

    // disable all the components
    ctlSettings(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::sendCmd(QString cmd)
{
    thread->cmdFIFO.enqueue(cmd);
    emit sendCmdSignal();
    ui->lineCmd->setText(cmd);
    return true;
}

void MainWindow::ctlSettings(bool a)
{
    ui->switchObjBtn->setEnabled(a);
    ui->groupBox->setEnabled(a);
    ui->cmdBtn->setEnabled(a);
    ui->lineCmd->setEnabled(a);
    ui->lineRsp->setEnabled(a);
    ui->zValue->setEnabled(a);
    ui->zSlider->setEnabled(a);
    ui->roughBtn->setEnabled(a);
    ui->fineBtn->setEnabled(a);
    ui->syncBtn->setEnabled(a);
    ui->escapeBtn->setEnabled(a);
    ui->sliderSetBtn->setEnabled(a);
    ui->speedSetBtn->setEnabled(a);
    return;
}

// applicatioin exit inquiry
// close interface & quit the command thread
// NOTE: the SDK library has a defect that if an interface is not closed,
// it will not open next time until rebooting OS
void MainWindow::closeEvent (QCloseEvent *e)
{
    if(quitSymbol)
    {
        thread->quit();
        thread->wait();
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
            thread->quitCmd = true;
            sendCmd("L 0,0");
            ui->statusbar->showMessage("Logging out from microscope, which will be fast.");
        }
        e->ignore();
    }
}

void MainWindow::receiveQuit(bool symbol)
{
    this->quitSymbol = symbol;
    return;
}

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
                quitSymbol = true;
                qApp->quit();
                return;
            default:
                emit sendRegister();
            }
        }
}

// slots function that receive a pointer to an interface address
void MainWindow::receivePointer(void *pInterface_new)
{
    this->pInterface = pInterface_new;
    this->thread->pInterface = pInterface_new;
    this->thread->start();

    //// Initialization Pipeline ////

    ui->statusbar->showMessage("Logging in, please wait a few seconds.");

    // 1.get objective lenses information
    sendCmd("GOB 1");
    sendCmd("GOB 2");
    sendCmd("GOB 3");
    sendCmd("GOB 4");
    sendCmd("GOB 5");
    sendCmd("GOB 6");

    // 2.log into remote mode
    sendCmd("L 1,0");

    // 3.log into setting mode
    sendCmd("OPE 0");

    // 4.get current objective lens
    sendCmd("OB?");

    // 5.get focus escape distance of current objective lens
    sendCmd("ESC2?");

    // 6.check focus near limit
    sendCmd("NL?");

    // 7.get current focus position
    sendCmd("FP?");

    // 8.set imaging mode to wide field at first
    on_wfBtn_clicked();

    //// Initialization Pipeline Ends ////
}

void MainWindow::receiveRsp(QString rsp)
{

    ui->lineRsp->setText(rsp);
    processCallback(rsp);

    if (thread->quitCmd)
    {
        // closeEvent
        this->ptr_closeIf(pInterface);
        quitSymbol = true;
        qApp->quit();
        return;
    }

}

// processing callback by categories
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
            /*
            QString magStr = str.section(",",3,3);
            int mediumIndex = str.section(",",4,4).toInt();
            QString mediumStr = "Unknown";
            switch(mediumIndex)
            {
            case 1:
                mediumStr = "dry";
                break;
            case 2:
                mediumStr = "water";
                break;
            case 3:
                mediumStr = "oil";
                break;
            case 4:
                mediumStr = "sili-oil";
                break;
            }
            QString objInfo = indexStr + ": " + nameStr + ", NA=" + NAStr
                    + ", " + magStr + "x, " + mediumStr;
            */
            QString objInfo;
            if( nameStr.contains("NONE", Qt::CaseInsensitive) )
                objInfo = indexStr + ": NONE";
            else
                objInfo = indexStr + ": " + nameStr + ", NA=" + NAStr;
            ui->objSelection->addItem(objInfo);
        }
    }

    // switch objective lens
    else if (str.contains("OB", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch objective lens.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Switching objective lens complete.", 3000);
        else if (str.contains(" ", Qt::CaseSensitive))
        {
            currObj = str.right(1).toInt()-1;
            ui->objLabel->setText(ui->objSelection->itemText(currObj));
            ui->objSelection->setCurrentIndex(currObj);
            double max = focusNearLimit[currObj]/100;

            ui->zValue->blockSignals(true);
            ui->zValue->setMaximum(max);
            ui->zValue->blockSignals(false);

            ui->zSlider->setMaximum((int)max);
            ui->maxLine->setText(QString::number(max, 10, 0));
        }
    }

    // escape distance
    else if (str.contains("ESC2 ", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get escape distance.", 3000);
        else
            escapeDist = str.right(1).toInt()*1000;
    }

    // setting mode
    else if (str.contains("OPE", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to change idle/setting mode.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Log in success.");
    }

    // focus position active notification
    else if (str.contains("NFP", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            if (ui->syncBtn->isChecked())
            {
                ui->statusbar->showMessage("Failed to turn on auto sync mode.", 3000);
                ui->syncBtn->setChecked(false);
            }
        else
            {
                ui->statusbar->showMessage("Failed to turn off auto sync mode.", 3000);
                ui->syncBtn->setChecked(true);
            }
        else if (str.contains("+", Qt::CaseSensitive))
            if (ui->syncBtn->isChecked())
                ui->statusbar->showMessage("Auto sync mode is turned on.", 3000);
            else
                ui->statusbar->showMessage("Auto sync mode is turned off.", 3000);
        else
        {
            str.remove("NFP ");
            currZ = str.toDouble() / 100;
            ui->zSlider->setValue((int)currZ);

            ui->zValue->blockSignals(true);
            ui->zValue->setValue(currZ);
            ui->zValue->blockSignals(false);

        }
    }

    // focus position
    else if (str.contains("FP ", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get current focus position.", 3000);
        else
        {
            currZ = str.replace("FP ", "").toDouble()/100;

            ui->zValue->blockSignals(true);
            ui->zValue->setValue(currZ);
            ui->zValue->blockSignals(false);

            ui->zSlider->setValue((int)currZ);
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

            ui->zValue->setMaximum(max);

            ui->zSlider->setMaximum((int)max);
            ui->maxLine->setText(QString::number(max, 10, 0));
        }
    }

    // shutter
    else if (str.contains("ESH2", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch imaging mode.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
        {
            if (firstImageMode)
            {
                ui->statusbar->showMessage("Initialization complete, ready to work.", 3000);
                ctlSettings(true);
                firstImageMode = false;
            }
            else
                ui->statusbar->showMessage("Switching imaging mode complete.", 3000);
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

    // focusing unit speed
    else if (str.contains("FSPD", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to set focusing unit speed.", 3000);
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Setting focusing unit speed complete.", 3000);
    }

}

bool MainWindow::focusMove(double target)
{
    QString cmd = "FG ";
    cmd.append(QString::number(target*100, 10, 0));
    sendCmd(cmd);
    ui->statusbar->showMessage("Moving Focus...", 3000);
    currZ = target;
    return true;
}

void MainWindow::on_switchObjBtn_clicked()
{
    QString switchObjCmd = "OB ";
    QString indexStr = ui->objSelection->currentText().left(1);
    switchObjCmd.append(indexStr);
    ui->statusbar->showMessage("Switching objective lens...", 3000);
    sendCmd(switchObjCmd);
    sendCmd("ESC2?");
    return;
}

void MainWindow::on_wfBtn_clicked()
{
    ui->statusbar->showMessage("Setting imaging mode to wide field...", 3000);
    sendCmd("MU2 5");
    // open shutter
    sendCmd("ESH2 0");
    return;
}

void MainWindow::on_conBtn_clicked()
{
    ui->statusbar->showMessage("Setting imaging mode to confocal...", 3000);
    sendCmd("MU2 4");
    // close shutter
    sendCmd("ESH2 1");
    return;
}

void MainWindow::on_fineBtn_clicked()
{
    if (ui->fineBtn->isChecked())
    {
        // form Unchecked to Checked
        ui->zValue->setSingleStep(0.01);
        ui->fineBtn->setChecked(true);
        ui->roughBtn->setChecked(false);
    }
    else
    {
        // form Checked to Unchecked
        ui->zValue->setSingleStep(0.1);
        ui->fineBtn->setChecked(false);
    }

    if (ui->zValue->value() != currZ)
        focusMove(ui->zValue->value());

    return;
}

void MainWindow::on_roughBtn_clicked()
{
    if (ui->roughBtn->isChecked())
    {
        // form Unchecked to Checked
        ui->zValue->setSingleStep(1);
        ui->roughBtn->setChecked(true);
        ui->fineBtn->setChecked(false);
    }
    else
    {
        // form Checked to Unchecked
        ui->zValue->setSingleStep(0.1);
        ui->roughBtn->setChecked(false);
    }

    if (ui->zValue->value() != currZ)
        focusMove(ui->zValue->value());

    return;
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
        ui->zSlider->setValue((int)currZ);
        ui->zValue->setValue(currZ);
        ui->escapeBtn->setChecked(true);

    }
    else
    {
        // from Checked to Unchecked
        focusMove(beforeEscape);
        ui->zSlider->setValue((int)currZ);
        ui->zValue->setValue(currZ);
        ui->escapeBtn->setChecked(false);

    }

    return;
}

void MainWindow::on_sliderSetBtn_clicked()
{
    int max = ui->maxLine->text().toInt();
    int min = ui->minLine->text().toInt();

    if (max>10500)
    {
        QMessageBox::warning(this,"Error",
                             "The maximum value should not exceed 10500 μm.");
        ui->maxLine->setText("10500");
    }
    else if (min<0)
    {
        QMessageBox::warning(this,"Error",
                             "The minimum value should not be negative.");
        ui->minLine->setText("0");
    }
    else
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
        sendCmd(cmd);

        ui->zSlider->setMaximum(max);
        ui->zValue->setMaximum((double)max);
        ui->zSlider->setMinimum(min);
        ui->zValue->setMinimum((double)min);
        ui->toolBox->setCurrentIndex(0);
    }
    return;
}

void MainWindow::on_speedSetBtn_clicked()
{
    int init = ui->initLine->text().toInt();
    int constant = ui->constantLine->text().toInt();
    int final = ui->finalLine->text().toInt();

    if (init>700 || init<1)
    {
        QMessageBox::warning(this,"Error",
                             "The init speed is not in range (1-700).");
        ui->initLine->setText("700");
    }
    else if (constant>3000 || constant<1)
    {
        QMessageBox::warning(this,"Error",
                             "The constant speed is not in range (1-3000).");
        ui->constantLine->setText("700");
    }
    else if (final>1000 || final<1)
    {
        QMessageBox::warning(this,"Error",
                             "The deceleration time is not in range (1-1000).");
        ui->finalLine->setText("60");
    }
    else
    {
        QString cmd = "FSPD ";
        cmd.append(QString::number(init*100));
        cmd.append(",");
        cmd.append(QString::number(constant*100));
        cmd.append(",");
        cmd.append(QString::number(final));
        ui->statusbar->showMessage("Setting focus unit speeds...", 3000);
        sendCmd(cmd);
    }
    return;
}

void MainWindow::on_zSlider_sliderReleased()
{
    if (ui->escapeBtn->isChecked())
        ui->escapeBtn->setChecked(false);

    double target = (double)ui->zSlider->value();

    focusMove(target);
    ui->zValue->setValue(currZ);
    ui->zSlider->setValue((int)currZ);

    return;
}

void MainWindow::on_zValue_valueChanged(double value)
{

    if (ui->escapeBtn->isChecked())
        ui->escapeBtn->setChecked(false);

    if (currZ != value)
    {
        focusMove(value);
        ui->zSlider->setValue((int)currZ);
        ui->zValue->setValue(currZ);
    }
    return;
}

void MainWindow::on_maxLine_returnPressed()
{
    on_sliderSetBtn_clicked();
}

void MainWindow::on_lineCmd_returnPressed()
{
    sendCmd(ui->lineCmd->text());
    ui->statusbar->showMessage("Success to send command.", 3000);
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
        }
    }
    else
    {
        if(ui->lineCmd->isVisible())
        {
            ui->lineCmd->setVisible(0);
            ui->lineRsp->setVisible(0);
        }
        else
        {
            ui->lineCmd->setVisible(1);
            ui->lineRsp->setVisible(1);
        }
    }
}


void MainWindow::on_syncBtn_clicked()
{
    if (ui->syncBtn->isChecked())
        // open active notification
        sendCmd("NFP 1");
    else
        // close active notification
        sendCmd("NFP 0");
}

