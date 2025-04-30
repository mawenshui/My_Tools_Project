// filedelegate.cpp
#include "filedelegate.h"

FileDelegate::FileDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

void FileDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                         const QModelIndex &index) const
{
    if(index.column() == COLUMN_NAME)
    {
        // 对文件名列特殊处理
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        // 获取文件信息
        QFileInfo fileInfo(index.data(QFileSystemModel::FilePathRole).toString());
        QString fileName = fileInfo.fileName();
        QString extension = fileInfo.suffix();
        // 计算文本绘制区域
        QRect textRect = opt.rect.adjusted(2, 0, -2, 0);
        // 绘制背景
        painter->save();
        if(opt.state & QStyle::State_Selected)
        {
            painter->fillRect(opt.rect, opt.palette.highlight());
        }
        else if(opt.state & QStyle::State_MouseOver)
        {
            QColor hoverColor = opt.palette.highlight().color();
            hoverColor.setAlpha(50);
            painter->fillRect(opt.rect, hoverColor);
        }
        painter->restore();
        // 绘制图标
        QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();
        QRect iconRect = opt.rect;
        iconRect.setWidth(opt.decorationSize.width());
        icon.paint(painter, iconRect, Qt::AlignCenter);
        // 调整文本区域
        textRect.setLeft(iconRect.right() + 4);
        // 智能截断文件名
        QString displayText = fileName;
        QFontMetrics fm(opt.font);
        int availableWidth = textRect.width() - 4;
        if(fm.horizontalAdvance(fileName) > availableWidth)
        {
            // 保留扩展名可见
            if(!extension.isEmpty())
            {
                int extWidth = fm.horizontalAdvance("." + extension);
                if(extWidth < availableWidth / 2)
                {
                    int baseWidth = availableWidth - extWidth - fm.horizontalAdvance("...");
                    QString baseName = fileInfo.completeBaseName();
                    baseName = fm.elidedText(baseName, Qt::ElideRight, baseWidth);
                    displayText = baseName + "..." + extension;
                }
                else
                {
                    displayText = fm.elidedText(fileName, Qt::ElideMiddle, availableWidth);
                }
            }
            else
            {
                displayText = fm.elidedText(fileName, Qt::ElideMiddle, availableWidth);
            }
        }
        // 绘制文本
        painter->save();
        if(opt.state & QStyle::State_Selected)
        {
            painter->setPen(opt.palette.highlightedText().color());
        }
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, displayText);
        painter->restore();
    }
    else
    {
        QStyledItemDelegate::paint(painter, option, index);
    }
}

QSize FileDelegate::sizeHint(const QStyleOptionViewItem &option,
                             const QModelIndex &index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(qMax(size.height(), 20)); // 最小行高
    return size;
}

QString FileDelegate::displayText(const QVariant &value, const QLocale &locale) const
{
    // 对大小列特殊处理
    if(value.type() == QVariant::LongLong || value.type() == QVariant::ULongLong)
    {
        qint64 size = value.toLongLong();
        if(size < 1024)
        {
            return QString("%1 B").arg(size);
        }
        else if(size < 1024 * 1024)
        {
            return QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
        }
        else if(size < 1024 * 1024 * 1024)
        {
            return QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
        }
        else
        {
            return QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
        }
    }
    return QStyledItemDelegate::displayText(value, locale);
}
