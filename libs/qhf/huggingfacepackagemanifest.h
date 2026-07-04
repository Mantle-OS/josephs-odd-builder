#pragma once

#include <QObject>
#include <QQmlEngine>

#include <QJsonObject>
#include <QJsonArray>

#include <qaiutils.h>
#include <pointer-macros.h>
#include <property-macros.h>
#include <objectmodel.h>

#include "huggingfacefilemanifest.h"
#include "huggingfacerepoinfo.h"

class HuggingFacePackageManifest : public QObject
{
    Q_OBJECT
    QP_RO(QString,                                          uid,            ""      ) // example   "modelId": "stabilityai/stable-diffusion-xl-base-1.0",
    QP_RO(bool,                                             isPrivate,      false   )
    QP_RO(QString,                                          pipelineTag,    ""      ) //
    QP_RO(int,                                              likes,          0       )
    QP_RO(QString,                                          author,         ""      ) // "author": "stabilityai",
    QP_RO(QString,                                          sha,            ""      ) //
    QP_RO(QString,                                          lastModified,   {}      ) //
    QP_RO(QString,                                          license,        ""      ) // cardData -> license
    QP_RO(bool,                                             isGated,        false   )
    QP_RO(bool,                                             isDisabled,     false   )

    QP_PTR_RO(ObjectListModel<HuggingFaceFileManifest>,     siblings                )
    QP_RO(QStringList,                                      spaces,         {}      )
    QP_RO(QString,                                          createdAt,      ""      )
    QP_RO(int,                                              usedStorage,    0       )

    QP_PTR_RO(ObjectListModel<HuggingFaceRepoBranch>,       branches                )
    // QP_RO(QString,                                          desciption,     ""      ) // cardData -> license
    QML_ELEMENT


public:
    explicit HuggingFacePackageManifest(QObject *parent = nullptr) :
        QObject{parent},
        m_siblings{new ObjectListModel<HuggingFaceFileManifest>{ this, "display", "rfilename"} },
        m_branches{new ObjectListModel<HuggingFaceRepoBranch>{this, "display", "name"}}
    {

    }





    ~HuggingFacePackageManifest()
    {
        if(!m_siblings->isEmpty())
            m_siblings->clear();

        if(m_siblings){
            delete m_siblings;
            m_siblings = nullptr;
        }

        if(!m_branches->isEmpty()){
            m_branches->clear();
        }
        if(m_branches){
            delete m_branches;
            m_branches = nullptr;
        }
    }

    void update(HuggingFacePackageManifest *other)
    {
        set_uid(other->get_uid());
        set_isPrivate(other->get_isPrivate());
        set_pipelineTag(other->m_pipelineTag);
        set_likes(other->get_likes());
        set_author(other->get_author());
        set_sha(other->get_sha());
        set_lastModified(other->get_lastModified());
        set_license(other->get_license());
        set_isGated(other->get_isGated());
        set_isDisabled(other->get_isDisabled());

        for(auto *i : other->get_siblings()->toList()){
            auto *ii = m_siblings->getByUid(i->get_rfilename());
            if(ii){
                ii->set_installed(i->get_installed());
                ii->set_outDir(i->get_outDir());
            }else{
                ii = new HuggingFaceFileManifest;
                ii->set_installed(i->get_installed());
                ii->set_outDir(i->get_outDir());
                ii->set_rfilename(i->get_rfilename());
                m_siblings->append(ii);
            }
        }
        if(!other->get_spaces().isEmpty()){
            for(const QString &space : other->get_spaces()){
                if(!m_spaces.contains(space)){
                    m_spaces.append(space);
                    // Q_EMIT spacesChanged()
                }
            }
        }
        set_createdAt(other->get_createdAt());
        set_usedStorage(other->get_usedStorage());


        if(!other->get_branches()->isEmpty()){
            for(auto *branch : other->get_branches()->toList()){
                if(!m_branches->contains(branch)){
                    m_branches->append(branch);
                }else{
                    auto *ii = m_branches->getByUid(branch->get_name());
                    ii->set_ref(branch->get_ref());
                    ii->set_targetCommit(branch->get_targetCommit());
                }
            }
        }
    }

