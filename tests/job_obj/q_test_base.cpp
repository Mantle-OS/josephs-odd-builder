#include "q_test_base.h"

#include <QMetaObject>
#include <QMetaProperty>
#include <QMetaType>

#include <QSequentialIterable>
#include <QAssociativeIterable>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <utility>

#include <QDebug>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>

BaseQObject::BaseQObject(QObject *parent):
    QObject(parent)
{

}


// START HERE
constexpr auto BaseQObjectDataStreamVersion = QDataStream::Qt_6_0;

enum class BaseQObjectBinaryType : quint8
{
    Null = 0,
    Variant,
    Object,
    List,
    Map
};

static BaseQObject *createBinaryObject(const QByteArray &className, BaseQObject *parent)
{
    const QByteArray pointerTypeName = className + '*';
    const QMetaType pointerType = QMetaType::fromName(pointerTypeName);
    const QMetaObject *metaObject = pointerType.metaObject();

    if (metaObject) {
        QObject *created = metaObject->newInstance(Q_ARG(QObject*, parent));

        if (!created)
            created = metaObject->newInstance();

        if (auto *object = qobject_cast<BaseQObject *>(created)) {
            if (!object->parent())
                object->setParent(parent);
            return object;
        }

        delete created;
    }

    return new BaseQObject(parent);
}

static bool writeBinaryValue(QDataStream &ds, const QVariant &value)
{
    ds.setVersion(BaseQObjectDataStreamVersion);

    if (!value.isValid() || value.isNull()) {
        ds << static_cast<quint8>(BaseQObjectBinaryType::Null);
        return ds.status() == QDataStream::Ok;
    }

    if (value.canConvert<BaseQObject *>()) {
        BaseQObject *object = qvariant_cast<BaseQObject *>(value);

        ds << static_cast<quint8>(BaseQObjectBinaryType::Object);
        ds << static_cast<bool>(object != nullptr);

        if (object) {
            ds << QByteArray(object->metaObject()->className());
            ds << *object;
        }

        return ds.status() == QDataStream::Ok;
    }

    if (value.typeId() == QMetaType::QVariantList || value.canConvert<QVariantList>()) {
        const QVariantList list = value.toList();

        ds << static_cast<quint8>(BaseQObjectBinaryType::List);
        ds << static_cast<quint32>(list.size());

        for (const QVariant &item : list) {
            if (!writeBinaryValue(ds, item))
                return false;
        }

        return ds.status() == QDataStream::Ok;
    }

    if (value.typeId() == QMetaType::QVariantMap || value.canConvert<QVariantMap>()) {
        const QVariantMap map = value.toMap();

        ds << static_cast<quint8>(BaseQObjectBinaryType::Map);
        ds << static_cast<quint32>(map.size());

        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            ds << it.key();

            if (!writeBinaryValue(ds, it.value()))
                return false;
        }

        return ds.status() == QDataStream::Ok;
    }

    ds << static_cast<quint8>(BaseQObjectBinaryType::Variant);
    ds << value;

    return ds.status() == QDataStream::Ok;
}

static bool readBinaryValue(QDataStream &ds, QVariant &value, BaseQObject *parent, BaseQObject *existingObject = nullptr)
{
    ds.setVersion(BaseQObjectDataStreamVersion);

    quint8 rawType = 0;
    ds >> rawType;

    if (ds.status() != QDataStream::Ok)
        return false;

    const auto type = static_cast<BaseQObjectBinaryType>(rawType);

    if (type == BaseQObjectBinaryType::Null) {
        value = QVariant();
        return true;
    }

    if (type == BaseQObjectBinaryType::Variant) {
        ds >> value;
        return ds.status() == QDataStream::Ok;
    }

    if (type == BaseQObjectBinaryType::Object) {
        bool exists = false;
        ds >> exists;

        if (ds.status() != QDataStream::Ok)
            return false;

        if (!exists) {
            value = QVariant();
            return true;
        }

        QByteArray className;
        ds >> className;

        if (ds.status() != QDataStream::Ok)
            return false;

        BaseQObject *object = existingObject;

        if (!object)
            object = createBinaryObject(className, parent);

        if (!object)
            return false;

        ds >> *object;

        if (ds.status() != QDataStream::Ok) {
            if (!existingObject)
                delete object;
            return false;
        }

        value = QVariant::fromValue(object);
        return true;
    }

    if (type == BaseQObjectBinaryType::List) {
        quint32 count = 0;
        ds >> count;

        if (ds.status() != QDataStream::Ok)
            return false;

        QVariantList list;
        list.reserve(static_cast<qsizetype>(count));

        for (quint32 i = 0; i < count; ++i) {
            QVariant item;

            if (!readBinaryValue(ds, item, parent))
                return false;

            list.push_back(std::move(item));
        }

        value = std::move(list);
        return true;
    }

    if (type == BaseQObjectBinaryType::Map) {
        quint32 count = 0;
        ds >> count;

        if (ds.status() != QDataStream::Ok)
            return false;

        QVariantMap map;

        for (quint32 i = 0; i < count; ++i) {
            QString key;
            QVariant item;

            ds >> key;

            if (ds.status() != QDataStream::Ok)
                return false;

            if (!readBinaryValue(ds, item, parent))
                return false;

            map.insert(std::move(key), std::move(item));
        }

        value = std::move(map);
        return true;
    }

    return false;
}

