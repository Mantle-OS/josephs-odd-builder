#ifndef HUGGINGFACEREPOINFO_H
#define HUGGINGFACEREPOINFO_H

#include <QObject>
#include <QQmlEngine>

#include <property-macros.h>
#include <qjsonobject.h>


class HuggingFaceRepoBranch : public QObject
{
    Q_OBJECT
    QP_RO(QString, name             , ""    )
    QP_RO(QString, ref              , ""    )
    QP_RO(QString, targetCommit     , ""    )
    QML_ELEMENT
public:
    explicit HuggingFaceRepoBranch(QObject *parent = nullptr);
    ~HuggingFaceRepoBranch() = default;

    void fromJson(const QJsonObject &obj){
        if(obj.contains("name"))
            set_name(obj["name"].toString());

        if(obj.contains("ref"))
            set_ref(obj["ref"].toString());

        if(obj.contains("targetCommit"))
            set_ref(obj["targetCommit"].toString());
    }

    QJsonObject toJson(){
        QJsonObject ret{};
        ret["name"] = get_name();
        ret["ref"] = get_ref();
        ret["targetCommit"] = get_targetCommit();
        return ret;
    }

};

#endif // HUGGINGFACEREPOINFO_H