    void fromJson(QJsonObject obj)
    {
        if(obj.contains("error")){
            qDebug() << "Goit a error bailing out" << obj["error"].toString();
            return;
        }

        if(!obj.contains("id") || !obj.contains("siblings")){
            qDebug() << "Misssing id property in main json object aborting";
            return;
        }
        set_uid(obj["id"].toString());
        if(obj.contains("private"))
            set_isPrivate(obj["private"].toBool());

        if(obj.contains("pipeline_tag"))
            set_pipelineTag(obj["pipeline_tag"].toString());

        if(obj.contains("likes"))
            set_likes(obj["likes"].toInt(0));

        if(obj.contains("author"))
            set_author(obj["author"].toString());

        if(obj.contains("sha"))
            set_sha(obj["sha"].toString());

        if(obj.contains("lastModified"))
            set_lastModified(obj["lastModified"].toString());

        if(obj.contains("gated"))
            set_isGated(obj["gated"].toBool());

        if(obj.contains("disabled"))
            set_isDisabled(obj["disabled"].toBool());

        if(obj.contains("cardData")){
            const QJsonObject cData = obj["cardData"].toObject();
            if(cData.contains("license"))
                set_license(cData["license"].toString());
        }

        if(obj.contains("siblings")){
            QJsonArray sibArr = obj["siblings"].toArray();
            for(const auto &sibVal: sibArr){
                QJsonObject sibObj = sibVal.toObject();
                if(sibObj.contains("rfilename")){
                    QString rfile = sibObj["rfilename"].toString();
                    if(rfile.contains(".safetensor") || rfile.contains(".gguf") || rfile.contains(".gguf") || rfile.contains(".md")){
                        HuggingFaceFileManifest *fManifest = m_siblings->getByUid(rfile);
                        if(!fManifest){
                            fManifest = new HuggingFaceFileManifest;
                            fManifest->set_rfilename(rfile);
                            m_siblings->append(fManifest);
                        }
                    }

                }
            }
        }
        if(obj.contains("spaces")){
            const QJsonArray spaceArr = obj["spaces"].toArray();
            for(const QJsonValueConstRef spaceVal : spaceArr){
                const QString space = spaceVal.toString();
                if(!m_spaces.contains(space))
                    m_spaces.append(space);
            }
        }

        if(obj.contains("createdAt"))
            set_createdAt(obj["createdAt"].toString());

        if(obj.contains("usedStorage"))
            set_usedStorage(obj["usedStorage"].toInt());


    }
    QJsonObject toJson()
    {
        QJsonObject ret{};
        ret["id"] = get_uid();
        ret["pipeline_tag"] = get_pipelineTag();
        ret["likes"] = get_likes();
        ret["author"] = get_author();
        ret["sha"] = get_sha();
        ret["lastModified"] = get_lastModified();
        ret["gated"] = get_isGated();
        ret["disabled"] = get_isDisabled();

        QJsonObject cardObj{};
        cardObj["license"] = get_license();
        ret["cardData"] = cardObj;

        if(!m_siblings->isEmpty()){
            QJsonArray sibArr{};
            for(auto *sib : m_siblings->toList()){
                QJsonObject sibObj{};
                sibObj["rfilename"] = sib->get_rfilename();
                sibArr.append(sibObj);
            }

            ret["siblings"] = sibArr;
        }

        if(!m_spaces.isEmpty()){
            QJsonArray spaceArr{};
            for(const QString &space : m_spaces){
                spaceArr.append(QJsonValue(space).toString());
            }
            ret["spaces"] = spaceArr;
        }

        ret["createdAt"] = get_createdAt();
        ret["usedStorage"] = get_usedStorage();

        return ret;
    }

Q_SIGNALS:


private:

};

