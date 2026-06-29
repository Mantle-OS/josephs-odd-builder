#include "qsdbaseparam.h"

#include <QMetaObject>
#include <QMetaProperty>

#include <QFile>
#include <QFileInfo>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

#include "qsdembedding.h"
#include "qsdlora.h"
#include "qaiutils.h"
#include "objectmodel.h"

#include <QDebug>

QSdBaseParam::QSdBaseParam(QObject *parent):
    QObject(parent)
{

}

void QSdBaseParam::debugJson(QJsonDocument::JsonFormat format){
    QJsonDocument jdoc;
    jdoc.setObject(toJson());
    qDebug().noquote() << "[JSON]\n" << jdoc.toJson(QJsonDocument::Indented).simplified().data();
}

void QSdBaseParam::debugYaml(){
    YAML::Node node = toYaml();
    YAML::Emitter emitter;
    emitter << node;
    qDebug().noquote() << "[YAML]\n" << QString::fromUtf8(emitter.c_str());
}

bool QSdBaseParam::saveToJsonFile(const QString &fileName)
{
    bool ret = false;
    QJsonDocument jdoc;
    jdoc.setObject(toJson());
    ret = QAiUtils::writeTextFile(fileName, jdoc.toJson());
    if(!ret)
        set_lastErrorString(QString("Failed writing file %1").arg(fileName));
    return ret;
}

QJsonObject QSdBaseParam::loadFromJsonFile(const QString &fileName)
{
    QJsonObject ret;
    auto data = QAiUtils::readTextFile(fileName).toLocal8Bit();
    if(!data.isEmpty()){
        QJsonParseError er;
        QJsonDocument jdoc = QJsonDocument::fromJson(data, &er);
        if(er.error == QJsonParseError::NoError){
            if(!jdoc.isEmpty()){
                ret = jdoc.object();
            }
        }else{
            set_lastErrorString(er.errorString());
        }
    }else{
        set_lastErrorString("No data in file json file.");
    }
    return ret;
}


QJsonObject QSdBaseParam::toJson() const {
    QJsonObject jsonObject;
    const QMetaObject *metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored() && property.isReadable()) {
            QVariant value = property.read(this);
            // skip object name it is not in the schema
            if(strcmp(property.name(), "objectName" ) == 0 ){
                continue;
            }
            if (value.typeId() == QMetaType::Int ||
                value.typeId() == QMetaType::Double ||
                value.typeId() == QMetaType::QString ||
                value.typeId() == QMetaType::Bool) {
                // Handle primitive types
                jsonObject[property.name()] = QJsonValue::fromVariant(value);
            }else if (value.typeId() == QMetaType::QStringList){
                const QStringList list = value.toStringList();
                if(!list.isEmpty()){
                    QJsonArray strList;
                    for (const QString &item : list) {
                        strList.push_back(item);
                    }
                    jsonObject[property.name()] = strList;
                }
            }else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
                auto li = qvariant_cast<ObjectListModelBase*>(value);
                if(!li->isEmpty()){
                    QJsonArray jsonArray;
                    for (auto i : li->toVarArray()){
                        if(i.canConvert<QSdBaseParam*>()){
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(i);
                            if(obj)
                                jsonArray.append(obj->toJson());
                        }else{
                            jsonArray.append(QJsonValue::fromVariant(i));
                        }
                    }
                    jsonObject[property.name()] = jsonArray;
                }
            }else if (value.typeId() == QMetaType::QVariantList){
                const QVariantList list = value.toList();
                if(!list.isEmpty()){
                    QJsonArray jsonArray;
                    for (const QVariant &item : list) {
                        if (item.canConvert<QSdBaseParam *>()) {
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(item);
                            if (obj)
                                jsonArray.append(obj->toJson());
                        }else{
                            jsonArray.append(QJsonValue::fromVariant(item));
                        }
                    }
                    jsonObject[property.name()] = jsonArray;
                }
            }else if (value.typeId() == QMetaType::QVariantMap){
                const QVariantMap map = value.toMap();
                if(!map.isEmpty()){
                    QJsonObject mapObject;
                    for (auto it = map.begin(); it != map.end(); ++it) {
                        if (it.value().canConvert<QSdBaseParam *>()) {
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(it.value());
                            if (obj)
                                mapObject[it.key()] = obj->toJson();
                        }else{
                            mapObject[it.key()] = QJsonValue::fromVariant(it.value());
                        }
                    }
                    jsonObject[property.name()] = mapObject;
                }
            }else if (value.canConvert<QSdBaseParam *>()){
                // Handle complex types
                QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(value);
                if (obj) {
                    jsonObject[property.name()] = obj->toJson();
                }
            }
        }
    }
    return jsonObject;
}

