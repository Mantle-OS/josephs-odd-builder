#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QtResource>
#include <qdiriterator.h>

Q_IMPORT_QML_PLUGIN(QHFPlugin)

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    Q_INIT_RESOURCE(qmake_QHF);
    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));
    const QUrl url(QStringLiteral("qrc:/QMLHFEXAMPLE/main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);
    engine.load(url);
    return app.exec();
}
