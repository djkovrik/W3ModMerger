/* File format:
 * [header] (19-20 bytes)
 * [archive name #1][filename #1][filename #2]...
 * [archive name #2][filename #1][filename #2]...
 * ...
 * [\0\0] - final terminator
 * [unnecessary data]
 *
 * All strings are terminated with \0
 */

#ifndef METADATASTORE_H
#define METADATASTORE_H

#include <QStringList>
#include <QFile>

struct MetadataStore
{
    void setFile(QString filename);
    bool fileIsNotSupported();
    void parse();

    bool isValid() const { return bundlesList.size() > 0; }

    QFile source;
    QStringList bundlesList;
    QStringList filesList;
};

#endif // METADATASTORE_H
