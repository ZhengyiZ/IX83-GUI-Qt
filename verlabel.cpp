#include "verlabel.h"

verLabel::verLabel(const QString &labelText, QWidget *parent)
    :QLabel(parent)
{
    this->setText(labelText);
    connect(this, SIGNAL(clicked()), this, SLOT(slotClicked()));
}

verLabel::~verLabel()
{

}

void verLabel::slotClicked()
{
    QMessageBox about;
    about.setWindowTitle("About");
    about.setText("This is a open-sourced program, written \n"
                  "in Qt 6.2.0 and complied by MSVC2019 x64.\n\n"
                  "Version: 2.1\n"
                  "Author: Zhengyi Zhan");
    about.setStandardButtons(QMessageBox::Ok);
    about.setDefaultButton(QMessageBox::Ok);
    about.setWindowFlags(Qt::WindowStaysOnTopHint);
    about.exec();
}

void verLabel::mousePressEvent(QMouseEvent*)
{
    emit clicked();
}