QDataStream &operator<<(QDataStream &ds, const BaseQObject &obj)
{
    ds.setVersion(BaseQObjectDataStreamVersion);

    const QMetaObject *metaObject = obj.metaObject();
    QList<QMetaProperty> properties;
    properties.reserve(metaObject->propertyCount());

    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);

        if (!property.isStored() || !property.isReadable())
            continue;

        if (strcmp(property.name(), "objectName") == 0)
            continue;

        properties.push_back(property);
    }

    ds << static_cast<quint32>(properties.size());

    for (const QMetaProperty &property : properties) {
        QByteArray payload;
        QDataStream payloadStream(&payload, QIODevice::WriteOnly);
        payloadStream.setVersion(BaseQObjectDataStreamVersion);

        const QVariant value = property.read(&obj);

        if (!writeBinaryValue(payloadStream, value)) {
            ds.setStatus(QDataStream::WriteFailed);
            return ds;
        }

        ds << QByteArray(property.name());
        ds << payload;

        if (ds.status() != QDataStream::Ok)
            return ds;
    }

    return ds;
}

QDataStream &operator>>(QDataStream &ds, BaseQObject &obj)
{
    ds.setVersion(BaseQObjectDataStreamVersion);

    const QMetaObject *metaObject = obj.metaObject();

    quint32 propertyCount = 0;
    ds >> propertyCount;

    if (ds.status() != QDataStream::Ok)
        return ds;

    for (quint32 i = 0; i < propertyCount; ++i) {
        QByteArray propertyName;
        QByteArray payload;

        ds >> propertyName;
        ds >> payload;

        if (ds.status() != QDataStream::Ok)
            return ds;

        const int propertyIndex = metaObject->indexOfProperty(propertyName.constData());

        if (propertyIndex < 0)
            continue;

        QMetaProperty property = metaObject->property(propertyIndex);

        if (!property.isStored() || !property.isWritable())
            continue;

        QVariant currentValue = property.read(&obj);
        BaseQObject *existingObject = nullptr;

        if (currentValue.canConvert<BaseQObject *>())
            existingObject = qvariant_cast<BaseQObject *>(currentValue);

        QDataStream payloadStream(payload);
        payloadStream.setVersion(BaseQObjectDataStreamVersion);

        QVariant value;

        if (!readBinaryValue(payloadStream, value, &obj, existingObject)) {
            ds.setStatus(QDataStream::ReadCorruptData);
            return ds;
        }

        if (existingObject)
            continue;

        if (value.isValid() && value.metaType() != property.metaType()) {
            if (value.typeId() == QMetaType::QVariantList) {
                QVariant restored(property.metaType(), nullptr);

                if (restored.canView<QSequentialIterable>()) {
                    QSequentialIterable iterable = restored.view<QSequentialIterable>();

                    for (const QVariant &item : value.toList())
                        iterable.addValue(item);

                    value = std::move(restored);
                } else if (!value.convert(property.metaType())) {
                    ds.setStatus(QDataStream::ReadCorruptData);
                    return ds;
                }
            } else if (value.typeId() == QMetaType::QVariantMap) {
                QVariant restored(property.metaType(), nullptr);

                if (restored.canView<QAssociativeIterable>()) {
                    QAssociativeIterable iterable = restored.view<QAssociativeIterable>();

                    const QVariantMap map = value.toMap();
                    for (auto it = map.cbegin(); it != map.cend(); ++it)
                        iterable.setValue(it.key(), it.value());

                    value = std::move(restored);
                } else if (!value.convert(property.metaType())) {
                    ds.setStatus(QDataStream::ReadCorruptData);
                    return ds;
                }
            } else if (!value.convert(property.metaType())) {
                ds.setStatus(QDataStream::ReadCorruptData);
                return ds;
            }
        }

        if (!property.write(&obj, value)) {
            ds.setStatus(QDataStream::ReadCorruptData);
            return ds;
        }
    }

    return ds;
}

