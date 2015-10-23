#include "http_context_qt.hpp"

#include "http_request_qt.hpp"

#include <mbgl/platform/log.hpp>

#include <QByteArray>
#include <QDir>
#include <QNetworkProxyFactory>
#include <QNetworkReply>
#include <QSslConfiguration>

namespace {
// Max number of request we are sending simultaneously to
// QNetworkAccessManager to not overload it with requests.
// Many get canceled before we need to make the actual get().
const int kPendingMax = 4;

} // namespace

namespace mbgl {

HTTPQtContext::HTTPQtContext() : m_manager(new QNetworkAccessManager(this))
{
    QNetworkProxyFactory::setUseSystemConfiguration(true);

#if QT_VERSION >= 0x050000
    m_ssl.setProtocol(QSsl::SecureProtocols);
#else
    // Qt 4 defines SecureProtocols as TLS1 or SSL3, but we don't want SSL3.
    m_ssl.setProtocol(QSsl::TlsV1);
#endif

    m_ssl.setCaCertificates(QSslCertificate::fromPath("ca-bundle.crt"));
    if (m_ssl.caCertificates().isEmpty()) {
        mbgl::Log::Warning(mbgl::Event::HttpRequest, "Could not load list of certificate authorities");
    }

    connect(m_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replyFinish(QNetworkReply*)));
}

void HTTPQtContext::request(HTTPQtRequest* req)
{
    QUrl url = req->url();

#if QT_VERSION < 0x050000
    if (m_pending.size() >= kPendingMax && m_pending.constFind(url) == m_pending.end()) {
        m_requestQueue.enqueue(req);
        return;
    }
#endif

    QPair<QNetworkReply*, QVector<HTTPQtRequest*>>& data = m_pending[url];
    QVector<HTTPQtRequest*>& requestsVector = data.second;
    requestsVector.append(req);

    if (requestsVector.size() > 1) {
        return;
    }

    QNetworkRequest networkRequest = req->networkRequest();
    networkRequest.setSslConfiguration(m_ssl);

    data.first = m_manager->get(networkRequest);
}

void HTTPQtContext::cancel(HTTPQtRequest* req)
{
#if QT_VERSION < 0x050000
    int queueIndex = m_requestQueue.indexOf(req);

    if (queueIndex != -1) {
        m_requestQueue.removeAt(queueIndex);
        return;
    }
#endif

    QUrl url = req->url();

    auto it = m_pending.find(url);
    if (it == m_pending.end()) {
        return;
    }

    QPair<QNetworkReply*, QVector<HTTPQtRequest*>>& data = it.value();
    QNetworkReply* reply = data.first;
    QVector<HTTPQtRequest*>& requestsVector = data.second;

    for (int i = 0; i < requestsVector.size(); ++i) {
        if (req == requestsVector.at(i)) {
            requestsVector.remove(i);
            break;
        }
    }

    if (requestsVector.empty()) {
        m_pending.erase(it);
#if QT_VERSION >= 0x050000
        reply->abort();
#else
        // XXX: We should be aborting the reply here
        // but a bug on Qt4 causes the connection of
        // other ongoing requests to drop if we call
        // abort() too often (and we do).
        Q_UNUSED(reply);
#endif
    }
}

void HTTPQtContext::replyFinish(QNetworkReply* reply)
{
    const QUrl& url = reply->request().url();

    auto it = m_pending.find(url);
    if (it == m_pending.end()) {
        reply->deleteLater();
#if QT_VERSION < 0x050000
        processQueue();
#endif
        return;
    }

    QVector<HTTPQtRequest*>& requestsVector = it.value().second;
    for (auto req : requestsVector) {
        req->handleNetworkReply(reply);
    }

    m_pending.erase(it);
    reply->deleteLater();
#if QT_VERSION < 0x050000
    processQueue();
#endif
}

#if QT_VERSION < 0x050000
void HTTPQtContext::processQueue()
{
    if (!m_requestQueue.isEmpty()) {
        request(m_requestQueue.dequeue());
    }
}
#endif

HTTPRequestBase* HTTPQtContext::createRequest(const Resource& resource,
        RequestBase::Callback callback, std::shared_ptr<const Response> response)
{
    return new HTTPQtRequest(this, resource, callback, response);
}

std::unique_ptr<HTTPContextBase> HTTPContextBase::createContext()
{
    return std::make_unique<HTTPQtContext>();
}

} // mbgl
