#pragma once

#include <QObject>
#include <QMetaObject>

#include <QDataStream>

#include <QJsonObject>
#include <QJsonDocument>

#include <yaml-cpp/yaml.h>
#include <property-macros.h>

class QSdBaseParam : public QObject
{
    Q_OBJECT
    QP_RW(QString,        lastErrorString,    ""          )
public:
    explicit QSdBaseParam(QObject *parent = nullptr);
    ~QSdBaseParam() = default;

    Q_INVOKABLE void debugJson(QJsonDocument::JsonFormat format = QJsonDocument::Compact);
    Q_INVOKABLE void debugYaml();

    bool saveToJsonFile(const QString &fileName);
    QJsonObject loadFromJsonFile(const QString &fileName);
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject &jsonObject);

    bool saveToYamlFile(const QString &fileName);
    YAML::Node loadFromYamlFile(const QString &fileName);
    virtual YAML::Node toYaml() const;
    virtual void fromYaml(const YAML::Node &yamlNode);
};
