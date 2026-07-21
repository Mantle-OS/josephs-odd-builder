#include "qjsonapiclient.h"
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QFuture>
#include <QDebug>

QJsonApiClient::QJsonApiClient(QObject *parent) :
    QObject{parent},
    m_token("NONE"),
    m_nam{new QNetworkAccessManager{this}}
{
    // ret object on errors
    m_errorJson["status"] = "error";
    m_errorJson["respCode"] = 404;
    m_errorJson["errorString"] = "Unknown Error";
}

QJsonApiClient::~QJsonApiClient()
{

    if(!m_reqQueue.isEmpty()){
        for(auto *i : m_reqQueue){
            if(i){
                delete i;
                i = nullptr;
            }
        }
    }

    if (m_currentReqItem){
        delete m_currentReqItem;
        m_currentReqItem = nullptr;
    }

    if(m_nam){
        delete m_nam;
        m_nam = nullptr;
    }
}

QFuture<QJsonObject> QJsonApiClient::request(const QString &path,
                                             QJsonApiClient::Method method,
                                             QJsonApiClient::AuthType authType,
                                             const QUrlQuery &query,
                                             const QJsonObject &data)
{
    QPromise<QJsonObject> promise;
    auto future = promise.future();
    m_reqQueue.enqueue(
        new ReqItem{
            path,
            method,
            authType,
            data,
            query,
            std::move(promise)
        });
    if (!m_busy)
        processReq();
    return future;
}

QMap<QString, QString> QJsonApiClient::headers() const
{
    return m_headers;
}

void QJsonApiClient::setHeaders(const QMap<QString, QString> &newHeaders)
{
    if(!m_headers.isEmpty())
        m_headers.clear();
    m_headers = newHeaders;
    Q_EMIT headersChanged();

}
void QJsonApiClient::appendHeader(const QString &key, const QString &val)

{
    if(key.isEmpty() || val.isEmpty())
        return;

    if(m_headers.contains(key)){
        const QString &cVal = m_headers[key];
        if(cVal != val){
            m_headers[key] = val;
            Q_EMIT headersChanged();
            return;
        }
    }
    m_headers.insert(key, val);
}

int QJsonApiClient::headersCount() const
{
    return m_headers.count();
}

bool QJsonApiClient::headersIsEmpty() const
{
    return m_headers.isEmpty();
}

void QJsonApiClient::reqFinished()
{
    auto *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        qWarning() << "[api-client] reqFinished invoked without a valid QNetworkReply sender!";
        if (m_currentReqItem) {
            m_currentReqItem->promise.addResult(m_errorJson, 0);
            m_currentReqItem->promise.finish();
            delete m_currentReqItem;
            m_currentReqItem = nullptr;
        }
        processReq();
        return;
    }

    QJsonObject obj;
    QJsonParseError jerr;
    int respCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray respData = reply->readAll();
    QNetworkReply::NetworkError respError = reply->error();
    if (respError != QNetworkReply::NoError) {
        obj["respCode"] = respCode;
        obj["errorString"] = reply->errorString();

        // over write the error string if there is data. Figure out what the parser says
        if (!respData.isEmpty()) {
            QJsonDocument errDoc = QJsonDocument::fromJson(respData, &jerr);
            if (errDoc.isNull()){
                obj["status"] = "error";
                obj["errorString"] = QString("JSON Parse Failure: %1 \nData: %2")
                                         .arg(jerr.errorString()
                                         .arg(respData)
                                        );
            } /*else {
                obj["errorString"] = QString("UNknown Error Data: %1")
                                         .arg(respData);
            }*/
        }
    } else {
        if (respData.isEmpty()) {
            // Handle HTTP 204 or empty string validation checkpoints cleanly
            obj["status"] = "success";
            obj["respCode"] = respCode;
        } else {
            QJsonDocument jsonDoc = QJsonDocument::fromJson(respData, &jerr);
            if (jsonDoc.isNull()) {
                obj["status"] = "error";
                obj["respCode"] = respCode;
                obj["errorString"] = QString("JSON Parse Failure: %1").arg(jerr.errorString());
            } else {
                obj = jsonDoc.object();
            }
        }
    }

    m_currentReqItem->promise.addResult(obj, 0);
    m_currentReqItem->promise.finish();

    reply->deleteLater();

    delete m_currentReqItem;
    m_currentReqItem = nullptr;

    processReq();
}

