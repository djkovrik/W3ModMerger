#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QCloseEvent>

QTextEdit* MainWindow::log = nullptr;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    log = ui->textEditLog;
    settings = new Settings(this);
    readStoredSettings();

    processingLabel = new QLabel;
    processingLabel->setIndent(5);
    processingLabel->setFixedWidth(400);

    mergerLabel = new QLabel;
    mergerLabel->setAlignment(Qt::AlignHCenter);
    mergerLabel->setMinimumSize(mergerLabel->sizeHint());
    mergerLabel->setIndent(5);

    statusBar()->addWidget(processingLabel);
    statusBar()->addPermanentWidget(mergerLabel);

    merger = new Merger(modListMergeable, settings, this);

    if ( !settings->isWccSpecified ) {
        settings->exec();
    }

    connect(ui->tableView, &ModTableView::itemDropped, this, &MainWindow::on_dragAndDrop);

    handleControls();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/** SLOTS **/

void MainWindow::on_buttonMods_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Mods folder path:"), QDir::currentPath() );

    if ( !directory.isEmpty() ) {
        ui->lineEditModsPath->setText( QDir::toNativeSeparators(directory) );
    }
}

void MainWindow::on_lineEditModsPath_textChanged(const QString& arg1)
{
    modsFolder.setPath(arg1);
    scanModsFolder();
}

void MainWindow::on_actionSettings_triggered()
{
    if (settings->exec()) {
        settings->fromWindowToVars();
    }
}

void MainWindow::on_tableView_clicked(const QModelIndex& index)
{
    currentRow = index.row();
    ui->tableView->selectRow(currentRow);

    handleControls();
    handleOrderButtons();
}

void MainWindow::on_tableView_customContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = ui->tableView->indexAt(pos);
    currentRowMenu = index.row();

    if (currentRowMenu == -1) {
        return;
    }

    QMenu* menu = new QMenu(this);
    QAction* action = new QAction("Open in Explorer", this);
    connect(action, &QAction::triggered, this, &MainWindow::openInExplorer);
    menu->addAction(action);
    menu->popup(ui->tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::on_buttonUp_clicked()
{
    if (merger->isRunning) {
        return;
    }

    modListMergeable.swap(currentRow, currentRow - 1);
    ui->tableView->selectRow(currentRow - 1);
    currentRow--;
    handleOrderButtons();
}

void MainWindow::on_buttonDown_clicked()
{
    if (merger->isRunning) {
        return;
    }

    modListMergeable.swap(currentRow, currentRow + 1);
    ui->tableView->selectRow(currentRow + 1);
    currentRow++;
    handleOrderButtons();
}

void MainWindow::on_buttonRecommended_clicked()
{
    for (auto mod : modListMergeable) {
        if ( mod->notes.isEmpty() && mod->modState == NOT_MERGED) {
            mod->checked = true;
        }
        else {
            mod->checked = false;
        }
    }

    handleControls();
}

void MainWindow::on_buttonDeselect_clicked()
{
    for (auto mod : modListMergeable) {
        mod->checked = false;
    }

    handleControls();
}

void MainWindow::on_buttonConflicts_clicked()
{
    checkForConflicts();

    /* Show conflicts report */
}

void MainWindow::on_buttonMerge_clicked()
{
    if (!settings->isWccSpecified) {
        sendToLog("wcc_lite.exe not found! Please check Mod Merger settings");
        return;
    }

    if (settings->autoCleanEnabled) {
        cleanWorkingDirs();
    }

    QDir d;
    d.mkpath(settings->pathUncooked);
    d.mkpath(settings->pathCooked);
    d.mkpath(settings->pathPacked);

    /** MERGING **/

    connect(merger, &Merger::mergingStarted, this, &MainWindow::handleControls);
    connect(merger, &Merger::toLog, this, &MainWindow::sendToLog);
    connect(merger, &Merger::toStatusbar, this, &MainWindow::sendToStatusbar);
    connect(merger, &Merger::mergingFinished, this, &MainWindow::on_mergeFinished);

    log->clear();
    merger->startMerging();
}

void MainWindow::on_buttonUnmerge_clicked()
{
    for (auto mod : modListMergeable)
        if (mod->modState == MERGED) {
            sendToLog("Unmerging " + mod->modName + "...");
            mod->modState = NOT_MERGED;
            mod->renameUnmerge();
        }

    QString mergedModFolder = modsFolder.absolutePath() + Constants::SLASH + settings->mergedModName;
    QDir mergedPack(mergedModFolder);
    mergedPack.removeRecursively();

    sendToLog(mergedModFolder + " removed.");
    sendToLog("Unmerging finished.");
    scanModsFolder();
    handleControls();
}

void MainWindow::sendToLog(const QString& str)
{
    log->append(str);
}

void MainWindow::sendToStatusbar(const QString& str)
{
    processingLabel->setText(str);
}

void MainWindow::on_mergeFinished()
{
    if (settings->autoInstallEnabled) {

        QString src = settings->pathPacked + Constants::SLASH + settings->mergedModName;
        QString dest = modsFolder.absolutePath() + Constants::SLASH + settings->mergedModName;

        folderCopy(src, dest, true);

        sendToLog("Merged pack installed to: " + dest);
    }

    showReport();
    scanModsFolder();
    handleControls();
}

void MainWindow::openInExplorer()
{
    QString path = modListMergeable.at(currentRowMenu)->contentPath;

    QStringList args;
    args << QDir::toNativeSeparators(path);

    QProcess::startDetached("explorer.exe", args);
}

void MainWindow::on_dragAndDrop(int src, int dest)
{
    if (src < 0 || src > modListMergeable.size() ||
            dest < 0 || dest > modListMergeable.size() ) {
        return;
    }

    Mod* temp = modListMergeable.at(src);
    modListMergeable.removeAt(src);
    modListMergeable.insert(dest, temp);

    ui->tableView->selectRow(dest);
    currentRow = dest;
    handleOrderButtons();
}

/** FUNCTIONS **/

void MainWindow::readStoredSettings()
{
    using namespace Constants;

    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, "W3ModMerger", "W3ModMergerSettings");
    QString currentDir = settings->currentDir();

    resize(storedSettings.value("MainWindow/Size", QSize(900, 700)).toSize());
    move(storedSettings.value("MainWindow/Position", QPoint(200, 200)).toPoint());

    QString path = storedSettings.value("General/ModsFolder", QString("")).toString();

    settings->pathWcc           = storedSettings.value( "General/WCC", QString(" ") ).toString();
    settings->pathUncooked      = storedSettings.value( "General/UncookedFolder", currentDir + DIR_UNCOOKED ).toString();
    settings->pathCooked        = storedSettings.value( "General/CookedFolder", currentDir + DIR_COOKED ).toString();
    settings->pathPacked        = storedSettings.value( "General/PackedFolder", currentDir + DIR_PACKED ).toString();
    settings->mergedModName     = storedSettings.value( "General/ModName", DEFAULT_MOD_NAME ).toString();

    settings->cmdUncook         = storedSettings.value( "General/cmdUncook", DEFAULT_UNCOOK ).toString();
    settings->cmdCook           = storedSettings.value( "General/cmdCook", DEFAULT_COOK ).toString();
    settings->cmdCache          = storedSettings.value( "General/cmdCache", DEFAULT_CACHE ).toString();
    settings->cmdPack           = storedSettings.value( "General/cmdPack", DEFAULT_PACK ).toString();
    settings->cmdMetadata       = storedSettings.value( "General/cmdMetadata", DEFAULT_META ).toString();

    settings->autoInstallEnabled = storedSettings.value( "General/AutoInstall", true ).toBool();
    settings->autoCleanEnabled   = storedSettings.value( "General/AutoClean", true ).toBool();
    settings->skipErrors         = storedSettings.value( "General/SkipErrors", true ).toBool();
    settings->dumpSwf            = storedSettings.value( "General/DumpSwf", true ).toBool();

    ui->lineEditModsPath->setText(path);
    settings->fromVarsToWindow();
}

