#include "ui/GalleryModel.h"

#include <QDateTime>
#include <QUrl>

GalleryModel::GalleryModel(CaptureDatabase* db, QObject* parent)
    : QAbstractListModel(parent)
    , m_db(db)
{
    refresh();
}

int GalleryModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_items.size();
}

QVariant GalleryModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_items.size())
        return {};
    const CaptureRecord& r = m_items.at(index.row());
    switch (role) {
    case FilePathRole:  return r.filePath;
    case FileUrlRole:   return QUrl::fromLocalFile(r.filePath);
    case TypeRole:      return r.type;
    case GameNameRole:  return r.gameName;
    case DateTextRole: {
        const QDateTime dt = QDateTime::fromString(r.createdAt, Qt::ISODate).toLocalTime();
        return dt.isValid() ? dt.toString(QStringLiteral("d MMM yyyy, HH:mm")) : r.createdAt;
    }
    case FavoriteRole:  return r.isFavorite;
    case ThumbnailRole: return r.thumbnailPath;
    case SourceRole:    return r.source;
    }
    return {};
}

QHash<int, QByteArray> GalleryModel::roleNames() const
{
    return {
        { FilePathRole,  "filePath" },
        { FileUrlRole,   "fileUrl" },
        { TypeRole,      "captureType" },
        { GameNameRole,  "gameName" },
        { DateTextRole,  "dateText" },
        { FavoriteRole,  "favorite" },
        { ThumbnailRole, "thumbnail" },
        { SourceRole,    "source" },
    };
}

void GalleryModel::setFilter(const QString& category, int gameId)
{
    m_category = category;
    m_gameId = gameId;
    refresh();
    emit filterChanged();
}

void GalleryModel::refresh()
{
    beginResetModel();
    m_items = m_db->listCaptures(m_category, m_gameId);
    endResetModel();
}

void GalleryModel::toggleFavorite(int row)
{
    if (row < 0 || row >= m_items.size())
        return;
    CaptureRecord& r = m_items[row];
    if (!m_db->setFavorite(r.id, !r.isFavorite))
        return;
    r.isFavorite = !r.isFavorite;
    const QModelIndex idx = index(row);
    emit dataChanged(idx, idx, { FavoriteRole });
    // A favorites view must drop unfavorited rows immediately.
    if (m_category == QLatin1String("favorites") && !r.isFavorite)
        refresh();
}

const CaptureRecord* GalleryModel::record(int row) const
{
    if (row < 0 || row >= m_items.size())
        return nullptr;
    return &m_items.at(row);
}

QVariantMap GalleryModel::get(int row) const
{
    const CaptureRecord* r = record(row);
    if (!r)
        return {};
    const QDateTime dt = QDateTime::fromString(r->createdAt, Qt::ISODate).toLocalTime();
    return {
        { QStringLiteral("filePath"),    r->filePath },
        { QStringLiteral("fileUrl"),     QUrl::fromLocalFile(r->filePath) },
        { QStringLiteral("captureType"), r->type },
        { QStringLiteral("gameName"),    r->gameName },
        { QStringLiteral("dateText"),    dt.isValid() ? dt.toString(QStringLiteral("d MMM yyyy, HH:mm")) : r->createdAt },
        { QStringLiteral("favorite"),    r->isFavorite },
        { QStringLiteral("thumbnail"),   r->thumbnailPath },
    };
}
