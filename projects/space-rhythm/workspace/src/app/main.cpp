#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QPointer>
#include <QQuickItem>
#include <QTimer>
#include <QVariant>
#include <QtGlobal>

#include <cstdio>
#include <memory>
#include <optional>

#include <space_rhythm/ui/scene_graph_attachment.hpp>
#include <space_rhythm/ui/view_models.hpp>
#include <space_rhythm/rendering/scene_graph_render_item.hpp>

namespace {

std::optional<space_rhythm::ui::MockScenario> mock_scenario(const QStringList& arguments)
{
    constexpr QLatin1StringView prefix{"--ui-scenario="};
    for (const auto& argument : arguments) {
        if (argument.startsWith(prefix)) {
            return space_rhythm::ui::parse_mock_scenario(
                argument.sliced(prefix.size()));
        }
    }
    return std::nullopt;
}

QString option_value(const QStringList& arguments, QLatin1StringView prefix)
{
    for (const auto& argument : arguments) {
        if (argument.startsWith(prefix)) {
            return argument.sliced(prefix.size());
        }
    }
    return {};
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

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
    QGuiApplication application(argc, argv);
    auto scenario = mock_scenario(QCoreApplication::arguments());
    std::unique_ptr<space_rhythm::ui::WorkspaceService> service;
    if (smoke_requested && !scenario) {
        scenario = space_rhythm::ui::MockScenario::idle;
    }
    if (scenario) {
        service = space_rhythm::ui::make_mock_workspace_service(*scenario);
    } else {
        const auto arguments = QCoreApplication::arguments();
        service = space_rhythm::ui::make_integrated_workspace_service({
            option_value(arguments, QLatin1StringView{"--project="}),
            option_value(arguments, QLatin1StringView{"--import="}),
            option_value(arguments, QLatin1StringView{"--export="}),
            false});
    }
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
    QPointer<QQuickItem> attached_timeline_host;
    QPointer<space_rhythm::rendering::SceneGraphRenderItem> preview_item;
    QPointer<space_rhythm::rendering::SceneGraphRenderItem> timeline_item;
    const auto synchronize_render_items = [&engine,
                                           &application_view_model,
                                           &attached_preview_host,
                                           &attached_timeline_host,
                                           &preview_item,
                                           &timeline_item]() {
        if (engine.rootObjects().isEmpty()) {
            return false;
        }
        auto* preview_host = engine.rootObjects().front()->findChild<QQuickItem*>(
            QStringLiteral("previewSceneGraphHost"));
        if (preview_host == nullptr) {
            return false;
        }
        if (attached_preview_host != preview_host) {
            preview_item = space_rhythm::ui::attach_scene_graph_render_item(*preview_host);
            preview_item->setObjectName(QStringLiteral("previewSceneGraphRenderItem"));
            attached_preview_host = preview_host;
        }
        auto* timeline_host = engine.rootObjects().front()->findChild<QQuickItem*>(
            QStringLiteral("timelineSceneGraphHost"));
        if (timeline_host != nullptr && attached_timeline_host != timeline_host) {
            timeline_item = space_rhythm::ui::attach_scene_graph_render_item(*timeline_host);
            timeline_item->setObjectName(QStringLiteral("timelineSceneGraphRenderItem"));
            attached_timeline_host = timeline_host;
        }
        const auto render_snapshot = application_view_model.renderSnapshot();
        const auto viewport = application_view_model.viewportTimeRange();
        const auto frame = application_view_model.authoritativeFrameIndex();
        if (preview_item) {
            preview_item->submit_render_snapshot(render_snapshot);
            preview_item->set_viewport_time_range(viewport);
            preview_item->set_frame_index(frame);
        }
        if (timeline_item) {
            timeline_item->submit_render_snapshot(render_snapshot);
            timeline_item->set_viewport_time_range(viewport);
            timeline_item->set_frame_index(frame);
        }
        return true;
    };

    QObject::connect(&application_view_model,
                     &space_rhythm::ui::ApplicationViewModel::viewStateChanged,
                     &application,
                     [&application, synchronize_render_items]() {
                         QTimer::singleShot(0, &application, synchronize_render_items);
                     });
    const bool preview_attached = synchronize_render_items();

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
