#ifndef IFSELECTION_H
#define IFSELECTION_H

#include <QDialog>
#include <QMessageBox>
#include "mainwindow.h"
#include "DLL.h"

namespace Ui {
class IFSelection;
}

class IFSelection : public QDialog
{
    Q_OBJECT

public:
    explicit IFSelection(QWidget *parent = nullptr,
                         void* pIF = nullptr,
                         ptr_EnumInterface ptr_enumIf = nullptr,
                         ptr_GetInterfaceInfo ptr_getInfo = nullptr,
                         ptr_OpenInterface ptr_openIf = nullptr,
                         ptr_CloseInterface ptr_closeIf = nullptr);
    ~IFSelection();

    void* pIF;
    ptr_EnumInterface ptr_enumIf;
    ptr_GetInterfaceInfo ptr_getInfo;
    ptr_OpenInterface ptr_openIf;
    ptr_CloseInterface ptr_closeIf;

signals:
    void sendPointer(void *);
    void sendQuitFromDialog(bool);

private slots:
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();


private:
    Ui::IFSelection *ui;
};

#endif // IFSELECTION_H
