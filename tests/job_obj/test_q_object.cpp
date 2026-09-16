#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <QObject>
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <yaml-cpp/yaml.h>

#include <array>
#include <memory>

#include "q_test_base.h"

namespace {

// =============================================================================
// BaseQObject Serialization Fixtures
// =============================================================================

class QtSubSensorConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(QString sensorTag READ sensorTag WRITE setSensorTag)
    Q_PROPERTY(float sampleRateHz READ sampleRateHz WRITE setSampleRateHz)
    Q_PROPERTY(bool calibrateOnBoot READ calibrateOnBoot WRITE setCalibrateOnBoot)
    Q_PROPERTY(int initialMode READ initialMode WRITE setInitialMode)

public:
    Q_INVOKABLE explicit QtSubSensorConfig(QObject* parent = nullptr)
        : BaseQObject(parent)
    {
    }

    ~QtSubSensorConfig() override = default;

    QtSubSensorConfig(const QtSubSensorConfig&) = delete;
    QtSubSensorConfig& operator=(const QtSubSensorConfig&) = delete;
    QtSubSensorConfig(QtSubSensorConfig&&) = delete;
    QtSubSensorConfig& operator=(QtSubSensorConfig&&) = delete;

    [[nodiscard]] QString sensorTag() const
    {
        return m_sensorTag;
    }

    void setSensorTag(const QString& sensorTag)
    {
        m_sensorTag = sensorTag;
    }

    [[nodiscard]] float sampleRateHz() const
    {
        return m_sampleRateHz;
    }

    void setSampleRateHz(float sampleRateHz)
    {
        m_sampleRateHz = sampleRateHz;
    }

    [[nodiscard]] bool calibrateOnBoot() const
    {
        return m_calibrateOnBoot;
    }

    void setCalibrateOnBoot(bool calibrateOnBoot)
    {
        m_calibrateOnBoot = calibrateOnBoot;
    }

    [[nodiscard]] int initialMode() const
    {
        return m_initialMode;
    }

    void setInitialMode(int initialMode)
    {
        m_initialMode = initialMode;
    }

private:
    QString m_sensorTag{"thermal_zone_0"};
    float m_sampleRateHz{100.0f};
    bool m_calibrateOnBoot{true};
    int m_initialMode{0};
};

class QtComputeNodeConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(QString nodeName READ nodeName WRITE setNodeName)
    Q_PROPERTY(int threadPoolSize READ threadPoolSize WRITE setThreadPoolSize)
    Q_PROPERTY(double memoryBudgetGb READ memoryBudgetGb WRITE setMemoryBudgetGb)
    Q_PROPERTY(QVariantList scalingFactors READ scalingFactors WRITE setScalingFactors)
    Q_PROPERTY(QtSubSensorConfig* primarySensor READ primarySensor WRITE setPrimarySensor)
    Q_PROPERTY(QVariantList auxiliarySensors READ auxiliarySensors WRITE setAuxiliarySensors)

