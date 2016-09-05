#ifndef SETTINGS_H
#define SETTINGS_H

#include <QDialog>
#include <QLineEdit>

namespace Ui {
    class Settings;
}

namespace Constants {
    const QString EXE_NAME      = "wcc_lite.exe";

    const QString DIR_UNCOOKED  = "\\Uncooked";
    const QString DIR_COOKED    = "\\Cooked";
    const QString DIR_PACKED    = "\\Packed";
    const QString DEFAULT_NAME  = "mod0000_MergedPack";

    const QString DEFAULT_UNCOOK = "uncook -indir=%MOD%\\content -outdir=%UNCOOK% -imgfmt=tga -skiperrors -dumpswf";
    const QString DEFAULT_COOK   = "cook -platform=pc -mod=%UNCOOK% -basedir=%UNCOOK% -outdir=%COOK%";
    const QString DEFAULT_CACHE  = "buildcache textures -basedir=%UNCOOK% -platform=pc -db=%COOK%\\cook.db -out=%PACK%\\%NAME%\\content\\texture.cache";
    const QString DEFAULT_PACK   = "pack -dir=%COOK% -outdir=%PACK%\\%NAME%\\content\\";
    const QString DEFAULT_META   = "metadatastore -path=%PACK%\\%NAME%\\content\\";

    const int PATH_LENGTH_LIMIT  = 150;
}

class Settings : public QDialog
{
    Q_OBJECT

public:
    explicit Settings(QWidget* parent = 0);
    ~Settings();

private:
    Ui::Settings* ui;

    QString rebuildCmd();
    void checkFolderLength(const QString& name, const QString& path, QLineEdit* widget);

signals:
    void toLog(QString message);

public:
    void fromWindowToVars();
    void fromVarsToWindow();

    QString currentDir() const;

    // Stored in QSettings
    QString pathWcc;
    QString pathUncooked;
    QString pathCooked;
    QString pathPacked;
    QString mergedModName;

    QString cmdUncook;
    QString cmdCook;
    QString cmdCache;
    QString cmdPack;
    QString cmdMetadata;

    bool isWccSpecified;
    bool autoInstallEnabled;
    bool autoCleanEnabled;
    bool skipErrors;
    bool dumpSwf;

private slots:
    void on_buttonReset_clicked();
    void on_buttonWcc_clicked();
    void on_buttonUncooked_clicked();
    void on_buttonCooked_clicked();
    void on_buttonPacked_clicked();
    void on_lineEditWcc_textChanged(const QString& arg1);
    void on_checkBoxSkipErrors_clicked();
    void on_checkBoxSwf_clicked();
};

#endif // SETTINGS_H