bool BaseQObject::saveToBinaryFile(const QString &fileName)
{
    if (!BaseQObject::createDirFromFile(fileName)) {
        setLastErrorString(QString("Failed to create directory for binary file %1").arg(fileName));
        return false;
    }

    QFile file(fileName);

    if (!file.open(QIODevice::WriteOnly)) {
        setLastErrorString(QString("Failed opening binary file %1: %2").arg(fileName, file.errorString()));
        return false;
    }

    const QByteArray data = toBinary();

    if (data.isEmpty()) {
        setLastErrorString(QString("No binary data generated for file %1").arg(fileName));
        return false;
    }

    if (file.write(data) != data.size()) {
        setLastErrorString(QString("Failed writing binary file %1: %2").arg(fileName, file.errorString()));
        return false;
    }

    if (!file.flush()) {
        setLastErrorString(QString("Failed flushing binary file %1: %2").arg(fileName, file.errorString()));
        return false;
    }

    return true;
}

QByteArray BaseQObject::loadFromBinaryFile(const QString &fileName)
{
    QByteArray data;
    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        setLastErrorString(QString("Failed opening binary file %1: %2").arg(fileName, file.errorString()));
        return data;
    }

    data = file.readAll();

    if (data.isEmpty()) {
        setLastErrorString(QString("No data in binary file %1").arg(fileName));
        return data;
    }

    fromBinary(data);
    return data;
}

QByteArray BaseQObject::toBinary() const
{
    QByteArray data;
    QDataStream ds(&data, QIODevice::WriteOnly);
    ds.setVersion(BaseQObjectDataStreamVersion);
    ds << *this;

    return data;
}

void BaseQObject::fromBinary(const QByteArray &data)
{
    if (data.isEmpty()) {
        setLastErrorString("No binary data.");
        return;
    }

    QDataStream ds(data);
    ds.setVersion(BaseQObjectDataStreamVersion);
    ds >> *this;

    if (ds.status() != QDataStream::Ok)
        setLastErrorString("Failed reading binary data.");
}

// END HERE
void BaseQObject::debugJson(QJsonDocument::JsonFormat format){
    QJsonDocument jdoc;
    jdoc.setObject(toJson());
    qDebug().noquote() << "[JSON]\n" << jdoc.toJson(QJsonDocument::Indented).simplified().data();
}

void BaseQObject::debugYaml(){
    YAML::Node node = toYaml();
    YAML::Emitter emitter;
    emitter << node;
    qDebug().noquote() << "[YAML]\n" << QString::fromUtf8(emitter.c_str());
}




bool BaseQObject::saveToJsonFile(const QString &fileName)
{
    bool ret = false;
    QJsonDocument jdoc;
    jdoc.setObject(toJson());
    ret = BaseQObject::writeTextFile(fileName, jdoc.toJson());
    if(!ret)
        setLastErrorString(QString("Failed writing file %1").arg(fileName));
    return ret;
}

QJsonObject BaseQObject::loadFromJsonFile(const QString &fileName)
{
    QJsonObject ret;
    auto data = BaseQObject::readTextFile(fileName).toLocal8Bit();
    if(!data.isEmpty()){
        QJsonParseError er;
        QJsonDocument jdoc = QJsonDocument::fromJson(data, &er);
        if(er.error == QJsonParseError::NoError){
            if(!jdoc.isEmpty()){
                ret = jdoc.object();
            }
        }else{
            setLastErrorString(er.errorString());
        }
    }else{
        setLastErrorString("No data in file json file.");
    }
    return ret;
}


