#ifndef HUGGINGFACEAPI_H
#define HUGGINGFACEAPI_H

#include "qaitypes.h"
#include "qaiusersession.h"
#include "qsecuremem.h"
#include <QObject>
#include <QFuture>
#include <QJsonObject>
#include <QUrl>
#include <QUrlQuery>
#include <QString>
#include <QStringList>

#include <qjsonapiclient.h>

inline const QString khfBaseUrl = QStringLiteral("https://huggingface.co");
inline const QString khfApiBasePath = QStringLiteral("api");

class HuggingFaceApi : public QJsonApiClient
{
    Q_OBJECT

public:
    explicit HuggingFaceApi(QObject *parent = nullptr) :
        QJsonApiClient{parent}
    {
        set_baseUrl(khfBaseUrl);

        const QString token = qEnvironmentVariable("HF_TOKEN", QString{});
        if (!token.isEmpty())
            set_token(token);


    }

    ~HuggingFaceApi() override = default;

    enum RepoType : quint8 {
        Model = 0,
        Dataset,
        Space
    };
    Q_ENUM(RepoType)


    void setSecureSession(const QAiUserSession *session, const QString &profileName = "default") noexcept {
        m_activeSession = session;
        m_profileName = profileName;
    }




    QFuture<QJsonObject> trending()
    {
        return request(apiPath("trending"),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> login()
    {
        return request(apiPath("whoami-v2"),
                       QJsonApiClient::Get,
                       QJsonApiClient::Bearer);
    }

    QFuture<QJsonObject> modelsTagsByType()
    {
        return request(apiPath("models-tags-by-type"),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> models(int limit = 1000)
    {
        QUrlQuery query;

        if (limit > 0 && limit < 100000)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        return request(apiPath("models"),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> searchModels(const QString &modelName, quint16 limit = 10)
    {
        QUrlQuery query;

        if (!modelName.isEmpty())
            query.addQueryItem(QStringLiteral("search"), modelName);

        if (limit > 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        return request(apiPath("models"),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> searchModelsWithKeyWords(const QString &modelName,
                                                  const QStringList &keywords,
                                                  quint16 limit = 10)
    {
        QUrlQuery query;

        if (!modelName.isEmpty())
            query.addQueryItem(QStringLiteral("search"), modelName);

        if (limit > 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        if (!keywords.isEmpty())
            query.addQueryItem(QStringLiteral("keywords"), keywords.join(QStringLiteral(",")));

        return request(apiPath("models"),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> searchModelsByAuthor(const QString &modelAuthor,
                                              quint16 limit = 10)
    {
        QUrlQuery query;

        if (!modelAuthor.isEmpty())
            query.addQueryItem(QStringLiteral("author"), modelAuthor);

        if (limit > 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        return request(apiPath("models"),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> repoByNS(const QString &ns,
                                  const QString &repo)
    {
        return request(apiPath(QStringLiteral("models/%1/%2")
                                   .arg(ns, repo)),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> repoById(const QString &repoId)
    {
        return request(apiPath(QStringLiteral("models/%1").arg(repoId)),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> repoRefs(const QString &ns,
                                  const QString &repo)
    {
        return request(apiPath(QStringLiteral("models/%1/%2/refs")
                                   .arg(ns, repo)),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> repoRefsById(const QString &repoId)
    {
        return request(apiPath(QStringLiteral("models/%1/refs").arg(repoId)),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> lfsFiles(const QString &ns,
                                  const QString &repo,
                                  qint32 limit = -1)
    {
        QUrlQuery query;

        if (limit > 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        return request(apiPath(QStringLiteral("models/%1/%2/lfs-files")
                                   .arg(ns, repo)),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> lfsFilesById(const QString &repoId,
                                      qint32 limit = -1)
    {
        QUrlQuery query;

        if (limit > 0)
            query.addQueryItem(QStringLiteral("limit"), QString::number(limit));

        return request(apiPath(QStringLiteral("models/%1/lfs-files").arg(repoId)),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> tree(const QString &repoId,
                              const QString &rev = QStringLiteral("main"),
                              const QString &path = QString{},
                              bool recursive = true)
    {
        QUrlQuery query;

        if (recursive)
            query.addQueryItem(QStringLiteral("recursive"), QStringLiteral("1"));

        QString reqPath = QStringLiteral("models/%1/tree/%2")
                              .arg(repoId, rev);

        if (!path.isEmpty())
            reqPath += QStringLiteral("/") + path;

        return request(apiPath(reqPath),
                       QJsonApiClient::Get,
                       authForRequest(),
                       query);
    }

    QFuture<QJsonObject> commits(const QString &repoId,
                                 const QString &rev = QStringLiteral("main"))
    {
        return request(apiPath(QStringLiteral("models/%1/commits/%2")
                                   .arg(repoId, rev)),
                       QJsonApiClient::Get,
                       authForRequest());
    }

    QFuture<QJsonObject> requestAccess(const QString &ns,
                                       const QString &repo,
                                       const QJsonObject &data = QJsonObject{})
    {
        return request(QStringLiteral("%1/%2/ask-access").arg(ns, repo),
                       QJsonApiClient::Post,
                       QJsonApiClient::Bearer,
                       QUrlQuery{},
                       data);
    }

    QUrl resolveFileUrl(const QString &repoId,
                        const QString &rev,
                        const QString &path) const
    {
        return QUrl(QStringLiteral("%1/%2/resolve/%3/%4")
                        .arg(khfBaseUrl, repoId, rev, path));
    }

protected:
    void prepareSecureHeaders(QNetworkRequest &req, QJsonApiClient::AuthType authType) override
    {
        if (authType == QJsonApiClient::None)
            return;

        // If a secure global session is active, override m_token lookup logic completely
        if (m_activeSession && m_activeSession->isActive()) {
            QSecureMem secureToken = m_activeSession->credential(QAi::Provider::HuggingFace, m_profileName);

            if (!secureToken.isEmpty()) {
                QByteArray authHeaderHeaderValue = "Bearer ";
                secureToken.appendTo(&authHeaderHeaderValue);

                req.setRawHeader("Authorization", authHeaderHeaderValue);

                // Securely scrub the temporary compilation buffer array instantly
                sodium_memzero(authHeaderHeaderValue.data(), authHeaderHeaderValue.size());
                return;
            }
        }
    }
    QString apiPath(const QString &path) const
    {
        QString p = path;
        while (p.startsWith(QLatin1Char('/')))
            p.remove(0, 1);
        return QStringLiteral("%1/%2").arg(khfApiBasePath, p);
    }

    QJsonApiClient::AuthType authForRequest() const
    {
        if (!get_token().isEmpty() || (m_activeSession && m_activeSession->isActive()))
            return QJsonApiClient::Bearer;
         return QJsonApiClient::None;
    }

private:
    const QAiUserSession* m_activeSession{nullptr};
    QString               m_profileName{"default"};
};

#endif // HUGGINGFACEAPI_H