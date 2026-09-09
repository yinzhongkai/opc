#include <QCoreApplication>
#include <QTimer>
#include <QtGlobal>

#include <cstdio>

#include <space_rhythm/build/core_skeleton.hpp>
#include <space_rhythm/media/media_adapter_skeleton.hpp>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    space_rhythm::build::core_link_anchor();
    space_rhythm::media::media_adapter_link_anchor();

    if (application.arguments().contains(QStringLiteral("--smoke"))) {
        std::fprintf(stdout, "SPACE_RHYTHM_WORKER_SMOKE_OK Qt=%s arch=x64\n", qVersion());
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
