#ifndef KLABEL_H
#define KLABEL_H

#include <QObject>
#include <QLabel>

class kLabel : public QLabel
{
    Q_OBJECT
public:
    kLabel(const QString &labelText, QWidget *parent);
    ~kLabel();

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent*);
};

#endif // KLABEL_H