void MainWindow::writeStoredSettings()
{
    QSettings storedSettings(QSettings::IniFormat, QSettings::UserScope, "W3ModMerger", "W3ModMergerSettings");

    storedSettings.setValue( "MainWindow/Size", size() );
    storedSettings.setValue( "MainWindow/Position", pos() );
    storedSettings.setValue( "General/ModsFolder", ui->lineEditModsPath->text() );

    storedSettings.setValue( "General/WCC", settings->pathWcc );
    storedSettings.setValue( "General/UncookedFolder", settings->pathUncooked );
    storedSettings.setValue( "General/CookedFolder", settings->pathCooked );
    storedSettings.setValue( "General/PackedFolder", settings->pathPacked );
    storedSettings.setValue( "General/ModName", settings->mergedModName );

    storedSettings.setValue( "General/cmdUncook", settings->cmdUncook );
    storedSettings.setValue( "General/cmdCook", settings->cmdCook );
    storedSettings.setValue( "General/cmdCache", settings->cmdCache );
    storedSettings.setValue( "General/cmdPack", settings->cmdPack );
    storedSettings.setValue( "General/cmdMetadata", settings->cmdMetadata );

    storedSettings.setValue( "General/AutoInstall", settings->autoInstallEnabled );
    storedSettings.setValue( "General/AutoClean", settings->autoCleanEnabled );
    storedSettings.setValue( "General/SkipErrors", settings->skipErrors );
    storedSettings.setValue( "General/DumpSwf", settings->dumpSwf );
}

void MainWindow::showReport()
{
    QString mergedPack = modsFolder.absolutePath() + Constants::SLASH +
                         settings->mergedModName + Constants::CONTENT_PFIX +
                         Constants::METADATA_PFIX;


    MetadataStore metadataAll;

    metadataAll.setFile(mergedPack);
    metadataAll.parse();

    sendToLog("Merged files: " + QString::number(metadataAll.filesList.size()));

    for (auto mod : modListMergeable) {
        if (mod->checked) {
            for (QString file : mod->metadata.filesList) {
                if (!metadataAll.filesList.contains(file)) {
                    sendToLog("WARNING: " + file + " was not merged! " + "[" + mod->modName + "]");
                }
            }
        }
    }
}

