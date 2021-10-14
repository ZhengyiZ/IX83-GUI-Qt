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

    // pass parameters
    this->pInterface = nullptr;
    this->ptr_closeIf = ptr_closeIf;

    // create a new wait sub thread
    thread = new CMDThread(nullptr, ptr_sendCmd, ptr_reCb);

    // emit command to thread
    connect(this, SIGNAL(sendCmd(QString)),
            thread, SLOT(receiveCmd(QString)), Qt::QueuedConnection);

    // receive response from thread
    connect(thread, SIGNAL(sendRsp(QString)),
            this, SLOT(receiveRsp(QString)), Qt::QueuedConnection);

    // in order to solve the problem that thread cannot draw QMessageBox
    connect(this, SIGNAL(sendRegister()), thread, SLOT(receiveRegister()));
    connect(thread, SIGNAL(sendRegisterResult(bool)), this, SLOT(receiveRegisterResult(bool)));

    // init settings
    ctlSettings(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ctlSettings(bool a)
{
    ui->switchObjBtn->setEnabled(a);
    ui->groupBox->setEnabled(a);
    ui->zValue->setEnabled(a);
    ui->zSlider->setEnabled(a);
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
                                  QMessageBox::Yes, QMessageBox::No )
                == QMessageBox::Yes){

            {
                thread->caseIndex = 9;
                emit sendCmd("L 0,0");
            }
            e->ignore();
        }
        else
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
                                          QMessageBox::Retry|QMessageBox::Cancel);
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
    this->thread->caseIndex = 1;
    this->thread->subIndex = 1;
    emit sendCmd("GOB 1");
}

