#include "bundle.h"

#include <QFile>

QFile& operator>>(QFile& input, Header& h)
{
    using namespace Constants;

    QByteArray buffer = input.read( MAGIC_WORD_SIZE );
    h.magic = QString(buffer.data());

    input.read( ( char* ) &h.totalSize, sizeof( h.totalSize ) );
    input.read( ( char* ) &h.dummySize, sizeof( h.dummySize ) );
    input.read( ( char* ) &h.dataOffset, sizeof( h.dataOffset ) );

    input.seek(HEADER_SIZE);

    return input;
}

QFile& operator>>(QFile& input, FileRecord& fr)
{
    using namespace Constants;

    char symbol;
    QByteArray buffer;

    do  {
        input.read( &symbol, 1 );
        buffer.append(symbol);
    }
    while ( symbol != '\0' );

    fr.filename = QString( buffer.data() );

    input.seek( input.pos() + NAME_RESERVED - fr.filename.size() - 1 );

    input.read( ( char* ) &fr.sizeUncompressed, sizeof( fr.sizeUncompressed ) );
    input.read( ( char* ) &fr.sizeCompressed, sizeof( fr.sizeCompressed ) );
    input.read( ( char* ) &fr.globalOffset, sizeof( fr.globalOffset ) );
    input.seek( input.pos() + 28 );
    input.read( ( char* ) &fr.algorithm, sizeof( fr.algorithm ) );

    return input;
}

BundleFile::BundleFile(QString path, QObject* parent) : QObject(parent), fullPath(path)
{
    using namespace Constants;

    QFile source(path);

    if (source.exists()) {
        source.open(QIODevice::ReadOnly);
    }

    source >> head;

    FileRecord temp;

    for ( int i = HEADER_SIZE; i < head.dataOffset + HEADER_SIZE; i += FILERECORD_SIZE ) {
        source >> temp;
        fileList.append(temp);
    }

    source.close();
}
