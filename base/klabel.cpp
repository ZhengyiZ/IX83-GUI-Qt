#include "klabel.h"

// a clickable label
kLabel::kLabel(const QString &labelText, QWidget *parent)
    :QLabel(parent)
{
    this->setText(labelText);
    connect(this, SIGNAL(clicked()), parent, SLOT(kLabelClicked()));
}

kLabel::~kLabel()
{

}

void kLabel::mousePressEvent(QMouseEvent*)
{
    emit clicked();
}