QJsonObject BaseQObject::toJson() const {
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
                value.typeId() == QMetaType::Float ||
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
            }
            // else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
            //     auto li = qvariant_cast<ObjectListModelBase*>(value);
            //     if(!li->isEmpty()){
            //         QJsonArray jsonArray;
            //         for (auto i : li->toVarArray()){
            //             if(i.canConvert<BaseQObject*>()){
            //                 BaseQObject *obj = qvariant_cast<BaseQObject *>(i);
            //                 if(obj)
            //                     jsonArray.append(obj->toJson());
            //             }else{
            //                 jsonArray.append(QJsonValue::fromVariant(i));
            //             }
            //         }
            //         jsonObject[property.name()] = jsonArray;
            //     }
            // }
            else if (value.typeId() == QMetaType::QVariantList){
                const QVariantList list = value.toList();
                if(!list.isEmpty()){
                    QJsonArray jsonArray;
                    for (const QVariant &item : list) {
                        if (item.canConvert<BaseQObject *>()) {
                            BaseQObject *obj = qvariant_cast<BaseQObject *>(item);
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
                        if (it.value().canConvert<BaseQObject *>()) {
                            BaseQObject *obj = qvariant_cast<BaseQObject *>(it.value());
                            if (obj)
                                mapObject[it.key()] = obj->toJson();
                        }else{
                            mapObject[it.key()] = QJsonValue::fromVariant(it.value());
                        }
                    }
                    jsonObject[property.name()] = mapObject;
                }
            }else if (value.canConvert<BaseQObject *>()){
                // Handle complex types
                BaseQObject *obj = qvariant_cast<BaseQObject *>(value);
                if (obj) {
                    jsonObject[property.name()] = obj->toJson();
                }
            }
        }
    }
    return jsonObject;
}

void BaseQObject::fromJson(const QJsonObject &jsonObject) {
    const QMetaObject *metaObject = this->metaObject();
    for (int i = 0; i < metaObject->propertyCount(); ++i) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored() /*&& property.isWritable() && jsonObject.contains(property.name())*/ ) {
            QVariant value = property.read(this);
            if (value.typeId() == QMetaType::Int ||
                value.typeId() == QMetaType::Double ||
                value.typeId() == QMetaType::Float ||
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
            }
            // else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
            //     QJsonArray jsonArray = jsonObject[property.name()].toArray();
            //     if(!jsonArray.isEmpty()){
            //         QObject *modelObj = qvariant_cast<QObject *>(value);
            //         auto *list = qobject_cast<ObjectListModelBase *>(modelObj);
            //         if(list){
            //             list->clear();
            //             const QMetaObject *itemMetaObject = list->itemMetaObject();
            //             if (!itemMetaObject){
            //                 qWarning() << "Item meta-object not defined for" << list->objectName();
            //                 return;
            //             }
            //             for (const QJsonValue &jsonValue : jsonArray) {
            //                 if (jsonValue.isObject()) {
            //                     // MVC NOT FULLY DEFINED YET !!!!!
            //                     if(strcmp(itemMetaObject->className(), "QSdLora") == 0){
            //                         auto *i = new QSdLora;
            //                         i->fromJson(jsonValue.toObject());
            //                         list->append(i);
            //                     }else if(strcmp(itemMetaObject->className(), "QSdEmbedding") == 0){
            //                         auto *i = new QSdEmbedding;
            //                         i->fromJson(jsonValue.toObject());
            //                         list->append(i);
            //                     } else if (strcmp(itemMetaObject->className(), "QSdImage") == 0){
            //                         auto *i = new QSdImage;
            //                         i->fromJson(jsonValue.toObject());
            //                         list->append(i);
            //                     }
            //                 }
            //             }
            //             property.write(this, list->toVarArray());
            //         }
            //     }
            // }

            else if (value.typeId() == QMetaType::QVariantList) {
                QJsonArray jsonArray = jsonObject[property.name()].toArray();
                if(!jsonArray.isEmpty()){
                    QVariantList list;
                    for (const QJsonValue &jsonValue : jsonArray) {
                        if (jsonValue.isObject()) {
                            BaseQObject *item = new BaseQObject();
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
                            BaseQObject *item = new BaseQObject();
                            item->fromJson(it.value().toObject());
                            map.insert(it.key(), QVariant::fromValue(item));
                        } else {
                            map.insert(it.key(), it.value().toVariant());
                        }
                    }
                    property.write(this, map);
                }
            }else if (value.canConvert<BaseQObject *>()) {
                BaseQObject *obj = qvariant_cast<BaseQObject *>(value);
                if (obj) {
                    obj->fromJson(jsonObject[property.name()].toObject());
                }
            }
        }
    }
}

