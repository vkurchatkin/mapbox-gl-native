#ifndef QSQLITECACHE
#define QSQLITECACHE

#include <QAbstractNetworkCache>
#include <QBuffer>
#include <QScopedPointer>
#include <QSqlDatabase>

class QBuffer;
class QString;

class QSqliteCachePrivate : public QAbstractNetworkCache {
    Q_OBJECT

public:
    QSqliteCachePrivate(const QString& path);
    ~QSqliteCachePrivate() override = default;

    bool isValid() const;
    void setCacheMaximumSize(qint64 size);

    // SqliteCache implementation.
    qint64 cacheSize() const override;
    QIODevice* data(const QUrl& url) override;
    void insert(QIODevice* device) override;
    QNetworkCacheMetaData metaData(const QUrl& url) override;
    QIODevice* prepare(const QNetworkCacheMetaData& metaData) override;
    bool remove(const QUrl& url) override;
    void updateMetaData(const QNetworkCacheMetaData& metaData) override;

public slots:
    // SqliteCache implementation.
    void clear() override;

private:
    qint64 cacheSoftSize() const;
    void pruneOldEntries();

    QSqlDatabase m_cache;

    qint64 m_pageSize = 1024;
    qint64 m_maximumCacheSize = -1;

    QSqlQuery* m_select = nullptr;
    QSqlQuery* m_insert = nullptr;
    QSqlQuery* m_update = nullptr;
    QSqlQuery* m_delete = nullptr;
    QSqlQuery* m_collect = nullptr;

    QNetworkCacheMetaData m_metaData;
    QScopedPointer<QBuffer> m_buffer;
};

#endif
