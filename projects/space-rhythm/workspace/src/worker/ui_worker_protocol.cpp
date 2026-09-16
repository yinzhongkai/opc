#include <space_rhythm/worker/ui_worker_protocol.hpp>

#include <QDataStream>
#include <QFile>
#include <QSaveFile>

#include <limits>

namespace space_rhythm::worker {
namespace {

constexpr quint32 import_magic = 0x53524931U; // SRI1
constexpr quint32 analysis_magic = 0x53524131U; // SRA1
constexpr quint32 maximum_records = 1'000'000U;

system::SystemError io_error(std::string stage)
{
    return {core::ErrorCategory::internal,
            system::SystemErrorCode::io_error,
            std::move(stage),
            "ui-worker-result:io",
            true,
            "ui.worker.result_io_error",
            {}};
}

core::ErrorInfo decode_error(std::string stage)
{
    return {core::schema_version,
            core::ErrorCategory::validation,
            core::ErrorCode::invalid_dto,
            std::move(stage),
            "ui-worker-result:invalid",
            false,
            "ui.worker.invalid_result",
            {},
            {}};
}

void configure(QDataStream& stream)
{
    stream.setVersion(QDataStream::Qt_6_0);
    stream.setByteOrder(QDataStream::LittleEndian);
}

void write_string(QDataStream& stream, const std::string& value)
{
    stream << QString::fromStdString(value);
}

std::string read_string(QDataStream& stream)
{
    QString value;
    stream >> value;
    return value.toStdString();
}

void write_key(QDataStream& stream, const media::StreamKey& key)
{
    write_string(stream, key.source_fingerprint_sha256);
    stream << static_cast<qint32>(key.stream_index);
}

media::StreamKey read_key(QDataStream& stream)
{
    media::StreamKey key;
    key.source_fingerprint_sha256 = read_string(stream);
    qint32 index{};
    stream >> index;
    key.stream_index = index;
    return key;
}

void write_selection_result(QDataStream& stream,
                            const media::StreamSelectionResult& selection)
{
    stream << static_cast<quint32>(selection.mode)
           << static_cast<quint32>(selection.selected.size());
    for (const auto& key : selection.selected) {
        write_key(stream, key);
    }
    stream << static_cast<quint32>(selection.candidates.size());
    for (const auto& key : selection.candidates) {
        write_key(stream, key);
    }
    write_string(stream, selection.reason);
}

bool read_count(QDataStream& stream, quint32& count)
{
    stream >> count;
    return stream.status() == QDataStream::Ok && count <= maximum_records;
}

bool read_selection_result(QDataStream& stream,
                           media::StreamSelectionResult& selection)
{
    quint32 mode{};
    quint32 count{};
    stream >> mode;
    if (mode > static_cast<quint32>(media::SelectionMode::all) ||
        !read_count(stream, count)) {
        return false;
    }
    selection.mode = static_cast<media::SelectionMode>(mode);
    selection.selected.reserve(count);
    for (quint32 index = 0; index < count; ++index) {
        selection.selected.push_back(read_key(stream));
    }
    if (!read_count(stream, count)) {
        return false;
    }
    selection.candidates.reserve(count);
    for (quint32 index = 0; index < count; ++index) {
        selection.candidates.push_back(read_key(stream));
    }
    selection.reason = read_string(stream);
    return stream.status() == QDataStream::Ok;
}

template <typename Writer>
system::Result<void> write_file(const std::filesystem::path& path, Writer writer)
{
    QSaveFile file(QString::fromStdWString(path.wstring()));
    if (!file.open(QIODevice::WriteOnly)) {
        return system::Result<void>::failure(io_error("ui.worker.result.open"));
    }
    QDataStream stream(&file);
    configure(stream);
    writer(stream);
    if (stream.status() != QDataStream::Ok || !file.commit()) {
        return system::Result<void>::failure(io_error("ui.worker.result.commit"));
    }
    return system::Result<void>::success();
}

} // namespace

system::Result<void> write_import_result(const std::filesystem::path& path,
                                         const ImportResultDto& result)
{
    return write_file(path, [&result](QDataStream& stream) {
        stream << import_magic << ui_worker_result_schema_version
               << QString::fromStdWString(result.source_path.wstring());
        write_string(stream, result.source_fingerprint_sha256);
        stream << static_cast<quint64>(result.source_size_bytes)
               << static_cast<qint64>(result.duration_ns) << result.has_video
               << result.has_audio;
        write_string(stream, result.ffmpeg_version);
        stream << static_cast<quint32>(result.stream_count);
        write_selection_result(stream, result.selection.video);
        write_selection_result(stream, result.selection.audio);
        const auto& origin = result.selection.presentation_origin;
        stream << static_cast<qint64>(origin.timestamp.ticks)
               << static_cast<qint64>(origin.timestamp.time_base.seconds_numerator)
               << static_cast<qint64>(origin.timestamp.time_base.seconds_denominator)
               << static_cast<quint32>(origin.timestamp.origin)
               << origin.provisional;
    });
}

core::Result<ImportResultDto> read_import_result(const std::filesystem::path& path)
{
    QFile file(QString::fromStdWString(path.wstring()));
    if (!file.open(QIODevice::ReadOnly)) {
        return core::Result<ImportResultDto>::failure(
            decode_error("ui.worker.import_result.open"));
    }
    QDataStream stream(&file);
    configure(stream);
    quint32 magic{};
    quint32 schema{};
    QString source_path;
    ImportResultDto result;
    qint64 duration{};
    quint64 source_size{};
    quint32 stream_count{};
    stream >> magic >> schema >> source_path;
    result.source_fingerprint_sha256 = read_string(stream);
    stream >> source_size >> duration >> result.has_video >> result.has_audio;
    result.ffmpeg_version = read_string(stream);
    stream >> stream_count;
    if (magic != import_magic || schema != ui_worker_result_schema_version ||
        !read_selection_result(stream, result.selection.video) ||
        !read_selection_result(stream, result.selection.audio)) {
        return core::Result<ImportResultDto>::failure(
            decode_error("ui.worker.import_result.header"));
    }
    qint64 ticks{};
    qint64 numerator{};
    qint64 denominator{};
    quint32 origin{};
    stream >> ticks >> numerator >> denominator >> origin
           >> result.selection.presentation_origin.provisional;
    if (stream.status() != QDataStream::Ok || denominator == 0 ||
        origin > static_cast<quint32>(media::TimestampOrigin::synthesized_duration)) {
        return core::Result<ImportResultDto>::failure(
            decode_error("ui.worker.import_result.decode"));
    }
    result.source_path = std::filesystem::path{source_path.toStdWString()};
    result.duration_ns = duration;
    result.source_size_bytes = source_size;
    result.stream_count = stream_count;
    auto& timestamp = result.selection.presentation_origin.timestamp;
    timestamp.ticks = ticks;
    timestamp.time_base = {numerator, denominator};
    timestamp.origin = static_cast<media::TimestampOrigin>(origin);
    return core::Result<ImportResultDto>::success(std::move(result));
}

system::Result<void> write_analysis_result(const std::filesystem::path& path,
                                           const AnalysisResultDto& result)
{
    return write_file(path, [&result](QDataStream& stream) {
        stream << analysis_magic << ui_worker_result_schema_version
               << static_cast<quint32>(result.status);
        write_string(stream, result.parameters_digest_sha256);
        stream << static_cast<quint32>(result.feature_frames.size());
        for (const auto& feature : result.feature_frames) {
            stream << static_cast<qint64>(feature.anchor_time_ns)
                   << static_cast<quint32>(feature.energy_ppm)
                   << static_cast<quint32>(feature.spectral_change_ppm);
        }
        stream << static_cast<quint32>(result.candidates.size());
        for (const auto& candidate : result.candidates) {
            write_string(stream, candidate.id.value);
            write_string(stream, candidate.analysis_revision.value);
            stream << static_cast<quint32>(candidate.kind)
                   << static_cast<qint64>(candidate.time_ns)
                   << static_cast<qint64>(candidate.duration_ns)
                   << static_cast<quint32>(candidate.strength_ppm)
                   << static_cast<quint32>(candidate.confidence_ppm);
            write_string(stream, candidate.source.producer_id);
            write_string(stream, candidate.source.producer_version);
            write_string(stream, candidate.source.input_fingerprint_sha256);
            write_string(stream, candidate.source.parameters_digest_sha256);
        }
    });
}

core::Result<AnalysisResultDto> read_analysis_result(const std::filesystem::path& path)
{
    QFile file(QString::fromStdWString(path.wstring()));
    if (!file.open(QIODevice::ReadOnly)) {
        return core::Result<AnalysisResultDto>::failure(
            decode_error("ui.worker.analysis_result.open"));
    }
    QDataStream stream(&file);
    configure(stream);
    quint32 magic{};
    quint32 schema{};
    quint32 status{};
    quint32 count{};
    AnalysisResultDto result;
    stream >> magic >> schema >> status;
    result.parameters_digest_sha256 = read_string(stream);
    if (magic != analysis_magic || schema != ui_worker_result_schema_version ||
        status > static_cast<quint32>(audio::AnalysisStatus::cancelled) ||
        !read_count(stream, count)) {
        return core::Result<AnalysisResultDto>::failure(
            decode_error("ui.worker.analysis_result.header"));
    }
    result.status = static_cast<audio::AnalysisStatus>(status);
    result.feature_frames.reserve(count);
    for (quint32 index = 0; index < count; ++index) {
        qint64 time{};
        quint32 energy{};
        quint32 spectral{};
        stream >> time >> energy >> spectral;
        if (energy > core::norm_ppm_max || spectral > core::norm_ppm_max) {
            return core::Result<AnalysisResultDto>::failure(
                decode_error("ui.worker.analysis_result.feature"));
        }
        result.feature_frames.push_back({time, energy, spectral});
    }
    if (!read_count(stream, count)) {
        return core::Result<AnalysisResultDto>::failure(
            decode_error("ui.worker.analysis_result.candidates"));
    }
    result.candidates.reserve(count);
    for (quint32 index = 0; index < count; ++index) {
        CandidateResultDto candidate;
        candidate.id.value = read_string(stream);
        candidate.analysis_revision.value = read_string(stream);
        quint32 kind{};
        qint64 time{};
        qint64 duration{};
        quint32 strength{};
        quint32 confidence{};
        stream >> kind >> time >> duration >> strength >> confidence;
        if (kind > static_cast<quint32>(audio::CandidateKind::beat) ||
            strength > core::norm_ppm_max || confidence > core::norm_ppm_max) {
            return core::Result<AnalysisResultDto>::failure(
                decode_error("ui.worker.analysis_result.candidate"));
        }
        candidate.kind = static_cast<audio::CandidateKind>(kind);
        candidate.time_ns = time;
        candidate.duration_ns = duration;
        candidate.strength_ppm = strength;
        candidate.confidence_ppm = confidence;
        candidate.source.producer_id = read_string(stream);
        candidate.source.producer_version = read_string(stream);
        candidate.source.input_fingerprint_sha256 = read_string(stream);
        candidate.source.parameters_digest_sha256 = read_string(stream);
        result.candidates.push_back(std::move(candidate));
    }
    if (stream.status() != QDataStream::Ok) {
        return core::Result<AnalysisResultDto>::failure(
            decode_error("ui.worker.analysis_result.decode"));
    }
    return core::Result<AnalysisResultDto>::success(std::move(result));
}

system::Result<system::DataReference> file_reference(
    const std::filesystem::path& path)
{
    const auto digest = system::sha256_file(path);
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (!digest || error) {
        return system::Result<system::DataReference>::failure(
            digest ? io_error("ui.worker.result.stat") : digest.error());
    }
    return system::Result<system::DataReference>::success(
        {system::DataReferenceKind::file,
         path.generic_string(),
         size,
         digest.value()});
}

} // namespace space_rhythm::worker
