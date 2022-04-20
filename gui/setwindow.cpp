#include "setwindow.h"
#include "ui_setwindow.h"

SetWindow::SetWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SetWindow)
{
    ui->setupUi(this);

    setWindowTitle("Focus Settings");
    // init settings
    ui->minLine->setValidator(new QIntValidator(0,10500,this));
    ui->maxLine->setValidator(new QIntValidator(0,10500,this));
    ui->initLine->setValidator(new QIntValidator(1,700,this));
    ui->constantLine->setValidator(new QIntValidator(1,3000,this));
    ui->finalLine->setValidator(new QIntValidator(1,1000,this));
}

SetWindow::~SetWindow()
{
    delete ui;
}

bool SetWindow::on_speedSetBtn_clicked()
{
    int init = ui->initLine->text().toInt();
    int constant = ui->constantLine->text().toInt();
    int final = ui->finalLine->text().toInt();

    if (init>700 || init<1)
    {
        QMessageBox::warning(this,"Error",
                             "The init speed is not in range (1-700).");
        ui->initLine->setText("700");
        return false;
    }
    else if (constant>3000 || constant<1)
    {
        QMessageBox::warning(this,"Error",
                             "The constant speed is not in range (1-3000).");
        ui->constantLine->setText("700");
        return false;
    }
    else if (final>1000 || final<1)
    {
        QMessageBox::warning(this,"Error",
                             "The deceleration time is not in range (1-1000).");
        ui->finalLine->setText("60");
        return false;
    }
    else
    {
        QString cmd = "FSPD ";
        cmd.append(QString::number(init*100));
        cmd.append(",");
        cmd.append(QString::number(constant*100));
        cmd.append(",");
        cmd.append(QString::number(final));
        emit sendFSPD(cmd);
        return true;
    }
}

bool SetWindow::on_sliderSetBtn_clicked()
{
    int max = ui->maxLine->text().toInt();
    int min = ui->minLine->text().toInt();

    if (max>10500)
    {
        QMessageBox::warning(this,"Error",
                             "The maximum value should not exceed 10500 μm.");
        ui->maxLine->setText("10500");
        return false;
    }
    else if (min<0)
    {
        QMessageBox::warning(this,"Error",
                             "The minimum value should not be negative.");
        ui->minLine->setText("0");
        return false;
    }
    else
    {
        emit sendSliderSettings(min, max);
        return true;
    }
}

void SetWindow::on_maxLine_returnPressed()
{
    if (on_sliderSetBtn_clicked())
        close();
}

void SetWindow::on_finalLine_returnPressed()
{
    if (on_speedSetBtn_clicked())
        close();
}

void SetWindow::receiveSliderMinMax(int min, int max)
{
    ui->minLine->setText(QString::number(min));
    ui->maxLine->setText(QString::number(max));
}
