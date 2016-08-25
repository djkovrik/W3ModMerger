#ifndef MODLISTMODEL_H
#define MODLISTMODEL_H

#include "singlemod.h"

#include <QStyledItemDelegate>
#include <QAbstractTableModel>

enum Columns { CHECKBOX = 0, MOD_NAME, HAS_BUNDLES, HAS_CACHE, HAS_SCRIPTS, STATUS, NOTES_LAST };

class ModlistModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    ModlistModel(QList<Mod*>& mods, QObject* parent = 0);

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    int columnCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    Qt::DropActions supportedDropActions() const;

private:
    QList<Mod*>& modList;
};

#endif // MODLISTMODEL_H
