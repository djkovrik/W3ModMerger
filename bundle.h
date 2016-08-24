#ifndef BUNDLE_H
#define BUNDLE_H

#include <QObject>
#include <QString>
#include <QList>

namespace Constants {
    const int MAGIC_WORD_SIZE   = 8;
    const int HEADER_SIZE       = 32;
    const int FILERECORD_SIZE   = 320;
    const int NAME_RESERVED     = 276; // = FILERECORD_SIZE - 44
}

struct Header {
    QString magic;      // 8
    int totalSize;      // 4
    int dummySize;      // 4
    int dataOffset;     // 4
    /*Unknown, SKIP*/   // 12
};

struct FileRecord {
    QString filename;
    int   sizeUncompressed; // 4
    int   sizeCompressed;   // 4
    int   globalOffset;     // 4
    /*Zeroes hash, SKIP*/   // 24
    /*unknown int, SKIP*/   // 4
    int   algorithm;        // 4
};                          // = 44

class BundleFile : public QObject
{
    Q_OBJECT

public:
    BundleFile(QString path, QObject* parent = 0);

    QString fullPath;
    Header head;
    QList<FileRecord> fileList;
};

#endif // BUNDLE_H
