#include <space_rhythm/worker/ui_worker_protocol.hpp>

#include <QCoreApplication>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>

#include <algorithm>
#include <cstdio>
#include <map>
#include <optional>

namespace space_rhythm::worker {
namespace {

system::SystemError worker_error(const core::ErrorInfo& error)
{
    return {error.category,
            error.category == core::ErrorCategory::cancelled
                ? system::SystemErrorCode::cancelled
                : system::SystemErrorCode::io_error,
            error.stage,
            error.diagnostic_id,
            error.retryable,
            error.message_key,
            error.context};
}

system::SystemError local_error(std::string stage, std::string message)
{
    return {core::ErrorCategory::internal,
            system::SystemErrorCode::io_error,
            std::move(stage),
            "ui-worker:failure",
            true,
            std::move(message),
            {}};
}

quint32 read_be32(const char* bytes)
{
    const auto* value = reinterpret_cast<const unsigned char*>(bytes);
    return (static_cast<quint32>(value[0]) << 24U) |
           (static_cast<quint32>(value[1]) << 16U) |
           (static_cast<quint32>(value[2]) << 8U) |
           static_cast<quint32>(value[3]);
}

void send_envelope(QLocalSocket* socket, const system::ProtocolEnvelope& envelope)
{
    const auto encoded = system::encode_protocol_frame(envelope);
    if (!encoded) {
        socket->abort();
        return;
    }
    socket->write(reinterpret_cast<const char*>(encoded.value().data()),
                  static_cast<qint64>(encoded.value().size()));
    socket->flush();
}

system::ProtocolEnvelope response(const system::ProtocolEnvelope& request,
                                  std::uint64_t sequence,
                                  std::string state)
{
    return {system::ipc_schema_version,
            system::ipc_protocol_version,
            request.message_id + ":" + std::to_string(sequence),
            request.request_id,
            request.job_id,
            sequence,
            system::MessageType::job_update,
            {{"state", std::move(state)}},
            {},
            std::nullopt};
}

core::TimeNs duration_ns(const media::MediaInfo& info)
{
    if (!info.duration) {
        return 10'000'000'000;
    }
    const auto mapped = core::scale_ticks(info.duration->ticks,
                                          info.duration->time_base,
                                          core::RoundingMode::nearest_ties_to_even);
    return mapped && mapped.value() > 0 ? mapped.value() : 10'000'000'000;
}

core::Result<ImportResultDto> import_media(const std::filesystem::path& path)
{
    auto source = media::MediaSource::open(path);
    if (!source) {
        return core::Result<ImportResultDto>::failure(source.error());
    }
    media::StreamSelectionRequest video;
    video.mode = media::SelectionMode::optional_default_then_lowest_index;
    media::StreamSelectionRequest audio;
    audio.mode = media::SelectionMode::optional_default_then_lowest_index;
    auto selection = source.value()->select(video, audio);
    if (!selection) {
        return core::Result<ImportResultDto>::failure(selection.error());
    }
    const auto& info = source.value()->info();
    ImportResultDto result;
    result.source_path = path;
    result.source_fingerprint_sha256 = info.source_fingerprint_sha256;
    std::error_code size_error;
    result.source_size_bytes = std::filesystem::file_size(path, size_error);
    if (size_error) {
        result.source_size_bytes = 0;
    }
    result.selection = std::move(selection.value());
    result.duration_ns = duration_ns(info);
    result.has_video = std::ranges::any_of(info.streams, [](const auto& stream) {
        return stream.kind == media::StreamKind::video;
    });
    result.has_audio = std::ranges::any_of(info.streams, [](const auto& stream) {
        return stream.kind == media::StreamKind::audio;
    });
    result.ffmpeg_version = info.probe_implementation.ffmpeg_version;
    result.stream_count = static_cast<std::uint32_t>(info.streams.size());
    return core::Result<ImportResultDto>::success(std::move(result));
}

core::Result<AnalysisResultDto> analyze_media(const std::filesystem::path& path)
{
    auto source = media::MediaSource::open(path);
    if (!source) {
        return core::Result<AnalysisResultDto>::failure(source.error());
    }
    media::StreamSelectionRequest video;
    video.mode = media::SelectionMode::optional_default_then_lowest_index;
    media::StreamSelectionRequest audio;
    audio.mode = media::SelectionMode::required_default_then_lowest_index;
    auto selection = source.value()->select(video, audio);
    if (!selection) {
        return core::Result<AnalysisResultDto>::failure(selection.error());
    }
    const auto stream = selection.value().audio.selected.front();
    std::vector<audio::DspPcmBuffer> buffers;
    audio::PcmNarrowAdapter adapter;
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
        buffers.push_back(std::move(adapted.value()));
        return media::PublishResult::accepted;
    };
    media::AudioOutputSpec output;
    output.sample_rate = 48'000;
    output.channels = 1;
    const auto decoded = source.value()->decode_audio(selection.value(),
                                                      stream,
                                                      output,
                                                      {},
                                                      callbacks);
    if (!decoded) {
        return core::Result<AnalysisResultDto>::failure(decoded.error());
    }
    if (adapter_error) {
        return core::Result<AnalysisResultDto>::failure(*adapter_error);
    }
    const auto parameters = audio::production_parameters(48'000);
    audio::Analyzer analyzer;
    const auto analyzed = analyzer.analyze(buffers, parameters);
    if (analyzed.status == audio::AnalysisStatus::failed) {
        return core::Result<AnalysisResultDto>::failure(
            analyzed.error.value_or(core::ErrorInfo{core::schema_version,
                                                     core::ErrorCategory::internal,
                                                     core::ErrorCode::internal_error,
                                                     "ui.worker.analysis",
                                                     "ui-worker:analysis",
                                                     true,
                                                     "ui.worker.analysis_failed",
                                                     {},
                                                     {}}));
    }
    AnalysisResultDto result;
    result.status = analyzed.status;
    result.parameters_digest_sha256 = parameters.parameters_digest_sha256;
    result.feature_frames.reserve(analyzed.feature_frames.size());
    for (const auto& feature : analyzed.feature_frames) {
        result.feature_frames.push_back(
            {feature.anchor_time_ns,
             feature.short_time_energy.normalized_ppm.value_or(0),
             feature.spectral_change.normalized_ppm.value_or(0)});
    }
    result.candidates.reserve(analyzed.candidates.size());
    for (const auto& candidate : analyzed.candidates) {
        result.candidates.push_back({candidate.id,
                                     candidate.analysis_revision,
                                     candidate.kind,
                                     candidate.time_ns,
                                     candidate.duration_ns,
                                     candidate.strength_ppm,
                                     candidate.confidence_ppm,
                                     candidate.source});
    }
    return core::Result<AnalysisResultDto>::success(std::move(result));
}

void handle_submit(QLocalSocket* socket, const system::ProtocolEnvelope& request)
{
    send_envelope(socket, response(request, 1, "accepted"));
    auto progress = response(request, 2, "progress");
    progress.payload.emplace("progressPpm", "150000");
    send_envelope(socket, progress);

    const auto delay = request.payload.find("delayMs");
    if (delay != request.payload.end()) {
        bool ok = false;
        const auto milliseconds = QString::fromStdString(delay->second).toUInt(&ok);
        if (ok && milliseconds > 0) {
            QThread::msleep(std::min(milliseconds, 30'000U));
        }
    }

    const auto operation = request.payload.find("operation");
    const auto output = request.payload.find("resultPath");
    if (operation == request.payload.end() || output == request.payload.end() ||
        request.references.empty()) {
        auto failed = response(request, 3, "failed");
        failed.error = local_error("ui.worker.submit", "ui.worker.invalid_request");
        send_envelope(socket, failed);
        return;
    }
    const auto source_path = std::filesystem::path{request.references.front().locator};
    const auto output_path = std::filesystem::path{output->second};
    std::optional<system::SystemError> error;
    if (operation->second == "ui.import") {
        const auto result = import_media(source_path);
        if (!result) {
            error = worker_error(result.error());
        } else if (const auto written = write_import_result(output_path, result.value());
                   !written) {
            error = written.error();
        }
    } else if (operation->second == "ui.analysis") {
        const auto result = analyze_media(source_path);
        if (!result) {
            error = worker_error(result.error());
        } else if (const auto written = write_analysis_result(output_path, result.value());
                   !written) {
            error = written.error();
        }
    } else {
        error = local_error("ui.worker.operation", "ui.worker.unsupported_operation");
    }
    if (error) {
        auto failed = response(request, 3, "failed");
        failed.error = std::move(error);
        send_envelope(socket, failed);
        return;
    }
    const auto reference = file_reference(output_path);
    if (!reference) {
        auto failed = response(request, 3, "failed");
        failed.error = reference.error();
        send_envelope(socket, failed);
        return;
    }
    auto succeeded = response(request, 3, "succeeded");
    succeeded.references.push_back(reference.value());
    send_envelope(socket, succeeded);
}

} // namespace

int run_ui_worker_server(const std::string_view server_name)
{
    auto* application = QCoreApplication::instance();
    if (!application || server_name.empty()) {
        return 2;
    }
    auto* server = new QLocalServer(application);
    server->setSocketOptions(QLocalServer::UserAccessOption);
    const auto name = QString::fromUtf8(server_name.data(),
                                        static_cast<qsizetype>(server_name.size()));
    QLocalServer::removeServer(name);
    if (!server->listen(name)) {
        return 3;
    }
    std::fprintf(stdout, "SPACE_RHYTHM_UI_WORKER_READY %s\n", name.toUtf8().constData());
    std::fflush(stdout);
    QObject::connect(server, &QLocalServer::newConnection, application, [server] {
        while (auto* socket = server->nextPendingConnection()) {
            auto* buffer = new QByteArray;
            auto* negotiated = new bool{false};
            QObject::connect(socket, &QLocalSocket::disconnected, socket,
                             [socket, buffer, negotiated] {
                                 delete buffer;
                                 delete negotiated;
                                 socket->deleteLater();
                             });
            QObject::connect(socket, &QLocalSocket::readyRead, socket,
                             [socket, buffer, negotiated] {
                buffer->append(socket->readAll());
                while (buffer->size() >= 4) {
                    const auto body_size = read_be32(buffer->constData());
                    if (body_size == 0 || body_size > system::max_ipc_frame_bytes) {
                        socket->abort();
                        return;
                    }
                    const auto frame_size = static_cast<qsizetype>(body_size) + 4;
                    if (buffer->size() < frame_size) {
                        return;
                    }
                    const auto raw = buffer->first(frame_size);
                    buffer->remove(0, frame_size);
                    const auto decoded = system::decode_protocol_frame(
                        std::vector<std::uint8_t>(
                            reinterpret_cast<const std::uint8_t*>(raw.constData()),
                            reinterpret_cast<const std::uint8_t*>(raw.constData()) + raw.size()));
                    if (!decoded) {
                        socket->abort();
                        return;
                    }
                    const auto request = decoded.value();
                    if (request.type == system::MessageType::handshake) {
                        *negotiated = true;
                        send_envelope(socket,
                                      {system::ipc_schema_version,
                                       system::ipc_protocol_version,
                                       request.message_id + ":ack",
                                       request.request_id,
                                       std::nullopt,
                                       0,
                                       system::MessageType::handshake_acknowledged,
                                       {{"selectedVersion", "1"},
                                        {"worker", "space-rhythm-ui-worker"}},
                                       {},
                                       std::nullopt});
                    } else if (*negotiated &&
                               request.type == system::MessageType::submit_job) {
                        handle_submit(socket, request);
                    } else {
                        socket->abort();
                        return;
                    }
                }
            });
        }
    });
    return application->exec();
}

} // namespace space_rhythm::worker
