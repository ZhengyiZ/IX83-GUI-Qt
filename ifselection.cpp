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

    // pass parameters
    this->pIF = pIF;
    this->ptr_enumIf = ptr_enumIf;
    this->ptr_getInfo = ptr_getInfo;
    this->ptr_openIf = ptr_openIf;
    this->ptr_closeIf = ptr_closeIf;

    // enumerate interfaces
    int static count = this->ptr_enumIf();
//    count = 3;    // cheat code in case you don't have an interface
    while (!count)
    {
        QMessageBox::StandardButton result =
                QMessageBox::critical(NULL,"Failed to enumerate interface",
                                           "There is no Olympus IX83 connected, "
                                           "please check physical connections and 1394 drivers.",
                                           QMessageBox::Retry|QMessageBox::Cancel);
        switch (result)
        {
        case QMessageBox::Retry:
            break;
        default:
            qApp->exit();
            return;
        }
    }

    // set widgets
    QString labelStr = ui->label->text();
    labelStr.append( QString::number(count) );
    ui->label->setText( labelStr );
    ui->comboBox->clear();
    for (int i=0; i<count; i++)
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
            QMessageBox::StandardButton button =
                    QMessageBox::critical(NULL,"Failed to open interface",
                                               "Cloud not open port!",
                                               QMessageBox::Retry|QMessageBox::Cancel);
            switch (button)
            {
            case QMessageBox::Cancel:
                emit sendQuitFromDialog(true);
                return;
            default:
                break;
            }
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
