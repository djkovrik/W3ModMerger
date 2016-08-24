#include "modlistmodel.h"

#include <QIcon>

ModlistModel::ModlistModel(QList<Mod*>& mods, QObject* parent) : QAbstractTableModel(parent), modList(mods)
{

}

int ModlistModel::rowCount(const QModelIndex& parent) const
{
    return modList.size();
}

int ModlistModel::columnCount(const QModelIndex& parent) const
{
    return Columns::NOTES_LAST + 1;
}

QVariant ModlistModel::data(const QModelIndex& index, int role) const
{
    QVariant result;

    if (!index.isValid()) {
        return result;
    }

    const Mod* currentMod = modList.at(index.row());

    static QIcon yesIcon(":/images/icons/yes.ico");
    static QIcon noIcon(":/images/icons/no.ico");

    if (role == Qt::BackgroundRole) {
        if (currentMod->modState == MERGED) {
            return QVariant(QColor(0, 255, 0, 40));
        }
        else if ((currentMod->modState == MERGED_PACK)) {
            return QVariant(QColor(0, 255, 0, 80));
        }
        if ((currentMod->modState == CORRUPTED)) {
            return QVariant(QColor(255, 0, 0, 150));
        }
    }

    if ( role == Qt::DecorationRole ) {
        switch( index.column() ) {
            case Columns::HAS_BUNDLES:
                result = ( currentMod->hasBundles ) ? yesIcon : noIcon;
                break;
            case Columns::HAS_CACHE:
                result = ( currentMod->hasCache ) ? yesIcon : noIcon;
                break;
            case Columns::HAS_SCRIPTS:
                result = ( currentMod->hasScripts ) ? yesIcon : noIcon;
                break;
        };
    }

    if ( role == Qt::CheckStateRole && index.column() == Columns::CHECKBOX ) {
        return currentMod->checked ? Qt::Checked : Qt::Unchecked;
    }

    if (role == Qt::FontRole ) {
        QFont font;

        if (currentMod->modState == MERGED_PACK) {
            font.setBold(true);
            font.setItalic(true);
        }
        else if (currentMod->modState == MERGED) {
            font.setItalic(true);
        }

        return font;
    }

    if (role != Qt::DisplayRole) {
        return result;
    }

    switch( index.column() ) {
        case Columns::MOD_NAME:
            result = currentMod->modName;
            break;
        case Columns::STATUS:
            if ( currentMod->modState == MERGED) {
                result = "Merged";
            }
            else if ( currentMod->modState == NOT_MERGED) {
                result = "Not merged";
            }
            else if (currentMod->modState == MERGED_PACK) {
                result = "Merged pack";
            }
            else if (currentMod->modState == CORRUPTED) {
                result = "Corrupted";
            }
            break;
        case Columns::NOTES_LAST:
            result = currentMod->notes;
            break;
    };

    return result;
}

QVariant ModlistModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case Columns::MOD_NAME:
                return tr("Mod name");
            case Columns::HAS_BUNDLES:
                return tr("Bundles");
            case Columns::HAS_CACHE:
                return tr("Textures");
            case Columns::HAS_SCRIPTS:
                return tr("Scripts");
            case Columns::STATUS:
                return tr("Status");
            case Columns::NOTES_LAST:
                return tr("Notes");
        }
    }

    return QVariant();
}

Qt::ItemFlags ModlistModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags result;

    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }

    result = QAbstractTableModel::flags(index);

    result |= Qt::ItemIsDragEnabled;
    result |= Qt::ItemIsDropEnabled;

    if (index.column() == Columns::CHECKBOX) {
        result |= Qt::ItemIsEnabled;
        result |= Qt::ItemIsUserCheckable;
    }

    return result;
}

bool ModlistModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == Qt::CheckStateRole) {
        modList.at( index.row() )->checked = value.toBool();
        emit dataChanged(index, index);
        return true;
    }

    return false;
}

Qt::DropActions ModlistModel::supportedDropActions() const
{
    return Qt::MoveAction;
}
