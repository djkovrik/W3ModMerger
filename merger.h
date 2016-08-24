#ifndef MERGER_H
#define MERGER_H

#include "settings.h"
#include "unpacker.h"

#include <QObject>
#include <QQueue>
#include <QProcess>

class Mod;
class Settings;

class Merger : public QObject
{
    Q_OBJECT

public:
    Merger(const QList<Mod*>& mods, const Settings* s, QObject* parent = 0);
    ~Merger();

    bool isRunning = false;

    void startMerging();
    void prepare();
    void uncookNext();
    void deleteImages();
    void cookAll();
    void unpackAll();
    void buildCache();
    void packAll();
    void generateMetadata();
    void finishing();

signals:
    void mergingStarted();
    void uncookingFinished();
    void imagesDeleted();
    void unpackingFinished();
    void mergingFinished();

    //Messaging
    void toLog(QString message);
    void toStatusbar(QString message);

private:
    const QList<Mod*>& modList;
    QList<Mod*> chosenMods;
    QQueue<QStringList> uncookQueue;

    const Settings* settings;
    Unpacker unpacker;
    QProcess* wcc;

    QStringList parseCmdArgs(QString cmd);
    QStringList parseCmdArgs(QString cmd, QString path);
    void processOutput();
};

#endif // MERGER_H
