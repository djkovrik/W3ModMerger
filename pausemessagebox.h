#ifndef PAUSEMESSAGEBOX_H
#define PAUSEMESSAGEBOX_H

#include <QDialog>

class QPushButton;
class QLabel;

class PauseMessagebox : public QDialog
{
    Q_OBJECT

public:
    PauseMessagebox(QString msg, QString dir, QString path, QWidget* parent = 0);
    ~PauseMessagebox();

private:
    QLabel* label;
    QPushButton* okButton;
    QPushButton* explorerButton;
};

#endif // PAUSEMESSAGEBOX_H
