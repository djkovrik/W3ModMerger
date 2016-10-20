#ifndef SINGLEMOD_H
#define SINGLEMOD_H

#include "metadatastore.h"
#include "bundle.h"

#include <QDir>
#include <QFileInfo>

namespace Constants {
    const QString CONTENT_PFIX = "/content";
    const QString SCRIPTS_PFIX = "/scripts";
    const QString CACHE_PFIX = "/texture.cache";
    const QString METADATA_PFIX = "/metadata.store";
    const QString RENAME_PFIX = "-merged";
    const QString SLASH = "/";

    const QString MERGED_SCRIPTS_NAME = "mod0000_MergedFiles";

    const QString ICON_CHECK = "gui_new\\icons\\inventory";
    const QString XML_CHECK  = ".xml";
    const QString SWF_CHECK  = ".redswf";
}

enum State {NOT_MERGED = 0, MERGED, MERGED_PACK, CORRUPTED};

class Mod : public QObject
{
    Q_OBJECT

public:
    Mod(QString path, QObject* parent = 0);

    void renameMerge();
    void renameUnmerge();

    QString folderPath;
    QString folderPathNative;
    QString contentPath;
    QString scriptsPath;
    QString cachePath;
    QString modName;

    bool    hasScripts = false;
    bool    hasCache = false;
    bool    hasMetadata = false;
    bool    hasBundles = false;
    bool    isMergeable = false;
    bool    checked = false;
    State   modState = NOT_MERGED;

    QString notes;
    QFileInfoList mergedResources;
    MetadataStore metadata;
    QList<BundleFile*> bundles;
    QFileInfo* checker;
};

#endif // SINGLEMOD_H
