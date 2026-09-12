#pragma once

#include <space_rhythm/audio/analysis.hpp>
#include <space_rhythm/media/media.hpp>
#include <space_rhythm/system/runtime.hpp>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace space_rhythm::worker {

inline constexpr std::uint32_t ui_worker_result_schema_version = 1;
inline constexpr std::string_view ui_worker_contract_version{"0.1.0"};

struct ImportResultDto {
    std::filesystem::path source_path;
    std::string source_fingerprint_sha256;
    std::uint64_t source_size_bytes{};
    media::MediaSelection selection;
    core::TimeNs duration_ns{};
    bool has_video{false};
    bool has_audio{false};
    std::string ffmpeg_version;
    std::uint32_t stream_count{};
};

struct FeatureResultDto {
    core::TimeNs anchor_time_ns{};
    core::NormPpm energy_ppm{};
    core::NormPpm spectral_change_ppm{};
};

struct CandidateResultDto {
    core::CandidateId id;
    core::AnalysisRevision analysis_revision;
    audio::CandidateKind kind{audio::CandidateKind::onset};
    core::TimeNs time_ns{};
    core::DurationNs duration_ns{};
    core::NormPpm strength_ppm{};
    core::NormPpm confidence_ppm{};
    audio::ProducerSource source;
};

struct AnalysisResultDto {
    audio::AnalysisStatus status{audio::AnalysisStatus::failed};
    std::string parameters_digest_sha256;
    std::vector<FeatureResultDto> feature_frames;
    std::vector<CandidateResultDto> candidates;
};

[[nodiscard]] system::Result<void> write_import_result(
    const std::filesystem::path& path, const ImportResultDto& result);
[[nodiscard]] core::Result<ImportResultDto> read_import_result(
    const std::filesystem::path& path);
[[nodiscard]] system::Result<void> write_analysis_result(
    const std::filesystem::path& path, const AnalysisResultDto& result);
[[nodiscard]] core::Result<AnalysisResultDto> read_analysis_result(
    const std::filesystem::path& path);
[[nodiscard]] system::Result<system::DataReference> file_reference(
    const std::filesystem::path& path);

int run_ui_worker_server(std::string_view server_name);

} // namespace space_rhythm::worker
