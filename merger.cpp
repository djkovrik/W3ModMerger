#include "merger.h"
#include "unpacker.h"

#include <QDirIterator>

Merger::Merger(const QList<Mod*>& mods, const Settings* s, QObject* parent)
    : QObject(parent), modList(mods), settings(s)
{
    wcc = new QProcess(this);
    QString workingDir = settings->pathWcc;
    wcc->setWorkingDirectory(workingDir.remove("wcc_lite.exe"));

    //Merger slots
    connect(this, &Merger::mergingStarted, this, &Merger::prepare);
    connect(this, &Merger::startImagesDeleting, this, &Merger::deleteImages);
    connect(this, &Merger::startCooking, this, &Merger::cookAll);
    connect(this, &Merger::skipCooking, this, &Merger::unpackAll);

    //Process slots
    connect(wcc, &QProcess::readyReadStandardOutput, this, &Merger::processOutput);
    connect(wcc, &QProcess::readyReadStandardError,  this, &Merger::processOutput);

    connect(wcc, &QProcess::errorOccurred,
        [=](QProcess::ProcessError error) {
            toLog("Process errror: " + QString::number( error ));
        }
    );
}

Merger::~Merger()
{

}

void Merger::startMerging()
{
    for (auto mod : modList) {
        if (mod->checked) {
            chosenMods.append(mod);
            if (mod->hasCache) {
                uncookQueue.enqueue( parseCmdArgs(settings->cmdUncook, mod->folderPathNative) );
            }
        }
    }

    if (chosenMods.size() == 0) {
        toLog("No mods chosen!");
        chosenMods.clear();
        uncookQueue.clear();
        finish();
    }
    else {
        isRunning = true;
        emit mergingStarted();
    }
}

void Merger::prepare()
{
    toLog("MERGING PROCESS STARTED.");

    nothingUncooked = uncookQueue.size() == 0;

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::uncookNext);

    uncookNext();
}

void Merger::uncookNext()
{
    toStatusbar("Uncooking...");

    if ( !uncookQueue.isEmpty() ) {
        QStringList args = uncookQueue.dequeue();
        QString name = args.at(1);
        name.remove(0, 7);
        toLog("Uncooking: " + name);
        wcc->start( settings->pathWcc, args );
    }
    else {
        disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);
        emit startImagesDeleting();
    }
}

void Merger::deleteImages()
{
    QString format = settings->cmdUncook.mid( settings->cmdUncook.indexOf("-imgfmt=") + QString("-imgfmt=").size() , 3 );
    QDirIterator iter(settings->pathUncooked, QDirIterator::Subdirectories);
    QString current;

    while (iter.hasNext()) {
        current = iter.next();

        if (current.endsWith(format, Qt::CaseInsensitive)) {
            QFile file(current);
            if ( file.remove() ) {
                toLog("   " + current + " removed.");
            }
        }
    }

    if (nothingUncooked) {
        emit skipCooking();
    }
    else {
        emit startCooking();
    }
}

void Merger::cookAll()
{
    toLog("\nCooking started...");
    toStatusbar("Cooking...");

    QStringList args = parseCmdArgs(settings->cmdCook);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::unpackAll);

    wcc->start( settings->pathWcc, args );
}

void Merger::unpackAll()
{
    toLog("Unpacking bundles...");
    toStatusbar("Unpacking...");

    Unpacker* unpack = new Unpacker(chosenMods, settings);

    if (nothingUncooked) {
        connect(unpack, &Unpacker::finished, this, &Merger::packAll);
    } else {
        connect(unpack, &Unpacker::finished, this, &Merger::cacheBuild);
    }

    connect(unpack, &Unpacker::toLog, this, &Merger::toLog);
    connect(unpack, &Unpacker::finished, unpack, &Unpacker::deleteLater);

    unpack->run();
}

void Merger::cacheBuild()
{
    toLog("Cache building started...");
    toStatusbar("Cache building...");

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::packAll);

    QStringList args = parseCmdArgs(settings->cmdCache);
    wcc->start( settings->pathWcc, args );

}

void Merger::packAll()
{
    toLog("Packing process started...");
    toStatusbar("Packing...");

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::generateMetadata);

    QStringList args = parseCmdArgs(settings->cmdPack);
    wcc->start( settings->pathWcc, args );
}

void Merger::generateMetadata()
{
    toLog("Metadata creation started...");
    toStatusbar("Generating metadata...");

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::finish);

    QStringList args = parseCmdArgs(settings->cmdMetadata);
    wcc->start( settings->pathWcc, args );
}

void Merger::finish()
{
    toLog("MERGING PROCESS FINISHED!");
    toStatusbar(" ");
    isRunning = false;
    emit mergingFinished();
}

/** UTILS **/

QStringList Merger::parseCmdArgs(QString cmd)
{
    QStringList templateArgs;
    templateArgs << cmd.split(" ");

    QStringList parsedArgs;

    for (QString arg : templateArgs) {
        arg.replace("%UNCOOK%", settings->pathUncooked);
        arg.replace("%COOK%", settings->pathCooked);
        arg.replace("%PACK%", settings->pathPacked);
        arg.replace("%NAME%", settings->mergedModName);
        parsedArgs << arg;
    }

    return parsedArgs;
}

QStringList Merger::parseCmdArgs(QString cmd, QString path)
{
    QStringList templateArgs;
    templateArgs << cmd.split(" ");

    QStringList parsedArgs;

    for (QString arg : templateArgs) {
        arg.replace("%MOD%", path);
        arg.replace("%UNCOOK%", settings->pathUncooked);
        parsedArgs << arg;
    }

    return parsedArgs;
}

void Merger::processOutput()
{
    toLog( QString(wcc->readAllStandardOutput()) );
    toLog( QString(wcc->readAllStandardError()) );
}
