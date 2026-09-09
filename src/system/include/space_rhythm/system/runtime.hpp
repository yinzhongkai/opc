#pragma once

#include <space_rhythm/core/timeline.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace space_rhythm::system {

inline constexpr std::uint32_t ipc_schema_version = 1;
inline constexpr std::uint32_t ipc_protocol_version = 1;
inline constexpr std::uint32_t project_schema_version = 2;
inline constexpr std::size_t max_ipc_frame_bytes = 64U * 1024U;

enum class SystemErrorCode {
    invalid_request,
    invalid_transition,
    request_conflict,
    message_conflict,
    out_of_order_message,
    protocol_version_mismatch,
    message_too_large,
    forbidden_inline_data,
    request_timeout,
    worker_crashed,
    stale_revision,
    cancelled,
    unsupported_project_schema,
    invalid_project,
    corrupt_project,
    disk_full,
    partial_write,
    io_error,
    asset_not_found,
    fingerprint_mismatch,
    cache_corrupt,
    cache_miss,
};

std::string_view to_string(SystemErrorCode code) noexcept;

struct SystemError {
    core::ErrorCategory category{core::ErrorCategory::internal};
    SystemErrorCode code{SystemErrorCode::io_error};
    std::string stage;
    std::string diagnostic_id;
    bool retryable{false};
    std::string message_key;
    std::map<std::string, std::string> context;

    bool operator==(const SystemError&) const = default;
};

template <typename T>
class Result final {
public:
    static Result success(T value)
    {
        return Result{std::move(value), std::nullopt};
    }

    static Result failure(SystemError error)
    {
        return Result{std::nullopt, std::move(error)};
    }

    [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] const T& value() const { return value_.value(); }
    [[nodiscard]] T& value() { return value_.value(); }
    [[nodiscard]] const SystemError& error() const { return error_.value(); }

private:
    Result(std::optional<T> value, std::optional<SystemError> error)
        : value_(std::move(value)), error_(std::move(error))
    {
    }

    std::optional<T> value_;
    std::optional<SystemError> error_;
};

template <>
class Result<void> final {
public:
    static Result success() { return Result{std::nullopt}; }
    static Result failure(SystemError error) { return Result{std::move(error)}; }

    [[nodiscard]] bool has_value() const noexcept { return !error_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] const SystemError& error() const { return error_.value(); }

private:
    explicit Result(std::optional<SystemError> error)
        : error_(std::move(error))
    {
    }

    std::optional<SystemError> error_;
};

enum class JobStatus {
    queued,
    running,
    cancelling,
    succeeded,
    failed,
    cancelled,
};

enum class JobUpdateType {
    accepted,
    progress,
    cancellation_acknowledged,
    succeeded,
    failed,
    cancelled,
};

enum class DataReferenceKind {
    file,
    cache,
};

struct DataReference {
    DataReferenceKind kind{DataReferenceKind::file};
    std::string locator;
    std::uint64_t byte_length{};
    std::string sha256;

    bool operator==(const DataReference&) const = default;
};

struct JobRequest {
    std::string request_id;
    std::string job_id;
    std::string operation;
    core::TimelineRevision base_timeline_revision{};
    std::string input_fingerprint;
    std::string parameters_digest;
    std::uint64_t timeout_ms{30'000};
    std::map<std::string, std::string> options;
    std::vector<DataReference> inputs;

    bool operator==(const JobRequest&) const = default;
};

struct JobUpdate {
    std::string request_id;
    std::string job_id;
    std::uint64_t sequence{};
    JobUpdateType type{JobUpdateType::accepted};
    core::NormPpm progress_ppm{};
    std::optional<core::TimelineRevision> result_base_revision;
    std::optional<DataReference> result;
    std::optional<SystemError> error;

    bool operator==(const JobUpdate&) const = default;
};

struct JobSnapshot {
    JobRequest request;
    JobStatus status{JobStatus::queued};
    core::NormPpm progress_ppm{};
    std::uint64_t last_sequence{};
    std::uint64_t deadline_ms{};
    std::optional<DataReference> result;
    std::optional<SystemError> error;

    bool operator==(const JobSnapshot&) const = default;
};

struct JobAction {
    JobSnapshot job;
    bool replayed{false};

    bool operator==(const JobAction&) const = default;
};

class JobCoordinator final {
public:
    JobCoordinator();
    ~JobCoordinator();

    JobCoordinator(const JobCoordinator&) = delete;
    JobCoordinator& operator=(const JobCoordinator&) = delete;

    Result<JobAction> submit(const JobRequest& request, std::uint64_t now_ms);
    Result<JobAction> apply_update(const JobUpdate& update,
                                   core::TimelineRevision current_timeline_revision);
    Result<JobAction> cancel(std::string_view request_id);
    std::vector<JobSnapshot> expire(std::uint64_t now_ms);
    std::vector<JobSnapshot> worker_disconnected();
    Result<JobSnapshot> snapshot(std::string_view request_id) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

enum class MessageType {
    handshake,
    handshake_acknowledged,
    submit_job,
    cancel_job,
    job_update,
    protocol_error,
};

struct ProtocolEnvelope {
    std::uint32_t schema_version{ipc_schema_version};
    std::uint32_t protocol_version{ipc_protocol_version};
    std::string message_id;
    std::string request_id;
    std::optional<std::string> job_id;
    std::uint64_t sequence{};
    MessageType type{MessageType::handshake};
    // Small command/progress metadata only. Frames, PCM, feature blocks and other
    // bulk data cross this boundary exclusively as verified DataReference values.
    std::map<std::string, std::string> payload;
    std::vector<DataReference> references;
    std::optional<SystemError> error;

