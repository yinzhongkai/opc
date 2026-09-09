#pragma once

#include <atomic>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <utility>

namespace space_rhythm::core {

inline constexpr std::uint32_t schema_version = 1;
inline constexpr std::string_view contract_version{"0.1.0"};
inline constexpr std::uint32_t norm_ppm_max = 1'000'000;

using TimeNs = std::int64_t;
using DurationNs = std::int64_t;
using TimelineRevision = std::uint64_t;
using NormPpm = std::uint32_t;

enum class ErrorCategory {
    validation,
    conflict,
    cancelled,
    compatibility,
    resource_limit,
    internal,
};

enum class ErrorCode {
    invalid_identifier,
    invalid_time_base,
    time_overflow,
    invalid_dto,
    invalid_track,
    invalid_event,
    duplicate_track_id,
    duplicate_event_id,
    track_not_found,
    event_not_found,
    track_not_empty,
    invalid_transaction,
    stale_revision,
    locked_event,
    history_conflict,
    idempotency_conflict,
    analysis_revision_conflict,
    revision_exhausted,
    transaction_cancelled,
    unsupported_schema,
    unsupported_feature,
    unknown_enum,
    resource_limit,
    invariant_violation,
    internal_error,
};

struct ErrorInfo {
    std::uint32_t schema_version{core::schema_version};
    ErrorCategory category{ErrorCategory::internal};
    ErrorCode code{ErrorCode::internal_error};
    std::string stage;
    std::string diagnostic_id;
    bool retryable{false};
    std::string message_key;
    std::map<std::string, std::string> context;
    std::shared_ptr<const ErrorInfo> cause;

    bool operator==(const ErrorInfo&) const = default;
};

std::string_view to_string(ErrorCategory category) noexcept;
std::string_view to_string(ErrorCode code) noexcept;

template <typename T>
class Result final {
public:
    static Result success(T value)
    {
        return Result{std::move(value), std::nullopt};
    }

    static Result failure(ErrorInfo error)
    {
        return Result{std::nullopt, std::move(error)};
    }

    [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] const T& value() const { return value_.value(); }
    [[nodiscard]] T& value() { return value_.value(); }
    [[nodiscard]] const ErrorInfo& error() const { return error_.value(); }

private:
    Result(std::optional<T> value, std::optional<ErrorInfo> error)
        : value_(std::move(value)), error_(std::move(error))
    {
    }

    std::optional<T> value_;
    std::optional<ErrorInfo> error_;
};

template <typename Tag>
struct OpaqueId {
    std::string value;

    auto operator<=>(const OpaqueId&) const = default;
};

struct ProjectIdTag;
struct TrackIdTag;
struct EventIdTag;
struct CandidateIdTag;
struct AnalysisRevisionTag;
struct TransactionIdTag;

using ProjectId = OpaqueId<ProjectIdTag>;
using TrackId = OpaqueId<TrackIdTag>;
using EventId = OpaqueId<EventIdTag>;
using CandidateId = OpaqueId<CandidateIdTag>;
using AnalysisRevision = OpaqueId<AnalysisRevisionTag>;
using TransactionId = OpaqueId<TransactionIdTag>;

Result<std::string> validate_identifier(std::string_view value);

enum class RoundingMode {
    floor,
    ceil,
    toward_zero,
    nearest_ties_to_even,
};

struct TimeBase {
    std::int64_t seconds_numerator{};
    std::int64_t seconds_denominator{};

    bool operator==(const TimeBase&) const = default;
};

struct TimeRange {
    TimeNs start_ns{};
    TimeNs end_ns{};

    [[nodiscard]] bool contains(TimeNs value) const noexcept
    {
        return value >= start_ns && value < end_ns;
    }

    [[nodiscard]] bool overlaps(const TimeRange& other) const noexcept
    {
        return start_ns < other.end_ns && other.start_ns < end_ns;
    }

