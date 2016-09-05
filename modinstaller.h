#ifndef MODINSTALLER_H
#define MODINSTALLER_H

#include <QObject>

class Installer : public QObject
{
    Q_OBJECT

signals:
    void finished();

public:
    Installer(QString src, QString dest, bool over);
    void run();
    void startCopying();

private:
    QString source;
    QString destination;
    bool overwrite;

    bool folderCopy(QString src, QString dest, bool over);
};

#endif // MODINSTALLER_H
