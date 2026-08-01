#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QtResource>

// TODO gonna need a whole JOB_STATIC or something to hgandle these later not tonight
// Q_IMPORT_QML_PLUGIN(QLlama)

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    // Q_INIT_RESOURCE(qmake_QLlama);

    QQmlApplicationEngine engine;
    engine.addImportPath(QStringLiteral("qrc:/"));

    const QUrl url(QStringLiteral("qrc:/QLlamaExample/main.qml"));
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