QUrl QJsonApiClient::setupUrl()
{
    QUrl ret(QString("%1/%2")
                 .arg(m_baseUrl)
                 .arg(m_currentReqItem->path)
             );

    if(m_currentReqItem->method == Custom){
        // some api's are crazy with the way they use query
        for (auto it = m_currentReqItem->data.begin(); it != m_currentReqItem->data.end(); ++it) {
            QString encodedKey = QUrl::toPercentEncoding(it.key());
            QString encodedValue = QUrl::toPercentEncoding(it.value().toString());
            m_currentReqItem->query.addQueryItem(encodedKey, encodedValue);
        }
    }

    if(!m_currentReqItem->query.isEmpty())
        ret.setQuery(m_currentReqItem->query);

    if(ret.isValid())
        return ret;

    return QUrl{};
}

void QJsonApiClient::processReq()
{
    if (m_reqQueue.isEmpty()) {
        set_busy(false);
        return;
    }

    set_busy(true);
    m_currentReqItem = m_reqQueue.dequeue();
    QJsonObject obj = m_errorJson;

    if(m_currentReqItem->authType == QJsonApiClient::None)
        set_anonymous(true);
    else
        set_anonymous(false);

    QUrl url = setupUrl();
    // qDebug() << url.toString();
    if(!url.isValid() || url.isEmpty()){
        qWarning() << "[api-client] Invalid URL computed:" << url.toString();
        m_currentReqItem->promise.addResult(m_errorJson, 0);
        m_currentReqItem->promise.finish();
        delete m_currentReqItem;
        m_currentReqItem = nullptr;
        QMetaObject::invokeMethod(this, "processReq", Qt::QueuedConnection);
        return;
    }

    QNetworkRequest req(url);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setMaximumRedirectsAllowed(m_maxRedirects);

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");

    if(m_authtype != QJsonApiClient::None && m_token.isEmpty()){
        qWarning() << "QJsonApiClient auth type is not NONE and NO token was provided. Bailing request";
        m_currentReqItem->promise.addResult(m_errorJson, 0);
        m_currentReqItem->promise.finish();
        delete m_currentReqItem;
        m_currentReqItem = nullptr;
        QMetaObject::invokeMethod(this, "processReq", Qt::QueuedConnection);
        return;
    }
    switch(m_currentReqItem->authType) {
    case None:
        break;
    case Bearer:
        if (!m_token.isEmpty() && m_token != "NONE") {
            req.setRawHeader("Authorization", QByteArray("Bearer " + m_token.toLocal8Bit()));
        }
        break;
    case Token:
        if (!m_token.isEmpty() && m_token != "NONE") {
            req.setRawHeader("Authorization", QByteArray("Token " + m_token.toLocal8Bit()));
        }
        break;
    case Basic:
        if (!m_token.isEmpty() && m_token != "NONE") {
            req.setRawHeader("Authorization", QByteArray("Basic " + m_token.toLocal8Bit()));
        }
        break;
    }

    // Invoke secure injection hook
    prepareSecureHeaders(req, m_currentReqItem->authType);

    if(!m_headers.isEmpty()){
        for (auto i = m_headers.cbegin(), end = m_headers.cend(); i != end; ++i){
            req.setRawHeader(i.key().toLocal8Bit(), i.value().toLocal8Bit());
        }
    }

    QNetworkReply *reply = nullptr;
    QJsonDocument jdoc;
    switch (m_currentReqItem->method) {
    case QJsonApiClient::Get:
        reply = m_nam->get(req);
        break;
    case QJsonApiClient::Post:
        jdoc.setObject(m_currentReqItem->data);
        reply = m_nam->post(req, jdoc.toJson(QJsonDocument::Compact));
        break;
    case QJsonApiClient::Put:
        jdoc.setObject(m_currentReqItem->data);
        reply = m_nam->put(req, jdoc.toJson(QJsonDocument::Compact));
        break;
    case QJsonApiClient::Delete:
        reply = m_nam->deleteResource(req);
        break;
    case QJsonApiClient::Custom:
        reply = m_nam->get(req);//, "GET" with crazy Url );
        break;
    default:
        obj["errorString"] = "Unsupported HTTP method";
        delete m_currentReqItem;
        return;
    }

    if (reply)
        connect(reply, &QNetworkReply::finished, this, &QJsonApiClient::reqFinished);

}
