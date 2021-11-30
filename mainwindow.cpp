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

    // pass parameters
    this->pInterface = nullptr;
    this->ptr_closeIf = ptr_closeIf;

    // create a new wait sub thread
    thread = new CMDThread(nullptr, ptr_sendCmd, ptr_reCb);

    // emit command to thread
    connect(this, SIGNAL(sendCmdSignal(QString)),
            thread, SLOT(receiveCmd(QString)), Qt::QueuedConnection);

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

    // disable all the components
    ctlSettings(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::sendCmd(QString cmd, int caseIn, int subIn)
{
    if (!thread->busy)
    {
        thread->caseIndex = caseIn;
        thread->subIndex = subIn;
        emit sendCmdSignal(cmd);
        return true;
    }
    else
    {
        QMessageBox::StandardButton result =
                QMessageBox::warning(NULL,"Fail to send command",
                                           "Microscope is busy, please try again later.",
                                           QMessageBox::Retry|QMessageBox::Ok, QMessageBox::Ok);
        switch (result)
        {
        case QMessageBox::Retry:
            this->sendCmd(cmd, caseIn, subIn); // recursion
            return false;
        default:
            return false;
        }
    }
}

void MainWindow::ctlSettings(bool a)
{
    ui->switchObjBtn->setEnabled(a);
    ui->groupBox->setEnabled(a);
    ui->lineCmd->setEnabled(a);
    ui->lineRsp->setEnabled(a);
    ui->zValue->setEnabled(a);
    ui->zSlider->setEnabled(a);
    ui->roughBtn->setEnabled(a);
    ui->fineBtn->setEnabled(a);
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
            sendCmd("L 0,0", 9, 0);
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
    // 1.get first objective lens information
    sendCmd("GOB 1", 1, 1);
    ui->statusbar->showMessage("Logging in, please wait a few seconds.");
}

void MainWindow::receiveRsp(QString rsp)
{

    processCallback(rsp);

    switch (thread->caseIndex)
    {

    //// Initialization Pipeline ////
    // 2.get other objective lenses and log into remote mode
    case 1:{
        if (thread->subIndex < 6)
        {
            int index = thread->subIndex + 1;
            QString cmd = "GOB ";
            cmd.append(QString::number(index));
            sendCmd(cmd, 1, index);
        }
        else
            sendCmd("L 1,0", 2, 0);
        return;
    }
    // 3.log into setting mode
    case 2:{
        sendCmd("OPE 0", 3, 1);
        return;
    }
    // 4.get current objective lens
    case 3:{
        sendCmd("OB?", 4, thread->subIndex);
        return;
    }
    // 5.get focus escape distance of current objective lens
    case 4:{
        sendCmd("ESC2?", 5, thread->subIndex);
        return;
    }
    // 6.check focus near limit
    case 5:{
        if(thread->subIndex == 1)
            sendCmd("NL?", 6, 0);
        return;
    }
    // 7.get current focus position
    case 6:{
        sendCmd("FP?", 7, 0);
        return;
    }
    // 8.set imaging mode to wide field at first
    case 7:{
        on_wfBtn_clicked();
        return;
    }
    //// Initialization Pipeline Ends ////

    case 8:{ // set shutter status
        QString cmd = "ESH2 ";
        cmd.append(QString::number(thread->subIndex));
        sendCmd(cmd, 0, 0);
        return;
    }
    case 9: // closeEvent
        this->ptr_closeIf(pInterface);
        quitSymbol = true;
        qApp->quit();
        return;
    case 10: // send button
        ui->lineRsp->setText(rsp);
        ui->statusbar->showMessage("Success to receive response.", 3000);
        return;
    default:
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
            ui->zValue->setMaximum(max);
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
        {
            this->logStatus = true;
            ui->statusbar->showMessage("Log in success.");
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
            qDebug() << QString::number(currZ) << Qt::endl;
            ui->zValue->setValue(currZ);
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
    if(sendCmd(cmd, 0, 0))
    {
        ui->statusbar->showMessage("Moving Focus...", 3000);
        currZ = target;
        return true;
    }
    else
        return false;
}

void MainWindow::on_switchObjBtn_clicked()
{
    QString switchObjCmd = "OB ";
    QString indexStr = ui->objSelection->currentText().left(1);
    switchObjCmd.append(indexStr);
    if (sendCmd(switchObjCmd, 3, 0))
        ui->statusbar->showMessage("Switching objective lens...", 3000);
    return;
}

void MainWindow::on_wfBtn_clicked()
{
    if (sendCmd("MU2 6", 8, 0))
        ui->statusbar->showMessage("Setting imaging mode to wide field...", 3000);
    return;
}

void MainWindow::on_conBtn_clicked()
{
    if (sendCmd("MU2 7", 8, 1))
        ui->statusbar->showMessage("Setting imaging mode to confocal...", 3000);
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

        if (focusMove(target))
        {
            ui->zSlider->setValue((int)currZ);
            ui->zValue->setValue(currZ);
            ui->escapeBtn->setChecked(true);
        }
        else
            ui->escapeBtn->setChecked(false);
    }
    else
    {
        // from Checked to Unchecked
        if (focusMove(beforeEscape))
        {
            ui->zSlider->setValue((int)currZ);
            ui->zValue->setValue(currZ);
            ui->escapeBtn->setChecked(false);
        }
        else
            ui->escapeBtn->setChecked(true);
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
                             "The maximum value should not exceed 10500μm.");
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
        if (sendCmd(cmd, 0, 0))
            ui->statusbar->showMessage("Setting focus near limit...", 3000);

        ui->zSlider->setMaximum(max);
        ui->zValue->setMaximum((double)max);
        ui->zSlider->setMinimum(min);
        ui->zValue->setMinimum((double)min);
        if (currZ != ui->zValue->value())
        {
            focusMove(ui->zValue->value());
            ui->zSlider->setValue((int)currZ);
            ui->zValue->setValue(currZ);
        }
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
        if (sendCmd(cmd, 0, 0))
            ui->statusbar->showMessage("Setting focus unit speeds...", 3000);
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
    if(firstAdvCmd)
    {
        if( QMessageBox::warning(this,"Enable advanced command mode?",
                                      "Advanced command mode may damage the frame and cause software errors,"
                                      " please make sure the command is entered correctly according to the manual.",
                                      QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes)
                == QMessageBox::Yes)
        {
            firstAdvCmd = false;
            if( sendCmd(ui->lineCmd->text(),10,0) )
                ui->statusbar->showMessage("Success to send command.", 3000);
        }
    }
    else
        if( sendCmd(ui->lineCmd->text(),10,0) )
            ui->statusbar->showMessage("Success to send command.", 3000);
}
