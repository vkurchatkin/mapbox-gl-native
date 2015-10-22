#ifndef PLATFORM_QT_HTTP_CONTEXT_QT
#define PLATFORM_QT_HTTP_CONTEXT_QT

#include <mbgl/storage/http_context_base.hpp>
#include <mbgl/storage/resource.hpp>

#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>
#include <QPair>
#include <QQueue>
#include <QSslConfiguration>
#include <QUrl>
#include <QVector>

namespace mbgl {

class HTTPRequestBase;
class HTTPQtRequest;

class HTTPQtContext : public QObject, public HTTPContextBase
{
    Q_OBJECT

public:
    HTTPQtContext();
    virtual ~HTTPQtContext() = default;

    // HTTPContextBase implementation.
    HTTPRequestBase* createRequest(const Resource&,
                                   RequestBase::Callback,
                                   std::shared_ptr<const Response>) final;

    void request(HTTPQtRequest*);
    void cancel(HTTPQtRequest*);

public slots:
    void replyFinish(QNetworkReply* reply);

private:
#if QT_VERSION < 0x050000
    void processQueue();
    QQueue<HTTPQtRequest*> m_requestQueue;
#endif

    QMap<QUrl, QPair<QNetworkReply*, QVector<HTTPQtRequest*>>> m_pending;
    QNetworkAccessManager *m_manager;
    QSslConfiguration m_ssl;
};

} // namespace mbgl

#endif // PLATFORM_QT_HTTP_CONTEXT_QT