    bool operator==(const TimeRange&) const = default;
};

Result<TimeNs> scale_ticks(std::int64_t ticks, TimeBase base, RoundingMode mode);
Result<TimeNs> checked_add(TimeNs lhs, TimeNs rhs);
Result<TimeNs> checked_subtract(TimeNs lhs, TimeNs rhs);
Result<TimeRange> make_range(TimeNs start, DurationNs duration);

struct VersionedOpaqueObject {
    std::string owner;
    std::uint32_t schema_version{1};
    std::vector<std::string> required_features;
    std::map<std::string, std::string> payload;

    bool operator==(const VersionedOpaqueObject&) const = default;
};

struct Track {
    TrackId id;
    std::uint32_t order_index{};
    std::optional<std::string> label;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const Track&) const = default;
};

enum class EventKind {
    shot,
    motion_peak,
    action_peak,
    beat,
    onset,
    band_energy,
    manual,
};

enum class EventOrigin {
    user,
    analysis,
    import,
};

Result<EventKind> parse_event_kind(std::string_view token);
std::string_view to_string(EventKind kind) noexcept;

struct EventSource {
    EventOrigin origin{EventOrigin::user};
    std::string producer_id;
    std::string producer_version;
    std::optional<std::string> input_fingerprint;
    std::optional<std::string> parameters_digest;
    std::optional<AnalysisRevision> analysis_revision;
    std::vector<CandidateId> candidate_ids;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const EventSource&) const = default;
};

struct RhythmEvent {
    EventId id;
    TrackId track_id;
    TimeNs time_ns{};
    DurationNs duration_ns{};
    EventKind kind{EventKind::manual};
    EventSource source;
    NormPpm strength_ppm{};
    std::optional<NormPpm> confidence_ppm;
    bool locked{false};
    bool user_edited{false};
    std::optional<VersionedOpaqueObject> sound_assignment;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const RhythmEvent&) const = default;
};

struct AnalysisCandidate {
    CandidateId id;
    std::optional<TrackId> proposed_track_id;
    TimeNs time_ns{};
    DurationNs duration_ns{};
    EventKind kind{EventKind::beat};
    EventSource source;
    NormPpm strength_ppm{};
    NormPpm confidence_ppm{};
    VersionedOpaqueObject payload;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const AnalysisCandidate&) const = default;
};

struct TimelineSnapshot {
    std::uint32_t schema_version{core::schema_version};
    std::string core_contract_version{contract_version};
    ProjectId project_id;
    TimelineRevision timeline_revision{};
    std::vector<Track> tracks;
    std::vector<RhythmEvent> events;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const TimelineSnapshot&) const = default;
};

enum class RemoveTrackPolicy {
    reject_non_empty,
    delete_events,
};

struct AddTrack {
    Track track;
};

struct RemoveTrack {
    TrackId track_id;
    RemoveTrackPolicy policy{RemoveTrackPolicy::reject_non_empty};
};

struct ReorderTracks {
    std::vector<TrackId> ordered_track_ids;
};

struct RenameTrack {
    TrackId track_id;
    std::optional<std::string> label;
};

struct AddEvent {
    RhythmEvent event;
};

struct RemoveEvent {
    EventId event_id;
};

struct MoveEvent {
    EventId event_id;
    TimeNs time_ns{};
    std::optional<TrackId> track_id;
};

struct PatchEvent {
    EventId event_id;
    std::optional<TrackId> track_id;
    std::optional<TimeNs> time_ns;
    std::optional<DurationNs> duration_ns;
    std::optional<EventKind> kind;
    std::optional<NormPpm> strength_ppm;
    bool patch_sound_assignment{false};
    std::optional<VersionedOpaqueObject> sound_assignment;
};

struct SetEventLocked {
    EventId event_id;
    bool locked{false};
};

struct BatchOffsetEvents {
    std::vector<EventId> event_ids;
    TimeNs delta_ns{};
};

