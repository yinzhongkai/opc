#include <space_rhythm/ui/workspace_service.hpp>

#include <space_rhythm/audio/analysis.hpp>
#include <space_rhythm/audio/render.hpp>
#include <space_rhythm/audio/preview_qt.hpp>
#include <space_rhythm/media/media.hpp>
#include <space_rhythm/media/playback_export.hpp>
#include <space_rhythm/rendering/offscreen_renderer.hpp>

#include <QDateTime>
#include <QFileInfo>
#include <QMetaObject>
#include <QObject>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace space_rhythm::ui {
namespace {

namespace analysis = space_rhythm::audio;
namespace audio_render = space_rhythm::audio::render;
namespace playback = space_rhythm::media::playback_export;

constexpr core::TimeNs default_duration_ns = 10'000'000'000;
constexpr core::DurationNs minimum_viewport_ns = 100'000'000;
constexpr std::uint64_t deterministic_seed = 0x5350414345524859ULL;

std::filesystem::path local_path(QString value)
{
    const QUrl url{value};
    if (url.isLocalFile()) {
        value = url.toLocalFile();
    }
    return std::filesystem::path{value.toStdWString()};
}

QString format_time(core::TimeNs time_ns)
{
    const auto total_ms = static_cast<std::uint64_t>(std::max<core::TimeNs>(0, time_ns) /
                                                     1'000'000);
    const auto milliseconds = total_ms % 1'000U;
    const auto total_seconds = total_ms / 1'000U;
    return QStringLiteral("%1:%2.%3")
        .arg(total_seconds / 60U, 2, 10, QLatin1Char('0'))
        .arg(total_seconds % 60U, 2, 10, QLatin1Char('0'))
        .arg(milliseconds, 3, 10, QLatin1Char('0'));
}

UiErrorDto ui_error(const core::ErrorInfo& error)
{
    return {QString::fromStdString(std::string{core::to_string(error.category)}),
            QString::fromStdString(std::string{core::to_string(error.code)}),
            QString::fromStdString(error.stage),
            QString::fromStdString(error.message_key),
            QString::fromStdString(error.diagnostic_id),
            error.retryable,
            true};
}

UiErrorDto ui_error(const system::SystemError& error)
{
    return {QString::fromStdString(std::string{core::to_string(error.category)}),
            QString::fromStdString(std::string{system::to_string(error.code)}),
            QString::fromStdString(error.stage),
            QString::fromStdString(error.message_key),
            QString::fromStdString(error.diagnostic_id),
            error.retryable,
            true};
}

UiErrorDto local_error(QString stage,
                       QString code,
                       QString message,
                       bool retryable = false)
{
    return {QStringLiteral("internal"),
            std::move(code),
            std::move(stage),
            std::move(message),
            QStringLiteral("ui-integrated-%1")
                .arg(QDateTime::currentMSecsSinceEpoch()),
            retryable,
            true};
}

std::optional<std::pair<double, double>> parse_pointer(const QString& argument)
{
    const auto separator = argument.indexOf(QLatin1Char('|'));
    if (separator <= 0) {
        return std::nullopt;
    }
    bool x_ok = false;
    bool width_ok = false;
    const auto x = argument.first(separator).toDouble(&x_ok);
    const auto width = argument.sliced(separator + 1).toDouble(&width_ok);
    if (!x_ok || !width_ok || !std::isfinite(x) || !std::isfinite(width) || width <= 0.0) {
        return std::nullopt;
    }
    return std::pair{x, width};
}

core::TimeNs media_duration(const media::MediaInfo& info)
{
    if (!info.duration) {
        return default_duration_ns;
    }
    const auto mapped = core::scale_ticks(info.duration->ticks,
                                          info.duration->time_base,
                                          core::RoundingMode::nearest_ties_to_even);
    return mapped && mapped.value() > 0 ? mapped.value() : default_duration_ns;
}

QString media_kind(const media::MediaInfo& info)
{
    const auto has_video = std::ranges::any_of(info.streams, [](const auto& stream) {
        return stream.kind == media::StreamKind::video;
    });
    const auto has_audio = std::ranges::any_of(info.streams, [](const auto& stream) {
        return stream.kind == media::StreamKind::audio;
    });
    return has_video && has_audio ? QStringLiteral("video+audio")
                                  : has_video ? QStringLiteral("video")
                                              : QStringLiteral("audio");
}

struct ImportWork {
    std::filesystem::path path;
    std::shared_ptr<media::MediaSource> source;
    media::MediaSelection selection;
    core::TimeNs duration_ns{default_duration_ns};
    std::optional<core::ErrorInfo> error;
};

struct AnalysisWork {
    analysis::AnalysisResult result;
    analysis::AnalysisParameters parameters;
    audio_render::RenderedPcm source_pcm;
};

struct JobMeta {
    QString request_id;
    QString operation;
    QString subject;
    QString stage;
    std::uint64_t sequence{1};
    core::TimelineRevision base_revision{};
};

class IntegratedWorkspaceService final : public QObject, public WorkspaceService {
public:
    explicit IntegratedWorkspaceService(IntegratedWorkspaceOptions options)
        : options_(std::move(options))
    {
        preview_timer_.setInterval(16);
        preview_timer_.setTimerType(Qt::PreciseTimer);
        QObject::connect(&preview_timer_, &QTimer::timeout, this, [this] { tick_preview(); });
        export_timer_.setInterval(0);
        QObject::connect(&export_timer_, &QTimer::timeout, this, [this] { pump_export(); });
        reset_project(QStringLiteral("未命名节奏"));
        snapshot_.route = UiRoute::start;
        snapshot_.dirty = false;
        if (!options_.project_path.isEmpty()) {
            QTimer::singleShot(0, this, [this] {
                post({bridge_schema_version,
                      qt_string(bridge_contract_version),
                      QStringLiteral("startup-open"),
                      UiCommandKind::open_project_from,
                      options_.project_path});
            });
        }
    }

    ~IntegratedWorkspaceService() override
    {
        cancellation_.cancel();
        for (auto& worker : workers_) {
            worker.request_stop();
            if (worker.joinable()) {
                worker.join();
            }
        }
        if (export_state_) {
            export_state_->renderer->cancel();
            export_state_->session->cancel();
        }
    }

    UiBridgeDescriptor descriptor() const override { return {}; }
    WorkspaceSnapshotDto current_snapshot() const override { return snapshot_; }

    void set_observer(Observer observer) override { observer_ = std::move(observer); }

