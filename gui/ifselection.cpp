#include "ifselection.h"
#include "ui_ifselection.h"

IFSelection::IFSelection(QWidget *parent, void* pIF,
                         ptr_EnumInterface ptr_enumIf,
                         ptr_GetInterfaceInfo ptr_getInfo,
                         ptr_OpenInterface ptr_openIf,
                         ptr_CloseInterface ptr_closeIf) :
    QDialog(parent),
    ui(new Ui::IFSelection)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowStaysOnTopHint);

    // pass parameters
    this->pIF = pIF;
    this->ptr_enumIf = ptr_enumIf;
    this->ptr_getInfo = ptr_getInfo;
    this->ptr_openIf = ptr_openIf;
    this->ptr_closeIf = ptr_closeIf;

    // enumerate interfaces
    portCount = this->ptr_enumIf();
//    portCount = 1;    // cheat code in case you don't have an interface
    while (!portCount)
    {
        QMessageBox::StandardButton result =
                QMessageBox::critical(this,"Connection Error",
                                           "There is no Olympus IX83 detected, "
                                           "please check the switch of CBH, "
                                           "1394 connections and 1394 drivers of your PC.",
                                           QMessageBox::Retry|QMessageBox::Cancel);
        switch (result)
        {
        case QMessageBox::Retry:
            portCount = this->ptr_enumIf();
            break;
        default:
            qApp->exit();
            return;
        }
    }

    // set widgets
    QString labelStr = ui->label->text();
    labelStr.append( QString::number(portCount) );
    ui->label->setText( labelStr );
    ui->comboBox->clear();
    for (int i=0; i<portCount; i++)
        ui->comboBox->addItem( QString::asprintf("Interface %d",i+1) );
}

IFSelection::~IFSelection()
{
    delete ui;
}

void IFSelection::on_buttonBox_accepted()
{
    // initialize
    void *pInterface = nullptr;

    // get a pointer to the selected interface address
    this->ptr_getInfo(ui->comboBox->currentIndex(), &pInterface);

    // check whether the interface is open or not
    bool result = false;
    while(!result)
    {
        result = this->ptr_openIf(pInterface);
//        result = true;   // cheat code
        if (!result)
        {
            QMessageBox::critical(this, "Interface Error",
                                  "Cloud not open port "
                                  + QString::number(ui->comboBox->currentIndex())
                                  + "! Reopening the program may solve the problem.",
                                  QMessageBox::Ok);
            emit sendQuitFromDialog(true);
            return;
        }
        else
        {
            // in case an interface is open before this one
            if (this->pIF != nullptr)
                this->ptr_closeIf(this->pIF);
            emit sendPointer(pInterface);
        }
    }
}

void IFSelection::on_buttonBox_rejected()
{
    this->close();
    return;
}