struct MergeParameters {
    std::string algorithm_version{"deterministic-fusion-v1"};
    TrackId default_track_id{std::string{"track-0"}};
    DurationNs protection_window_ns{};
    DurationNs minimum_spacing_ns{};
    DurationNs density_window_ns{1'000'000'000};
    std::uint32_t max_events_per_density_window{64};
    std::uint32_t strength_weight{1};
    std::uint32_t confidence_weight{1};
    std::uint64_t seed{};
    bool replace_unprotected_analysis{true};

    bool operator==(const MergeParameters&) const = default;
};

struct MergeAnalysisCandidates {
    AnalysisRevision analysis_revision;
    std::vector<AnalysisCandidate> candidates;
    MergeParameters parameters;
};

using Operation = std::variant<AddTrack,
                               RemoveTrack,
                               ReorderTracks,
                               RenameTrack,
                               AddEvent,
                               RemoveEvent,
                               MoveEvent,
                               PatchEvent,
                               SetEventLocked,
                               BatchOffsetEvents,
                               MergeAnalysisCandidates>;

enum class TransactionOrigin {
    user,
    analysis,
    system,
};

struct TransactionRequest {
    TransactionId transaction_id;
    TimelineRevision base_timeline_revision{};
    TransactionOrigin origin{TransactionOrigin::user};
    std::vector<Operation> operations;
    std::optional<std::string> coalescing_key;
};

enum class TransactionStatus {
    committed,
    no_change,
    rejected,
    cancelled,
};

enum class SuppressionReason {
    protected_manual,
    protected_locked,
    duplicate_candidate,
    merge_window,
    density_limit,
};

struct SuppressedCandidate {
    CandidateId candidate_id;
    SuppressionReason reason{SuppressionReason::merge_window};

    bool operator==(const SuppressedCandidate&) const = default;
};

struct ChangeSummary {
    std::size_t track_count_before{};
    std::size_t track_count_after{};
    std::size_t event_count_before{};
    std::size_t event_count_after{};

    bool operator==(const ChangeSummary&) const = default;
};

struct TransactionResult {
    TransactionId transaction_id;
    TransactionStatus status{TransactionStatus::rejected};
    TimelineRevision before_revision{};
    TimelineRevision after_revision{};
    std::optional<ErrorInfo> error;
    std::optional<ChangeSummary> change_summary;
    std::vector<SuppressedCandidate> suppression_report;

    bool operator==(const TransactionResult&) const = default;
};

class CancellationToken final {
public:
    CancellationToken();
    void cancel() const noexcept;
    [[nodiscard]] bool is_cancelled() const noexcept;

private:
    std::shared_ptr<std::atomic_bool> cancelled_;
};

struct SchemaEnvelope {
    std::uint32_t schema_version{core::schema_version};
    std::vector<std::string> required_features;
    std::map<std::string, VersionedOpaqueObject> extensions;

    bool operator==(const SchemaEnvelope&) const = default;
};

struct DecodedDtoShape {
    bool missing_required_field{false};
    bool duplicate_field{false};
    bool type_mismatch{false};
    std::vector<std::string> unknown_top_level_fields;
};

Result<SchemaEnvelope> validate_schema_envelope(
    SchemaEnvelope envelope,
    const std::set<std::string>& supported_features = {});
Result<DecodedDtoShape> validate_decoded_dto_shape(DecodedDtoShape shape);
Result<ErrorInfo> validate_error_info(ErrorInfo error);
ErrorInfo wrap_error(ErrorInfo cause, std::string stage);
ErrorInfo map_current_exception(std::string stage) noexcept;

class Timeline final {
public:
    explicit Timeline(ProjectId project_id);
    explicit Timeline(TimelineSnapshot loaded_snapshot);
    ~Timeline();

    Timeline(const Timeline&) = delete;
    Timeline& operator=(const Timeline&) = delete;
    Timeline(Timeline&&) = delete;
    Timeline& operator=(Timeline&&) = delete;

    [[nodiscard]] std::shared_ptr<const TimelineSnapshot> snapshot() const;
    TransactionResult submit(const TransactionRequest& request,
                             const CancellationToken* cancellation = nullptr);
    TransactionResult undo(TimelineRevision expected_current_revision);
    TransactionResult redo(TimelineRevision expected_current_revision);

    void mark_saved();
    [[nodiscard]] bool is_dirty() const;
    [[nodiscard]] std::size_t undo_depth() const;
    [[nodiscard]] std::size_t redo_depth() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

Result<TimelineSnapshot> validate_and_normalize_snapshot(TimelineSnapshot snapshot);

} // namespace space_rhythm::core
