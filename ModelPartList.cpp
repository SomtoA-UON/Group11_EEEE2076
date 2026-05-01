/**     @file ModelPartList.h
  *
  *     EEEE2076 - Software Engineering & VR Project
  *
  *     Template for model part list that will be used to create the trewview.
  *
  *     P Evans 2022
  */

#include "ModelPartList.h"
#include "ModelPart.h"
#include <QColor>

ModelPartList::ModelPartList(const QString& data, QObject* parent) : QAbstractItemModel(parent) {
    /* Have option to specify number of visible properties for each item in tree - the root item
     * acts as the column headers
     */
    rootItem = new ModelPart({ tr("Part Name"), tr("Visible"), tr("Colour"), tr("G"), tr("B") });
}



ModelPartList::~ModelPartList() {
    delete rootItem;
}


int ModelPartList::columnCount(const QModelIndex& parent) const {
    Q_UNUSED(parent);

    return rootItem->columnCount();
}


QVariant ModelPartList::data(const QModelIndex& index, int role) const {
    /* If the item index isnt valid, return a new, empty QVariant (QVariant is generic datatype
     * that could be any valid QT class) */
    if (!index.isValid())
        return QVariant();

    /* Get a pointer to the item referred to by the QModelIndex */
    ModelPart* item = static_cast<ModelPart*>(index.internalPointer());

    /* Return text data for display in each column */
    if (role == Qt::DisplayRole) {
        return item->data(index.column());
    }

    /* Return a colour swatch in the R column (column 2) using the part's RGB values.
     * Qt uses DecorationRole to render a coloured icon next to the text. */
    if (role == Qt::DecorationRole && index.column() == 2) {
        int r = item->data(2).toInt();
        int g = item->data(3).toInt();
        int b = item->data(4).toInt();
        return QColor(r, g, b);
    }

    return QVariant();
}


Qt::ItemFlags ModelPartList::flags(const QModelIndex& index) const {
    if (!index.isValid())
        return Qt::NoItemFlags;

    return QAbstractItemModel::flags(index);
}


QVariant ModelPartList::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}


QModelIndex ModelPartList::index(int row, int column, const QModelIndex& parent) const {
    ModelPart* parentItem;

    if (!parent.isValid() || !hasIndex(row, column, parent))
        parentItem = rootItem;              // default to selecting root 
    else
        parentItem = static_cast<ModelPart*>(parent.internalPointer());

    ModelPart* childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);


    return QModelIndex();
}


QModelIndex ModelPartList::parent(const QModelIndex& index) const {
    if (!index.isValid())
        return QModelIndex();

    ModelPart* childItem = static_cast<ModelPart*>(index.internalPointer());
    ModelPart* parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}


int ModelPartList::rowCount(const QModelIndex& parent) const {
    ModelPart* parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<ModelPart*>(parent.internalPointer());

    return parentItem->childCount();
}


ModelPart* ModelPartList::getRootItem() {
    return rootItem;
}



QModelIndex ModelPartList::appendChild(QModelIndex& parent, const QList<QVariant>& data) {
    ModelPart* parentPart;

    if (parent.isValid())
        parentPart = static_cast<ModelPart*>(parent.internalPointer());
    else {
        parentPart = rootItem;
        parent = createIndex(0, 0, rootItem);
    }

    beginInsertRows(parent, rowCount(parent), rowCount(parent));

    ModelPart* childPart = new ModelPart(data, parentPart);

    parentPart->appendChild(childPart);

    QModelIndex child = createIndex(0, 0, childPart);

    endInsertRows();

    emit layoutChanged();

    return child;
}

void ModelPartList::refreshModel() {
    beginResetModel();
    endResetModel();
}

void ModelPartList::removeItem(const QModelIndex& index) {
    if (!index.isValid())
        return;

    ModelPart* item = static_cast<ModelPart*>(index.internalPointer());
    ModelPart* parentPart = item->parentItem();

    /* Determine the parent QModelIndex (invalid = root) */
    QModelIndex parentIndex = parent(index);

    int row = index.row();
    beginRemoveRows(parentIndex, row, row);

    /* Remove from the parent's child list and delete the subtree */
    parentPart->removeChild(row);

    endRemoveRows();
}