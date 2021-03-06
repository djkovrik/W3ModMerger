#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "modinstaller.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QCloseEvent>
#include <QTextDocumentWriter>
#include <QDesktopServices>
#include <QMessageBox>

#include <algorithm>

QTextEdit* MainWindow::log = nullptr;

MainWindow::MainWindow(QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    log = ui->textEditLog;
    ui->treeWidget->hide();

    // Status bar
    processingLabel = new QLabel(this);
    processingLabel->setIndent(5);
    processingLabel->setFixedWidth(400);

    reportLabel = new QLabel(this);
    reportLabel->setIndent(5);
    reportLabel->setFixedWidth(150);
    reportLabel->setText( tr("Mergeable mods: X", "User should not see this.") );

    mergerLabel = new QLabel(this);
    mergerLabel->setAlignment(Qt::AlignHCenter);
    mergerLabel->setMinimumSize(mergerLabel->sizeHint());
    mergerLabel->setIndent(5);

    statusBar()->addWidget(processingLabel);
    statusBar()->addPermanentWidget(reportLabel);
    statusBar()->addPermanentWidget(mergerLabel);

    // Settings
    settings = new Settings(this);
    connect(settings, &Settings::toLog, this, &MainWindow::sendToLog);
    readStoredSettings();

    if ( !settings->isWccSpecified ) {
        settings->exec();
    }

    // Signals
    connect(ui->tableView, &ModTableView::itemDropped, this, &MainWindow::on_dragAndDrop);
    connect(this, &MainWindow::modsListChanged, this, &MainWindow::checkForConflicts);

    // Misc
    merger = new Merger(modListMergeable, settings, this);
    handleControls();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/** SLOTS **/

void MainWindow::on_buttonMods_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Mods folder path:", "Folder selection dialog title."), QDir::currentPath() );

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

    if ( modListMergeable.at(currentRowMenu)->modState == NOT_MERGED ) {
        QAction* actionHide = new QAction( tr("Hide from the list", "Mod context menu item."), this);
        connect(actionHide, &QAction::triggered, this, &MainWindow::markAsHidden);
        menu->addAction(actionHide);
    }

    QAction* actionExplorer = new QAction( tr("Open in Explorer", "Mod context menu item."), this);
    connect(actionExplorer, &QAction::triggered, this, &MainWindow::openInExplorer);
    menu->addAction(actionExplorer);

    menu->popup(ui->tableView->viewport()->mapToGlobal(pos));
}

void MainWindow::on_treeWidget_customContextMenuRequested(const QPoint& pos)
{
    QMenu* menu = new QMenu(this);
    QAction* actionExpand = new QAction( tr("Expand All", "Conflicts report context menu item."), this);
    QAction* actionCollapse = new QAction( tr("Collapse All", "Conflicts report context menu item."), this);

    menu->addAction(actionExpand);
    menu->addAction(actionCollapse);

    connect(actionExpand, &QAction::triggered,
        [=]() { ui->treeWidget->expandAll(); }
    );

    connect(actionCollapse, &QAction::triggered,
        [=]() { ui->treeWidget->collapseAll(); }
    );

    menu->popup(ui->treeWidget->viewport()->mapToGlobal(pos));
}

void MainWindow::on_textEditLog_customContextMenuRequested(const QPoint& pos)
{
    if (merger->isRunning) {
        return;
    }

    QMenu* menu = new QMenu(this);
    QAction* actionSaveLog = new QAction( tr("Save log as...", "Log context menu item."), this);
    menu->addAction(actionSaveLog);

    connect(actionSaveLog, &QAction::triggered, this, &MainWindow::saveLogToFile);

    menu->popup(log->viewport()->mapToGlobal(pos));
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

    emit modsListChanged();
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

    emit modsListChanged();
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

    emit modsListChanged();
}

void MainWindow::on_buttonSelectAll_clicked()
{
    for (auto mod : modListMergeable) {
        mod->checked = true;
    }

    handleControls();

    emit modsListChanged();
}

void MainWindow::on_buttonDeselect_clicked()
{
    for (auto mod : modListMergeable) {
        mod->checked = false;
    }

    handleControls();

    emit modsListChanged();
}

