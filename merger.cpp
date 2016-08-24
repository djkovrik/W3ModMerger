#include "merger.h"
#include "singlemod.h"

#include <QDirIterator>

Merger::Merger(const QList<Mod*>& mods, const Settings* s, QObject* parent)
    : QObject(parent), modList(mods), settings(s)
{
    wcc = new QProcess(this);
    QString workingDir = settings->pathWcc;
    wcc->setWorkingDirectory(workingDir.remove("wcc_lite.exe"));

    //Merger slots
    connect(this, &Merger::mergingStarted, this, &Merger::prepare);
    connect(this, &Merger::uncookingFinished, this, &Merger::deleteImages);
    connect(this, &Merger::imagesDeleted, this, &Merger::cookAll);
    connect(this, &Merger::unpackingFinished, this, &Merger::buildCache);

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
        emit mergingFinished();
    }
    else {
        isRunning = true;
        emit mergingStarted();
    }
}

void Merger::prepare()
{
    toLog("MERGING PROCESS STARTED.");
    toStatusbar("Uncooking...");

    connect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &Merger::uncookNext);

    uncookNext();
}

void Merger::uncookNext()
{
    if ( !uncookQueue.isEmpty() ) {
        QStringList args = uncookQueue.dequeue();
        QString name = args.at(1);
        name.remove(0, 7);
        toLog("Uncooking: " + name);
        wcc->start( settings->pathWcc, args );
    }
    else {
        disconnect(wcc, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished), 0, 0);
        emit uncookingFinished();
    }
}

void Merger::deleteImages()
{
    toLog("Deleting unnecessary files...");

    QString format = settings->cmdUncook.mid( settings->cmdUncook.indexOf("-imgfmt=") + QString("-imgfmt=").size() , 3 );
    QDirIterator iter(settings->pathUncooked, QDirIterator::Subdirectories);
    QString current;

    while (iter.hasNext()) {
        current = iter.next();

        if (current.endsWith(format)) {
            QFile file(current);
            if ( file.remove() ) {
                toLog("   " + current + " removed.");
            }
        }
    }
    emit imagesDeleted();
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

    for (auto mod : chosenMods) {

        for (BundleFile* bundle : mod->bundles) {

            QFile source(bundle->fullPath);
            source.open(QIODevice::ReadOnly);

            for (FileRecord record : bundle->fileList) {

                if (record.filename.endsWith(".xbm")) {
                    continue;
                }

                toLog("   Extracting: " + record.filename);

                QString dirPath = record.filename.mid(0, record.filename.lastIndexOf('\\'));

                QDir d;
                d.mkpath(settings->pathCooked + Constants::SLASH + dirPath);
                QFile result(settings->pathCooked + Constants::SLASH + record.filename);

                if (! result.open(QIODevice::WriteOnly)) {
                    toLog("File is not opened: " + result.fileName());
                    continue;
                }

                QDataStream writer(&result);

                source.seek( record.globalOffset );

                QByteArray encoded = source.read( record.sizeCompressed );
                QByteArray decoded;
                decoded.reserve(record.sizeUncompressed);

                unpacker.unpack(record.algorithm,
                                record.sizeCompressed,
                                record.sizeUncompressed,
                                encoded.data(), decoded.data() );

                writer.writeRawData(decoded.data(), record.sizeUncompressed);
            }

            source.close();
        }
        mod->renameMerge();
    }

    emit unpackingFinished();
}

void Merger::buildCache()
{
    toLog("\nCache building started...");
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
            this, &Merger::finishing);

    QStringList args = parseCmdArgs(settings->cmdMetadata);
    wcc->start( settings->pathWcc, args );
}

void Merger::finishing()
{
    wcc->deleteLater();
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