bool BaseQObject::saveToYamlFile(const QString &fileName)
{
    YAML::Node yamlNode = toYaml();
    YAML::Emitter out;
    out << yamlNode;
    if (out.good()) {
        bool ret = BaseQObject::writeTextFile(fileName, QString::fromStdString(out.c_str()));
        if(!ret)
            setLastErrorString(QString("Failed to write yaml to file").arg(fileName));
        return ret;
    } else {
        Q_EMIT setLastErrorString(QString("Error generating YAML string: %1")
                                      .arg(QString::fromStdString(out.GetLastError())));
    }
    return false;
}

YAML::Node BaseQObject::loadFromYamlFile(const QString &fileName)
{
    YAML::Node node = YAML::Node(YAML::NodeType::Undefined);
    auto data = BaseQObject::readTextFile(fileName);
    try{
        node = YAML::Load(data.toStdString());
        fromYaml(node);
        // qDebug() << node.IsNull() << node.Type();
        return node;
    } catch (const YAML::Exception& e) {
        setLastErrorString(QString("Error parsing YAML file: %1 %2" ).arg(e.what(), fileName));
        return node;
    }
    return node;
}

YAML::Node BaseQObject::toYaml() const {
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

            // else if(strcmp(value.typeName(),  "ObjectListModelBase*") == 0){
            //     auto li = qvariant_cast<ObjectListModelBase*>(value);
            //     if(!li->isEmpty()){
            //         YAML::Node listNode = YAML::Node(YAML::NodeType::Sequence);
            //         for (auto i : li->toVarArray()){
            //             if(i.canConvert<BaseQObject*>()){
            //                 BaseQObject *obj = qvariant_cast<BaseQObject *>(i);
            //                 if(obj)
            //                     listNode.push_back(obj->toYaml());
            //             }else{
            //                 listNode.push_back(i.toString().toStdString());
            //             }
            //         }
            //         yamlNode[property.name()] = listNode;
            //     }
            // }

            else if (value.typeId() == QMetaType::QVariantList) {
                // Handle QList
                const QVariantList list = value.toList();
                if(!list.isEmpty()){
                    YAML::Node listNode = YAML::Node(YAML::NodeType::Sequence);
                    for (const QVariant &item : list) {
                        if (item.canConvert<BaseQObject *>()) {
                            BaseQObject *obj = qvariant_cast<BaseQObject *>(item);
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
                        if (it.value().canConvert<BaseQObject *>()) {
                            BaseQObject *obj = qvariant_cast<BaseQObject *>(it.value());
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
            else if (value.canConvert<BaseQObject *>()) {
                BaseQObject *obj = qvariant_cast<BaseQObject *>(value);
                if (obj) {
                    yamlNode[property.name()] = obj->toYaml();
                }
            }
        }
    }

    return yamlNode;
}

void BaseQObject::fromYaml(const YAML::Node &yamlNode)
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
            } else if (value.typeId() == QMetaType::Float){
                property.write(this, QVariant(yamlNode[property.name()].as<float>()));
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
            // else if(strcmp(value.typeName(), "ObjectListModelBase*") == 0 ){
            //     YAML::Node listNode = yamlNode[property.name()];
            //     // qDebug() <<"LIST FOUND" << property.name();
            //     if(listNode.size() > 0){
            //         QObject *modelObj = qvariant_cast<QObject *>(value);
            //         auto *list = qobject_cast<ObjectListModelBase *>(modelObj);
            //         if(list){
            //             list->clear();
            //             const QMetaObject *itemMetaObject = list->itemMetaObject();
            //             for (const YAML::Node &itemNode : listNode) {
            //                 if(itemNode.IsMap()){
            //                     // qDebug() << "List class name " << itemMetaObject->className() << "VS" <<  property.name();
            //                     // FIXME get the type of lists [testlist, progargs, bootargs]
            //                     if(strcmp(itemMetaObject->className(), "QSdLora") == 0){
            //                         auto *i = new QSdLora;
            //                         i->fromYaml(itemNode);
            //                         list->append(i);
            //                     }else if(strcmp(itemMetaObject->className(), "QSdEmbedding") == 0){
            //                         auto *i = new QSdEmbedding;
            //                         i->fromYaml(itemNode);
            //                         list->append(i);
            //                     }else if (strcmp(itemMetaObject->className(), "QSdImage") == 0){
            //                         auto *i = new QSdImage;
            //                         i->fromYaml(itemNode);
            //                         list->append(i);
            //                     }
            //                 }else{
            //                     // should not get here.
            //                     qDebug() << "Breakpoint ";
            //                 }

            //             }
            //             property.write(this, list->toVarArray());
            //         }
            //     }
            // }

            // Handle QList
            else if (value.typeId() == QMetaType::QVariantList) {
                YAML::Node listNode = yamlNode[property.name()];
                if(listNode.size() > 0){
                    QVariantList list;
                    for (const YAML::Node &itemNode : listNode) {
                        if (itemNode.IsMap()) {
                            BaseQObject *item = new BaseQObject();
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
                            BaseQObject *item = new BaseQObject();
                            item->fromYaml(it->second);
                            map.insert(QString::fromStdString(it->first.as<std::string>()), QVariant::fromValue(item));
                        } else {
                            map.insert(QString::fromStdString(it->first.as<std::string>()), QVariant(QString::fromStdString(it->second.as<std::string>())));
                        }
                    }
                    property.write(this, map);
                }
            }


            else if (value.canConvert<BaseQObject *>()) {
                // Handle complex types
                BaseQObject *obj = qvariant_cast<BaseQObject *>(value);
                if (obj) {
                    // qDebug() << yamlNode[property.name()]
                    obj->fromYaml(yamlNode[property.name()]);
                }
            }
        }
    }
}