void MainWindow::on_buttonConflicts_toggled(bool checked)
{
    ui->treeWidget->setVisible(checked);
    ui->buttonMerge->setVisible(!checked);
    ui->buttonUnmerge->setVisible(!checked);
    ui->tableView->horizontalHeader()->setStretchLastSection(!checked);

    if (checked) {
        ui->tableView->hideColumn(Columns::STATUS);
        ui->tableView->hideColumn(Columns::NOTES_LAST);
    }
    else {
        ui->tableView->showColumn(Columns::STATUS);
        ui->tableView->showColumn(Columns::NOTES_LAST);
    }

    checkForConflicts();
}

void MainWindow::on_buttonMerge_clicked()
{
    if (!settings->isWccSpecified) {
        sendToLog( tr("wcc_lite.exe not found! Please check Mod Merger settings", "Log warning message.") );
        return;
    }

    QDir d;
    d.mkpath(settings->pathUncooked);
    d.mkpath(settings->pathCooked);
    d.mkpath(settings->pathPacked);

    /** MERGING **/

    /// SAVE MERGING ORDER
    if (settings->saveMergingOrder) {

        settings->mergingOrder.clear();
        for (auto mod : modListMergeable) {
            settings->mergingOrder << mod->modName;
        }
    }

    connect(merger, &Merger::mergingStarted, this, &MainWindow::handleControls);
    connect(merger, &Merger::toLog, this, &MainWindow::sendToLog);
    connect(merger, &Merger::toStatusbar, this, &MainWindow::sendToStatusbar);

    if (settings->autoInstallEnabled) {
        connect(merger, &Merger::mergingFinished, this, &MainWindow::installMergedPack);
    }
    else {
        connect(merger, &Merger::mergingFinished, this, &MainWindow::on_mergeFinished);
    }

    log->clear();
    merger->startMerging();
}

