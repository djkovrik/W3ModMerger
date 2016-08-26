#include "settings.h"
#include "ui_settings.h"

#include <QDir>
#include <QFileDialog>

Settings::Settings(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::Settings)
{
    ui->setupUi(this);
}

Settings::~Settings()
{
    delete ui;
}

void Settings::fromWindowToVars()
{
    pathWcc             = ui->lineEditWcc->text();
    pathUncooked        = ui->lineEditUncooked->text();
    pathCooked          = ui->lineEditCooked->text();
    pathPacked          = ui->lineEditPacked->text();
    mergedModName       = ui->lineEditModName->text();

    cmdUncook           = ui->lineCmdUncook->text();
    cmdCook             = ui->lineCmdCook->text();
    cmdCache            = ui->lineCmdCache->text();
    cmdPack             = ui->lineCmdPack->text();
    cmdMetadata         = ui->lineCmdMetadata->text();

    isWccSpecified      = pathWcc.endsWith(Constants::EXE_NAME, Qt::CaseInsensitive);
    autoInstallEnabled  = ui->checkBoxAutoInstall->isChecked();
    autoCleanEnabled    = ui->checkBoxCleanDirs->isChecked();
    skipErrors          = ui->checkBoxSkipErrors->isChecked();
    dumpSwf             = ui->checkBoxSwf->isChecked();
}

void Settings::fromVarsToWindow()
{
    ui->lineEditWcc->setText(pathWcc);
    ui->lineEditUncooked->setText(pathUncooked);
    ui->lineEditCooked->setText(pathCooked);
    ui->lineEditPacked->setText(pathPacked);
    ui->lineEditModName->setText(mergedModName);

    ui->lineCmdUncook->setText(cmdUncook);
    ui->lineCmdCook->setText(cmdCook);
    ui->lineCmdCache->setText(cmdCache);
    ui->lineCmdPack->setText(cmdPack);
    ui->lineCmdMetadata->setText(cmdMetadata);

    ui->checkBoxAutoInstall->setChecked(autoInstallEnabled);
    ui->checkBoxCleanDirs->setChecked(autoCleanEnabled);
    ui->checkBoxSkipErrors->setChecked(skipErrors);
    ui->checkBoxSwf->setChecked(dumpSwf);
}

QString Settings::currentDir() const
{
    QDir dir;
    return QDir::toNativeSeparators( dir.absolutePath() );
}

QString Settings::rebuildCmd()
{
    QString result = Constants::DEFAULT_UNCOOK;
    result.remove(" -skiperrors -dumpswf");

    if (skipErrors) {
        result += " -skiperrors";
    }

    if (dumpSwf) {
        result += " -dumpswf";
    }

    return result;
}

void Settings::on_buttonReset_clicked()
{
    using namespace Constants;

    //pathWcc           = "";
    pathUncooked        = currentDir() + DIR_UNCOOKED;
    pathCooked          = currentDir() + DIR_COOKED;
    pathPacked          = currentDir() + DIR_PACKED;
    mergedModName		= DEFAULT_MOD_NAME;

    cmdUncook           = DEFAULT_UNCOOK;
    cmdCook             = DEFAULT_COOK;
    cmdCache            = DEFAULT_CACHE;
    cmdPack             = DEFAULT_PACK;
    cmdMetadata         = DEFAULT_META;

    isWccSpecified      = pathWcc.endsWith(EXE_NAME, Qt::CaseInsensitive);
    autoInstallEnabled  = true;
    autoCleanEnabled    = true;
    skipErrors          = true;
    dumpSwf             = true;

    fromVarsToWindow();
}


void Settings::on_buttonWcc_clicked()
{
    QString directory = QFileDialog::getOpenFileName( this, tr("wcc_lite.exe location::"), QDir::currentPath(), Constants::EXE_NAME );

    if ( !directory.isEmpty() ) {
        pathWcc = QDir::toNativeSeparators(directory);
        ui->lineEditWcc->setText(pathWcc);
    }
}

void Settings::on_buttonUncooked_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Uncooked folder path:"), QDir::currentPath() );

    if ( !directory.isEmpty() ) {
        pathUncooked = QDir::toNativeSeparators(directory);
        ui->lineEditUncooked->setText(pathUncooked);
    }
}


void Settings::on_buttonCooked_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Cooked folder path:"), QDir::currentPath() );

    if ( !directory.isEmpty() ) {
        pathCooked = QDir::toNativeSeparators(directory);
        ui->lineEditCooked->setText(pathCooked);
    }
}
void Settings::on_buttonPacked_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Packed folder path:"), QDir::currentPath() );

    if ( !directory.isEmpty() ) {
        pathPacked = QDir::toNativeSeparators(directory);
        ui->lineEditPacked->setText(pathPacked);
    }
}

void Settings::on_lineEditWcc_textChanged(const QString& arg1)
{
    pathWcc = arg1;
    isWccSpecified = pathWcc.endsWith(Constants::EXE_NAME, Qt::CaseInsensitive);
    ui->buttonOk->setEnabled( isWccSpecified );

    if ( isWccSpecified ) {
        ui->lineEditWcc->setStyleSheet("QLineEdit { background: rgb(0, 255, 0); }");
    }
    else {
        ui->lineEditWcc->setStyleSheet("QLineEdit { background: rgb(255, 0, 0); }");
    }
}

void Settings::on_checkBoxSkipErrors_clicked()
{
    skipErrors = ui->checkBoxSkipErrors->isChecked();
    cmdUncook = rebuildCmd();
    fromVarsToWindow();
}

void Settings::on_checkBoxSwf_clicked()
{
    dumpSwf = ui->checkBoxSwf->isChecked();
    cmdUncook = rebuildCmd();
    fromVarsToWindow();
}
