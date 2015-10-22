#include "http_request_qt.hpp"

#include "http_context_qt.hpp"

#include <mbgl/storage/response.hpp>
#include <mbgl/util/parsedate.h>
#include <mbgl/util/string.hpp>
#include <mbgl/util/time.hpp>

#include <QByteArray>
#include <QNetworkReply>
#include <QPair>

namespace mbgl {

HTTPQtRequest::HTTPQtRequest(HTTPQtContext* context, const Resource& resource_,
    Callback callback, std::shared_ptr<const Response> response)
    : HTTPRequestBase(resource_, callback)
    , m_context(context)
    , m_existingResponse(response)
{
    m_context->request(this);
}

QUrl HTTPQtRequest::url() const
{
    return QUrl::fromPercentEncoding(QByteArray(resource.url.data(), resource.url.size()));
}

QNetworkRequest HTTPQtRequest::networkRequest() const
{
    QNetworkRequest req = QNetworkRequest(url());
    req.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    req.setRawHeader("User-Agent", "MapboxGL/1.0 [Qt]");

    if (!etag().isEmpty()) {
        req.setRawHeader("If-None-Match", etag().toLatin1());
    } else if (!modified().isEmpty()) {
        req.setRawHeader("If-Modified-Since", modified().toLatin1());
    }

    return req;
}

void HTTPQtRequest::handleNetworkReply(QNetworkReply *reply)
{
    std::shared_ptr<mbgl::Response> response = std::make_shared<mbgl::Response>();

    using Error = Response::Error;

    // Handle non-HTTP errors (i.e. like connection).
    if (reply->error() && reply->error() < 100) {
        response->error = std::make_unique<Error>(
            Error::Reason::Connection, reply->errorString().toStdString());
        notify(std::move(response));
        delete this;

        return;
    }

    if (reply->error() == 202 && url().scheme() == "file") {
        response->error = std::make_unique<Error>(
            Error::Reason::NotFound, reply->errorString().toStdString());
        notify(std::move(response));
        delete this;

        return;
    }

    QPair<QByteArray, QByteArray> line;
    foreach(line, reply->rawHeaderPairs()) {
        QString header = QString(line.first).toLower();

        if (header == "last-modified") {
            response->modified = Seconds(parse_date(line.second.constData()));
        } else if (header == "etag") {
            response->etag = std::string(line.second.constData(), line.second.size());
        } else if (header == "cache-control") {
            response->expires = parseCacheControl(line.second.constData());
        } else if (header == "expires") {
            response->expires = Seconds(parse_date(line.second.constData()));
        }
    }

    QByteArray bytes = reply->readAll();
    response->data = std::make_shared<std::string>(bytes.data(), bytes.size());

    int responseCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    switch(responseCode) {
    case 0: // Asset.
        break;
    case 200:
        break;
    case 304:
        if (!m_existingResponse) {
            break;
        }

        if (m_existingResponse->error) {
            response->error = std::make_unique<Error>(*m_existingResponse->error);
        }

        response->modified = m_existingResponse->modified;
        response->etag = m_existingResponse->etag;
        response->data = m_existingResponse->data;
        break;
    case 404:
        response->error = std::make_unique<Error>(
            Error::Reason::NotFound, "HTTP status code 404");
        break;
    default:
        response->error = std::make_unique<Error>(
            Error::Reason::Server, "HTTP status code " + util::toString(responseCode));
    }

    notify(std::move(response));
    delete this;
}

void HTTPQtRequest::cancel()
{
    m_context->cancel(this);
    delete this;
}

QString HTTPQtRequest::etag() const
{
    if (!m_existingResponse) {
        return QString();
    }

    return m_existingResponse->etag.c_str();
}

QString HTTPQtRequest::modified() const
{
    if (!m_existingResponse || m_existingResponse->modified == Seconds::zero()) {
        return QString();
    }

    return util::rfc1123(m_existingResponse->modified.count()).c_str();
}

} // namespace mbgl