void QSdBaseParam::fromJson(const QJsonObject &jsonObject) {
    const QMetaObject *metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored() /*&& property.isWritable() && jsonObject.contains(property.name())*/ ) {
            QVariant value = property.read(this);
            if (value.typeId() == QMetaType::Int ||
                value.typeId() == QMetaType::Double ||
                value.typeId() == QMetaType::QString ||
                value.typeId() == QMetaType::Bool ||
                value.typeId() == QMetaType::IsEnumeration) {
                // Handle primitive types
                property.write(this, jsonObject[property.name()].toVariant());
            } else if (value.typeId() == QMetaType::QStringList){
                QJsonArray jsonArray = jsonObject[property.name()].toArray();
                if(!jsonArray.isEmpty()){
                    QStringList list;
                    for(auto i : jsonArray)
                        list.append(i.toString());

                    property.write(this, list);
                }
            }else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
                QJsonArray jsonArray = jsonObject[property.name()].toArray();
                if(!jsonArray.isEmpty()){
                    QObject *modelObj = qvariant_cast<QObject *>(value);
                    auto *list = qobject_cast<ObjectListModelBase *>(modelObj);
                    if(list){
                        list->clear();
                        const QMetaObject *itemMetaObject = list->itemMetaObject();
                        if (!itemMetaObject){
                            qWarning() << "Item meta-object not defined for" << list->objectName();
                            return;
                        }
                        for (const QJsonValue &jsonValue : jsonArray) {
                            if (jsonValue.isObject()) {
                                // // qDebug() << "List class name " << itemMetaObject->className() << "VS" <<  property.name();
                                // // FIXME There has to be a better way for reflection here
                                if(strcmp(itemMetaObject->className(), "QSdLora") == 0){
                                    auto *i = new QSdLora;
                                    i->fromJson(jsonValue.toObject());
                                    list->append(i);
                                }else if(strcmp(itemMetaObject->className(), "QSdEmbedding") == 0){
                                    auto *i = new QSdEmbedding;
                                    i->fromJson(jsonValue.toObject());
                                    list->append(i);
                                }
                            }
                        }
                        property.write(this, list->toVarArray());
                    }
                }
            }else if (value.typeId() == QMetaType::QVariantList) {
                QJsonArray jsonArray = jsonObject[property.name()].toArray();
                if(!jsonArray.isEmpty()){
                    QVariantList list;
                    for (const QJsonValue &jsonValue : jsonArray) {
                        if (jsonValue.isObject()) {
                            QSdBaseParam *item = new QSdBaseParam();
                            item->fromJson(jsonValue.toObject());
                            list.append(QVariant::fromValue(item));
                        } else {
                            list.append(jsonValue.toVariant());
                        }
                    }
                    property.write(this, list);
                }
            }else if (value.typeId() == QMetaType::QVariantMap) {
                QJsonObject mapObject = jsonObject[property.name()].toObject();
                if(!mapObject.isEmpty()){
                    QVariantMap map;
                    for (auto it = mapObject.begin(); it != mapObject.end(); ++it) {
                        if (it.value().isObject()) {
                            QSdBaseParam *item = new QSdBaseParam();
                            item->fromJson(it.value().toObject());
                            map.insert(it.key(), QVariant::fromValue(item));
                        } else {
                            map.insert(it.key(), it.value().toVariant());
                        }
                    }
                    property.write(this, map);
                }
            }else if (value.canConvert<QSdBaseParam *>()) {
                QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(value);
                if (obj) {
                    obj->fromJson(jsonObject[property.name()].toObject());
                }
            }
        }
    }
}

bool QSdBaseParam::saveToYamlFile(const QString &fileName)
{
    YAML::Node yamlNode = toYaml();
    YAML::Emitter out;
    out << yamlNode;
    if (out.good()) {
        bool ret = QAiUtils::writeTextFile(fileName, QString::fromStdString(out.c_str()));
        if(!ret)
            set_lastErrorString(QString("Failed to write yaml to file").arg(fileName));
        return ret;
    } else {
        Q_EMIT set_lastErrorString(QString("Error generating YAML string: %1")
                                         .arg(QString::fromStdString(out.GetLastError())
                                              )
                                     );
    }
    return false;
}

