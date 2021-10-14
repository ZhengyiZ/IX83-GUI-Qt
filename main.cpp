#include <QApplication>
#include <QLibrary>
#include <QMessageBox>
#include <QStyleFactory>
#include "mainwindow.h"
#include "ifselection.h"
#include "DLL.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    QApplication a(argc, argv);

    //// load DLL library ////
    int libLoadResult[4];
    QLibrary pm("msl_pm.dll");
    QLibrary pd("msl_pd_1394.dll");
    QLibrary fsi("fsi1394.dll");
    QLibrary gt("gt_log.dll");
    libLoadResult[0] = pm.load();
    libLoadResult[1] = pd.load();
    libLoadResult[2] = fsi.load();
    libLoadResult[3] = gt.load();
    // determine whether the loading is successful
    int count = 0;
    for(int i = 0; i < 4; i++)
    {
        if (libLoadResult[i] == 0) count++;
    }
    if (count != 0)
    {
        QMessageBox::critical(NULL,"Missing DLLs","Could not load library!\n" \
                                                  "Please check all the DLLs and CONFIG file are in the installation directory\n" \
                                                  "Here is the requiring DLL and CONFIG file list:\n" \
                                                  "msl_pm.dll\n" \
                                                  "msl_pd_1394.dll\n" \
                                                  "fsi1394.dll\n" \
                                                  "gt_log.dll\n"
                                                  "(*)gtlib.config");
        return 0;
    }
    //// end loading DLL library ////

    //// load function pointer ////
    ptr_Initialize ptr_init = (ptr_Initialize)pm.resolve("MSL_PM_Initialize");
    ptr_EnumInterface ptr_enumIf = (ptr_EnumInterface)pm.resolve("MSL_PM_EnumInterface");
    ptr_GetInterfaceInfo ptr_getInfo = (ptr_GetInterfaceInfo)pm.resolve("MSL_PM_GetInterfaceInfo");
    ptr_OpenInterface ptr_openIf = (ptr_OpenInterface)pm.resolve("MSL_PM_OpenInterface");
    ptr_SendCommand ptr_sendCmd = (ptr_SendCommand)pm.resolve("MSL_PM_SendCommand");
    ptr_RegisterCallback ptr_reCb = (ptr_RegisterCallback)pm.resolve("MSL_PM_RegisterCallback");
    ptr_CloseInterface ptr_closeIf = (ptr_CloseInterface)pm.resolve("MSL_PM_CloseInterface");
    if (!ptr_init || !ptr_getInfo || !ptr_enumIf
            || !ptr_openIf || !ptr_sendCmd
            || !ptr_reCb || !ptr_closeIf)
    {
        QMessageBox::critical(NULL,"Failed to link functions","Please confirm DLLs are intact!");
        return 0;
    }
    //// end loading function pointer ////

    ptr_init();

    // create IFSelection dialog and mainwindow at the same time
    // then connect their signals and slots
    IFSelection *IFdialog = new IFSelection(nullptr, nullptr,
                                            ptr_enumIf, ptr_getInfo,
                                            ptr_openIf, ptr_closeIf);
    static MainWindow *w = new MainWindow(nullptr, ptr_sendCmd,
                                          ptr_reCb, ptr_closeIf);
    QObject::connect(IFdialog, SIGNAL(sendPointer(void*)),
                     w, SLOT(receivePointer(void*)));
    QObject::connect(IFdialog, SIGNAL(sendQuitFromDialog(bool)),
                     w, SLOT(receiveQuit(bool)));

    if( IFdialog->exec() == QDialog::Rejected )
    {
        a.exit();
        return 0;
    }

    if (w->quitSymbol)
        return 0;
    else
    {
        w->show();
        return a.exec();
    }

}
