#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTimer>
#include <QtGlobal>

#include <cstdio>

#include <space_rhythm/build/core_skeleton.hpp>
#include <space_rhythm/media/media_adapter_skeleton.hpp>

int main(int argc, char* argv[])
{
    const bool smoke_requested = [&]() {
        for (int index = 1; index < argc; ++index) {
            if (QString::fromLocal8Bit(argv[index]) == QStringLiteral("--smoke")) {
                return true;
            }
        }
        return false;
    }();

    if (smoke_requested && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "offscreen");
    }

    QGuiApplication application(argc, argv);
    QQmlApplicationEngine engine;
    space_rhythm::build::core_link_anchor();
    space_rhythm::media::media_adapter_link_anchor();
    engine.loadFromModule("SpaceRhythm", "Main");

    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "SPACE_RHYTHM_APP_SMOKE_FAILED: QML root was not created\n");
        return 2;
    }

    if (smoke_requested) {
        std::fprintf(stdout, "SPACE_RHYTHM_APP_SMOKE_OK Qt=%s arch=x64\n", qVersion());
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