void MainWindow::on_buttonUnmerge_clicked()
{
    for (auto mod : modListMergeable)
        if (mod->modState == MERGED) {
            sendToLog( tr("Unmerging %1%2", "Log message (looks like Unmerging modName...)").arg(mod->modName).arg("...") );
            mod->modState = NOT_MERGED;
            mod->renameUnmerge();
        }

    QString mergedModFolder = modsFolder.absolutePath() + Constants::SLASH + settings->mergedModName;
    QDir mergedPack(mergedModFolder);
    mergedPack.removeRecursively();

    sendToLog( tr("%1 removed.", "Mod removal log message.").arg(mergedModFolder) );
    sendToLog( tr("Unmerging finished.", "Log message.") );
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

void MainWindow::installMergedPack()
{
    sendToLog( tr("Installation...", "Log message.") );
    QString src = settings->pathPacked + Constants::SLASH + settings->mergedModName;
    QString dest = modsFolder.absolutePath() + Constants::SLASH + settings->mergedModName;

    Installer* install = new Installer(src, dest, true);
    connect(install, &Installer::finished, this, &MainWindow::on_mergeFinished);
    connect(install, &Installer::finished, this, [=]() { sendToLog(  tr("Merged pack installed to: %1", "Log message.").arg(dest) );});
    connect(install, &Installer::finished, install, &Installer::deleteLater);
    install->run();
}

void MainWindow::on_mergeFinished()
{
    sendToLog( tr("Merging process finished!", "Log message.") );
    sendToStatusbar(" ");
    showReport();
    scanModsFolder();
    handleControls();

    if (settings->autoCleanEnabled) {
        cleanWorkingDirs();
    }
}

void MainWindow::openInExplorer() const
{
    QString path = modListMergeable.at(currentRowMenu)->contentPath;

    QStringList args;
    args << QDir::toNativeSeparators(path);

    QProcess::startDetached("explorer.exe", args);
}

void MainWindow::markAsHidden()
{
    settings->hiddenMods.append( modListMergeable.at(currentRowMenu)->modName );

    scanModsFolder();
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
    modsListChanged();
}

void MainWindow::saveLogToFile()
{
    QString filename = QFileDialog::getSaveFileName( this, tr("Save Log as...", "Save dialog title."),
                                                     QDir::currentPath(),"Text files (*.txt)" );

    if ( filename.isEmpty() ) {
        return;
    }

    QTextDocumentWriter txtFile(filename, "plaintext");

    if ( txtFile.write(log->document()) ) {

        QMessageBox msgBox;
        msgBox.setText( tr("Log file has been created!", "Messagebox text.") );
        msgBox.setInformativeText( tr("Do you want to open it?", "Messagebox text (\"it\" means the log file).") );
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes", "Messagebox button."));
        msgBox.setButtonText(QMessageBox::No, tr("No", "Messagebox button."));

        if (msgBox.exec() == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
        }
    }
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
    settings->mergedModName     = storedSettings.value( "General/ModName", DEFAULT_NAME ).toString();

    settings->cmdUncook         = storedSettings.value( "General/cmdUncook", DEFAULT_UNCOOK ).toString();
    settings->cmdCook           = storedSettings.value( "General/cmdCook", DEFAULT_COOK ).toString();
    settings->cmdCache          = storedSettings.value( "General/cmdCache", DEFAULT_CACHE ).toString();
    settings->cmdPack           = storedSettings.value( "General/cmdPack", DEFAULT_PACK ).toString();
    settings->cmdMetadata       = storedSettings.value( "General/cmdMetadata", DEFAULT_META ).toString();

    settings->autoInstallEnabled = storedSettings.value( "General/AutoInstall", true ).toBool();
    settings->autoCleanEnabled   = storedSettings.value( "General/AutoClean", true ).toBool();
    settings->skipErrors         = storedSettings.value( "General/SkipErrors", true ).toBool();
    settings->dumpSwf            = storedSettings.value( "General/DumpSwf", true ).toBool();
    settings->saveMergingOrder   = storedSettings.value( "General/SaveMergingOrder", true ).toBool();
    settings->showPauseMessage   = storedSettings.value( "General/ShowPauseMessage", false ).toBool();

    settings->mergingOrder       = storedSettings.value( "List/MergingOrder", QVariant() ).toStringList();
    settings->hiddenMods         = storedSettings.value( "List/HiddenMods", QVariant() ).toStringList();

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
    storedSettings.setValue( "General/SaveMergingOrder", settings->saveMergingOrder );
    storedSettings.setValue( "General/ShowPauseMessage", settings->showPauseMessage );

    storedSettings.setValue( "List/MergingOrder", settings->mergingOrder );
    storedSettings.setValue( "List/HiddenMods", settings->hiddenMods );
}

void MainWindow::showReport()
{
    QString mergedPack = settings->pathPacked + Constants::SLASH +
                         settings->mergedModName + Constants::CONTENT_PFIX +
                         Constants::METADATA_PFIX;


    MetadataStore metadataAll;

    metadataAll.setFile(mergedPack);
    metadataAll.parse();

    sendToLog( tr("Merged files: %1", "Log message.").arg( QString::number(metadataAll.filesList.size()) ));

    for (auto mod : modListMergeable) {
        if (mod->checked) {
            for (QString file : mod->metadata.filesList) {
                if (!metadataAll.filesList.contains(file)) {
                    sendToLog( tr( "WARNING: %1 was not merged! [%2]", "Log warning message. 1 = filename, 2 = modname").arg(file).arg(mod->modName));
                }
            }
        }
    }
}

void MainWindow::scanModsFolder()
{
    ui->lineEditModsPath->setReadOnly( modsFolder.absolutePath().endsWith("Mods", Qt::CaseInsensitive) );

    if ( !(modsFolder.exists() && modsFolder.absolutePath().endsWith("Mods", Qt::CaseInsensitive)) ) {
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

        if ( settings->hiddenMods.contains( mod->modName ) ) {
            continue;
        }

        if ( (mod->isMergeable || mod->modState == MERGED ) && mod->modName != Constants::MERGED_SCRIPTS_NAME ) {
            modListMergeable.append(mod);
        }

        if (mod->modName == settings->mergedModName) {
            mod->modState = MERGED_PACK;
            mod->notes.clear();
        }
    }

    /// RESTORE MERGING ORDER
    if (settings->saveMergingOrder) {

        QList<Mod*> restoredOrder;
        QStringList savedOrder = settings->mergingOrder;

        // Move merged pack on top of the list
        Mod* temp = nullptr;
        int index = indexByName(Constants::DEFAULT_NAME);

        if (index != -1) {
            temp = modListMergeable.at(index);
            modListMergeable.removeAt(index);
            restoredOrder.append(temp);
        }

        // Handle the rest of the list
        for (auto name : savedOrder) {
            index = indexByName(name);

            if (index != -1) {
                temp = modListMergeable.at(index);
                modListMergeable.removeAt(index);
                restoredOrder.append(temp);
            }
        }
        restoredOrder.append(modListMergeable);
        modListMergeable.clear();
        modListMergeable.append(restoredOrder);
    }

    model = new ModlistModel(modListMergeable, this);
    ui->tableView->setModel(model);
    updateTableView();

    connect(model, &ModlistModel::dataChanged,
        [=]() { checkForConflicts(); }
    );

    reportLabel->setText( tr("Mergeable mods: %1", "Status bar text.").arg(QString::number(modListMergeable.size())) );
}

void MainWindow::updateTableView()
{
    ui->tableView->setColumnWidth(0, 23);
    ui->tableView->setColumnWidth(1, 300);
    ui->tableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->tableView->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableView->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableView->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->tableView->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
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

    ui->buttonConflicts->setEnabled( !merger->isRunning );
    ui->buttonMerge->setEnabled( !merger->isRunning && mergeEnabled );
    ui->buttonUnmerge->setEnabled( !merger->isRunning && unmergeEnabled);

    ui->tableView->setEnabled( !merger->isRunning );
    ui->treeWidget->setEnabled( !merger->isRunning );
    ui->buttonMods->setEnabled( !merger->isRunning );
    ui->buttonRecommended->setEnabled( !merger->isRunning );
    ui->buttonSelectAll->setEnabled( !merger->isRunning );
    ui->buttonDeselect->setEnabled( !merger->isRunning );
    ui->menuBar->setEnabled( !merger->isRunning );

    if (merger->isRunning) {
        mergerLabel->setText( tr("Merger: running", "Status bar text, displays current process state.") );
    }
    else {
        mergerLabel->setText( tr("Merger: not running", "Status bar text, displays current process state.") );
    }
    ui->tableView->setFocus();
}


void MainWindow::cleanWorkingDirs() const
{
    QDir uncooked(settings->pathUncooked);
    QDir cooked(settings->pathCooked);
    QDir packed(settings->pathPacked);

    uncooked.removeRecursively();
    cooked.removeRecursively();

    if (settings->autoInstallEnabled) {
        packed.removeRecursively();
    }
}

void MainWindow::checkForConflicts()
{
    conflicts.clear();

    for (auto mod : modListMergeable) {
        if (mod->checked) {
            for (auto fileName : mod->metadata.filesList) {
                conflicts.insert(fileName, mod->modName);
            }
        }
    }
    refreshConflictsReport();
}

void MainWindow::refreshConflictsReport()
{
    QTreeWidget* tree = ui->treeWidget;
    tree->clear();
    QTreeWidgetItem* parent = tree->invisibleRootItem();

    QTreeWidgetItem* temp_head;
    QTreeWidgetItem* temp_item;

    QList<QString> keyList = conflicts.keys();
    keyList.erase(std::unique(keyList.begin(), keyList.end()), keyList.end());
    std::sort(keyList.begin(), keyList.end());

    for (auto key : keyList) {
        QList<QString> values = conflicts.values(key);

        if (values.size() > 1) {
            temp_head = new QTreeWidgetItem(parent);
            temp_head->setText(0, key);

            for (auto iter = values.rbegin(); iter != values.rend(); iter++) {
                temp_item = new QTreeWidgetItem(temp_head);
                temp_item->setText(0, *iter);
            }
        }
    }
    ui->treeWidget->resizeColumnToContents(1);
}

int MainWindow::indexByName(QString name) const
{
    for (int i = 0; i < modListMergeable.size(); ++i) {
        if (modListMergeable.at(i)->modName == name) {
            return i;
            break;
        }
    }
    return -1;
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    writeStoredSettings();
    event->accept();
}
