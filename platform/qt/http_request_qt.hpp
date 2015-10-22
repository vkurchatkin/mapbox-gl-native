#ifndef PLATFORM_QT_HTTP_REQUEST_QT
#define PLATFORM_QT_HTTP_REQUEST_QT

#include <mbgl/storage/http_request_base.hpp>

#include <QNetworkRequest>
#include <QUrl>

class QNetworkReply;

namespace mbgl {

class HTTPQtContext;
class Response;

class HTTPQtRequest : public HTTPRequestBase
{
public:
    HTTPQtRequest(HTTPQtContext*, const Resource&, Callback, std::shared_ptr<const Response>);
    virtual ~HTTPQtRequest() = default;

    QUrl url() const;
    QNetworkRequest networkRequest() const;

    void handleNetworkReply(QNetworkReply *reply);

    // HTTPRequestBase implementation.
    void cancel() final;

private:
    QString etag() const;
    QString modified() const;

    HTTPQtContext* m_context;
    std::shared_ptr<const Response> m_existingResponse;
};

} // namespace mbgl

#endif // PLATFORM_QT_HTTP_REQUEST_QT