public:
    Q_INVOKABLE explicit QtComputeNodeConfig(QObject* parent = nullptr)
        : BaseQObject(parent),
        m_primarySensor(new QtSubSensorConfig(this))
    {
        m_scalingFactors = {
            1.0f,
            0.5f,
            0.25f
        };
    }

    ~QtComputeNodeConfig() override = default;

    QtComputeNodeConfig(const QtComputeNodeConfig&) = delete;
    QtComputeNodeConfig& operator=(const QtComputeNodeConfig&) = delete;
    QtComputeNodeConfig(QtComputeNodeConfig&&) = delete;
    QtComputeNodeConfig& operator=(QtComputeNodeConfig&&) = delete;

    [[nodiscard]] QString nodeName() const
    {
        return m_nodeName;
    }

    void setNodeName(const QString& nodeName)
    {
        m_nodeName = nodeName;
    }

    [[nodiscard]] int threadPoolSize() const
    {
        return m_threadPoolSize;
    }

    void setThreadPoolSize(int threadPoolSize)
    {
        m_threadPoolSize = threadPoolSize;
    }

    [[nodiscard]] double memoryBudgetGb() const
    {
        return m_memoryBudgetGb;
    }

    void setMemoryBudgetGb(double memoryBudgetGb)
    {
        m_memoryBudgetGb = memoryBudgetGb;
    }

    [[nodiscard]] QVariantList scalingFactors() const
    {
        return m_scalingFactors;
    }

    void setScalingFactors(const QVariantList& scalingFactors)
    {
        m_scalingFactors = scalingFactors;
    }

    [[nodiscard]] QtSubSensorConfig* primarySensor() const
    {
        return m_primarySensor;
    }

    void setPrimarySensor(QtSubSensorConfig* primarySensor)
    {
        if (m_primarySensor == primarySensor)
            return;

        if (m_primarySensor && m_primarySensor->parent() == this)
            delete m_primarySensor;

        m_primarySensor = primarySensor;

        if (m_primarySensor && !m_primarySensor->parent())
            m_primarySensor->setParent(this);
    }

    [[nodiscard]] QVariantList auxiliarySensors() const
    {
        return m_auxiliarySensors;
    }

    void setAuxiliarySensors(const QVariantList& auxiliarySensors)
    {
        clearAuxiliarySensors();

        m_auxiliarySensors = auxiliarySensors;

        for (const QVariant& value : m_auxiliarySensors) {
            if (!value.canConvert<BaseQObject*>())
                continue;

            BaseQObject* object = qvariant_cast<BaseQObject*>(value);

            if (object && !object->parent())
                object->setParent(this);
        }
    }

    void addAuxiliarySensor(QtSubSensorConfig* sensor)
    {
        if (!sensor)
            return;

        if (!sensor->parent())
            sensor->setParent(this);

        m_auxiliarySensors.push_back(QVariant::fromValue(static_cast<BaseQObject*>(sensor)));
    }

    void clearAuxiliarySensors()
    {
        for (const QVariant& value : m_auxiliarySensors) {
            if (!value.canConvert<BaseQObject*>())
                continue;

            BaseQObject* object = qvariant_cast<BaseQObject*>(value);

            if (object && object->parent() == this)
                delete object;
        }

        m_auxiliarySensors.clear();
    }

private:
    QString m_nodeName{"worker-node-alpha"};
    int m_threadPoolSize{16};
    double m_memoryBudgetGb{32.5};
    QVariantList m_scalingFactors;
    QtSubSensorConfig* m_primarySensor{nullptr};
    QVariantList m_auxiliarySensors;
};

class QtPrimitiveConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(int mode READ mode WRITE setMode)
    Q_PROPERTY(QByteArray rawByte READ rawByte WRITE setRawByte)
    Q_PROPERTY(int wideChar READ wideChar WRITE setWideChar)
    Q_PROPERTY(int utf8Char READ utf8Char WRITE setUtf8Char)
    Q_PROPERTY(int utf16Char READ utf16Char WRITE setUtf16Char)
    Q_PROPERTY(quint32 utf32Char READ utf32Char WRITE setUtf32Char)

public:
    Q_INVOKABLE explicit QtPrimitiveConfig(QObject* parent = nullptr)
        : BaseQObject(parent)
    {
    }

    ~QtPrimitiveConfig() override = default;

    QtPrimitiveConfig(const QtPrimitiveConfig&) = delete;
    QtPrimitiveConfig& operator=(const QtPrimitiveConfig&) = delete;
    QtPrimitiveConfig(QtPrimitiveConfig&&) = delete;
    QtPrimitiveConfig& operator=(QtPrimitiveConfig&&) = delete;

    [[nodiscard]] int mode() const
    {
        return m_mode;
    }

    void setMode(int mode)
    {
        m_mode = mode;
    }

    [[nodiscard]] QByteArray rawByte() const
    {
        return m_rawByte;
    }

    void setRawByte(const QByteArray& rawByte)
    {
        m_rawByte = rawByte;
    }

    [[nodiscard]] int wideChar() const
    {
        return m_wideChar;
    }

    void setWideChar(int wideChar)
    {
        m_wideChar = wideChar;
    }

    [[nodiscard]] int utf8Char() const
    {
        return m_utf8Char;
    }

    void setUtf8Char(int utf8Char)
    {
        m_utf8Char = utf8Char;
    }

    [[nodiscard]] int utf16Char() const
    {
        return m_utf16Char;
    }

    void setUtf16Char(int utf16Char)
    {
        m_utf16Char = utf16Char;
    }

    [[nodiscard]] quint32 utf32Char() const
    {
        return m_utf32Char;
    }

    void setUtf32Char(quint32 utf32Char)
    {
        m_utf32Char = utf32Char;
    }

private:
    int m_mode{0};
    QByteArray m_rawByte{1, '\0'};
    int m_wideChar{'A'};
    int m_utf8Char{'B'};
    int m_utf16Char{'C'};
    quint32 m_utf32Char{'D'};
};

class QtErrorStateConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(QString value READ value WRITE setValue)

