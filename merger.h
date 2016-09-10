#ifndef MERGER_H
#define MERGER_H

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
    bool nothingUncooked = true;
    bool shouldPause = false;

    void startMerging();
    void prepare();
    void uncookNext();
    void deleteImages();
    void cookAll();
    void unpackAll();
    void cacheBuild();
    void packAll();
    void generateMetadata();
    void finish();

signals:
    // Full process
    void mergingStarted();
    void startImagesDeleting();
    void startCooking();
    void mergingFinished();
    //No cooking & cache
    void skipCooking();

    //Messaging
    void toLog(QString message);
    void toStatusbar(QString message);

private:
    const QList<Mod*>& modList;
    QList<Mod*> chosenMods;
    QQueue<QStringList> uncookQueue;

    const Settings* settings;
    QProcess* wcc;

    QStringList parseCmdArgs(QString cmd);
    QStringList parseCmdArgs(QString cmd, QString path);
    void processOutput();
    void pause(QString message, QString folder, QString path, bool showNextTime);
};

#endif // MERGER_H
