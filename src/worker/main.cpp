#include <QCoreApplication>
#include <QTimer>
#include <QtGlobal>

#include <cstdio>

#include <space_rhythm/build/core_skeleton.hpp>
#include <space_rhythm/media/media_adapter_skeleton.hpp>
#include <space_rhythm/system/runtime.hpp>
#include <space_rhythm/worker/ui_worker_protocol.hpp>

int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    space_rhythm::build::core_link_anchor();
    space_rhythm::media::media_adapter_link_anchor();

    const auto arguments = application.arguments();
    const auto mock_worker_index = arguments.indexOf(QStringLiteral("--mock-worker"));
    if (mock_worker_index >= 0 && mock_worker_index + 1 < arguments.size()) {
        return space_rhythm::system::run_mock_worker(
            arguments.at(mock_worker_index + 1).toStdString());
    }
    const auto ui_worker_index = arguments.indexOf(QStringLiteral("--ui-worker"));
    if (ui_worker_index >= 0 && ui_worker_index + 1 < arguments.size()) {
        return space_rhythm::worker::run_ui_worker_server(
            arguments.at(ui_worker_index + 1).toStdString());
    }

    if (arguments.contains(QStringLiteral("--smoke"))) {
        std::fprintf(stdout, "SPACE_RHYTHM_WORKER_SMOKE_OK Qt=%s arch=x64\n", qVersion());
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