void MainWindow::scanModsFolder()
{
    ui->lineEditModsPath->setReadOnly( modsFolder.absolutePath().endsWith("Mods") );

    if ( !(modsFolder.exists() && modsFolder.absolutePath().endsWith("Mods")) ) {
        ui->tableView->setModel(nullptr);
        return;
    }

    modListFull.clear();
    modListMergeable.clear();

    modsFolder.setNameFilters(QStringList("mod*"));
    modsFolder.setFilter(QDir::Dirs);

    QFileInfoList mods = modsFolder.entryInfoList();

    for (QFileInfo m : mods) {
        Mod* mod = new Mod(m.absoluteFilePath(), this);
        modListFull.append(mod);

        if ( (mod->isMergeable || mod->modState == MERGED ) && mod->modName != Constants::MERGED_SCRIPTS_NAME ) {
            modListMergeable.append(mod);
        }

        if (mod->modName == settings->mergedModName) {
            mod->modState = MERGED_PACK;
        }
    }

    model = new ModlistModel(modListMergeable, this);
    ui->tableView->setModel(model);
    updateTableView();
}

void MainWindow::updateTableView()
{
    ui->tableView->setColumnWidth(0, 23);
    ui->tableView->setColumnWidth(1, 300);
    ui->tableView->setColumnWidth(2, 50);
    ui->tableView->setColumnWidth(3, 55);
    ui->tableView->setColumnWidth(4, 50);
    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setStretchLastSection(true);
    ui->tableView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}


void MainWindow::handleOrderButtons()
{
    ui->buttonDown->setEnabled( currentRow != modListMergeable.size() - 1 );
    ui->buttonUp->setEnabled( currentRow != 0 );
}

void MainWindow::handleControls()
{
    bool mergeEnabled = false;
    bool unmergeEnabled = false;

    for (auto mod : modListMergeable) {
        if (mod->checked && mod->modState == NOT_MERGED ) {
            mergeEnabled = true;
        }
        if (mod->modState == MERGED) {
            unmergeEnabled = true;
        }
    }

    // Block merging if merged pack exists
    for (auto mod : modListMergeable) {
        if (mod->modState == MERGED_PACK) {
            mergeEnabled = false;
            break;
        }
    }

    if (!settings->isWccSpecified) {
        mergeEnabled = false;
    }

    ui->buttonConflicts->setEnabled( !merger->isRunning && mergeEnabled );
    ui->buttonMerge->setEnabled( !merger->isRunning && mergeEnabled );
    ui->buttonUnmerge->setEnabled( !merger->isRunning && unmergeEnabled);

    ui->tableView->setEnabled( !merger->isRunning );
    ui->buttonMods->setEnabled( !merger->isRunning );
    ui->buttonRecommended->setEnabled( !merger->isRunning );
    ui->buttonDeselect->setEnabled( !merger->isRunning );
    ui->menuBar->setEnabled( !merger->isRunning );

    if (merger->isRunning) {
        mergerLabel->setText("Merger: running");
    }
    else {
        mergerLabel->setText("Merger: not running");
    }
    ui->tableView->setFocus();
}


void MainWindow::cleanWorkingDirs()
{
    QDir uncooked(settings->pathUncooked);
    QDir cooked(settings->pathCooked);
    QDir packed(settings->pathPacked);

    uncooked.removeRecursively();
    cooked.removeRecursively();
    packed.removeRecursively();
}

void MainWindow::checkForConflicts()
{
    conflictsList.clear();

    for (auto mod : modListMergeable) {
        if (mod->checked) {
            for (auto fileName : mod->metadata.filesList) {
                conflictsList.insert(fileName, mod->modName);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    writeStoredSettings();
    event->accept();
}

// Some stuff from stackoverflow
bool MainWindow::folderCopy(QString source, QString destination, bool overwrite)
{
    QDir originDirectory(source);

    if (! originDirectory.exists()) {
        return false;
    }

    QDir destinationDirectory(destination);

    if(destinationDirectory.exists() && !overwrite) {
        return false;
    }
    else if(destinationDirectory.exists() && overwrite) {
        destinationDirectory.removeRecursively();
    }

    originDirectory.mkpath(destination);

    foreach (QString directoryName, originDirectory.entryList(QDir::Dirs | \
             QDir::NoDotAndDotDot)) {
        QString destinationPath = destination + "/" + directoryName;
        originDirectory.mkpath(destinationPath);
        folderCopy(source + "/" + directoryName, destinationPath, overwrite);
    }

    foreach (QString fileName, originDirectory.entryList(QDir::Files)) {
        QFile::copy(source + "/" + fileName, destination + "/" + fileName);
    }

    /*! Possible race-condition mitigation? */
    QDir finalDestination(destination);
    finalDestination.refresh();

    if(finalDestination.exists()) {
        return true;
    }

    return false;
}
