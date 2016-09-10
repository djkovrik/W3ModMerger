#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "settings.h"
#include "modlistmodel.h"
#include "merger.h"

#include <QMainWindow>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QMultiMap>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    static QTextEdit* log;

    QList<Mod*> modListFull;
    QList<Mod*> modListMergeable;
    QMultiMap<QString, QString> conflicts;
    Settings* settings;
    Merger* merger;

signals:
    void modsListChanged();

private slots:
    // Buttons
    void on_buttonMods_clicked();
    void on_lineEditModsPath_textChanged(const QString& arg1);
    void on_actionSettings_triggered();
    void on_tableView_clicked(const QModelIndex& index);
    void on_tableView_customContextMenuRequested(const QPoint& pos);
    void on_treeWidget_customContextMenuRequested(const QPoint& pos);
    void on_buttonUp_clicked();
    void on_buttonDown_clicked();
    void on_buttonRecommended_clicked();
    void on_buttonDeselect_clicked();
    void on_buttonConflicts_toggled(bool checked);
    void on_buttonMerge_clicked();
    void on_buttonUnmerge_clicked();
    // Misc
    void sendToLog(const QString& str);
    void sendToStatusbar(const QString& str);
    void installMergedPack();
    void on_mergeFinished();
    void openInExplorer() const;
    void on_dragAndDrop(int src, int dest);

private:
    Ui::MainWindow* ui;
    QDir modsFolder;
    ModlistModel* model;
    int currentRow;
    int currentRowMenu;

    // Status bar
    QLabel* mergerLabel;
    QLabel* processingLabel;

    void readStoredSettings();
    void writeStoredSettings();
    void showReport();
    void scanModsFolder();
    void updateTableView();
    void handleOrderButtons();
    void handleControls();
    void cleanWorkingDirs() const;
    void checkForConflicts();
    void refreshConflictsReport();
    int indexByName(QString name) const;
    void closeEvent(QCloseEvent* event);
};

#endif // MAINWINDOW_H
