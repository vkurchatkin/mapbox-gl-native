#ifndef QFILESOURCE
#define QFILESOURCE

#include <mbgl/storage/file_source.hpp>

#include <QMap>
#include <QMutex>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QSslConfiguration>
#include <QString>
#include <QThread>
#include <QUrl>
#include <QVector>

class QFileSourceWorkerPrivate : public QObject
{
    Q_OBJECT

public:
    QFileSourceWorkerPrivate();
    virtual ~QFileSourceWorkerPrivate() = default;

public slots:
    void cacheDatabaseSet(const QString& path, qint64 maximumSize);
    void handleUrlRequest(mbgl::Request*);
    void handleUrlCancel(mbgl::Request*);
    void replyFinish(QNetworkReply* reply);

private:
#if QT_VERSION < 0x050000
    void processQueue();

    QQueue<mbgl::Request*> m_requestQueue;
#endif
    QMap<QUrl, QPair<QNetworkReply*, QVector<mbgl::Request*>>> m_pending;
    QNetworkAccessManager *m_manager;
    QSslConfiguration m_ssl;
};

class QFileSourcePrivate : public QObject, public mbgl::FileSource
{
    Q_OBJECT

public:
    QFileSourcePrivate();
    virtual ~QFileSourcePrivate();

    void setAccessToken(const QString& token);
    void setCacheDatabase(const QString& path, qint64 maximumSize);

    // FileSource implementation.
    mbgl::Request* request(const mbgl::Resource&, Callback) override;
    void cancel(mbgl::Request*) override;

signals:
    void cacheDatabaseSet(const QString& path, qint64 maximumSize);
    void urlRequested(mbgl::Request*);
    void urlCanceled(mbgl::Request*);

private:
    std::string accessToken() const;

    QThread m_workerThread;

    std::string m_token;
    mutable QMutex m_tokenMutex;
};

#endif
