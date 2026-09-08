#include <QByteArray>
#include <QCoreApplication>
#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QSysInfo>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>

#include <cstdio>

namespace {
void checkpoint(const char *message)
{
    std::fprintf(stderr, "SPACE_RHYTHM_CHECKPOINT %s\n", message);
    std::fflush(stderr);
}
}

int main(int argc, char *argv[])
{
    static_assert(sizeof(void *) == 8, "T-012 requires an x64 binary");

    checkpoint("before-qguiapplication");
    QGuiApplication app(argc, argv);
    checkpoint("after-qguiapplication");
    const QByteArray architecture = QSysInfo::currentCpuArchitecture().toUtf8();
    const QByteArray buildAbi = QSysInfo::buildAbi().toUtf8();
    std::fprintf(stderr,
                 "SPACE_RHYTHM_QT_SMOKE qt=%s arch=%s buildAbi=%s\n",
                 qVersion(), architecture.constData(), buildAbi.constData());
    std::fflush(stderr);

    QQmlApplicationEngine engine;
    checkpoint("after-qml-engine");
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(2); },
        Qt::QueuedConnection);
    engine.loadFromModule("SpaceRhythm.BuildSmoke", "Main");
    checkpoint("after-qml-load");

    if (engine.rootObjects().isEmpty())
        return 2;

    const bool multimediaReady =
        engine.rootObjects().constFirst()->property("multimediaReady").toBool();
    std::fprintf(stderr,
                 "SPACE_RHYTHM_QML_SMOKE multimediaReady=%s\n",
                 multimediaReady ? "true" : "false");
    std::fflush(stderr);
    if (!multimediaReady)
        return 3;

    QTimer::singleShot(250, &app, [&app] {
        checkpoint("event-loop-ok");
        app.quit();
    });
    return app.exec();
}