    bool operator==(const ProtocolEnvelope&) const = default;
};

Result<ProtocolEnvelope> validate_protocol_envelope(ProtocolEnvelope envelope);
Result<std::vector<std::uint8_t>> encode_protocol_frame(const ProtocolEnvelope& envelope);
Result<ProtocolEnvelope> decode_protocol_frame(const std::vector<std::uint8_t>& frame);

class MessageTransport {
public:
    // Domain/job code depends on this transport-neutral interface; QLocalSocket is
    // confined to LocalIpcClient's private implementation.
    virtual ~MessageTransport() = default;
    virtual Result<void> send(const ProtocolEnvelope& envelope) = 0;
    virtual Result<ProtocolEnvelope> receive(std::uint32_t timeout_ms) = 0;
};

class LocalIpcClient final : public MessageTransport {
public:
    LocalIpcClient();
    ~LocalIpcClient() override;

    LocalIpcClient(const LocalIpcClient&) = delete;
    LocalIpcClient& operator=(const LocalIpcClient&) = delete;

    Result<void> connect_to(std::string_view server_name, std::uint32_t timeout_ms);
    Result<void> send(const ProtocolEnvelope& envelope) override;
    Result<ProtocolEnvelope> receive(std::uint32_t timeout_ms) override;
    void close();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

int run_mock_worker(std::string_view server_name);

struct AssetReference {
    std::string asset_id;
    std::filesystem::path stored_path;
    std::uint64_t size_bytes{};
    std::string fingerprint_sha256;

    bool operator==(const AssetReference&) const = default;
};

struct ProjectDocument {
    std::uint32_t schema_version{project_schema_version};
    std::string app_version;
    core::TimelineSnapshot timeline;
    std::vector<AssetReference> assets;
    std::map<std::string, std::string> settings;
    std::map<std::string, std::string> extensions;

    bool operator==(const ProjectDocument&) const = default;
};

struct LoadedProject {
    ProjectDocument document;
    std::optional<std::uint32_t> migrated_from_schema;

    bool operator==(const LoadedProject&) const = default;
};

enum class SaveFault {
    none,
    disk_full,
    partial_write,
    before_commit,
};

enum class RecoverySource {
    primary,
    autosave,
};

struct RecoveryResult {
    LoadedProject project;
    RecoverySource source{RecoverySource::primary};
    std::uint64_t source_modified_unix_ms{};
    std::optional<SystemError> ignored_recovery_error;
};

class ProjectStore final {
public:
    Result<std::vector<std::uint8_t>> serialize(const ProjectDocument& document) const;
    Result<LoadedProject> deserialize(const std::vector<std::uint8_t>& bytes) const;
    Result<void> save(const std::filesystem::path& path,
                      const ProjectDocument& document,
                      SaveFault fault = SaveFault::none) const;
    Result<LoadedProject> load(const std::filesystem::path& path) const;
    Result<void> save_autosave(const std::filesystem::path& primary_path,
                               const ProjectDocument& document,
                               SaveFault fault = SaveFault::none) const;
    Result<void> mark_session_clean(const std::filesystem::path& primary_path,
                                    bool clean) const;
    Result<RecoveryResult> recover(const std::filesystem::path& primary_path) const;

    static std::filesystem::path autosave_path(const std::filesystem::path& primary_path);
    static std::filesystem::path session_marker_path(
        const std::filesystem::path& primary_path);
};

Result<std::string> sha256_file(const std::filesystem::path& path);

struct RelocationResult {
    std::filesystem::path resolved_path;
    std::size_t candidates_checked{};
};

Result<RelocationResult> relocate_asset(
    const AssetReference& asset,
    const std::vector<std::filesystem::path>& search_roots,
    std::size_t maximum_candidates = 4'096);

struct CacheKeyInput {
    std::string asset_fingerprint;
    std::string algorithm_version;
    std::string parameters_digest;
    std::string tool_version;
};

struct CacheLookup {
    bool hit{false};
    bool rebuild_required{false};
    std::vector<std::uint8_t> payload;
    std::optional<SystemError> diagnostic;
};

class RebuildableCache final {
public:
    explicit RebuildableCache(std::filesystem::path root);

    static std::string make_key(const CacheKeyInput& input);
    Result<void> put(std::string_view key,
                     const std::vector<std::uint8_t>& payload,
                     SaveFault fault = SaveFault::none) const;
    CacheLookup get(std::string_view key) const;
    Result<std::uint64_t> prune(std::uint64_t maximum_bytes) const;

private:
    std::filesystem::path root_;
};

} // namespace space_rhythm::system