QString BaseQObject::lastErrorString() const
{
    return m_lastErrorString;
}

void BaseQObject::setLastErrorString(const QString &newLastErrorString)
{
    if (m_lastErrorString != newLastErrorString){
        m_lastErrorString = newLastErrorString;
        Q_EMIT lastErrorStringChanged();
    }
}

bool BaseQObject::createDirFromFile(const QString &fileName){
    bool ret = false;
    QFileInfo fi(fileName);
    QDir d = fi.absoluteDir();
    if(!d.exists()){
        if(d.mkpath(d.absolutePath())){
            ret = true;
        }else{
            // LOG
        }
    }else{
        // we already have the dir return true
        ret = true;
    }
    return ret;
}

bool BaseQObject::createDir(const QString &dirName)
{
    bool ret = false;
    QDir d (dirName);
    if(!d.exists()){
        if(d.mkpath(dirName)){
            ret = true;
        }else{
            // LOG
        }
    }else{
        // we already have the dir return true
        ret = true;
    }
    return ret;
}

bool BaseQObject::writeTextFile(const QString &fileName, const QString &content)
{
    bool ret = false;

    if (BaseQObject::createDirFromFile(fileName)) {
        QFile f(fileName);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << content;
            if (f.flush())
                ret = true;
            else
                qWarning() << "Failed to flush the file buffer:" << f.errorString();
            f.close();
            if (f.error() != QFile::NoError) {
                qWarning() << "Error after closing the file:" << f.errorString();
                ret = false;
            }
        } else {
            qWarning() << "Failed to open file for writing:" << f.errorString();
        }
    } else {
        qWarning() << "Failed to create directory for file:" << fileName;
    }
    return ret;
}

QString BaseQObject::readTextFile(const QString &fileName)
{
    QString ret;
    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        ret = in.readAll();
        f.close();
    }else{
        // log
    }
    return ret;
}

bool BaseQObject::fileExists(const QString &filePath)
{
    return QFile::exists(filePath) && QFileInfo(filePath).isFile();
}

bool BaseQObject::dirExists(const QString &dirPath)
{
    return QFile::exists(dirPath) && QFileInfo(dirPath).isDir();
}