    void post(UiCommand command) override
    {
        if (command.schema_version != bridge_schema_version ||
            command.contract_version != qt_string(bridge_contract_version)) {
            fail(local_error(QStringLiteral("ui.command"),
                             QStringLiteral("contract-mismatch"),
                             QStringLiteral("ui.bridge.contract_mismatch")));
            return;
        }
        switch (command.kind) {
        case UiCommandKind::create_project:
            reset_project(QStringLiteral("未命名节奏"));
            snapshot_.route = UiRoute::workspace;
            snapshot_.dirty = true;
            publish();
            if (!options_.import_path.isEmpty()) {
                start_import(command.request_id + QStringLiteral("-import"), options_.import_path);
            }
            break;
        case UiCommandKind::open_project:
            reset_project(QStringLiteral("示例节奏项目"));
            snapshot_.route = UiRoute::workspace;
            snapshot_.dirty = true;
            publish();
            break;
        case UiCommandKind::open_project_from:
            open_project(command.argument);
            break;
        case UiCommandKind::return_to_start:
            stop_preview();
            snapshot_.route = UiRoute::start;
            publish();
            break;
        case UiCommandKind::activate_stage:
            snapshot_.active_stage = command.argument;
            publish();
            break;
        case UiCommandKind::import_asset:
            start_import(command.request_id, command.argument);
            break;
        case UiCommandKind::start_analysis:
            start_analysis(command.request_id);
            break;
        case UiCommandKind::cancel_active_task:
            request_cancel();
            break;
        case UiCommandKind::retry_failed_operation:
            retry_failed();
            break;
        case UiCommandKind::save_project:
            save_project(snapshot_.last_saved_path);
            break;
        case UiCommandKind::save_project_to:
            save_project(command.argument);
            break;
        case UiCommandKind::export_project:
            start_export(command.request_id,
                         snapshot_.last_export_path.isEmpty() ? options_.export_path
                                                              : snapshot_.last_export_path);
            break;
        case UiCommandKind::export_project_to:
            start_export(command.request_id, command.argument);
            break;
        case UiCommandKind::toggle_preview:
            toggle_preview();
            break;
        case UiCommandKind::stop_preview:
            stop_preview();
            break;
        case UiCommandKind::timeline_zoom:
            zoom_timeline(command.argument);
            break;
        case UiCommandKind::timeline_pan:
            pan_timeline(command.argument);
            break;
        case UiCommandKind::timeline_seek:
            seek_timeline(command.argument);
            break;
        case UiCommandKind::timeline_select:
            select_event(command.argument);
            break;
        case UiCommandKind::timeline_add_manual_event:
            add_manual_event(command.argument);
            break;
        case UiCommandKind::timeline_toggle_selection:
            toggle_event_selection(command.argument);
            break;
        case UiCommandKind::timeline_drag_begin:
            begin_drag(command.argument);
            break;
        case UiCommandKind::timeline_drag_update:
            update_drag(command.argument);
            break;
        case UiCommandKind::timeline_drag_end:
            drag_.reset();
            break;
        case UiCommandKind::toggle_selected_event_lock:
            toggle_lock();
            break;
        case UiCommandKind::batch_offset_selected:
            batch_offset(command.argument);
            break;
        case UiCommandKind::undo:
            apply_history(true);
            break;
        case UiCommandKind::redo:
            apply_history(false);
            break;
        case UiCommandKind::reconnect_worker:
            snapshot_.worker_connected = true;
            snapshot_.state = stable_state();
            snapshot_.error.reset();
            publish();
            break;
        case UiCommandKind::simulate_worker_disconnect:
            for (const auto& job : coordinator_.worker_disconnected()) {
                const auto found = jobs_.find(job.request.request_id);
                if (found != jobs_.end()) {
                    found->second.stage = QStringLiteral("worker_disconnected");
                }
            }
            cancellation_.cancel();
            active_job_.reset();
            snapshot_.worker_connected = false;
            snapshot_.state = WorkspaceState::failed;
            snapshot_.error = local_error(QStringLiteral("worker.transport"),
                                          QStringLiteral("worker-disconnected"),
                                          QStringLiteral("ui.worker.disconnected"),
                                          true);
            publish();
            break;
        case UiCommandKind::recover_autosave:
        case UiCommandKind::open_primary:
            snapshot_.state = stable_state();
            snapshot_.recovery_available = false;
            publish();
            break;
        }
    }

private:
    struct MediaState {
        std::filesystem::path path;
        std::shared_ptr<media::MediaSource> source;
        media::MediaSelection selection;
        core::TimeNs duration_ns{default_duration_ns};
    };

    struct DragState {
        core::EventId event_id;
        core::TimeNs initial_time_ns{};
        core::TimeNs anchor_time_ns{};
        QString coalescing_key;
    };

    struct ExportState {
        QString request_id;
        std::shared_ptr<const playback::FrozenExportSnapshot> frozen;
        std::unique_ptr<rendering::OffscreenRenderSession> renderer;
        std::unique_ptr<playback::ExportSession> session;
        rendering::FrameIndex next_frame{};
        rendering::FrameIndex frame_count{};
    };

    void reset_project(const QString& title)
    {
        core::TimelineSnapshot initial;
        initial.project_id = core::ProjectId{"project-ui"};
        initial.tracks.push_back(
            core::Track{core::TrackId{"track-0"}, 0, std::string{"Main"}, {}});
        timeline_ = std::make_unique<core::Timeline>(std::move(initial));
        media_.reset();
        analysis_features_.clear();
        analysis_revision_.clear();
        preview_pcm_.reset();
        selected_events_.clear();
        snapshot_ = {};
        snapshot_.route = UiRoute::workspace;
        snapshot_.project_id = QStringLiteral("project-ui");
        snapshot_.project_title = title;
        snapshot_.active_stage = QStringLiteral("import");
        snapshot_.timeline_range = {0, default_duration_ns};
        snapshot_.viewport_range = snapshot_.timeline_range;
        snapshot_.template_parameters = template_parameters();
        snapshot_.worker_connected = true;
        rebuild_render_snapshot();
        sync_timeline_state();
    }

    QVector<TemplateParameterDto> template_parameters() const
    {
        QVector<TemplateParameterDto> result;
        const auto& definition = rendering::template_definition(
            rendering::VisualTemplate::rhythm_line_pulse);
        for (const auto& parameter : definition.integer_parameters) {
            result.push_back({QString::fromStdString(parameter.name),
                              QString::fromStdString(parameter.name),
                              QStringLiteral("工程值"),
                              parameter.minimum,
                              parameter.maximum,
                              parameter.default_value,
                              parameter.default_value,
                              false});
        }
        return result;
    }

    void publish()
    {
        sync_tasks();
        sync_timeline_state();
        if (observer_) {
            observer_(snapshot_);
        }
    }

    void sync_timeline_state()
    {
        if (!timeline_) {
            return;
        }
        const auto value = timeline_->snapshot();
        snapshot_.timeline_revision = value->timeline_revision;
        snapshot_.dirty = timeline_->is_dirty();
        snapshot_.can_undo = timeline_->undo_depth() > 0;
        snapshot_.can_redo = timeline_->redo_depth() > 0;
        sync_selected_event(*value);
    }

    void sync_selected_event(const core::TimelineSnapshot& value)
    {
        std::erase_if(selected_events_, [&value](const core::EventId& selected_id) {
            return std::ranges::none_of(value.events, [&selected_id](const auto& event) {
                return event.id == selected_id;
            });
        });
        if (!selected_events_.empty()) {
            snapshot_.selected_event_id = QString::fromStdString(
                selected_events_.back().value);
        }
        const auto selected = std::ranges::find_if(value.events, [this](const auto& event) {
            return QString::fromStdString(event.id.value) == snapshot_.selected_event_id;
        });
        if (selected == value.events.end()) {
            snapshot_.selected_event_id.clear();
            snapshot_.selected_event_text = QStringLiteral("未选择事件");
            snapshot_.selected_event_locked = false;
            return;
        }
        snapshot_.selected_event_locked = selected->locked;
        snapshot_.selected_event_text = selected_events_.size() > 1
                                            ? QStringLiteral("已选择 %1 个事件 · 主事件 %2")
                                                  .arg(selected_events_.size())
                                                  .arg(QString::fromStdString(selected->id.value))
                                            : QStringLiteral("%1 · %2 · %3")
                                                  .arg(QString::fromStdString(selected->id.value),
                                                       QString::fromLatin1(
                                                           core::to_string(selected->kind)),
                                                       format_time(selected->time_ns));
    }

    void sync_tasks()
    {
        snapshot_.tasks.clear();
        for (const auto& request_id : job_order_) {
            const auto found = jobs_.find(request_id.toStdString());
            if (found == jobs_.end()) {
                continue;
            }
            const auto coordinated = coordinator_.snapshot(found->first);
            if (!coordinated) {
                continue;
            }
            const auto& job = coordinated.value();
            snapshot_.tasks.push_back({found->second.request_id,
                                       found->second.operation,
                                       found->second.subject,
                                       found->second.stage,
                                       job.status,
                                       job.progress_ppm,
                                       job.error && job.error->retryable,
                                       job.error ? QString::fromStdString(job.error->diagnostic_id)
                                                 : QString{}});
        }
    }

