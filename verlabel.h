#ifndef VERLABEL_H
#define VERLABEL_H

#include <QObject>
#include <QLabel>
#include <QMessageBox>

class verLabel : public QLabel
{
    Q_OBJECT
public:
    verLabel(const QString &labelText, QWidget *parent);
    ~verLabel();

signals:
    void clicked();

public slots:
    void slotClicked();

protected:
    void mousePressEvent(QMouseEvent*);
};

#endif // VERLABEL_H
