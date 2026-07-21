#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QUrl>
#include <QQueue>
#include <QUrlQuery>
#include <QJsonObject>

#include <QNetworkAccessManager>
#include <QNetworkRequest>

#include <QFuture>
#include <QPromise>

#include "property-macros.h"

#include "qaiutils_export.h"

class QAIUTILS_EXPORT QJsonApiClient : public QObject {
    QP_RW(QString   , baseUrl           , ""        )
    QP_RW(bool      , testing           , false     )
    QP_RW(bool      , anonymous         , true      )
    QP_RW(bool      , busy              , false     )
    QP_RW(int       , maxRedirects      , -1        )
    QP_RW(QString   , token             , ""        ) // Later this will be QSecureMem even though it is almost impopssible to not expose this to memory becasue networking is stupid on this end.
    Q_OBJECT
public:
    explicit QJsonApiClient(QObject *parent = nullptr);
    ~QJsonApiClient();

    enum Method {
        Get,
        Post,
        Put,
        Delete,
        Custom
    };
    Q_ENUMS(Method)

    enum AuthType{
        None,
        Bearer,
        Token,
        Basic
    };
    Q_ENUMS(AuthType)

    QFuture<QJsonObject> request(const QString &path,
                                 QJsonApiClient::Method method = QJsonApiClient::Get,
                                 QJsonApiClient::AuthType authType = QJsonApiClient::None,
                                 const QUrlQuery &query = QUrlQuery(),
                                 const QJsonObject &data = QJsonObject()
                                 );

    QMap<QString, QString> headers() const;
    void setHeaders(const QMap<QString, QString> &newHeaders);
    void appendHeader(const QString &key, const QString &val);
    int headersCount()const;
    bool headersIsEmpty()const;


protected:
  QP_RW(AuthType, authtype, AuthType::None)
  QUrl setupUrl();
  virtual void prepareSecureHeaders(QNetworkRequest &req, QJsonApiClient::AuthType authType)
  {
      Q_UNUSED(req);
      Q_UNUSED(authType);
  }

Q_SIGNALS:
    void headersChanged();

protected Q_SLOTS:
    void reqFinished();

private:
    QNetworkAccessManager   *m_nam = nullptr;

    QMap<QString, QString>  m_headers;
    QJsonObject m_errorJson{};

    struct ReqItem {
        QString                     path;
        QJsonApiClient::Method      method;
        QJsonApiClient::AuthType    authType;
        QJsonObject                 data;
        QUrlQuery                   query;
        QPromise<QJsonObject>       promise;
    };

    ReqItem *m_currentReqItem = nullptr;
    QQueue<ReqItem*> m_reqQueue;
    void processReq();
};