    bool begin_job(QString request_id, QString operation, QString subject, QString stage)
    {
        if (active_job_) {
            fail(local_error(QStringLiteral("ui.job.submit"),
                             QStringLiteral("job-active"),
                             QStringLiteral("ui.job.already_active"),
                             true));
            return false;
        }
        JobMeta meta{request_id,
                     std::move(operation),
                     std::move(subject),
                     std::move(stage),
                     1,
                     timeline_->snapshot()->timeline_revision};
        system::JobRequest request;
        request.request_id = request_id.toStdString();
        request.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
        request.operation = "ui-workspace";
        request.base_timeline_revision = meta.base_revision;
        request.input_fingerprint = media_ ? media_->source->info().source_fingerprint_sha256
                                           : std::string(64, '0');
        request.parameters_digest = std::string(64, 'a');
        request.timeout_ms = 120'000;
        const auto submitted = coordinator_.submit(
            request, static_cast<std::uint64_t>(QDateTime::currentMSecsSinceEpoch()));
        if (!submitted) {
            fail(ui_error(submitted.error()));
            return false;
        }
        jobs_.insert_or_assign(request.request_id, meta);
        job_order_.push_back(request_id);
        active_job_ = request.request_id;
        system::JobUpdate accepted;
        accepted.request_id = request.request_id;
        accepted.job_id = request.job_id;
        accepted.sequence = 1;
        accepted.type = system::JobUpdateType::accepted;
        const auto update = coordinator_.apply_update(accepted, meta.base_revision);
        if (!update) {
            fail(ui_error(update.error()));
            active_job_.reset();
            return false;
        }
        cancellation_ = core::CancellationToken{};
        snapshot_.state = WorkspaceState::running;
        snapshot_.error.reset();
        publish();
        return true;
    }

    void job_progress(const QString& request_id, core::NormPpm progress, QString stage)
    {
        const auto key = request_id.toStdString();
        const auto found = jobs_.find(key);
        if (!active_job_ || *active_job_ != key || found == jobs_.end()) {
            return;
        }
        found->second.stage = std::move(stage);
        system::JobUpdate update;
        update.request_id = key;
        update.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
        update.sequence = ++found->second.sequence;
        update.type = system::JobUpdateType::progress;
        update.progress_ppm = progress;
        [[maybe_unused]] const auto applied = coordinator_.apply_update(
            update, timeline_->snapshot()->timeline_revision);
        publish();
    }

    void finish_job_success(const QString& request_id)
    {
        const auto key = request_id.toStdString();
        const auto found = jobs_.find(key);
        if (!active_job_ || *active_job_ != key || found == jobs_.end()) {
            return;
        }
        system::JobUpdate update;
        update.request_id = key;
        update.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
        update.sequence = ++found->second.sequence;
        update.type = system::JobUpdateType::succeeded;
        update.result_base_revision = found->second.base_revision;
        update.result = system::DataReference{system::DataReferenceKind::cache,
                                              "ui-integrated/result",
                                              0,
                                              std::string(64, 'b')};
        [[maybe_unused]] const auto applied = coordinator_.apply_update(
            update, found->second.base_revision);
        active_job_.reset();
        snapshot_.state = stable_state();
        snapshot_.error.reset();
        publish();
    }

    void finish_job_failure(const QString& request_id, UiErrorDto error)
    {
        const auto key = request_id.toStdString();
        const auto found = jobs_.find(key);
        if (!active_job_ || *active_job_ != key) {
            return;
        }
        if (found != jobs_.end()) {
            system::JobUpdate update;
            update.request_id = key;
            update.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
            update.sequence = ++found->second.sequence;
            update.type = system::JobUpdateType::failed;
            update.error = system::SystemError{core::ErrorCategory::internal,
                                               system::SystemErrorCode::io_error,
                                               error.stage.toStdString(),
                                               error.diagnostic_id.toStdString(),
                                               error.retryable,
                                               error.message_key.toStdString(),
                                               {}};
            [[maybe_unused]] const auto applied = coordinator_.apply_update(
                update, timeline_->snapshot()->timeline_revision);
        }
        active_job_.reset();
        fail(std::move(error));
    }

    void finish_job_cancelled(const QString& request_id)
    {
        const auto key = request_id.toStdString();
        const auto found = jobs_.find(key);
        if (!active_job_ || *active_job_ != key) {
            return;
        }
        if (found != jobs_.end()) {
            system::JobUpdate acknowledged;
            acknowledged.request_id = key;
            acknowledged.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
            acknowledged.sequence = ++found->second.sequence;
            acknowledged.type = system::JobUpdateType::cancellation_acknowledged;
            [[maybe_unused]] const auto ack = coordinator_.apply_update(
                acknowledged, timeline_->snapshot()->timeline_revision);
            auto cancelled = acknowledged;
            cancelled.sequence = ++found->second.sequence;
            cancelled.type = system::JobUpdateType::cancelled;
            [[maybe_unused]] const auto terminal = coordinator_.apply_update(
                cancelled, timeline_->snapshot()->timeline_revision);
        }
        active_job_.reset();
        snapshot_.state = stable_state();
        snapshot_.error.reset();
        publish();
    }

    void start_import(const QString& request_id, const QString& path_value)
    {
        const auto path = local_path(path_value);
        if (path.empty() || !begin_job(request_id,
                                       QStringLiteral("导入媒体"),
                                       QFileInfo(path_value).fileName(),
                                       QStringLiteral("probe"))) {
            return;
        }
        last_failed_kind_ = UiCommandKind::import_asset;
        last_failed_argument_ = path_value;
        const auto cancellation = cancellation_;
        workers_.emplace_back([this, request_id, path, cancellation] {
            auto work = std::make_shared<ImportWork>();
            work->path = path;
            auto opened = media::MediaSource::open(path, {}, &cancellation);
            if (!opened) {
                work->error = opened.error();
            } else {
                work->source = opened.value();
                work->duration_ns = media_duration(work->source->info());
                media::StreamSelectionRequest video;
                video.mode = media::SelectionMode::optional_default_then_lowest_index;
                media::StreamSelectionRequest audio;
                audio.mode = media::SelectionMode::optional_default_then_lowest_index;
                auto selected = work->source->select(video, audio, &cancellation);
                if (!selected) {
                    work->error = selected.error();
                } else {
                    work->selection = std::move(selected.value());
                }
            }
            QMetaObject::invokeMethod(this, [this, request_id, work] {
                if (!active_job_ || *active_job_ != request_id.toStdString()) {
                    return;
                }
                if (work->error) {
                    if (work->error->category == core::ErrorCategory::cancelled) {
                        finish_job_cancelled(request_id);
                    } else {
                        finish_job_failure(request_id, ui_error(*work->error));
                    }
                    return;
                }
                media_ = MediaState{work->path,
                                    std::move(work->source),
                                    std::move(work->selection),
                                    work->duration_ns};
                snapshot_.timeline_range = {0, work->duration_ns};
                snapshot_.viewport_range = snapshot_.timeline_range;
                const auto& info = media_->source->info();
                snapshot_.assets = {{QStringLiteral("asset-primary"),
                                     QString::fromStdWString(media_->path.filename().wstring()),
                                     media_kind(info),
                                     QStringLiteral("ready"),
                                     format_time(media_->duration_ns),
                                     QStringLiteral("FFmpeg %1 · %2 个流")
                                         .arg(QString::fromStdString(
                                                  info.probe_implementation.ffmpeg_version))
                                         .arg(info.streams.size())}};
                snapshot_.active_stage = QStringLiteral("analyze");
                rebuild_render_snapshot();
                finish_job_success(request_id);
            }, Qt::QueuedConnection);
        });
    }