void MainWindow::receiveRsp(QString rsp)
{
    qDebug() << " > " << rsp << Qt::endl;

    processCallback(rsp);

    switch (thread->caseIndex)
    {

    //// Initialization Pipeline ////
    // 2.get other objective lenses and current objective lens
    case 1:{
        if (thread->subIndex < 6)
        {
            thread->subIndex++;
            QString objCmd = "GOB ";
            objCmd.append(QString::number(thread->subIndex));
            thread->caseIndex = 1;
            emit sendCmd(objCmd);
        }
        else
        {
            thread->caseIndex = 2;
            emit sendCmd("OB?");
        }
        return;
    }
    // 3.get focus escape distance of current objective lens
    case 2:{
        // if not log in
        if(this->logStatus == false)
            thread->caseIndex = 3;
        else
            thread->caseIndex = 0;
        emit sendCmd("OB?");
        return;
    }
    // 4.log in to remote mode
    case 3:{
        thread->caseIndex = 4;
        emit sendCmd("L 1,0");
        return;
    }
    // 5.get into setting mode
    case 4:{
        thread->caseIndex = 5;
        emit sendCmd("OPE 0");
        return;
    }
    // 6.check current focus position
    case 5:{
        thread->caseIndex = 6;
        emit sendCmd("FP?");
        return;
    }
    // 7.check focus near limit
    case 6:{
        thread->caseIndex = 7;
        emit sendCmd("NL?");
        return;
    }
    // 8.set imaging mode to wide field at first
    case 7:{
        on_wfBtn_clicked();
        return;
    }
    //// Initialization Pipeline Ends ////

    case 8:{ // set shutter status
        thread->caseIndex = 0;
        QString cmd = "ESH2 ";
        cmd.append(QString::number(thread->subIndex));
        emit sendCmd(cmd);
        return;
    }
    case 9: // closeEvent
        this->ptr_closeIf(pInterface);
        quitSymbol = true;
        qApp->quit();
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
            ui->statusbar->showMessage("Failed to get objective lens information.");
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
            ui->statusbar->showMessage("Failed to switch objective lens.");
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Switching objective lens complete.");
        else if (str.contains(" ", Qt::CaseSensitive))
        {
            currObj = str.right(1).toInt()-1;
            ui->objLabel->setText(ui->objSelection->itemText(currObj));

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
            ui->statusbar->showMessage("Failed to get escape distance.");
        else
            escapeDist = str.right(1).toInt()*1000;
    }

    // setting mode
    else if (str.contains("OPE", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to change idle/setting mode.");
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
            ui->statusbar->showMessage("Failed to get current focus position.");
        else
        {
            currZ = str.replace("FP ", "").toDouble()/100;
            ui->zValue->setValue(currZ);
            ui->zSlider->setValue((int)currZ);
        }
    }

    // focus near limit
    else if (str.contains("NL", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to get focus near limit.");
        else if (str.contains(" ", Qt::CaseSensitive))
        {
            str.replace("NL ", "");
            for(int i=0; i<6; i++)
                focusNearLimit[i] = str.section(",",i,i).toInt();

            double max = focusNearLimit[currObj]/100;
            ui->zValue->setMaximum(max);
            ui->zSlider->setMaximum((int)max);
            ui->maxLine->setText(QString::number(max, 10, 0));
        }
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Setting focus near limit complete.");
    }

    // shutter
    else if (str.contains("ESH2", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to switch imaging mode.");
        else if (str.contains("+", Qt::CaseSensitive))
        {
            ui->statusbar->showMessage("Switching imaging mode complete.");
            ctlSettings(true);
        }
    }

    // move focus position
    else if (str.contains("FG", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to move focus.");
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Moving focus complete.");
    }

    // focusing unit speed
    else if (str.contains("FSPD", Qt::CaseSensitive))
    {
        if (str.contains("!", Qt::CaseSensitive))
            ui->statusbar->showMessage("Failed to set focusing unit speed.");
        else if (str.contains("+", Qt::CaseSensitive))
            ui->statusbar->showMessage("Setting focusing unit speed complete.");
    }

}

void MainWindow::focusMove()
{
    QString cmd = "FG ";
    cmd.append(QString::number(currZ*100, 10, 0));
    thread->caseIndex = 0;
    emit sendCmd(cmd);
    return;
}

void MainWindow::on_switchObjBtn_clicked()
{
    QString switchObjCmd = "OB ";
    QString indexStr = ui->objSelection->currentText().left(1);
    switchObjCmd.append(indexStr);
    thread->caseIndex = 1;
    thread->subIndex = 7;
    emit sendCmd(switchObjCmd);
    return;
}

void MainWindow::on_wfBtn_clicked()
{
    thread->caseIndex = 8;
    thread->subIndex = 0;
    emit sendCmd("MU2 6");
    return;
}

void MainWindow::on_conBtn_clicked()
{
    thread->caseIndex = 8;
    thread->subIndex = 1;
    emit sendCmd("MU2 7");
    return;
}

void MainWindow::on_fineBtn_clicked()
{
    if (ui->fineBtn->isChecked())
    {
        // form Unchecked to Checked
        ui->zValue->setSingleStep(0.01);
        ui->fineBtn->setChecked(true);
    }
    else
    {
        // form Checked to Unchecked
        ui->zValue->setSingleStep(0.1);
        ui->fineBtn->setChecked(false);
    }

    if (ui->zValue->value() != currZ)
    {
        currZ = ui->zValue->value();
        focusMove();
    }

    return;
}

void MainWindow::on_escapeBtn_clicked()
{
    if (ui->escapeBtn->isChecked())
    {
        // from Unchecked to Checked
        beforeEscape = currZ;
        if (currZ - escapeDist >= 0)
            currZ = currZ - escapeDist;
        else
            currZ = 0;

        focusMove();

        ui->zSlider->setValue((int)currZ);
        ui->zValue->setValue(currZ);

        ui->escapeBtn->setChecked(true);
    }
    else
    {
        // from Checked to Unchecked
        currZ = beforeEscape;

        focusMove();

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
        thread->caseIndex = 0;
        emit sendCmd(cmd);

        ui->zSlider->setMaximum(max);
        ui->zSlider->setMinimum(min);
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
        thread->caseIndex = 0;
        emit sendCmd(cmd);
    }

    return;
}

void MainWindow::on_zSlider_sliderReleased()
{
    if (ui->escapeBtn->isChecked())
        ui->escapeBtn->setChecked(false);

    currZ = (double)ui->zSlider->value();

    focusMove();

    ui->zValue->setValue(currZ);

    return;
}

void MainWindow::on_zValue_valueChanged(double value)
{
    if (ui->escapeBtn->isChecked())
        ui->escapeBtn->setChecked(false);

    if (currZ != value)
    {
        currZ = value;
        focusMove();
        ui->zSlider->setValue((int)value);
    }

    return;
}

void MainWindow::on_pushButton_clicked()
{
    thread->caseIndex = 0;
    emit sendCmd("FP?");
    return;
}