YAML::Node QSdBaseParam::loadFromYamlFile(const QString &fileName)
{
    YAML::Node node = YAML::Node(YAML::NodeType::Undefined);
    auto data = QAiUtils::readTextFile(fileName);
    try{
        node = YAML::Load(data.toStdString());
        fromYaml(node);
        // qDebug() << node.IsNull() << node.Type();
        return node;
    } catch (const YAML::Exception& e) {
        set_lastErrorString(QString("Error parsing YAML file: %1 %2" ).arg(e.what(), fileName));
        return node;
    }
    return node;
}

YAML::Node QSdBaseParam::toYaml() const {
    YAML::Node yamlNode;
    const QMetaObject *metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored() && property.isReadable()) {
            QVariant value = property.read(this);
            // no need to track the object name
            if(strcmp(property.name(), "objectName" ) == 0 ){
                continue;
            }
            if (value.typeId() == QMetaType::QString ||
                value.typeId() == QMetaType::QByteArray) {
                yamlNode[property.name()] = value.toString().toStdString();
            }else if(value.typeId() == QMetaType::Int){
                yamlNode[property.name()] = value.toInt();
            }else if(value.typeId() == QMetaType::Double){
                yamlNode[property.name()] = value.toDouble();
            }else if(value.typeId() == QMetaType::Bool){
                yamlNode[property.name()] = value.toBool();
            }else if(value.typeId() == QMetaType::Float){
                yamlNode[property.name()] = value.toFloat();
            }
            else if (value.typeId() == QMetaType::QStringList){
                const QStringList list = value.toStringList();
                if(!list.isEmpty()){
                    YAML::Node strListNode = YAML::Node(YAML::NodeType::Sequence);
                    for (const QString &item : list) {
                        strListNode.push_back(item.toStdString());
                    }
                    yamlNode[property.name()] = strListNode;
                }
            }

            else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
                auto li = qvariant_cast<ObjectListModelBase*>(value);
                if(!li->isEmpty()){
                    YAML::Node listNode = YAML::Node(YAML::NodeType::Sequence);
                    for (auto i : li->toVarArray()){
                        if(i.canConvert<QSdBaseParam*>()){
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(i);
                            if(obj)
                                listNode.push_back(obj->toYaml());
                        }else{
                            listNode.push_back(i.toString().toStdString());
                        }
                    }
                    yamlNode[property.name()] = listNode;
                }
            }

            else if (value.typeId() == QMetaType::QVariantList) {
                // Handle QList
                const QVariantList list = value.toList();
                if(!list.isEmpty()){
                    YAML::Node listNode = YAML::Node(YAML::NodeType::Sequence);
                    for (const QVariant &item : list) {
                        if (item.canConvert<QSdBaseParam *>()) {
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(item);
                            if (obj)
                                listNode.push_back(obj->toYaml());
                        } else {
                            listNode.push_back(item.toString().toStdString());
                        }
                    }
                    yamlNode[property.name()] = listNode;
                }
            }
            else if (value.typeId() == QMetaType::QVariantMap) {
                // Handle QMap
                const QVariantMap map = value.toMap();
                if(map.isEmpty()){
                    YAML::Node mapNode = YAML::Node(YAML::NodeType::Map);
                    for (auto it = map.begin(); it != map.end(); ++it) {
                        if (it.value().canConvert<QSdBaseParam *>()) {
                            QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(it.value());
                            if (obj)
                                mapNode[it.key().toStdString()] = obj->toYaml();
                        } else {
                            mapNode[it.key().toStdString()] = it.value().toString().toStdString();
                        }
                    }
                    yamlNode[property.name()] = mapNode;
                }
            }
            // Handle complex types
            else if (value.canConvert<QSdBaseParam *>()) {
                QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(value);
                if (obj) {
                    yamlNode[property.name()] = obj->toYaml();
                }
            }
        }
    }

    return yamlNode;
}

