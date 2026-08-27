#pragma once

#include <QObject>
#include <QMetaObject>

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QDataStream>
#include <QTextStream>

#include <QJsonObject>
#include <QJsonDocument>

#include <yaml-cpp/yaml.h>


class BaseQObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString lastErrorString READ lastErrorString WRITE setLastErrorString NOTIFY lastErrorStringChanged FINAL)

public:
    explicit BaseQObject(QObject *parent = nullptr);
    ~BaseQObject() = default;

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

    bool saveToBinaryFile(const QString &fileName);
    QByteArray loadFromBinaryFile(const QString &fileName);
    virtual QByteArray toBinary() const;
    virtual void fromBinary(const QByteArray &data);


    QString lastErrorString() const;
    void setLastErrorString(const QString &newLastErrorString);

    [[nodiscard]] static bool createDirFromFile(const QString &fileName);
    [[nodiscard]] static bool createDir(const QString &dirName);
    [[nodiscard]] static bool writeTextFile(const QString &fileName, const QString &content);
    [[nodiscard]] static QString readTextFile(const QString &fileName);
    [[nodiscard]] static bool fileExists(const QString &filePath);
    [[nodiscard]] static bool dirExists(const QString &dirPath);

Q_SIGNALS:
    void lastErrorStringChanged();

private:
    QString m_lastErrorString;
};

QDataStream &operator<<(QDataStream &ds, const BaseQObject &obj);
QDataStream &operator>>(QDataStream &ds, BaseQObject &obj) ;