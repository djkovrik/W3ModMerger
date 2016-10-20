#include "merger.h"
#include "unpacker.h"
#include "pausemessagebox.h"

#include <QDirIterator>

Merger::Merger(const QList<Mod*>& mods, const Settings* s, QObject* parent)
    : QObject(parent), modList(mods), settings(s)
{
    shouldPause = settings->showPauseMessage;

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
        toLog( tr("No mods chosen!", "Log warning message.") );
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
    toLog( tr("Merging process started!", "Log message.") );

    nothingUncooked = uncookQueue.size() == 0;

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::uncookNext);

    uncookNext();
}

void Merger::uncookNext()
{
    toStatusbar( tr("Uncooking...", "Statusbar text (you can keep it untranslated if you want).") );

    if ( !uncookQueue.isEmpty() ) {
        QStringList args = uncookQueue.dequeue();
        QString name = args.at(1);
        name.remove(0, 7);
        toLog( tr("Uncooking: %1", "Log message (you can keep it untranslated if you want).").arg(name) );
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
                toLog( tr("   %1 removed.", "File removal message.").arg(current) );
            }
        }
    }

    pause( tr("Merging has been paused! Please delete unnecessary files and press \"OK\" to continue.", "Pause message (check Merger page for more info)."),
           "Uncooked",
           settings->pathUncooked,
           true );

    if (nothingUncooked) {
        emit skipCooking();
    }
    else {
        emit startCooking();
    }
}

void Merger::cookAll()
{
    toLog( tr("\nCooking started...", "Log message (you can keep it untranslated if you want)."));
    toStatusbar( tr("Cooking...", "Statusbar text (you can keep it untranslated if you want).") );

    QStringList args = parseCmdArgs(settings->cmdCook);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::unpackAll);

    wcc->start( settings->pathWcc, args );
}

void Merger::unpackAll()
{
    toLog( tr("\nUnpacking bundles...", "Log message (you can keep it untranslated if you want).") );
    toStatusbar( tr("Unpacking...", "Statusbar text.") );

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
    pause( tr("Merging has been paused again! Please delete unnecessary files and press \"OK\" to continue.", "Pause message (check Merger page for more info)."),
           "Cooked",
           settings->pathCooked,
           false );

    toLog( tr("\nCache building started...", "Log message.") );
    toStatusbar( tr("Cache building...", "Statusbar text.") );

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::packAll);

    QStringList args = parseCmdArgs(settings->cmdCache);
    wcc->start( settings->pathWcc, args );

}

void Merger::packAll()
{
    pause( tr("Merging has been paused again! Please delete unnecessary files and press \"OK\" to continue.", "Pause message (check Merger page for more info)."),
           "Cooked",
           settings->pathCooked,
           false );

    toLog( tr("\nPacking process started...", "Log message (you can keep it untranslated if you want)."));
    toStatusbar( tr("Packing...", "Statusbar text (you can keep it untranslated if you want).") );

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::generateMetadata);

    QStringList args = parseCmdArgs(settings->cmdPack);
    wcc->start( settings->pathWcc, args );
}

void Merger::generateMetadata()
{
    toLog( tr("\nMetadata creation started...", "Log message (you can keep it untranslated if you want).") );
    toStatusbar( tr("Generating metadata...", "Statusbar text (you can keep it untranslated if you want)."));

    disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::finish);

    QStringList args = parseCmdArgs(settings->cmdMetadata);
    wcc->start( settings->pathWcc, args );
}

void Merger::finish()
{
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

void Merger::pause(QString message, QString folder, QString path, bool showNextTime)
{
    if (shouldPause) {
        shouldPause = showNextTime;
        PauseMessagebox msg(message, folder, path);

        toStatusbar( tr("Paused...", "Statusbar text.") );
        toLog( tr("\nMerging is paused so you can delete unnecessary files from the %1 folder.", "Pause log message.").arg(folder) );

        if (folder == "Uncooked") {
            toLog( tr("Please remember which files were deleted because you have to delete the same files from the Cooked folder during the next pause.", "Pause log message.") );
        }

        msg.exec();
    }
}
