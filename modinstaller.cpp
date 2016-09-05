#include "modinstaller.h"

#include <QDir>
#include <QThread>
#include <QProgressDialog>

Installer::Installer(QString src, QString dest, bool over) : source(src), destination(dest), overwrite(over)
{

}

void Installer::run()
{
    QProgressDialog* progress = new QProgressDialog("Installation, please wait...", 0, 0, 0);
    progress->setWindowModality(Qt::WindowModal);
    connect(this, &Installer::finished, progress, &QProgressDialog::close);
    connect(this, &Installer::finished, progress, &QProgressDialog::deleteLater);
    progress->show();

    QThread* thread = new QThread;

    moveToThread(thread);

    connect(thread, &QThread::started, this, &Installer::startCopying);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}

void Installer::startCopying()
{
    folderCopy(source, destination, overwrite);
    emit finished();
}

// Some stuff from stackoverflow
bool Installer::folderCopy(QString src, QString dest, bool over)
{
    QDir originDirectory(src);

    if (! originDirectory.exists()) {
        return false;
    }

    QDir destinationDirectory(dest);

    if(destinationDirectory.exists() && !over) {
        return false;
    }
    else if(destinationDirectory.exists() && over) {
        destinationDirectory.removeRecursively();
    }

    originDirectory.mkpath(dest);

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | \
             QDir::NoDotAndDotDot)) {
        QString destinationPath = dest + "/" + directoryName;
        originDirectory.mkpath(destinationPath);
        folderCopy(src + "/" + directoryName, destinationPath, over);
    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files)) {
        QFile::copy(src + "/" + fileName, dest + "/" + fileName);
    }

    QDir finalDestination(destination);
    finalDestination.refresh();

    if(finalDestination.exists()) {
        return true;
    }

    return false;
}
