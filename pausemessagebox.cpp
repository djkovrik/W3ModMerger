#include "pausemessagebox.h"

#include <QProcess>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>

PauseMessagebox::PauseMessagebox(QString msg, QString dir, QString path, QWidget* parent) : QDialog(parent)
{
    setWindowFlags(Qt::WindowTitleHint);

    label = new QLabel(msg);
    okButton = new QPushButton("OK");
    explorerButton = new QPushButton(tr("Open %1 folder", "Pause messagebox button.").arg(dir) );
    okButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    explorerButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept );
    connect(explorerButton, &QPushButton::clicked, this, [=]() { QProcess::startDetached("explorer.exe", QStringList(path)); } );

    QHBoxLayout* topLayout = new QHBoxLayout;
    QHBoxLayout* buttonLayout = new QHBoxLayout;

    topLayout->addWidget(label);
    buttonLayout->addSpacerItem(new QSpacerItem(20, 0, QSizePolicy::Maximum));
    buttonLayout->addWidget(explorerButton);
    buttonLayout->addWidget(okButton);

    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
}

PauseMessagebox::~PauseMessagebox()
{
    label->deleteLater();
    okButton->deleteLater();
    explorerButton->deleteLater();
}
