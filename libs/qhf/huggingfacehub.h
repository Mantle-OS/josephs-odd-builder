#ifndef HUGGINGFACEHUB_H
#define HUGGINGFACEHUB_H

#include <QObject>
#include <QQmlEngine>
#include <QJsonArray>
#include <QJsonObject>

#include <property-macros.h>
#include <pointer-macros.h>
#include <objectmodel.h>
#include <qdownloader.h>

#include "huggingfacepackagemanifest.h"
#include "huggingfaceusermanager.h"
#include "huggingfacecache.h"
#include "huggingfaceapi.h"

class HuggingFaceHub : public QObject
{
    Q_OBJECT
    QP_PTR_RO(HuggingFaceCache,         cache           )
    QP_PTR_RO(HuggingFaceUserManager,   userManager     )
    QP_PTR_RO(QDownloader,              downloadManager )
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit HuggingFaceHub(QObject *parent = nullptr) :
        QObject{parent},
        m_cache{new HuggingFaceCache{this}},
        m_userManager{new HuggingFaceUserManager{this}},
        m_downloadManager{new QDownloader{this}},
        m_api{new HuggingFaceApi{this}}
    {}
    ~HuggingFaceHub()
    {
        connect(m_userManager, &HuggingFaceUserManager::loggedIn,
                this, &HuggingFaceHub::loggedIn);
    }

    Q_INVOKABLE void login(){

    }


    Q_INVOKABLE void search(const QString &ns, const QString &repo){
        auto *item = m_cache->get_cache()->getByUid(QString("%1/%2").arg(ns).arg(repo));
        if(m_api){
            m_api->repoByNS(ns, repo).then([&](QJsonObject obj){
                if(obj.contains("id")){
                    if(!item)
                        item = new HuggingFacePackageManifest;

                    item->fromJson(obj);
                    auto *i = m_cache->get_cache()->getByUid(obj["id"].toString());

                    if(i){
                        gatherRefs(ns, repo, i);
                    }
                }
            });
        }
    }


Q_SIGNALS:
    void searchDone();
    void loggedIn(bool);

private:
    HuggingFaceApi *m_api = nullptr;
    void gatherRefs(const QString &ns, const QString &repo, HuggingFacePackageManifest *package = nullptr){
        if(m_api && package){
            m_api->repoRefs(ns, repo).then([&](QJsonObject obj){                
                if(obj.contains("branches")){
                    QJsonArray branches =  obj["branches"].toArray();
                    for(const auto &jref : branches){
                        QJsonObject branchObj = jref.toObject();
                        if(branchObj.contains("name")){
                            HuggingFaceRepoBranch *branch = package->get_branches()->getByUid(obj["name"].toString());
                            if(branch){
                                branch->fromJson(branchObj);
                            }else{
                                branch = new HuggingFaceRepoBranch;
                                branch->fromJson(branchObj);
                                package->get_branches()->append(branch);
                            }
                        }
                    }
                    Q_EMIT searchDone();
                }
                if(m_cache->get_cache()->getByUid(package->get_uid())){
                    m_cache->updatePackage(package);
                } else {
                    m_cache->get_cache()->append(package);
                }
            });
        }
    }

};

#endif // HUGGINGFACEHUB_H