void QSdBaseParam::fromYaml(const YAML::Node &yamlNode)
{
    const QMetaObject *metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); ++i) {

        QMetaProperty property = metaObject->property(i);

        if (property.isStored() && yamlNode[property.name()] ){ //&& property.isWritable() && yamlNode[property.name()]) {
            QVariant value = property.read(this);


            if(strcmp(property.name(), "objectName" ) == 0 )
                continue;
            if (value.typeId() == QMetaType::Int) {
                property.write(this, QVariant(yamlNode[property.name()].as<int>()));
            } else if (value.typeId() == QMetaType::Double) {
                property.write(this, QVariant(yamlNode[property.name()].as<double>()));
            } else if (value.typeId() == QMetaType::QString) {
                property.write(this, QVariant(QString::fromStdString(yamlNode[property.name()].as<std::string>())));
            } else if (value.typeId() == QMetaType::Bool) {
                property.write(this, yamlNode[property.name()].as<bool>());
            }

            else if (value.typeId() == QMetaType::QStringList){
                YAML::Node listNode = yamlNode[property.name()];
                QStringList list;
                for (const YAML::Node &itemNode : listNode) {
                    list.append(
                        QString::fromStdString(itemNode.as<std::string>()));
                }
                property.write(this, list);
            }
            else if(strcmp(value.typeName(), "ObjectListModelBase*") == 0 ){
                YAML::Node listNode = yamlNode[property.name()];
                // qDebug() <<"LIST FOUND" << property.name();
                if(listNode.size() > 0){
                    QObject *modelObj = qvariant_cast<QObject *>(value);
                    auto *list = qobject_cast<ObjectListModelBase *>(modelObj);
                    if(list){
                        list->clear();
                        const QMetaObject *itemMetaObject = list->itemMetaObject();
                        for (const YAML::Node &itemNode : listNode) {
                            if(itemNode.IsMap()){
                                // qDebug() << "List class name " << itemMetaObject->className() << "VS" <<  property.name();
                                // FIXME get the type of lists [testlist, progargs, bootargs]
                                if(strcmp(itemMetaObject->className(), "QSdLora") == 0){
                                    auto *i = new QSdLora;
                                    i->fromYaml(itemNode);
                                    list->append(i);
                                }else if(strcmp(itemMetaObject->className(), "QSdEmbedding") == 0){
                                    auto *i = new QSdEmbedding;
                                    i->fromYaml(itemNode);
                                    list->append(i);
                                }
                                // Kit Session UsbDevice UartDevice User FlashTarget AvahiItem AvahiTxRecord
                            }else{
                                // should not get here.
                                qDebug() << "Breakpoint ";
                            }

                        }
                        property.write(this, list->toVarArray());
                    }
                }
            }

            // Handle QList
            else if (value.typeId() == QMetaType::QVariantList) {
                YAML::Node listNode = yamlNode[property.name()];
                if(listNode.size() > 0){
                    QVariantList list;
                    for (const YAML::Node &itemNode : listNode) {
                        if (itemNode.IsMap()) {
                            QSdBaseParam *item = new QSdBaseParam();
                            item->fromYaml(itemNode);
                            list.append(QVariant::fromValue(item));
                        } else {
                            list.append(QVariant(QString::fromStdString(itemNode.as<std::string>())));
                        }
                    }
                    property.write(this, list);
                }
            }

            // Handle QMap
            else if (value.typeId() == QMetaType::QVariantMap) {
                YAML::Node mapNode = yamlNode[property.name()];
                if(mapNode.size() > 0){
                    QVariantMap map;
                    for (auto it = mapNode.begin(); it != mapNode.end(); ++it) {
                        if (it->second.IsMap()) {
                            // we really might want to make a seperate class for this
                            QSdBaseParam *item = new QSdBaseParam();
                            item->fromYaml(it->second);
                            map.insert(QString::fromStdString(it->first.as<std::string>()), QVariant::fromValue(item));
                        } else {
                            map.insert(QString::fromStdString(it->first.as<std::string>()), QVariant(QString::fromStdString(it->second.as<std::string>())));
                        }
                    }
                    property.write(this, map);
                }
            }


            else if (value.canConvert<QSdBaseParam *>()) {
                // Handle complex types
                QSdBaseParam *obj = qvariant_cast<QSdBaseParam *>(value);
                if (obj) {
                    // qDebug() << yamlNode[property.name()]
                    obj->fromYaml(yamlNode[property.name()]);
                }
            }
        }
    }
}