    void start_analysis(const QString& request_id)
    {
        if (!media_ || media_->selection.audio.selected.empty()) {
            fail(local_error(QStringLiteral("audio.decode"),
                             QStringLiteral("missing-audio"),
                             QStringLiteral("ui.analysis.audio_required")));
            return;
        }
        if (!begin_job(request_id,
                       QStringLiteral("节奏分析"),
                       QString::fromStdWString(media_->path.filename().wstring()),
                       QStringLiteral("decode_audio"))) {
            return;
        }
        last_failed_kind_ = UiCommandKind::start_analysis;
        last_failed_argument_.clear();
        const auto source = media_->source;
        const auto selection = media_->selection;
        const auto stream = selection.audio.selected.front();
        const auto cancellation = cancellation_;
        workers_.emplace_back([this, request_id, source, selection, stream, cancellation] {
            auto work = std::make_shared<AnalysisWork>();
            work->parameters = analysis::production_parameters(48'000);
            std::vector<analysis::DspPcmBuffer> buffers;
            analysis::PcmNarrowAdapter adapter;
            std::optional<core::ErrorInfo> adapter_error;
            media::AudioCallbacks callbacks;
            callbacks.on_format_changed = [](const media::FormatChanged&) {
                return media::PublishResult::accepted;
            };
            callbacks.on_pcm = [&](media::PcmBuffer pcm) {
                auto adapted = adapter.adapt(pcm);
                if (!adapted) {
                    adapter_error = adapted.error();
                    return media::PublishResult::closed;
                }
                auto dsp = std::move(adapted.value());
                for (std::uint64_t frame = 0; frame < dsp.valid_frame_count; ++frame) {
                    const auto sample = dsp.sample(frame, 0);
                    if (!sample) {
                        adapter_error = sample.error();
                        return media::PublishResult::closed;
                    }
                    work->source_pcm.interleaved_f32.push_back(sample.value());
                }
                buffers.push_back(std::move(dsp));
                return cancellation.is_cancelled() ? media::PublishResult::cancelled
                                                   : media::PublishResult::accepted;
            };
            media::AudioOutputSpec output;
            output.sample_rate = 48'000;
            output.channels = 1;
            auto decoded = source->decode_audio(selection,
                                                stream,
                                                output,
                                                {},
                                                callbacks,
                                                &cancellation);
            if (!decoded) {
                work->result.error = decoded.error();
                work->result.status = decoded.error().category == core::ErrorCategory::cancelled
                                          ? analysis::AnalysisStatus::cancelled
                                          : analysis::AnalysisStatus::failed;
            } else if (adapter_error) {
                work->result.error = std::move(adapter_error);
                work->result.status = analysis::AnalysisStatus::failed;
            } else {
                work->source_pcm.sample_rate = audio_render::render_sample_rate;
                work->source_pcm.channel_count = 1;
                work->source_pcm.frame_count = static_cast<std::uint64_t>(
                    work->source_pcm.interleaved_f32.size());
                QMetaObject::invokeMethod(this, [this, request_id] {
                    job_progress(request_id, 450'000, QStringLiteral("feature_extract"));
                }, Qt::QueuedConnection);
                analysis::Analyzer analyzer;
                work->result = analyzer.analyze(buffers,
                                                work->parameters,
                                                {},
                                                &cancellation);
            }
            QMetaObject::invokeMethod(this, [this, request_id, work] {
                complete_analysis(request_id, *work);
            }, Qt::QueuedConnection);
        });
    }

    void complete_analysis(const QString& request_id, const AnalysisWork& work)
    {
        if (!active_job_ || *active_job_ != request_id.toStdString()) {
            return;
        }
        const auto job = jobs_.find(request_id.toStdString());
        if (job == jobs_.end() || job->second.base_revision !=
                                      timeline_->snapshot()->timeline_revision) {
            finish_job_failure(
                request_id,
                ui_error(core::ErrorInfo{core::schema_version,
                                         core::ErrorCategory::conflict,
                                         core::ErrorCode::stale_revision,
                                         "ui.analysis.publish",
                                         "ui-integrated:stale-analysis",
                                         true,
                                         "stale_revision",
                                         {},
                                         {}}));
            return;
        }
        if (work.result.status == analysis::AnalysisStatus::cancelled) {
            finish_job_cancelled(request_id);
            return;
        }
        if (work.result.status == analysis::AnalysisStatus::failed) {
            finish_job_failure(request_id,
                               work.result.error ? ui_error(*work.result.error)
                                                 : local_error(
                                                       QStringLiteral("audio.analysis"),
                                                       QStringLiteral("analysis-failed"),
                                                       QStringLiteral("ui.analysis.failed"),
                                                       true));
            return;
        }
        job_progress(request_id, 800'000, QStringLiteral("merge_timeline"));
        core::MergeAnalysisCandidates merge;
        merge.analysis_revision = work.result.candidates.empty()
                                      ? core::AnalysisRevision{"analysis-empty"}
                                      : work.result.candidates.front().analysis_revision;
        merge.parameters.default_track_id = core::TrackId{"track-0"};
        merge.parameters.protection_window_ns = 20'000'000;
        merge.parameters.minimum_spacing_ns = 10'000'000;
        merge.parameters.seed = deterministic_seed;
        for (const auto& candidate : work.result.candidates) {
            core::EventSource source;
            source.origin = core::EventOrigin::analysis;
            source.producer_id = candidate.source.producer_id;
            source.producer_version = candidate.source.producer_version;
            source.input_fingerprint = candidate.source.input_fingerprint_sha256;
            source.parameters_digest = candidate.source.parameters_digest_sha256;
            source.analysis_revision = candidate.analysis_revision;
            source.candidate_ids = {candidate.id};
            merge.candidates.push_back({candidate.id,
                                        core::TrackId{"track-0"},
                                        candidate.time_ns,
                                        candidate.duration_ns,
                                        candidate.kind == analysis::CandidateKind::beat
                                            ? core::EventKind::beat
                                            : core::EventKind::onset,
                                        std::move(source),
                                        candidate.strength_ppm,
                                        candidate.confidence_ppm,
                                        core::VersionedOpaqueObject{
                                            "space-rhythm.audio-analysis", 1, {}, {}},
                                        {}});
        }
        core::TransactionRequest transaction;
        transaction.transaction_id = core::TransactionId{
            QStringLiteral("analysis-%1").arg(request_id).toStdString()};
        transaction.base_timeline_revision = timeline_->snapshot()->timeline_revision;
        transaction.origin = core::TransactionOrigin::analysis;
        transaction.operations = {std::move(merge)};
        const auto result = timeline_->submit(transaction);
        if (result.status == core::TransactionStatus::rejected) {
            finish_job_failure(request_id, ui_error(*result.error));
            return;
        }
        analysis_features_ = work.result.feature_frames;
        analysis_parameters_ = work.parameters;
        preview_pcm_ = work.source_pcm;
        analysis_revision_ = work.result.candidates.empty()
                                 ? std::string{"analysis-empty"}
                                 : work.result.candidates.front().analysis_revision.value;
        snapshot_.active_stage = QStringLiteral("edit");
        rebuild_render_snapshot();
        autosave();
        finish_job_success(request_id);
    }

    void open_project(const QString& path_value)
    {
        snapshot_.route = UiRoute::workspace;
        snapshot_.state = WorkspaceState::loading;
        publish();
        QTimer::singleShot(0, this, [this, path_value] {
            const auto loaded = project_store_.load(local_path(path_value));
            if (!loaded) {
                fail(ui_error(loaded.error()));
                return;
            }
            timeline_ = std::make_unique<core::Timeline>(loaded.value().document.timeline);
            snapshot_.project_id = QString::fromStdString(
                loaded.value().document.timeline.project_id.value);
            const auto title = loaded.value().document.settings.find("project.title");
            snapshot_.project_title = title == loaded.value().document.settings.end()
                                          ? QFileInfo(path_value).completeBaseName()
                                          : QString::fromStdString(title->second);
            snapshot_.last_saved_path = path_value;
            snapshot_.read_only = !QFileInfo(QString::fromStdWString(
                local_path(path_value).wstring())).isWritable();
            snapshot_.state = stable_state();
            const auto media_path = loaded.value().document.settings.find("media.path");
            if (media_path != loaded.value().document.settings.end()) {
                start_import(QStringLiteral("open-media-%1")
                                 .arg(QDateTime::currentMSecsSinceEpoch()),
                             QString::fromStdWString(
                                 std::filesystem::path{media_path->second}.wstring()));
                return;
            }
            rebuild_render_snapshot();
            publish();
        });
    }

    system::ProjectDocument project_document() const
    {
        system::ProjectDocument document;
        document.app_version = "0.1.0";
        document.timeline = *timeline_->snapshot();
        document.settings["project.title"] = snapshot_.project_title.toStdString();
        if (media_) {
            document.settings["media.path"] = media_->path.string();
            std::error_code error;
            const auto size = std::filesystem::file_size(media_->path, error);
            document.assets.push_back({"asset-primary",
                                       media_->path,
                                       error ? 0 : size,
                                       media_->source->info().source_fingerprint_sha256});
        }
        return document;
    }

    void save_project(QString path_value)
    {
        if (path_value.isEmpty()) {
            path_value = options_.project_path;
        }
        if (path_value.isEmpty()) {
            fail(local_error(QStringLiteral("project.save"),
                             QStringLiteral("path-required"),
                             QStringLiteral("ui.project.path_required")));
            return;
        }
        const auto saved = project_store_.save(local_path(path_value), project_document());
        if (!saved) {
            fail(ui_error(saved.error()));
            return;
        }
        timeline_->mark_saved();
        snapshot_.last_saved_path = path_value;
        snapshot_.state = stable_state();
        snapshot_.error.reset();
        [[maybe_unused]] const auto marker = project_store_.mark_session_clean(
            local_path(path_value), true);
        publish();
    }

    void autosave()
    {
        if (snapshot_.last_saved_path.isEmpty() || snapshot_.read_only) {
            return;
        }
        [[maybe_unused]] const auto saved = project_store_.save_autosave(
            local_path(snapshot_.last_saved_path), project_document());
        [[maybe_unused]] const auto marker = project_store_.mark_session_clean(
            local_path(snapshot_.last_saved_path), false);
    }

    std::vector<rendering::RenderSeries> render_series() const
    {
        if (analysis_features_.empty() || !media_) {
            return {};
        }
        rendering::RenderSeries series;
        series.id = rendering::RenderSeriesId{"series-energy"};
        series.analysis_revision = core::AnalysisRevision{analysis_revision_};
        series.definition_id = "short-time-energy";
        series.source_contract_version = std::string{analysis::contract_version};
        series.source_schema_version = analysis::schema_version;
        series.input_fingerprint_sha256 = media_->source->info().source_fingerprint_sha256;
        series.parameters_digest_sha256 = analysis_parameters_.parameters_digest_sha256;
        series.content_digest_sha256 = analysis_parameters_.parameters_digest_sha256;
        for (const auto& feature : analysis_features_) {
            if (!snapshot_.timeline_range.contains(feature.anchor_time_ns) ||
                (!series.samples.empty() &&
                 series.samples.back().time_ns == feature.anchor_time_ns)) {
                continue;
            }
            series.samples.push_back({feature.anchor_time_ns,
                                      feature.short_time_energy.normalized_ppm.value_or(0),
                                      feature.spectral_change.normalized_ppm});
        }
        return {std::move(series)};
    }

    void rebuild_render_snapshot()
    {
        const auto timeline_snapshot = timeline_->snapshot();
        auto parameters = rendering::make_template_parameters(
            rendering::VisualTemplate::rhythm_line_pulse);
        if (!parameters) {
            fail(ui_error(parameters.error()));
            return;
        }
        rendering::RenderRecipe recipe;
        recipe.recipe_id = rendering::RenderRecipeId{
            "recipe-ui-" + std::to_string(timeline_snapshot->timeline_revision)};
        recipe.project_id = timeline_snapshot->project_id;
        recipe.timeline_revision = timeline_snapshot->timeline_revision;
        recipe.template_parameters = std::move(parameters.value());
        recipe.output.width_px = 640;
        recipe.output.height_px = 360;
        recipe.time_range = snapshot_.timeline_range;
        recipe.frame_rate = {30, 1};
        recipe.deterministic_seed = deterministic_seed;
        recipe.required_features = {"render.rgba8-srgb-v1", "render.snapshot-v1"};
        auto series = render_series();
        for (const auto& value : series) {
            recipe.feature_inputs.push_back({value.analysis_revision,
                                             value.source_contract_version,
                                             value.source_schema_version,
                                             value.input_fingerprint_sha256,
                                             value.parameters_digest_sha256,
                                             value.content_digest_sha256});
        }
        auto made = rendering::make_render_snapshot(
            rendering::RenderSnapshotId{
                "snapshot-ui-" + std::to_string(timeline_snapshot->timeline_revision)},
            std::move(recipe),
            *timeline_snapshot,
            std::move(series));
        if (!made) {
            fail(ui_error(made.error()));
            return;
        }
        snapshot_.render_snapshot = std::move(made.value());
    }

    std::optional<core::TimeNs> pointer_time(const QString& argument) const
    {
        if (!snapshot_.render_snapshot) {
            return std::nullopt;
        }
        const auto pointer = parse_pointer(argument);
        if (!pointer) {
            return std::nullopt;
        }
        const auto width_sp = static_cast<std::int64_t>(std::llround(
            pointer->second * static_cast<double>(rendering::subpixels_per_logical_pixel)));
        const auto x_sp = static_cast<std::int64_t>(std::llround(
            pointer->first * static_cast<double>(rendering::subpixels_per_logical_pixel)));
        rendering::CoordinateTransform transform{snapshot_.render_snapshot->id(),
                                                 snapshot_.timeline_revision,
                                                 snapshot_.viewport_range,
                                                 {0, 0, width_sp, 100 * 1024}};
        const auto mapped = rendering::item_x_sp_to_time(transform, x_sp);
        return mapped ? std::optional<core::TimeNs>{mapped.value()} : std::nullopt;
    }

    void select_event(const QString& argument)
    {
        const auto best = event_at(argument);
        selected_events_ = best ? std::vector<core::EventId>{*best}
                                : std::vector<core::EventId>{};
        snapshot_.selected_event_id = best ? QString::fromStdString(best->value) : QString{};
        publish();
    }

    std::optional<core::EventId> event_at(const QString& argument) const
    {
        const auto time = pointer_time(argument);
        if (!time) {
            return std::nullopt;
        }
        const auto value = timeline_->snapshot();
        const auto tolerance = std::max<core::DurationNs>(
            5'000'000,
            (snapshot_.viewport_range.end_ns - snapshot_.viewport_range.start_ns) / 40);
        const core::RhythmEvent* best = nullptr;
        core::DurationNs distance = std::numeric_limits<core::DurationNs>::max();
        for (const auto& event : value->events) {
            const auto candidate_distance = static_cast<core::DurationNs>(
                std::llabs(event.time_ns - *time));
            if (candidate_distance < distance) {
                best = &event;
                distance = candidate_distance;
            }
        }
        return best && distance <= tolerance ? std::optional<core::EventId>{best->id}
                                             : std::nullopt;
    }

    void toggle_event_selection(const QString& argument)
    {
        const auto event_id = event_at(argument);
        if (!event_id) {
            return;
        }
        const auto selected = std::ranges::find(selected_events_, *event_id);
        if (selected == selected_events_.end()) {
            selected_events_.push_back(*event_id);
        } else if (selected_events_.size() > 1) {
            selected_events_.erase(selected);
        }
        snapshot_.selected_event_id = QString::fromStdString(selected_events_.back().value);
        publish();
    }

    void begin_drag(const QString& argument)
    {
        select_event(argument);
        if (snapshot_.selected_event_id.isEmpty() || snapshot_.selected_event_locked ||
            !timeline_edit_allowed()) {
            return;
        }
        const auto anchor = pointer_time(argument);
        const auto value = timeline_->snapshot();
        const auto selected = std::ranges::find_if(value->events, [this](const auto& event) {
            return QString::fromStdString(event.id.value) == snapshot_.selected_event_id;
        });
        if (!anchor || selected == value->events.end()) {
            return;
        }
        drag_ = DragState{selected->id,
                          selected->time_ns,
                          *anchor,
                          QStringLiteral("drag-%1").arg(snapshot_.selected_event_id)};
    }

    void add_manual_event(const QString& argument)
    {
        const auto time = pointer_time(argument);
        if (!time || !timeline_edit_allowed()) {
            return;
        }
        core::RhythmEvent event;
        event.id = core::EventId{"manual-" + std::to_string(++transaction_sequence_)};
        event.track_id = core::TrackId{"track-0"};
        event.time_ns = *time;
        event.kind = core::EventKind::manual;
        event.source.origin = core::EventOrigin::user;
        event.source.producer_id = "space-rhythm.ui";
        event.source.producer_version = std::string{bridge_contract_version};
        event.strength_ppm = 800'000;
        event.user_edited = true;
        snapshot_.selected_event_id = QString::fromStdString(event.id.value);
        selected_events_ = {event.id};
        core::TransactionRequest transaction;
        transaction.transaction_id = core::TransactionId{
            "add-manual-" + std::to_string(transaction_sequence_)};
        transaction.base_timeline_revision = timeline_->snapshot()->timeline_revision;
        transaction.operations = {core::AddEvent{std::move(event)}};
        apply_transaction(std::move(transaction));
    }

    void update_drag(const QString& argument)
    {
        if (!drag_ || !timeline_edit_allowed()) {
            return;
        }
        const auto current = pointer_time(argument);
        if (!current) {
            return;
        }
        const auto delta = core::checked_subtract(*current, drag_->anchor_time_ns);
        if (!delta) {
            fail(ui_error(delta.error()));
            return;
        }
        const auto target = core::checked_add(drag_->initial_time_ns, delta.value());
        if (!target) {
            fail(ui_error(target.error()));
            return;
        }
        core::TransactionRequest transaction;
        transaction.transaction_id = core::TransactionId{
            QStringLiteral("%1-%2")
                .arg(drag_->coalescing_key)
                .arg(++transaction_sequence_)
                .toStdString()};
        transaction.base_timeline_revision = timeline_->snapshot()->timeline_revision;
        transaction.operations = {core::MoveEvent{drag_->event_id,
                                                   std::clamp(target.value(),
                                                              snapshot_.timeline_range.start_ns,
                                                              snapshot_.timeline_range.end_ns - 1),
                                                   std::nullopt}};
        transaction.coalescing_key = drag_->coalescing_key.toStdString();
        apply_transaction(std::move(transaction));
    }

    void toggle_lock()
    {
        if (snapshot_.selected_event_id.isEmpty() || !timeline_edit_allowed()) {
            return;
        }
        core::TransactionRequest transaction;
        transaction.transaction_id = core::TransactionId{
            "lock-" + std::to_string(++transaction_sequence_)};
        transaction.base_timeline_revision = timeline_->snapshot()->timeline_revision;
        transaction.operations = {core::SetEventLocked{
            core::EventId{snapshot_.selected_event_id.toStdString()},
            !snapshot_.selected_event_locked}};
        apply_transaction(std::move(transaction));
    }

    void batch_offset(const QString& milliseconds)
    {
        bool valid = false;
        const auto value = milliseconds.toLongLong(&valid);
        if (!valid || snapshot_.selected_event_id.isEmpty() || !timeline_edit_allowed() ||
            value > std::numeric_limits<core::TimeNs>::max() / 1'000'000 ||
            value < std::numeric_limits<core::TimeNs>::min() / 1'000'000) {
            return;
        }
        core::TransactionRequest transaction;
        transaction.transaction_id = core::TransactionId{
            "batch-offset-" + std::to_string(++transaction_sequence_)};
        transaction.base_timeline_revision = timeline_->snapshot()->timeline_revision;
        transaction.operations = {core::BatchOffsetEvents{
            selected_events_,
            static_cast<core::TimeNs>(value) * 1'000'000}};
        apply_transaction(std::move(transaction));
    }

    void apply_transaction(core::TransactionRequest transaction)
    {
        const auto result = timeline_->submit(transaction);
        if (result.status == core::TransactionStatus::rejected) {
            fail(ui_error(*result.error));
            return;
        }
        rebuild_render_snapshot();
        autosave();
        publish();
    }

    void apply_history(bool undo)
    {
        if (!timeline_edit_allowed()) {
            return;
        }
        const auto revision = timeline_->snapshot()->timeline_revision;
        const auto result = undo ? timeline_->undo(revision) : timeline_->redo(revision);
        if (result.status == core::TransactionStatus::rejected) {
            fail(ui_error(*result.error));
            return;
        }
        rebuild_render_snapshot();
        autosave();
        publish();
    }

    void zoom_timeline(const QString& argument)
    {
        bool valid = false;
        const auto steps = argument.toInt(&valid);
        if (!valid || steps == 0 || steps < -16 || steps > 16) {
            return;
        }
        auto duration = snapshot_.viewport_range.end_ns - snapshot_.viewport_range.start_ns;
        for (auto index = 0; index < std::abs(steps); ++index) {
            duration = steps > 0 ? duration * 4 / 5 : duration * 5 / 4;
        }
        const auto full = snapshot_.timeline_range.end_ns - snapshot_.timeline_range.start_ns;
        duration = std::clamp(duration, std::min(minimum_viewport_ns, full), full);
        const auto center = snapshot_.viewport_range.start_ns +
                            (snapshot_.viewport_range.end_ns -
                             snapshot_.viewport_range.start_ns) /
                                2;
        auto start = center - duration / 2;
        start = std::clamp(start,
                           snapshot_.timeline_range.start_ns,
                           snapshot_.timeline_range.end_ns - duration);
        snapshot_.viewport_range = {start, start + duration};
        publish();
    }

    void pan_timeline(const QString& argument)
    {
        const auto pointer = parse_pointer(argument);
        if (!pointer) {
            return;
        }
        const auto duration = snapshot_.viewport_range.end_ns -
                              snapshot_.viewport_range.start_ns;
        const auto delta = static_cast<core::DurationNs>(std::llround(
            -pointer->first / pointer->second * static_cast<double>(duration)));
        const auto full_start = snapshot_.timeline_range.start_ns;
        const auto full_end = snapshot_.timeline_range.end_ns;
        const auto start = std::clamp(snapshot_.viewport_range.start_ns + delta,
                                      full_start,
                                      full_end - duration);
        snapshot_.viewport_range = {start, start + duration};
        publish();
    }

    void seek_timeline(const QString& argument)
    {
        const auto time = pointer_time(argument);
        if (!time) {
            return;
        }
        snapshot_.preview_time_ns = *time;
        update_frame_for_time(*time);
        if (snapshot_.preview_state == PreviewState::playing) {
            preview_timer_.stop();
            if (!create_preview_clock(*time, std::chrono::steady_clock::now())) {
                return;
            }
            preview_timer_.start();
        } else if (preview_clock_) {
            const auto update = preview_clock_->seek(*time, std::chrono::steady_clock::now());
            if (update) {
                apply_preview_update(update.value());
            }
        }
        publish();
    }

    void toggle_preview()
    {
        if (!snapshot_.render_snapshot) {
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        if (snapshot_.preview_state == PreviewState::playing && preview_clock_) {
            update_audio_clock();
            const auto paused = preview_clock_->pause(now);
            if (paused) {
                apply_preview_update(paused.value());
            }
            snapshot_.preview_state = PreviewState::paused;
            preview_timer_.stop();
            audio_preview_.stop();
            audio_clock_active_ = false;
            publish();
            return;
        }
        if (snapshot_.preview_state == PreviewState::paused && preview_clock_ &&
            preview_clock_->clock_mode() == playback::PreviewClockMode::monotonic) {
            const auto resumed = preview_clock_->resume(now);
            if (!resumed) {
                fail(ui_error(resumed.error()));
                return;
            }
        } else {
            const auto start_time = snapshot_.preview_time_ns >=
                                            snapshot_.timeline_range.end_ns
                                        ? snapshot_.timeline_range.start_ns
                                        : snapshot_.preview_time_ns;
            if (!create_preview_clock(start_time, now)) {
                return;
            }
        }
        snapshot_.preview_state = PreviewState::playing;
        snapshot_.active_stage = QStringLiteral("preview");
        preview_timer_.start();
        publish();
    }

    bool create_preview_clock(core::TimeNs playhead,
                              std::chrono::steady_clock::time_point now)
    {
        preview_clock_.reset();
        audio_preview_.stop();
        audio_clock_active_ = false;
        playback::PreviewConfiguration configuration;
        configuration.timeline_range = snapshot_.timeline_range;
        configuration.initial_playhead_time_ns = playhead;
        audio_clock_active_ = start_audio_preview(playhead);
        if (audio_clock_active_) {
            configuration.audio_sample_rate = audio_render::render_sample_rate;
        }
        for (rendering::FrameIndex index = 0;; ++index) {
            const auto time = rendering::frame_time_ns(
                snapshot_.render_snapshot->recipe(), index);
            if (!time || time.value() >= snapshot_.timeline_range.end_ns) {
                break;
            }
            configuration.video_frames.push_back({index, time.value()});
        }
        // T-019's C++ monotonic fallback is authoritative when no QAudioSink
        // processed-frame source is active. QML never advances this clock.
        auto created = playback::PreviewSynchronizer::create(std::move(configuration), now);
        if (!created) {
            audio_preview_.stop();
            audio_clock_active_ = false;
            fail(ui_error(created.error()));
            return false;
        }
        preview_clock_ = std::move(created.value());
        return true;
    }

    void tick_preview()
    {
        if (!preview_clock_) {
            return;
        }
        update_audio_clock();
        const auto update = preview_clock_->tick(std::chrono::steady_clock::now());
        if (!update) {
            snapshot_.preview_state = PreviewState::error;
            preview_timer_.stop();
            fail(ui_error(update.error()));
            return;
        }
        apply_preview_update(update.value());
        if (snapshot_.preview_time_ns >= snapshot_.timeline_range.end_ns) {
            preview_timer_.stop();
            audio_preview_.stop();
            snapshot_.preview_state = PreviewState::paused;
        }
        publish();
    }

    bool start_audio_preview(core::TimeNs playhead)
    {
        if (options_.headless || !preview_pcm_ || preview_pcm_->frame_count == 0 ||
            playhead < snapshot_.timeline_range.start_ns) {
            return false;
        }
        const auto offset_time = playhead - snapshot_.timeline_range.start_ns;
        const auto first_frame = static_cast<std::uint64_t>(
            static_cast<long double>(offset_time) * audio_render::render_sample_rate /
            1'000'000'000.0L);
        if (first_frame >= preview_pcm_->frame_count) {
            return false;
        }
        audio_render::RenderedPcm sliced;
        sliced.sample_rate = preview_pcm_->sample_rate;
        sliced.channel_count = preview_pcm_->channel_count;
        sliced.first_frame = 0;
        sliced.frame_count = preview_pcm_->frame_count - first_frame;
        const auto channel_count = static_cast<std::size_t>(sliced.channel_count);
        const auto begin = static_cast<std::size_t>(first_frame) * channel_count;
        sliced.interleaved_f32.assign(preview_pcm_->interleaved_f32.begin() +
                                         static_cast<std::ptrdiff_t>(begin),
                                     preview_pcm_->interleaved_f32.end());
        const auto started = audio_preview_.start_default(sliced);
        return started.started;
    }

    void update_audio_clock()
    {
        if (!audio_clock_active_ || !preview_clock_) {
            return;
        }
        const auto frames = audio_preview_.processed_frames();
        if (!frames) {
            return;
        }
        const auto updated = preview_clock_->set_audio_played_frames(*frames);
        if (!updated) {
            audio_clock_active_ = false;
            audio_preview_.stop();
        }
    }

    void apply_preview_update(const playback::PreviewUpdate& update)
    {
        snapshot_.preview_time_ns = update.playhead_time_ns;
        if (update.video_frame) {
            snapshot_.frame_index = update.video_frame->frame_index;
        }
    }

    void update_frame_for_time(core::TimeNs time)
    {
        if (!snapshot_.render_snapshot) {
            return;
        }
        const auto& recipe = snapshot_.render_snapshot->recipe();
        const auto relative = std::max<core::TimeNs>(0, time - recipe.time_range.start_ns);
        const auto numerator = static_cast<long double>(relative) * recipe.frame_rate.numerator;
        const auto denominator = static_cast<long double>(1'000'000'000) *
                                 recipe.frame_rate.denominator;
        snapshot_.frame_index = static_cast<rendering::FrameIndex>(numerator / denominator);
    }

    void stop_preview()
    {
        preview_timer_.stop();
        audio_preview_.stop();
        audio_clock_active_ = false;
        preview_clock_.reset();
        snapshot_.preview_state = PreviewState::stopped;
        snapshot_.preview_time_ns = snapshot_.timeline_range.start_ns;
        snapshot_.frame_index = 0;
        publish();
    }

    void start_export(const QString& request_id, const QString& path_value)
    {
        if (!media_ || path_value.isEmpty() || !snapshot_.render_snapshot ||
            !begin_job(request_id,
                       QStringLiteral("安全导出"),
                       QFileInfo(path_value).fileName(),
                       QStringLiteral("freeze_snapshot"))) {
            return;
        }
        last_failed_kind_ = UiCommandKind::export_project_to;
        last_failed_argument_ = path_value;
        playback::ExportFreezeRequest freeze;
        freeze.job_id = QStringLiteral("job-%1").arg(request_id).toStdString();
        freeze.current_timeline_revision = timeline_->snapshot()->timeline_revision;
        freeze.render_snapshot = snapshot_.render_snapshot;
        freeze.media.source_fingerprint_sha256 =
            media_->source->info().source_fingerprint_sha256;
        freeze.media.selection = media_->selection;
        freeze.audio_parameters.deterministic_seed = deterministic_seed;
        freeze.time_range = snapshot_.render_snapshot->recipe().time_range;
        freeze.frame_rate = snapshot_.render_snapshot->recipe().frame_rate;
        freeze.deterministic_seed = deterministic_seed;
        freeze.include_audio = !media_->selection.audio.selected.empty();
        auto frozen = playback::freeze_export(std::move(freeze));
        if (!frozen) {
            finish_job_failure(request_id, ui_error(frozen.error()));
            return;
        }
        rendering::OffscreenSessionConfig render_config;
        render_config.snapshot = snapshot_.render_snapshot;
        render_config.expected_snapshot_id = snapshot_.render_snapshot->id();
        render_config.expected_timeline_revision = snapshot_.timeline_revision;
        render_config.viewport_time_range = snapshot_.timeline_range;
        render_config.backend_preference = options_.headless
                                               ? rendering::BackendPreference::software_only
                                               : rendering::BackendPreference::
                                                     default_gpu_with_software_fallback;
        render_config.max_outstanding_frames = 2;
        render_config.max_outstanding_bytes =
            static_cast<std::uint64_t>(snapshot_.render_snapshot->recipe().output.width_px) *
            snapshot_.render_snapshot->recipe().output.height_px * 4U * 2U;
        auto renderer = rendering::OffscreenRenderSession::create(render_config);
        if (!renderer) {
            finish_job_failure(request_id, ui_error(renderer.error()));
            return;
        }
        auto session = playback::ExportSession::create(frozen.value(),
                                                       local_path(path_value),
                                                       playback::make_ffmpeg_test_encoder(),
                                                       &cancellation_);
        if (!session) {
            finish_job_failure(request_id, ui_error(session.error()));
            return;
        }
        rendering::FrameIndex frame_count = 0;
        while (true) {
            const auto time = rendering::frame_time_ns(
                snapshot_.render_snapshot->recipe(), frame_count);
            if (!time || time.value() >= snapshot_.timeline_range.end_ns) {
                break;
            }
            ++frame_count;
        }
        export_state_ = std::make_unique<ExportState>(ExportState{request_id,
                                                                  frozen.value(),
                                                                  std::move(renderer.value()),
                                                                  std::move(session.value()),
                                                                  0,
                                                                  frame_count});
        snapshot_.last_export_path = path_value;
        export_timer_.start();
    }

    void pump_export()
    {
        if (!export_state_) {
            export_timer_.stop();
            return;
        }
        auto& state = *export_state_;
        if (cancellation_.is_cancelled()) {
            state.renderer->cancel();
            state.session->cancel();
            const auto request_id = state.request_id;
            export_state_.reset();
            export_timer_.stop();
            finish_job_cancelled(request_id);
            return;
        }
        if (state.next_frame < state.frame_count) {
            const auto rendered = state.renderer->render_frame(state.next_frame, &cancellation_);
            if (!rendered) {
                export_failure(rendered.error());
                return;
            }
            const auto taken = state.renderer->try_take();
            if (taken.status != rendering::FrameTakeStatus::frame || !taken.frame) {
                export_failure(local_error(QStringLiteral("render.offscreen.take"),
                                           QStringLiteral("frame-unavailable"),
                                           QStringLiteral("ui.export.frame_unavailable"),
                                           true));
                return;
            }
            const auto written = state.session->write_video_frame(*taken.frame);
            if (!written) {
                export_failure(written.error());
                return;
            }
            ++state.next_frame;
            const auto progress = static_cast<core::NormPpm>(
                std::min<std::uint64_t>(900'000,
                    100'000 + (800'000 * state.next_frame) /
                                  std::max<rendering::FrameIndex>(1, state.frame_count)));
            job_progress(state.request_id, progress, QStringLiteral("render_frames"));
            return;
        }
        state.renderer->finish();
        if (state.frozen->includes_audio()) {
            const auto duration = state.frozen->time_range().end_ns -
                                  state.frozen->time_range().start_ns;
            const auto frame_count = static_cast<std::uint64_t>(std::ceil(
                static_cast<long double>(duration) * audio_render::render_sample_rate /
                1'000'000'000.0L));
            audio_render::RenderRequest request;
            request.start_time_ns = state.frozen->time_range().start_ns;
            request.frame_count = frame_count;
            request.parameters = state.frozen->audio_parameters();
            request.events = state.frozen->render_snapshot().timeline().events;
            audio_render::DeterministicMixer mixer;
            const auto prepared = mixer.prepare(std::move(request),
                                                std::span<const audio_render::Timbre>{});
            if (!prepared) {
                export_failure(prepared.error());
                return;
            }
            const auto pcm = mixer.render(prepared.value(), &cancellation_);
            if (!pcm) {
                export_failure(pcm.error());
                return;
            }
            const auto written = state.session->write_audio_pcm(pcm.value());
            if (!written) {
                export_failure(written.error());
                return;
            }
        }
        const auto finished = state.session->finish();
        if (!finished) {
            export_failure(finished.error());
            return;
        }
        const auto request_id = state.request_id;
        export_state_.reset();
        export_timer_.stop();
        finish_job_success(request_id);
    }

    void export_failure(const core::ErrorInfo& error)
    {
        if (!export_state_) {
            return;
        }
        const auto request_id = export_state_->request_id;
        export_state_->renderer->cancel();
        export_state_->session->cancel();
        export_state_.reset();
        export_timer_.stop();
        finish_job_failure(request_id, ui_error(error));
    }

    void export_failure(UiErrorDto error)
    {
        if (!export_state_) {
            return;
        }
        const auto request_id = export_state_->request_id;
        export_state_->renderer->cancel();
        export_state_->session->cancel();
        export_state_.reset();
        export_timer_.stop();
        finish_job_failure(request_id, std::move(error));
    }

    void request_cancel()
    {
        if (!active_job_) {
            return;
        }
        const auto cancelled = coordinator_.cancel(*active_job_);
        if (!cancelled) {
            fail(ui_error(cancelled.error()));
            return;
        }
        cancellation_.cancel();
        snapshot_.state = WorkspaceState::cancelling;
        publish();
        if (export_state_) {
            pump_export();
        }
    }

    void retry_failed()
    {
        snapshot_.error.reset();
        snapshot_.state = stable_state();
        const auto request_id = QStringLiteral("retry-%1")
                                    .arg(QDateTime::currentMSecsSinceEpoch());
        if (last_failed_kind_ == UiCommandKind::import_asset) {
            start_import(request_id, last_failed_argument_);
        } else if (last_failed_kind_ == UiCommandKind::start_analysis) {
            start_analysis(request_id);
        } else if (last_failed_kind_ == UiCommandKind::export_project_to) {
            start_export(request_id, last_failed_argument_);
        } else {
            publish();
        }
    }

    void fail(UiErrorDto error)
    {
        snapshot_.error = std::move(error);
        snapshot_.state = WorkspaceState::failed;
        publish();
    }

    WorkspaceState stable_state() const noexcept
    {
        return snapshot_.read_only ? WorkspaceState::read_only : WorkspaceState::idle;
    }

    bool timeline_edit_allowed() const noexcept
    {
        return snapshot_.route == UiRoute::workspace && !snapshot_.read_only &&
               snapshot_.state == WorkspaceState::idle;
    }

    IntegratedWorkspaceOptions options_;
    Observer observer_;
    WorkspaceSnapshotDto snapshot_;
    std::unique_ptr<core::Timeline> timeline_;
    std::optional<MediaState> media_;
    std::vector<analysis::AudioFeatureFrame> analysis_features_;
    analysis::AnalysisParameters analysis_parameters_;
    std::string analysis_revision_;
    std::optional<audio_render::RenderedPcm> preview_pcm_;
    system::ProjectStore project_store_;
    system::JobCoordinator coordinator_;
    std::map<std::string, JobMeta> jobs_;
    QVector<QString> job_order_;
    std::optional<std::string> active_job_;
    core::CancellationToken cancellation_;
    std::vector<std::jthread> workers_;
    QTimer preview_timer_;
    std::unique_ptr<playback::PreviewSynchronizer> preview_clock_;
    audio_render::QtAudioPreview audio_preview_;
    bool audio_clock_active_{false};
    QTimer export_timer_;
    std::unique_ptr<ExportState> export_state_;
    std::optional<DragState> drag_;
    std::vector<core::EventId> selected_events_;
    std::uint64_t transaction_sequence_{};
    UiCommandKind last_failed_kind_{UiCommandKind::return_to_start};
    QString last_failed_argument_;
};

} // namespace

std::unique_ptr<WorkspaceService> make_integrated_workspace_service(
    IntegratedWorkspaceOptions options)
{
    return std::make_unique<IntegratedWorkspaceService>(std::move(options));
}

} // namespace space_rhythm::ui
