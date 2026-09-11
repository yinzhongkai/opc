#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>

#include <cstdio>
#include <memory>

#include <space_rhythm/ui/scene_graph_attachment.hpp>
#include <space_rhythm/ui/view_models.hpp>

namespace {

space_rhythm::ui::MockScenario mock_scenario(const QStringList& arguments)
{
    constexpr QLatin1StringView prefix{"--ui-scenario="};
    for (const auto& argument : arguments) {
        if (argument.startsWith(prefix)) {
            return space_rhythm::ui::parse_mock_scenario(
                argument.sliced(prefix.size()));
        }
    }
    return space_rhythm::ui::MockScenario::start;
}

} // namespace

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
    auto scenario = mock_scenario(QCoreApplication::arguments());
    if (smoke_requested && scenario == space_rhythm::ui::MockScenario::start) {
        scenario = space_rhythm::ui::MockScenario::idle;
    }
    auto service = space_rhythm::ui::make_mock_workspace_service(scenario);
    space_rhythm::ui::ApplicationViewModel application_view_model(std::move(service));
    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("appViewModel"),
         QVariant::fromValue(static_cast<QObject*>(&application_view_model))},
    });
    engine.loadFromModule("SpaceRhythm", "Main");

    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "SPACE_RHYTHM_APP_SMOKE_FAILED: QML root was not created\n");
        return 2;
    }

    QPointer<QQuickItem> attached_preview_host;
    const auto attach_preview_if_available = [&engine, &attached_preview_host]() {
        if (engine.rootObjects().isEmpty()) {
            return false;
        }
        auto* preview_host = engine.rootObjects().front()->findChild<QQuickItem*>(
            QStringLiteral("previewSceneGraphHost"));
        if (preview_host == nullptr) {
            return false;
        }
        if (attached_preview_host != preview_host) {
            [[maybe_unused]] auto* scene_graph_item =
                space_rhythm::ui::attach_scene_graph_render_item(*preview_host);
            attached_preview_host = preview_host;
        }
        return true;
    };

    QObject::connect(&application_view_model,
                     &space_rhythm::ui::ApplicationViewModel::viewStateChanged,
                     &application,
                     [&application, attach_preview_if_available]() {
                         QTimer::singleShot(0, &application, attach_preview_if_available);
                     });
    const bool preview_attached = attach_preview_if_available();

    if (smoke_requested) {
        if (!preview_attached) {
            std::fprintf(stderr,
                         "SPACE_RHYTHM_APP_SMOKE_FAILED: preview scene graph host was not found\n");
            return 3;
        }
        std::fprintf(stdout, "SPACE_RHYTHM_APP_SMOKE_OK Qt=%s arch=x64\n", qVersion());
        QTimer::singleShot(0, &application, &QCoreApplication::quit);
    }

    return application.exec();
}
