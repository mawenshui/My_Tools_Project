// previewmodel.h
#ifndef PREVIEWMODEL_H
#define PREVIEWMODEL_H

#include <QAbstractTableModel>
#include "alldefine.h"

class PreviewModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit PreviewModel(QObject *parent = nullptr);

    // 模型接口
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    // 数据访问
    const QList<PreviewItem>& getItems() const
    {
        return m_items;
    }
    void updateData(const QList<PreviewItem>& newItems);
    void clear();

private:
    QList<PreviewItem> m_items;
};

#endif // PREVIEWMODEL_H
