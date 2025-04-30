// filedelegate.h
#ifndef FILEDELEGATE_H
#define FILEDELEGATE_H

#include <QStyledItemDelegate>
#include <QPainter>
#include <QFileInfo>
#include <QTextLayout>
#include <QTextOption>
#include <QFileSystemModel>
#include "alldefine.h"

class FileDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit FileDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override;

    QString displayText(const QVariant &value, const QLocale &locale) const override;
};

#endif // FILEDELEGATE_H
