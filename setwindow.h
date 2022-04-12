#ifndef SETWINDOW_H
#define SETWINDOW_H

#include <QWidget>
#include <QMessageBox>
#include <QIntValidator>

namespace Ui {
class SetWindow;
}

class SetWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SetWindow(QWidget *parent = nullptr);
    ~SetWindow();

private slots:
    void receiveSliderMinMax(int min, int max);

    void on_sliderSetBtn_clicked();

    void on_maxLine_returnPressed();

    void on_speedSetBtn_clicked();

signals:
    void sendSliderSettings(int min, int max);
    void sendFSPD(QString FSPD);

private:
    Ui::SetWindow *ui;
};

#endif // SETWINDOW_H