public:
    Q_INVOKABLE explicit QtErrorStateConfig(QObject* parent = nullptr)
        : BaseQObject(parent)
    {
    }

    ~QtErrorStateConfig() override = default;

    QtErrorStateConfig(const QtErrorStateConfig&) = delete;
    QtErrorStateConfig& operator=(const QtErrorStateConfig&) = delete;
    QtErrorStateConfig(QtErrorStateConfig&&) = delete;
    QtErrorStateConfig& operator=(QtErrorStateConfig&&) = delete;

    [[nodiscard]] QString value() const
    {
        return m_value;
    }

    void setValue(const QString& value)
    {
        m_value = value;
    }

private:
    QString m_value{"payload"};
};

class QtContainerConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(QVariantList values READ values WRITE setValues)
    Q_PROPERTY(QVariantList names READ names WRITE setNames)

public:
    Q_INVOKABLE explicit QtContainerConfig(QObject* parent = nullptr)
        : BaseQObject(parent)
    {
    }

    ~QtContainerConfig() override = default;

    QtContainerConfig(const QtContainerConfig&) = delete;
    QtContainerConfig& operator=(const QtContainerConfig&) = delete;
    QtContainerConfig(QtContainerConfig&&) = delete;
    QtContainerConfig& operator=(QtContainerConfig&&) = delete;

    [[nodiscard]] QVariantList values() const
    {
        return m_values;
    }

    void setValues(const QVariantList& values)
    {
        m_values = values;
    }

    [[nodiscard]] QVariantList names() const
    {
        return m_names;
    }

    void setNames(const QVariantList& names)
    {
        m_names = names;
    }

private:
    QVariantList m_values;
    QVariantList m_names;
};

class QtBinaryMapConfig : public BaseQObject {
    Q_OBJECT

    Q_PROPERTY(QVariantMap entries READ entries WRITE setEntries)

public:
    Q_INVOKABLE explicit QtBinaryMapConfig(QObject* parent = nullptr)
        : BaseQObject(parent)
    {
    }

    ~QtBinaryMapConfig() override = default;

    QtBinaryMapConfig(const QtBinaryMapConfig&) = delete;
    QtBinaryMapConfig& operator=(const QtBinaryMapConfig&) = delete;
    QtBinaryMapConfig(QtBinaryMapConfig&&) = delete;
    QtBinaryMapConfig& operator=(QtBinaryMapConfig&&) = delete;

    [[nodiscard]] QVariantMap entries() const
    {
        return m_entries;
    }

    void setEntries(const QVariantMap& entries)
    {
        m_entries = entries;
    }

private:
    QVariantMap m_entries;
};

void registerQtSerializationTypes()
{
    static const int subSensorType =
        qRegisterMetaType<QtSubSensorConfig*>("QtSubSensorConfig*");

    static_cast<void>(subSensorType);
}

// =============================================================================
// Signals and Connections Parity
// =============================================================================

