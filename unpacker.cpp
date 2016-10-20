/**
 *  Compression types:
 *  0 - Not compressed
 *  1 - Zlib
 *  2 - Doboz ???
 *  3 - Doboz
 *  4 - Lz4 (not used)
 *  5 - Lz4
 *
 *  UPDATE: looks like mods packed with lz4 only
 */

#include "unpacker.h"
#include "libs/lz4.h"

#include <QThread>

Unpacker::Unpacker(QList<Mod*>& list, const Settings* s) : modsList(list), settings(s)
{

}

void Unpacker::run()
{
    QThread* thread = new QThread;

    moveToThread(thread);

    connect(thread, &QThread::started, this, &Unpacker::startUnpacking);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);

    thread->start();
}

void Unpacker::startUnpacking()
{
    for (auto mod : modsList) {

        for (BundleFile* bundle : mod->bundles) {

            QFile source(bundle->fullPath);

            if (! source.open(QIODevice::ReadOnly)) {
                toLog( tr("File is not opened: %1", "Warns about failed file opening.").arg(bundle->fullPath) );
                continue;
            }

            for (FileRecord record : bundle->fileList) {

                toLog( tr("   Extracting: %1", "File extraction message.").arg(record.filename) );

                QString dirPath = record.filename.mid(0, record.filename.lastIndexOf('\\'));

                QDir d;
                d.mkpath(settings->pathCooked + Constants::SLASH + dirPath);
                QFile result(settings->pathCooked + Constants::SLASH + record.filename);

                if (! result.open(QIODevice::WriteOnly)) {
                    toLog( tr("File is not saved: %1", "Warns about failed file saving.").arg(result.fileName() ));
                    continue;
                }

                QDataStream writer(&result);

                source.seek( record.globalOffset );

                QByteArray encoded = source.read( record.sizeCompressed );
                QByteArray decoded;
                decoded.reserve(record.sizeUncompressed);

                unpack(record.algorithm,
                       record.sizeCompressed,
                       record.sizeUncompressed,
                       encoded.data(), decoded.data() );

                writer.writeRawData(decoded.data(), record.sizeUncompressed);
                result.close();
            }
            source.close();
        }
        mod->renameMerge();
    }
    emit finished();
}

void Unpacker::unpack(int algo, int sizec, int sizeu, char* buf_compressed, char* buf_uncompressed)
{
    if (algo == 5) {
        LZ4_decompress_safe(buf_compressed, buf_uncompressed, sizec, sizeu);
    }
    else {
        toLog("Unsupported compression algorithm: " + QString::number(algo));
    }
}
