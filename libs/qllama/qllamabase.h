#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QQmlEngine>

#include <property-macros.h>

#include <qjsondocument.h>
#include <yaml-cpp/yaml.h>

class QLlamaBase : public QObject
{
    Q_OBJECT
    QP_RW(QString,        lastErrorString,    ""          )
public:
    explicit QLlamaBase(QObject *parent = nullptr);
    ~QLlamaBase() = default;

    Q_INVOKABLE void debugJson(QJsonDocument::JsonFormat format = QJsonDocument::Compact);
    Q_INVOKABLE void debugYaml();

    // JSON serialization/deserialization
    bool saveToJsonFile(const QString &fileName);
    QJsonObject loadFromJsonFile(const QString &fileName);
    virtual QJsonObject toJson() const;
    virtual void fromJson(const QJsonObject &jsonObject);

    // YAML serialization/deserialization
    bool saveToYamlFile(const QString &fileName);
    YAML::Node loadFromYamlFile(const QString &fileName);
    virtual YAML::Node toYaml() const;
    virtual void fromYaml(const YAML::Node &yamlNode);
};