class QtSensorNode : public QObject {
    Q_OBJECT

public:
    explicit QtSensorNode(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~QtSensorNode() override = default;

    QtSensorNode(const QtSensorNode&) = delete;
    QtSensorNode& operator=(const QtSensorNode&) = delete;
    QtSensorNode(QtSensorNode&&) = delete;
    QtSensorNode& operator=(QtSensorNode&&) = delete;

    void emitReading(int channel, double value)
    {
        emit readingEmitted(channel, value);
    }

    [[nodiscard]] int connectionCount() const
    {
        return receivers(SIGNAL(readingEmitted(int,double)));
    }

signals:
    void readingEmitted(int channel, double value);
};

class QtControllerNode : public QObject {
    Q_OBJECT

public:
    explicit QtControllerNode(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~QtControllerNode() override = default;

    QtControllerNode(const QtControllerNode&) = delete;
    QtControllerNode& operator=(const QtControllerNode&) = delete;
    QtControllerNode(QtControllerNode&&) = delete;
    QtControllerNode& operator=(QtControllerNode&&) = delete;

    void handleReading(int channel, double value)
    {
        lastChannel = channel;
        lastValue = value;
        ++invocationCount;
    }

    int lastChannel{-1};
    double lastValue{0.0};
    int invocationCount{0};
};

} // namespace

#ifdef JOB_TEST_BENCHMARKS

// =============================================================================
// BaseQObject Serialization Benchmarks
// =============================================================================

TEST_CASE("BaseQObject serialization benchmarks", "[qt][qobject][serialization][benchmark]")
{
    registerQtSerializationTypes();

    QtComputeNodeConfig config;
    config.setNodeName("benchmark-compute-node");
    config.setThreadPoolSize(64);
    config.setMemoryBudgetGb(128.0);

    QVariantList scalingFactors;
    scalingFactors.reserve(256);

    for (int i = 0; i < 256; ++i)
        scalingFactors.push_back(1.0f);

    config.setScalingFactors(scalingFactors);

    config.primarySensor()->setSensorTag("thermal_zone_0");
    config.primarySensor()->setSampleRateHz(100.0f);
    config.primarySensor()->setCalibrateOnBoot(true);
    config.primarySensor()->setInitialMode(0);

    for (int i = 0; i < 16; ++i) {
        auto* sensor = new QtSubSensorConfig(&config);
        sensor->setSensorTag(QString("sensor_channel_%1").arg(i));
        sensor->setSampleRateHz(static_cast<float>(1000.0 / (i + 1)));
        config.addAuxiliarySensor(sensor);
    }

    const QByteArray binaryBuffer = config.toBinary();
    const QJsonObject jsonPayload = config.toJson();
    const YAML::Node yamlPayload = config.toYaml();

    BENCHMARK("QObject Binary Serialization (toBinary)")
    {
        const QByteArray buffer = config.toBinary();
        return buffer.size();
    };

    BENCHMARK("QObject Binary Deserialization (fromBinary)")
    {
        QtComputeNodeConfig restored;
        restored.fromBinary(binaryBuffer);
        return restored.threadPoolSize();
    };

    BENCHMARK("QObject JSON Serialization (toJson)")
    {
        return config.toJson();
    };

    BENCHMARK("QObject JSON Deserialization (fromJson)")
    {
        QtComputeNodeConfig restored;
        restored.fromJson(jsonPayload);
        return restored.threadPoolSize();
    };

    BENCHMARK("QObject YAML Serialization (toYaml)")
    {
        return config.toYaml();
    };

    BENCHMARK("QObject YAML Deserialization (fromYaml)")
    {
        QtComputeNodeConfig restored;
        restored.fromYaml(yamlPayload);
        return restored.threadPoolSize();
    };
}

TEST_CASE("BaseQObject nested serialization stress benchmark", "[qt][qobject][serialization][benchmark][stress]")
{
    registerQtSerializationTypes();

    QtComputeNodeConfig config;
    config.setNodeName("nested-stress-node");
    config.setThreadPoolSize(128);
    config.setMemoryBudgetGb(256.0);

    QVariantList scalingFactors;
    scalingFactors.reserve(4096);

    for (int i = 0; i < 4096; ++i)
        scalingFactors.push_back(0.5f);

    config.setScalingFactors(scalingFactors);

    for (int i = 0; i < 256; ++i) {
        auto* sensor = new QtSubSensorConfig(&config);
        sensor->setSensorTag(QString("stress_sensor_%1").arg(i));
        sensor->setSampleRateHz(static_cast<float>(i + 1));
        sensor->setCalibrateOnBoot((i % 2) == 0);
        sensor->setInitialMode((i % 3) == 0 ? 1 : 0);
        config.addAuxiliarySensor(sensor);
    }

    BENCHMARK("QObject Large nested binary roundtrip")
    {
        const QByteArray buffer = config.toBinary();

        QtComputeNodeConfig restored;
        restored.fromBinary(buffer);

        return restored.threadPoolSize();
    };

    BENCHMARK("QObject Large nested JSON roundtrip")
    {
        const QJsonObject json = config.toJson();

        QtComputeNodeConfig restored;
        restored.fromJson(json);

        return restored.threadPoolSize();
    };

    BENCHMARK("QObject Large nested YAML roundtrip")
    {
        const YAML::Node yaml = config.toYaml();

        QtComputeNodeConfig restored;
        restored.fromYaml(yaml);

        return restored.threadPoolSize();
    };
}

// =============================================================================
// QObject Object Benchmarks
// =============================================================================

TEST_CASE("QObject Object benchmarks", "[qt][qobject][object][benchmark]")
{
    auto sensor = std::make_unique<QtSensorNode>();
    auto controller = std::make_unique<QtControllerNode>();

    const auto connection = QObject::connect(
        sensor.get(),
        &QtSensorNode::readingEmitted,
        controller.get(),
        &QtControllerNode::handleReading,
        Qt::DirectConnection);

    REQUIRE(connection);

    BENCHMARK("Connected QObject slot invocation")
    {
        sensor->emitReading(1, 42.0);
        return controller->invocationCount;
    };

    BENCHMARK("QObject connectionCount - one live connection")
    {
        return sensor->connectionCount();
    };

    BENCHMARK("QObject signalsBlocked - unblocked")
    {
        return sensor->signalsBlocked();
    };

    BENCHMARK("QObject blockSignals toggle")
    {
        const bool previous = sensor->blockSignals(!sensor->signalsBlocked());
        return previous;
    };

    BENCHMARK("QObject makeUnique allocation and destruction")
    {
        auto object = std::make_unique<QtSensorNode>();
        return object.get();
    };

    BENCHMARK("QObject makeShared allocation and destruction")
    {
        auto object = std::make_shared<QtSensorNode>();
        return object.get();
    };
}

// =============================================================================
// QObject Signal Benchmarks
// =============================================================================

TEST_CASE("QObject signal benchmarks", "[qt][qobject][signal][benchmark]")
{
    QtSensorNode emptySensor;

    BENCHMARK("QObject direct emit (0 connected slots)")
    {
        emptySensor.emitReading(1, 42.0);
        return emptySensor.connectionCount();
    };

    QtSensorNode singleSensor;
    QtControllerNode singleController;

    const auto singleConnection = QObject::connect(
        &singleSensor,
        &QtSensorNode::readingEmitted,
        &singleController,
        &QtControllerNode::handleReading,
        Qt::DirectConnection);

    REQUIRE(singleConnection);

    BENCHMARK("QObject direct emit (1 connected slot)")
    {
        singleSensor.emitReading(1, 42.0);
        return singleController.invocationCount;
    };

    QtSensorNode fanoutSensor;
    std::array<QtControllerNode, 8> fanoutControllers;

    for (auto& controller : fanoutControllers) {
        const auto connection = QObject::connect(
            &fanoutSensor,
            &QtSensorNode::readingEmitted,
            &controller,
            &QtControllerNode::handleReading,
            Qt::DirectConnection);

        REQUIRE(connection);
    }

    BENCHMARK("QObject direct emit (8 fan-out slots)")
    {
        fanoutSensor.emitReading(1, 42.0);
        return fanoutControllers[0].invocationCount;
    };

    QtSensorNode largeFanoutSensor;
    std::array<QtControllerNode, 32> largeFanoutControllers;

    for (auto& controller : largeFanoutControllers) {
        const auto connection = QObject::connect(
            &largeFanoutSensor,
            &QtSensorNode::readingEmitted,
            &controller,
            &QtControllerNode::handleReading,
            Qt::DirectConnection);

        REQUIRE(connection);
    }

    BENCHMARK("QObject direct emit (32 fan-out slots)")
    {
        largeFanoutSensor.emitReading(1, 42.0);
        return largeFanoutControllers[0].invocationCount;
    };

    BENCHMARK("QObject connect and disconnect churn")
    {
        QtSensorNode sensor;
        QtControllerNode controller;

        const auto connection = QObject::connect(
            &sensor,
            &QtSensorNode::readingEmitted,
            &controller,
            &QtControllerNode::handleReading,
            Qt::DirectConnection);

        const bool disconnected = QObject::disconnect(connection);
        return disconnected;
    };
}

// =============================================================================
// QObject Connection Flag Benchmarks
// =============================================================================

TEST_CASE("QObject connection flag benchmarks", "[qt][qobject][benchmark][connection][flags]")
{
    constexpr auto DirectUnique =
        static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::UniqueConnection);

    constexpr auto DirectSingleShot =
        static_cast<Qt::ConnectionType>(Qt::DirectConnection | Qt::SingleShotConnection);

    BENCHMARK("QObject reflected Unique connect and disconnect")
    {
        QtSensorNode sensor;
        QtControllerNode controller;

        const auto connection = QObject::connect(
            &sensor,
            &QtSensorNode::readingEmitted,
            &controller,
            &QtControllerNode::handleReading,
            DirectUnique);

        const bool disconnected = QObject::disconnect(connection);
        return disconnected;
    };

    BENCHMARK("QObject reflected SingleShot connect and emit")
    {
        QtSensorNode sensor;
        QtControllerNode controller;

        const auto connection = QObject::connect(
            &sensor,
            &QtSensorNode::readingEmitted,
            &controller,
            &QtControllerNode::handleReading,
            DirectSingleShot);

        sensor.emitReading(1, 42.0);

        return controller.invocationCount + static_cast<int>(static_cast<bool>(connection));
    };
}

#endif

#include "test_q_object.moc"