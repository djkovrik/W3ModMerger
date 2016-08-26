#include "metadatastore.h"

void MetadataStore::setFile(QString filename)
{
    source.setFileName(filename);
}

bool MetadataStore::fileIsNotSupported()
{
    source.seek(0);
    QByteArray magic = source.read(4);

    if ( magic.toHex() == QByteArray("0356544d") ) {
        return false;
    }

    return true;
}

void MetadataStore::parse()
{
    bundlesList.clear();
    filesList.clear();

    if (source.exists()) {
        source.open(QIODevice::ReadOnly);
    }
    else {
        return;
    }

    if (fileIsNotSupported()) {
        return;
    }

    source.seek(0);
    QByteArray allData = source.readAll();

    int startPos = 19;

    for (; startPos < 22; startPos++)
        if ( QChar(allData.at(startPos)).isLetterOrNumber() ) {
            break;
        }

    QByteArray terminator;
    terminator.append('\0').append('\0');
    int endPos = allData.indexOf(terminator, startPos);

    QByteArray textData = allData.mid(startPos, endPos - startPos);
    QList<QByteArray> rawList = textData.split('\0');

    QStringList fullList;

    for (QByteArray item : rawList) {
        fullList << QString(item);
    }

    for (QString item : fullList) {
        if ( item.endsWith(".bundle", Qt::CaseInsensitive) ) {
            bundlesList << item;
        }
        else {
            filesList << item;
        }
    }

    filesList.sort();
    source.close();
}
