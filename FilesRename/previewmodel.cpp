// previewmodel.cpp
#include "previewmodel.h"
#include <QColor>

PreviewModel::PreviewModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int PreviewModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_items.count();
}

int PreviewModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3; // 固定3列：原文件名、新文件名、状态
}

QVariant PreviewModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || index.row() >= m_items.size())
    {
        return QVariant();
    }
    const PreviewItem &item = m_items.at(index.row());
    switch(role)
    {
        case Qt::DisplayRole:
            switch(index.column())
            {
                case 0:
                    return item.original;
                case 1:
                    return item.newName;
                case 2:
                    return item.error;
            }
            break;
        case Qt::ForegroundRole:
            if(!item.valid)
            {
                return QColor(Qt::red);
            }
            if(!item.error.isEmpty())
            {
                return QColor(Qt::darkYellow);
            }
            return QColor(Qt::black);
        case Qt::TextAlignmentRole:
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        case Qt::ToolTipRole:
            if(!item.error.isEmpty())
            {
                return tr("错误: %1").arg(item.error);
            }
            break;
    }
    return QVariant();
}

QVariant PreviewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if(orientation == Qt::Horizontal && role == Qt::DisplayRole)
    {
        switch(section)
        {
            case 0:
                return tr("原文件名");
            case 1:
                return tr("新文件名");
            case 2:
                return tr("状态信息");
        }
    }
    return QVariant();
}

void PreviewModel::updateData(const QList<PreviewItem>& newItems)
{
    beginResetModel();
    m_items = newItems;
    endResetModel();
}

void PreviewModel::clear()
{
    beginResetModel();
    m_items.clear();
    endResetModel();
}
