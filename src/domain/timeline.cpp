#include <space_rhythm/core/timeline.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <functional>
#include <iomanip>
#include <intrin.h>
#include <limits>
#include <mutex>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace space_rhythm::core {
namespace {

constexpr std::size_t max_transaction_operations = 100'000;
constexpr std::size_t max_history_entries = 1'024;

std::atomic_uint64_t next_diagnostic_id{1};

ErrorInfo make_error(ErrorCategory category,
                     ErrorCode code,
                     std::string_view stage,
                     bool retryable = false)
{
    const auto sequence = next_diagnostic_id.fetch_add(1, std::memory_order_relaxed);
    return ErrorInfo{schema_version,
                     category,
                     code,
                     std::string{stage},
                     "core:" + std::to_string(sequence),
                     retryable,
                     std::string{to_string(code)},
                     {},
                     {}};
}

ErrorInfo validation_error(ErrorCode code, std::string_view stage)
{
    return make_error(ErrorCategory::validation, code, stage);
}

ErrorInfo conflict_error(ErrorCode code, std::string_view stage, bool retryable = false)
{
    return make_error(ErrorCategory::conflict, code, stage, retryable);
}

bool is_ascii_token(std::string_view value)
{
    if (value.empty()) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9') || character == '_'
            || character == '-' || character == '.' || character == ':';
    });
}

bool is_nonempty_ascii(std::string_view value)
{
    return !value.empty()
        && std::ranges::all_of(value, [](const unsigned char character) {
               return character >= 0x20U && character <= 0x7eU;
           });
}

template <typename Id>
std::optional<ErrorInfo> validate_id(const Id& id, std::string_view stage)
{
    const auto result = validate_identifier(id.value);
    if (!result) {
        return validation_error(ErrorCode::invalid_identifier, stage);
    }
    return std::nullopt;
}

std::optional<ErrorInfo> validate_opaque(const VersionedOpaqueObject& object,
                                         std::string_view stage)
{
    if (!is_ascii_token(object.owner) || object.owner.find('.') == std::string::npos
        || object.schema_version == 0) {
        return validation_error(ErrorCode::invalid_dto, stage);
    }
    std::set<std::string> features;
    for (const auto& feature : object.required_features) {
        if (!is_ascii_token(feature) || !features.insert(feature).second) {
            return validation_error(ErrorCode::invalid_dto, stage);
        }
    }
    return std::nullopt;
}

std::optional<ErrorInfo> validate_extensions(
    const std::map<std::string, VersionedOpaqueObject>& extensions,
    std::string_view stage)
{
    for (const auto& [key, object] : extensions) {
        if (!is_ascii_token(key) || key.find('.') == std::string::npos) {
            return validation_error(ErrorCode::invalid_dto, stage);
        }
        if (const auto error = validate_opaque(object, stage)) {
            return error;
        }
    }
    return std::nullopt;
}

std::optional<ErrorInfo> validate_source(const EventSource& source,
                                         const std::optional<NormPpm>& confidence,
                                         std::string_view stage)
{
    if (!validate_identifier(source.producer_id)
        || !is_nonempty_ascii(source.producer_version)) {
        return validation_error(ErrorCode::invalid_event, stage);
    }
    std::set<CandidateId> candidate_ids;
    for (const auto& candidate_id : source.candidate_ids) {
        if (validate_id(candidate_id, stage) || !candidate_ids.insert(candidate_id).second) {
            return validation_error(ErrorCode::invalid_event, stage);
        }
    }
    if (source.origin == EventOrigin::analysis) {
        if (!source.analysis_revision || !source.input_fingerprint
            || !is_nonempty_ascii(*source.input_fingerprint) || !source.parameters_digest
            || !is_nonempty_ascii(*source.parameters_digest)) {
            return validation_error(ErrorCode::invalid_event, stage);
        }
        if (validate_id(*source.analysis_revision, stage)) {
            return validation_error(ErrorCode::invalid_event, stage);
        }
    } else if (source.origin == EventOrigin::user) {
        if (source.analysis_revision || confidence) {
            return validation_error(ErrorCode::invalid_event, stage);
        }
    }
    return validate_extensions(source.extensions, stage);
}

std::optional<ErrorInfo> validate_event(const RhythmEvent& event,
                                        const std::set<TrackId>& track_ids,
                                        std::string_view stage)
{
    if (validate_id(event.id, stage)) {
        return validation_error(ErrorCode::invalid_identifier, stage);
    }
    if (validate_id(event.track_id, stage)) {
        return validation_error(ErrorCode::invalid_identifier, stage);
    }
    if (!track_ids.contains(event.track_id)) {
        return validation_error(ErrorCode::track_not_found, stage);
    }
    if (event.time_ns < 0 || event.duration_ns < 0 || event.strength_ppm > norm_ppm_max
        || (event.confidence_ppm && *event.confidence_ppm > norm_ppm_max)) {
        return validation_error(ErrorCode::invalid_event, stage);
    }
    if (!checked_add(event.time_ns, event.duration_ns)) {
        return validation_error(ErrorCode::time_overflow, stage);
    }
    if (const auto error = validate_source(event.source, event.confidence_ppm, stage)) {
        return error;
    }
    if (event.sound_assignment) {
        if (const auto error = validate_opaque(*event.sound_assignment, stage)) {
            return error;
        }
    }
    return validate_extensions(event.extensions, stage);
}

std::optional<ErrorInfo> validate_candidate(const AnalysisCandidate& candidate,
                                            const std::set<TrackId>& track_ids,
                                            const TrackId& default_track,
                                            const AnalysisRevision& revision,
                                            std::string_view stage)
{
    if (validate_id(candidate.id, stage)) {
        return validation_error(ErrorCode::invalid_identifier, stage);
    }
    const auto& track_id = candidate.proposed_track_id.value_or(default_track);
    if (!track_ids.contains(track_id)) {
        return validation_error(ErrorCode::track_not_found, stage);
    }
    if (candidate.time_ns < 0 || candidate.duration_ns < 0
        || candidate.kind == EventKind::manual || candidate.strength_ppm > norm_ppm_max
        || candidate.confidence_ppm > norm_ppm_max
        || candidate.source.origin != EventOrigin::analysis
        || !candidate.source.analysis_revision
        || *candidate.source.analysis_revision != revision) {
        return validation_error(ErrorCode::invalid_event, stage);
    }
    if (!checked_add(candidate.time_ns, candidate.duration_ns)) {
        return validation_error(ErrorCode::time_overflow, stage);
    }
    if (const auto error = validate_source(
            candidate.source, std::optional<NormPpm>{candidate.confidence_ppm}, stage)) {
        return error;
    }
    if (const auto error = validate_opaque(candidate.payload, stage)) {
        return error;
    }
    return validate_extensions(candidate.extensions, stage);
}

std::uint64_t magnitude(std::int64_t value) noexcept
{
    if (value >= 0) {
        return static_cast<std::uint64_t>(value);
    }
    return static_cast<std::uint64_t>(-(value + 1)) + 1U;
}

struct WideUnsigned {
    std::array<std::uint64_t, 3> limbs{};
};

bool multiply(WideUnsigned& value, std::uint64_t multiplier) noexcept
{
    std::uint64_t carry = 0;
    for (auto& limb : value.limbs) {
        std::uint64_t high = 0;
        const auto low = _umul128(limb, multiplier, &high);
        const auto with_carry = low + carry;
        const bool carry_overflow = with_carry < low;
        limb = with_carry;
        carry = high + static_cast<std::uint64_t>(carry_overflow);
        if (carry < high) {
            return false;
        }
    }
    return carry == 0;
}

std::pair<WideUnsigned, std::uint64_t> divide(WideUnsigned value,
                                               std::uint64_t divisor) noexcept
{
    WideUnsigned quotient;
    std::uint64_t remainder = 0;
    for (std::size_t index = value.limbs.size(); index-- > 0;) {
        quotient.limbs[index] =
            _udiv128(remainder, value.limbs[index], divisor, &remainder);
    }
    return {quotient, remainder};
}

bool increment(WideUnsigned& value) noexcept
{
    for (auto& limb : value.limbs) {
        ++limb;
        if (limb != 0) {
            return true;
        }
    }
    return false;
}

bool request_cancelled(const CancellationToken* cancellation) noexcept
{
    return cancellation != nullptr && cancellation->is_cancelled();
}

std::optional<std::size_t> event_index(const TimelineSnapshot& snapshot,
                                       const EventId& event_id)
{
    const auto iterator = std::ranges::find(snapshot.events, event_id, &RhythmEvent::id);
    if (iterator == snapshot.events.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(snapshot.events.begin(), iterator));
}

std::optional<std::size_t> track_index(const TimelineSnapshot& snapshot,
                                       const TrackId& track_id)
{
    const auto iterator = std::ranges::find(snapshot.tracks, track_id, &Track::id);
    if (iterator == snapshot.tracks.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(std::distance(snapshot.tracks.begin(), iterator));
}

bool semantic_equal(const TimelineSnapshot& lhs, const TimelineSnapshot& rhs)
{
    return lhs.schema_version == rhs.schema_version
        && lhs.core_contract_version == rhs.core_contract_version
        && lhs.project_id == rhs.project_id && lhs.tracks == rhs.tracks
        && lhs.events == rhs.events && lhs.extensions == rhs.extensions;
}

std::string canonical_opaque(const VersionedOpaqueObject& object)
{
    std::ostringstream stream;
    stream << std::quoted(object.owner) << ':' << object.schema_version << ':';
    for (const auto& feature : object.required_features) {
        stream << std::quoted(feature) << ',';
    }
    stream << ':';
    for (const auto& [key, value] : object.payload) {
        stream << std::quoted(key) << '=' << std::quoted(value) << ',';
    }
    return stream.str();
}

void append_extensions(
    std::ostringstream& stream,
    const std::map<std::string, VersionedOpaqueObject>& extensions)
{
    for (const auto& [key, value] : extensions) {
        stream << std::quoted(key) << '{' << canonical_opaque(value) << '}';
    }
}

void append_source(std::ostringstream& stream, const EventSource& source)
{
    stream << static_cast<int>(source.origin) << std::quoted(source.producer_id)
           << std::quoted(source.producer_version);
    const auto append_optional = [&stream](const std::optional<std::string>& value) {
        stream << value.has_value();
        if (value) {
            stream << std::quoted(*value);
        }
    };
    append_optional(source.input_fingerprint);
    append_optional(source.parameters_digest);
    stream << source.analysis_revision.has_value();
    if (source.analysis_revision) {
        stream << std::quoted(source.analysis_revision->value);
    }
    for (const auto& id : source.candidate_ids) {
        stream << std::quoted(id.value);
    }
    append_extensions(stream, source.extensions);
}

void append_event(std::ostringstream& stream, const RhythmEvent& event)
{
    stream << std::quoted(event.id.value) << std::quoted(event.track_id.value)
           << event.time_ns << ':' << event.duration_ns << ':'
           << static_cast<int>(event.kind) << ':' << event.strength_ppm << ':'
           << event.confidence_ppm.has_value();
    if (event.confidence_ppm) {
        stream << *event.confidence_ppm;
    }
    stream << event.locked << event.user_edited;
    append_source(stream, event.source);
    stream << event.sound_assignment.has_value();
    if (event.sound_assignment) {
        stream << canonical_opaque(*event.sound_assignment);
    }
    append_extensions(stream, event.extensions);
}

void append_candidate(std::ostringstream& stream, const AnalysisCandidate& candidate)
{
    stream << std::quoted(candidate.id.value) << candidate.proposed_track_id.has_value();
    if (candidate.proposed_track_id) {
        stream << std::quoted(candidate.proposed_track_id->value);
    }
    stream << candidate.time_ns << ':' << candidate.duration_ns << ':'
           << static_cast<int>(candidate.kind) << ':' << candidate.strength_ppm << ':'
           << candidate.confidence_ppm;
    append_source(stream, candidate.source);
    stream << canonical_opaque(candidate.payload);
    append_extensions(stream, candidate.extensions);
}

std::string canonical_merge(const MergeAnalysisCandidates& merge)
{
    std::ostringstream stream;
    stream << std::quoted(merge.analysis_revision.value)
           << std::quoted(merge.parameters.algorithm_version)
           << std::quoted(merge.parameters.default_track_id.value)
           << merge.parameters.protection_window_ns << ':'
           << merge.parameters.minimum_spacing_ns << ':'
           << merge.parameters.density_window_ns << ':'
           << merge.parameters.max_events_per_density_window << ':'
           << merge.parameters.strength_weight << ':'
           << merge.parameters.confidence_weight << ':' << merge.parameters.seed << ':'
           << merge.parameters.replace_unprotected_analysis;
    std::vector<std::string> candidates;
    candidates.reserve(merge.candidates.size());
    for (const auto& candidate : merge.candidates) {
        std::ostringstream item;
        append_candidate(item, candidate);
        candidates.push_back(item.str());
    }
    std::ranges::sort(candidates);
    for (const auto& candidate : candidates) {
        stream << '{' << candidate << '}';
    }
    return stream.str();
}

std::string canonical_request(const TransactionRequest& request)
{
    std::ostringstream stream;
    stream << request.base_timeline_revision << ':' << static_cast<int>(request.origin)
           << ':' << request.coalescing_key.has_value();
    if (request.coalescing_key) {
        stream << std::quoted(*request.coalescing_key);
    }
    for (const auto& operation : request.operations) {
        stream << '[' << operation.index() << ':';
        std::visit(
            [&stream](const auto& value) {
                using Type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Type, AddTrack>) {
                    stream << std::quoted(value.track.id.value) << ':'
                           << value.track.order_index << ':' << value.track.label.has_value();
                    if (value.track.label) {
                        stream << std::quoted(*value.track.label);
                    }
                    append_extensions(stream, value.track.extensions);
                } else if constexpr (std::is_same_v<Type, RemoveTrack>) {
                    stream << std::quoted(value.track_id.value) << ':'
                           << static_cast<int>(value.policy);
                } else if constexpr (std::is_same_v<Type, ReorderTracks>) {
                    for (const auto& id : value.ordered_track_ids) {
                        stream << std::quoted(id.value);
                    }
                } else if constexpr (std::is_same_v<Type, RenameTrack>) {
                    stream << std::quoted(value.track_id.value) << ':'
                           << value.label.has_value();
                    if (value.label) {
                        stream << std::quoted(*value.label);
                    }
                } else if constexpr (std::is_same_v<Type, AddEvent>) {
                    append_event(stream, value.event);
                } else if constexpr (std::is_same_v<Type, RemoveEvent>) {
                    stream << std::quoted(value.event_id.value);
                } else if constexpr (std::is_same_v<Type, MoveEvent>) {
                    stream << std::quoted(value.event_id.value) << ':' << value.time_ns << ':'
                           << value.track_id.has_value();
                    if (value.track_id) {
                        stream << std::quoted(value.track_id->value);
                    }
                } else if constexpr (std::is_same_v<Type, PatchEvent>) {
                    stream << std::quoted(value.event_id.value) << ':'
                           << value.track_id.has_value() << ':' << value.time_ns.has_value()
                           << ':' << value.duration_ns.has_value() << ':'
                           << value.kind.has_value() << ':' << value.strength_ppm.has_value()
                           << ':' << value.patch_sound_assignment;
                    if (value.track_id) {
                        stream << std::quoted(value.track_id->value);
                    }
                    if (value.time_ns) {
                        stream << *value.time_ns;
                    }
                    if (value.duration_ns) {
                        stream << *value.duration_ns;
                    }
                    if (value.kind) {
                        stream << static_cast<int>(*value.kind);
                    }
                    if (value.strength_ppm) {
                        stream << *value.strength_ppm;
                    }
                    if (value.patch_sound_assignment && value.sound_assignment) {
                        stream << canonical_opaque(*value.sound_assignment);
                    }
                } else if constexpr (std::is_same_v<Type, SetEventLocked>) {
                    stream << std::quoted(value.event_id.value) << ':' << value.locked;
                } else if constexpr (std::is_same_v<Type, BatchOffsetEvents>) {
                    stream << value.delta_ns << ':';
                    for (const auto& id : value.event_ids) {
                        stream << std::quoted(id.value);
                    }
                } else if constexpr (std::is_same_v<Type, MergeAnalysisCandidates>) {
                    stream << canonical_merge(value);
                }
            },
            operation);
        stream << ']';
    }
    return stream.str();
}

std::set<std::string> affected_entities(const TransactionRequest& request)
{
    std::set<std::string> entities;
    for (const auto& operation : request.operations) {
        std::visit(
            [&entities](const auto& value) {
                using Type = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<Type, AddTrack>) {
                    entities.insert("track:" + value.track.id.value);
                } else if constexpr (std::is_same_v<Type, RemoveTrack>
                                     || std::is_same_v<Type, RenameTrack>) {
                    entities.insert("track:" + value.track_id.value);
                } else if constexpr (std::is_same_v<Type, ReorderTracks>) {
                    for (const auto& id : value.ordered_track_ids) {
                        entities.insert("track:" + id.value);
                    }
                } else if constexpr (std::is_same_v<Type, AddEvent>) {
                    entities.insert("event:" + value.event.id.value);
                } else if constexpr (std::is_same_v<Type, RemoveEvent>
                                     || std::is_same_v<Type, MoveEvent>
                                     || std::is_same_v<Type, PatchEvent>
                                     || std::is_same_v<Type, SetEventLocked>) {
                    entities.insert("event:" + value.event_id.value);
                } else if constexpr (std::is_same_v<Type, BatchOffsetEvents>) {
                    for (const auto& id : value.event_ids) {
                        entities.insert("event:" + id.value);
                    }
                } else if constexpr (std::is_same_v<Type, MergeAnalysisCandidates>) {
                    entities.insert("analysis:" + value.analysis_revision.value);
                }
            },
            operation);
    }
    return entities;
}

std::uint64_t stable_hash(std::string_view value, std::uint64_t seed) noexcept
{
    std::uint64_t hash = 14695981039346656037ULL ^ seed;
    for (const unsigned char character : value) {
        hash ^= character;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string hex64(std::uint64_t value)
{
    std::array<char, 16> buffer{};
    constexpr std::string_view digits{"0123456789abcdef"};
    for (std::size_t index = buffer.size(); index-- > 0;) {
        buffer[index] = digits[static_cast<std::size_t>(value & 0xFU)];
        value >>= 4U;
    }
    return std::string{buffer.data(), buffer.size()};
}

EventId generated_event_id(const AnalysisCandidate& candidate,
                            const AnalysisRevision& revision,
                            std::uint64_t seed)
{
    const auto identity = revision.value + ':' + candidate.id.value;
    return EventId{"analysis-" + hex64(stable_hash(identity, seed)) + '-'
                   + hex64(stable_hash(identity, seed ^ 0x9e3779b97f4a7c15ULL))};
}

bool within_window(TimeNs lhs, TimeNs rhs, DurationNs window) noexcept
{
    if (window < 0) {
        return false;
    }
    const auto distance = lhs >= rhs ? static_cast<std::uint64_t>(lhs - rhs)
                                     : static_cast<std::uint64_t>(rhs - lhs);
    return distance <= static_cast<std::uint64_t>(window);
}

bool protected_manual(const RhythmEvent& event) noexcept
{
    return event.kind == EventKind::manual || event.source.origin == EventOrigin::user
        || event.user_edited;
}

struct ApplyContext {
    TransactionOrigin origin{TransactionOrigin::user};
    std::vector<SuppressedCandidate> suppression;
    std::map<std::string, std::string> pending_analysis_revisions;
};

std::optional<ErrorInfo> apply_merge(
    TimelineSnapshot& working,
    const MergeAnalysisCandidates& merge,
    ApplyContext& context,
    const std::unordered_map<std::string, std::string>& known_analysis_revisions)
{
    constexpr std::string_view stage{"transaction.merge"};
    if (validate_id(merge.analysis_revision, stage)
        || !is_ascii_token(merge.parameters.algorithm_version)
        || validate_id(merge.parameters.default_track_id, stage)
        || merge.parameters.protection_window_ns < 0
        || merge.parameters.minimum_spacing_ns < 0
        || merge.parameters.density_window_ns <= 0
        || merge.parameters.max_events_per_density_window == 0) {
        return validation_error(ErrorCode::invalid_transaction, stage);
    }

    std::set<TrackId> track_ids;
    std::map<TrackId, std::uint32_t> track_orders;
    for (const auto& track : working.tracks) {
        track_ids.insert(track.id);
        track_orders.emplace(track.id, track.order_index);
    }
    if (!track_ids.contains(merge.parameters.default_track_id)) {
        return validation_error(ErrorCode::track_not_found, stage);
    }

    for (const auto& candidate : merge.candidates) {
        if (const auto error = validate_candidate(candidate,
                                                  track_ids,
                                                  merge.parameters.default_track_id,
                                                  merge.analysis_revision,
                                                  stage)) {
            return error;
        }
    }

    const auto signature = canonical_merge(merge);
    const auto known = known_analysis_revisions.find(merge.analysis_revision.value);
    if (known != known_analysis_revisions.end() && known->second != signature) {
        return conflict_error(ErrorCode::analysis_revision_conflict, stage);
    }
    const auto pending = context.pending_analysis_revisions.find(merge.analysis_revision.value);
    if (pending != context.pending_analysis_revisions.end() && pending->second != signature) {
        return conflict_error(ErrorCode::analysis_revision_conflict, stage);
    }
    context.pending_analysis_revisions[merge.analysis_revision.value] = signature;

    std::set<CandidateId> seen_ids;
    std::vector<const AnalysisCandidate*> eligible;
    eligible.reserve(merge.candidates.size());
    for (const auto& candidate : merge.candidates) {
        if (!seen_ids.insert(candidate.id).second) {
            context.suppression.push_back(
                SuppressedCandidate{candidate.id, SuppressionReason::duplicate_candidate});
            continue;
        }
        const auto& target_track =
            candidate.proposed_track_id.value_or(merge.parameters.default_track_id);
        const auto manual = std::ranges::find_if(working.events, [&](const RhythmEvent& event) {
            return event.track_id == target_track && protected_manual(event)
                && within_window(event.time_ns,
                                 candidate.time_ns,
                                 merge.parameters.protection_window_ns);
        });
        if (manual != working.events.end()) {
            context.suppression.push_back(
                SuppressedCandidate{candidate.id, SuppressionReason::protected_manual});
            continue;
        }
        const auto locked = std::ranges::find_if(working.events, [&](const RhythmEvent& event) {
            return event.track_id == target_track && event.locked
                && within_window(event.time_ns,
                                 candidate.time_ns,
                                 merge.parameters.protection_window_ns);
        });
        if (locked != working.events.end()) {
            context.suppression.push_back(
                SuppressedCandidate{candidate.id, SuppressionReason::protected_locked});
            continue;
        }
        eligible.push_back(&candidate);
    }

    const auto score = [&](const AnalysisCandidate& candidate) {
        return static_cast<std::uint64_t>(candidate.strength_ppm)
                * merge.parameters.strength_weight
            + static_cast<std::uint64_t>(candidate.confidence_ppm)
                * merge.parameters.confidence_weight;
    };
    std::ranges::sort(eligible, [&](const AnalysisCandidate* lhs,
                                    const AnalysisCandidate* rhs) {
        const auto lhs_track = lhs->proposed_track_id.value_or(
            merge.parameters.default_track_id);
        const auto rhs_track = rhs->proposed_track_id.value_or(
            merge.parameters.default_track_id);
        return std::tuple{track_orders.at(lhs_track),
                          std::numeric_limits<std::uint64_t>::max() - score(*lhs),
                          stable_hash(lhs->id.value, merge.parameters.seed),
                          lhs->time_ns,
                          lhs->id.value}
            < std::tuple{track_orders.at(rhs_track),
                         std::numeric_limits<std::uint64_t>::max() - score(*rhs),
                         stable_hash(rhs->id.value, merge.parameters.seed),
                         rhs->time_ns,
                         rhs->id.value};
    });

    std::vector<const AnalysisCandidate*> accepted;
    std::map<std::pair<TrackId, std::uint64_t>, std::uint32_t> density_counts;
    for (const auto* candidate : eligible) {
        const auto track_id =
            candidate->proposed_track_id.value_or(merge.parameters.default_track_id);
        const auto too_close = std::ranges::find_if(
            accepted, [&](const AnalysisCandidate* previous) {
                const auto previous_track = previous->proposed_track_id.value_or(
                    merge.parameters.default_track_id);
                return previous_track == track_id
                    && within_window(previous->time_ns,
                                     candidate->time_ns,
                                     merge.parameters.minimum_spacing_ns);
            });
        if (too_close != accepted.end()) {
            context.suppression.push_back(
                SuppressedCandidate{candidate->id, SuppressionReason::merge_window});
            continue;
        }
        const auto bucket = static_cast<std::uint64_t>(candidate->time_ns)
            / static_cast<std::uint64_t>(merge.parameters.density_window_ns);
        auto& count = density_counts[{track_id, bucket}];
        if (count >= merge.parameters.max_events_per_density_window) {
            context.suppression.push_back(
                SuppressedCandidate{candidate->id, SuppressionReason::density_limit});
            continue;
        }
        ++count;
        accepted.push_back(candidate);
    }

    if (merge.parameters.replace_unprotected_analysis) {
        std::set<TrackId> affected_tracks;
        for (const auto& candidate : merge.candidates) {
            affected_tracks.insert(
                candidate.proposed_track_id.value_or(merge.parameters.default_track_id));
        }
        working.events.erase(
            std::remove_if(working.events.begin(),
                           working.events.end(),
                           [&](const RhythmEvent& event) {
                               return affected_tracks.contains(event.track_id)
                                   && event.source.origin == EventOrigin::analysis
                                   && !event.locked && !protected_manual(event);
                           }),
            working.events.end());
    }

    for (const auto* candidate : accepted) {
        EventSource source = candidate->source;
        source.analysis_revision = merge.analysis_revision;
        source.candidate_ids = {candidate->id};
        working.events.push_back(RhythmEvent{
            generated_event_id(*candidate, merge.analysis_revision, merge.parameters.seed),
            candidate->proposed_track_id.value_or(merge.parameters.default_track_id),
            candidate->time_ns,
            candidate->duration_ns,
            candidate->kind,
            std::move(source),
            candidate->strength_ppm,
            candidate->confidence_ppm,
            false,
            false,
            std::nullopt,
            candidate->extensions});
    }
    std::ranges::sort(context.suppression, {}, &SuppressedCandidate::candidate_id);
    return std::nullopt;
}

std::optional<ErrorInfo> apply_operation(
    TimelineSnapshot& working,
    const Operation& operation,
    ApplyContext& context,
    const std::unordered_map<std::string, std::string>& known_analysis_revisions)
{
    constexpr std::string_view stage{"transaction.apply"};
    return std::visit(
        [&](const auto& value) -> std::optional<ErrorInfo> {
            using Type = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Type, AddTrack>) {
                if (validate_id(value.track.id, stage)) {
                    return validation_error(ErrorCode::invalid_identifier, stage);
                }
                if (track_index(working, value.track.id)) {
                    return validation_error(ErrorCode::duplicate_track_id, stage);
                }
                if (value.track.order_index > working.tracks.size()) {
                    return validation_error(ErrorCode::invalid_track, stage);
                }
                for (auto& track : working.tracks) {
                    if (track.order_index >= value.track.order_index) {
                        ++track.order_index;
                    }
                }
                working.tracks.push_back(value.track);
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, RemoveTrack>) {
                const auto index = track_index(working, value.track_id);
                if (!index) {
                    return validation_error(ErrorCode::track_not_found, stage);
                }
                const auto has_events = std::ranges::any_of(
                    working.events,
                    [&](const RhythmEvent& event) { return event.track_id == value.track_id; });
                if (has_events && value.policy == RemoveTrackPolicy::reject_non_empty) {
                    return validation_error(ErrorCode::track_not_empty, stage);
                }
                if (value.policy == RemoveTrackPolicy::delete_events
                    && std::ranges::any_of(working.events, [&](const RhythmEvent& event) {
                           return event.track_id == value.track_id && event.locked;
                       })) {
                    return conflict_error(ErrorCode::locked_event, stage);
                }
                working.events.erase(
                    std::remove_if(working.events.begin(),
                                   working.events.end(),
                                   [&](const RhythmEvent& event) {
                                       return event.track_id == value.track_id;
                                   }),
                    working.events.end());
                const auto removed_order = working.tracks[*index].order_index;
                working.tracks.erase(working.tracks.begin()
                                     + static_cast<std::ptrdiff_t>(*index));
                for (auto& track : working.tracks) {
                    if (track.order_index > removed_order) {
                        --track.order_index;
                    }
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, ReorderTracks>) {
                if (value.ordered_track_ids.size() != working.tracks.size()) {
                    return validation_error(ErrorCode::invalid_track, stage);
                }
                std::set<TrackId> ordered;
                for (std::size_t index = 0; index < value.ordered_track_ids.size(); ++index) {
                    const auto& id = value.ordered_track_ids[index];
                    if (!ordered.insert(id).second) {
                        return validation_error(ErrorCode::invalid_track, stage);
                    }
                    const auto found = track_index(working, id);
                    if (!found) {
                        return validation_error(ErrorCode::track_not_found, stage);
                    }
                    working.tracks[*found].order_index = static_cast<std::uint32_t>(index);
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, RenameTrack>) {
                const auto index = track_index(working, value.track_id);
                if (!index) {
                    return validation_error(ErrorCode::track_not_found, stage);
                }
                working.tracks[*index].label = value.label;
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, AddEvent>) {
                if (event_index(working, value.event.id)) {
                    return validation_error(ErrorCode::duplicate_event_id, stage);
                }
                working.events.push_back(value.event);
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, RemoveEvent>) {
                const auto index = event_index(working, value.event_id);
                if (!index) {
                    return validation_error(ErrorCode::event_not_found, stage);
                }
                if (working.events[*index].locked) {
                    return conflict_error(ErrorCode::locked_event, stage);
                }
                working.events.erase(working.events.begin()
                                     + static_cast<std::ptrdiff_t>(*index));
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, MoveEvent>) {
                const auto index = event_index(working, value.event_id);
                if (!index) {
                    return validation_error(ErrorCode::event_not_found, stage);
                }
                auto& event = working.events[*index];
                if (event.locked) {
                    return conflict_error(ErrorCode::locked_event, stage);
                }
                const auto changed = event.time_ns != value.time_ns
                    || (value.track_id && event.track_id != *value.track_id);
                event.time_ns = value.time_ns;
                if (value.track_id) {
                    event.track_id = *value.track_id;
                }
                if (changed && context.origin == TransactionOrigin::user) {
                    event.user_edited = true;
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, PatchEvent>) {
                const auto index = event_index(working, value.event_id);
                if (!index) {
                    return validation_error(ErrorCode::event_not_found, stage);
                }
                auto& event = working.events[*index];
                if (event.locked) {
                    return conflict_error(ErrorCode::locked_event, stage);
                }
                auto patched = event;
                if (value.track_id) {
                    patched.track_id = *value.track_id;
                }
                if (value.time_ns) {
                    patched.time_ns = *value.time_ns;
                }
                if (value.duration_ns) {
                    patched.duration_ns = *value.duration_ns;
                }
                if (value.kind) {
                    patched.kind = *value.kind;
                }
                if (value.strength_ppm) {
                    patched.strength_ppm = *value.strength_ppm;
                }
                if (value.patch_sound_assignment) {
                    patched.sound_assignment = value.sound_assignment;
                }
                if (patched != event && context.origin == TransactionOrigin::user) {
                    patched.user_edited = true;
                }
                event = std::move(patched);
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, SetEventLocked>) {
                const auto index = event_index(working, value.event_id);
                if (!index) {
                    return validation_error(ErrorCode::event_not_found, stage);
                }
                working.events[*index].locked = value.locked;
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, BatchOffsetEvents>) {
                std::set<EventId> seen;
                for (const auto& id : value.event_ids) {
                    if (!seen.insert(id).second) {
                        return validation_error(ErrorCode::invalid_transaction, stage);
                    }
                    const auto index = event_index(working, id);
                    if (!index) {
                        return validation_error(ErrorCode::event_not_found, stage);
                    }
                    if (working.events[*index].locked) {
                        return conflict_error(ErrorCode::locked_event, stage);
                    }
                    const auto moved = checked_add(working.events[*index].time_ns,
                                                   value.delta_ns);
                    if (!moved) {
                        return validation_error(ErrorCode::time_overflow, stage);
                    }
                    working.events[*index].time_ns = moved.value();
                    if (context.origin == TransactionOrigin::user && value.delta_ns != 0) {
                        working.events[*index].user_edited = true;
                    }
                }
                return std::nullopt;
            } else if constexpr (std::is_same_v<Type, MergeAnalysisCandidates>) {
                return apply_merge(working, value, context, known_analysis_revisions);
            }
        },
        operation);
}

TransactionResult rejected(TransactionId transaction_id,
                           TimelineRevision revision,
                           ErrorInfo error)
{
    return TransactionResult{std::move(transaction_id),
                             TransactionStatus::rejected,
                             revision,
                             revision,
                             std::move(error),
                             std::nullopt,
                             {}};
}

TransactionResult no_change(TransactionId transaction_id, TimelineRevision revision)
{
    return TransactionResult{std::move(transaction_id),
                             TransactionStatus::no_change,
                             revision,
                             revision,
                             std::nullopt,
                             std::nullopt,
                             {}};
}

TransactionResult cancelled(TransactionId transaction_id, TimelineRevision revision)
{
    return TransactionResult{
        std::move(transaction_id),
        TransactionStatus::cancelled,
        revision,
        revision,
        make_error(ErrorCategory::cancelled,
                   ErrorCode::transaction_cancelled,
                   "transaction.cancel"),
        std::nullopt,
        {}};
}

} // namespace

std::string_view to_string(ErrorCategory category) noexcept
{
    switch (category) {
    case ErrorCategory::validation:
        return "validation";
    case ErrorCategory::conflict:
        return "conflict";
    case ErrorCategory::cancelled:
        return "cancelled";
    case ErrorCategory::compatibility:
        return "compatibility";
    case ErrorCategory::media:
        return "media";
    case ErrorCategory::resource_limit:
        return "resource_limit";
    case ErrorCategory::internal:
        return "internal";
    }
    return "internal";
}

std::string_view to_string(ErrorCode code) noexcept
{
    switch (code) {
    case ErrorCode::invalid_identifier:
        return "invalid_identifier";
    case ErrorCode::invalid_time_base:
        return "invalid_time_base";
    case ErrorCode::time_overflow:
        return "time_overflow";
    case ErrorCode::invalid_dto:
        return "invalid_dto";
    case ErrorCode::invalid_track:
        return "invalid_track";
    case ErrorCode::invalid_event:
        return "invalid_event";
    case ErrorCode::duplicate_track_id:
        return "duplicate_track_id";
    case ErrorCode::duplicate_event_id:
        return "duplicate_event_id";
    case ErrorCode::track_not_found:
        return "track_not_found";
    case ErrorCode::event_not_found:
        return "event_not_found";
    case ErrorCode::track_not_empty:
        return "track_not_empty";
    case ErrorCode::invalid_transaction:
        return "invalid_transaction";
    case ErrorCode::stale_revision:
        return "stale_revision";
    case ErrorCode::locked_event:
        return "locked_event";
    case ErrorCode::history_conflict:
        return "history_conflict";
    case ErrorCode::idempotency_conflict:
        return "idempotency_conflict";
    case ErrorCode::analysis_revision_conflict:
        return "analysis_revision_conflict";
    case ErrorCode::revision_exhausted:
        return "revision_exhausted";
    case ErrorCode::transaction_cancelled:
        return "transaction_cancelled";
    case ErrorCode::unsupported_schema:
        return "unsupported_schema";
    case ErrorCode::unsupported_feature:
        return "unsupported_feature";
    case ErrorCode::unknown_enum:
        return "unknown_enum";
    case ErrorCode::unsupported_media:
        return "unsupported_media";
    case ErrorCode::corrupt_media:
        return "corrupt_media";
    case ErrorCode::missing_required_stream:
        return "missing_required_stream";
    case ErrorCode::stream_not_found:
        return "stream_not_found";
    case ErrorCode::stream_type_mismatch:
        return "stream_type_mismatch";
    case ErrorCode::timestamp_unavailable:
        return "timestamp_unavailable";
    case ErrorCode::timestamp_origin_unavailable:
        return "timestamp_origin_unavailable";
    case ErrorCode::timestamp_origin_changed:
        return "timestamp_origin_changed";
    case ErrorCode::timestamp_discontinuity:
        return "timestamp_discontinuity";
    case ErrorCode::seek_unreachable:
        return "seek_unreachable";
    case ErrorCode::unsupported_display_transform:
        return "unsupported_display_transform";
    case ErrorCode::format_changed:
        return "format_changed";
    case ErrorCode::decode_failed:
        return "decode_failed";
    case ErrorCode::invalid_pcm_buffer:
        return "invalid_pcm_buffer";
    case ErrorCode::unsupported_pcm_format:
        return "unsupported_pcm_format";
    case ErrorCode::unsupported_channel_layout:
        return "unsupported_channel_layout";
    case ErrorCode::non_finite_pcm:
        return "non_finite_pcm";
    case ErrorCode::pcm_out_of_range:
        return "pcm_out_of_range";
    case ErrorCode::pcm_discontinuity:
        return "pcm_discontinuity";
    case ErrorCode::resample_timing_unavailable:
        return "resample_timing_unavailable";
    case ErrorCode::timestamp_mismatch:
        return "timestamp_mismatch";
    case ErrorCode::invalid_analysis_parameters:
        return "invalid_analysis_parameters";
    case ErrorCode::unsupported_parameter_schema:
        return "unsupported_parameter_schema";
    case ErrorCode::invalid_feature_frame:
        return "invalid_feature_frame";
    case ErrorCode::invalid_analysis_candidate:
        return "invalid_analysis_candidate";
    case ErrorCode::cancelled:
        return "cancelled";
    case ErrorCode::resource_limit:
        return "resource_limit";
    case ErrorCode::invariant_violation:
        return "invariant_violation";
    case ErrorCode::internal_error:
        return "internal_error";
    }
    return "internal_error";
}

Result<std::string> validate_identifier(std::string_view value)
{
    if (value.empty() || value.size() > 128) {
        return Result<std::string>::failure(
            validation_error(ErrorCode::invalid_identifier, "identifier"));
    }
    const auto first = static_cast<unsigned char>(value.front());
    const auto is_alphanumeric = [](const unsigned char character) {
        return (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9');
    };
    if (!is_alphanumeric(first)) {
        return Result<std::string>::failure(
            validation_error(ErrorCode::invalid_identifier, "identifier"));
    }
    for (const unsigned char character : value) {
        if (!is_alphanumeric(character) && character != '.' && character != '_'
            && character != ':' && character != '-') {
            return Result<std::string>::failure(
                validation_error(ErrorCode::invalid_identifier, "identifier"));
        }
    }
    return Result<std::string>::success(std::string{value});
}

Result<TimeNs> scale_ticks(std::int64_t ticks, TimeBase base, RoundingMode mode)
{
    if (base.seconds_numerator <= 0 || base.seconds_denominator <= 0) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::invalid_time_base, "time.scale"));
    }
    WideUnsigned product{{magnitude(ticks), 0, 0}};
    if (!multiply(product, static_cast<std::uint64_t>(base.seconds_numerator))
        || !multiply(product, 1'000'000'000ULL)) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.scale"));
    }
    auto [quotient, remainder] =
        divide(product, static_cast<std::uint64_t>(base.seconds_denominator));
    const bool negative = ticks < 0;
    bool round_away_from_zero = false;
    switch (mode) {
    case RoundingMode::floor:
        round_away_from_zero = negative && remainder != 0;
        break;
    case RoundingMode::ceil:
        round_away_from_zero = !negative && remainder != 0;
        break;
    case RoundingMode::toward_zero:
        break;
    case RoundingMode::nearest_ties_to_even: {
        const auto denominator = static_cast<std::uint64_t>(base.seconds_denominator);
        const auto complement = denominator - remainder;
        round_away_from_zero = remainder > complement
            || (remainder == complement && (quotient.limbs[0] & 1U) != 0);
        break;
    }
    }
    if (round_away_from_zero && !increment(quotient)) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.scale"));
    }
    if (quotient.limbs[1] != 0 || quotient.limbs[2] != 0) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.scale"));
    }
    const auto limit = negative
        ? static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) + 1U
        : static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    if (quotient.limbs[0] > limit) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.scale"));
    }
    if (!negative) {
        return Result<TimeNs>::success(static_cast<TimeNs>(quotient.limbs[0]));
    }
    if (quotient.limbs[0] == limit) {
        return Result<TimeNs>::success(std::numeric_limits<TimeNs>::min());
    }
    return Result<TimeNs>::success(-static_cast<TimeNs>(quotient.limbs[0]));
}

Result<TimeNs> checked_add(TimeNs lhs, TimeNs rhs)
{
    if ((rhs > 0 && lhs > std::numeric_limits<TimeNs>::max() - rhs)
        || (rhs < 0 && lhs < std::numeric_limits<TimeNs>::min() - rhs)) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.add"));
    }
    return Result<TimeNs>::success(lhs + rhs);
}

Result<TimeNs> checked_subtract(TimeNs lhs, TimeNs rhs)
{
    if ((rhs < 0 && lhs > std::numeric_limits<TimeNs>::max() + rhs)
        || (rhs > 0 && lhs < std::numeric_limits<TimeNs>::min() + rhs)) {
        return Result<TimeNs>::failure(
            validation_error(ErrorCode::time_overflow, "time.subtract"));
    }
    return Result<TimeNs>::success(lhs - rhs);
}

Result<TimeRange> make_range(TimeNs start, DurationNs duration)
{
    if (start < 0 || duration < 0) {
        return Result<TimeRange>::failure(
            validation_error(ErrorCode::invalid_event, "time.range"));
    }
    const auto end = checked_add(start, duration);
    if (!end) {
        return Result<TimeRange>::failure(end.error());
    }
    return Result<TimeRange>::success(TimeRange{start, end.value()});
}

Result<EventKind> parse_event_kind(std::string_view token)
{
    constexpr std::array pairs{
        std::pair{std::string_view{"shot"}, EventKind::shot},
        std::pair{std::string_view{"motion_peak"}, EventKind::motion_peak},
        std::pair{std::string_view{"action_peak"}, EventKind::action_peak},
        std::pair{std::string_view{"beat"}, EventKind::beat},
        std::pair{std::string_view{"onset"}, EventKind::onset},
        std::pair{std::string_view{"band_energy"}, EventKind::band_energy},
        std::pair{std::string_view{"manual"}, EventKind::manual},
    };
    const auto found = std::ranges::find(pairs, token, &decltype(pairs)::value_type::first);
    if (found == pairs.end()) {
        return Result<EventKind>::failure(
            make_error(ErrorCategory::compatibility, ErrorCode::unknown_enum, "dto.enum"));
    }
    return Result<EventKind>::success(found->second);
}

std::string_view to_string(EventKind kind) noexcept
{
    switch (kind) {
    case EventKind::shot:
        return "shot";
    case EventKind::motion_peak:
        return "motion_peak";
    case EventKind::action_peak:
        return "action_peak";
    case EventKind::beat:
        return "beat";
    case EventKind::onset:
        return "onset";
    case EventKind::band_energy:
        return "band_energy";
    case EventKind::manual:
        return "manual";
    }
    return "manual";
}

CancellationToken::CancellationToken()
    : cancelled_(std::make_shared<std::atomic_bool>(false))
{
}

void CancellationToken::cancel() const noexcept
{
    cancelled_->store(true, std::memory_order_release);
}

bool CancellationToken::is_cancelled() const noexcept
{
    return cancelled_->load(std::memory_order_acquire);
}

Result<SchemaEnvelope> validate_schema_envelope(
    SchemaEnvelope envelope,
    const std::set<std::string>& supported_features)
{
    if (envelope.schema_version != schema_version) {
        return Result<SchemaEnvelope>::failure(make_error(ErrorCategory::compatibility,
                                                          ErrorCode::unsupported_schema,
                                                          "dto.schema"));
    }
    std::set<std::string> seen;
    for (const auto& feature : envelope.required_features) {
        if (!is_ascii_token(feature) || !seen.insert(feature).second) {
            return Result<SchemaEnvelope>::failure(
                validation_error(ErrorCode::invalid_dto, "dto.schema"));
        }
        if (!supported_features.contains(feature)) {
            return Result<SchemaEnvelope>::failure(make_error(
                ErrorCategory::compatibility,
                ErrorCode::unsupported_feature,
                "dto.feature"));
        }
    }
    if (const auto error = validate_extensions(envelope.extensions, "dto.extensions")) {
        return Result<SchemaEnvelope>::failure(*error);
    }
    return Result<SchemaEnvelope>::success(std::move(envelope));
}

Result<DecodedDtoShape> validate_decoded_dto_shape(DecodedDtoShape shape)
{
    if (shape.missing_required_field || shape.duplicate_field || shape.type_mismatch
        || !shape.unknown_top_level_fields.empty()) {
        return Result<DecodedDtoShape>::failure(
            validation_error(ErrorCode::invalid_dto, "dto.shape"));
    }
    return Result<DecodedDtoShape>::success(std::move(shape));
}

Result<ErrorInfo> validate_error_info(ErrorInfo error)
{
    if (error.schema_version != schema_version || error.stage.empty()
        || error.diagnostic_id.empty() || error.message_key.empty()
        || !is_ascii_token(error.stage) || !validate_identifier(error.diagnostic_id)
        || !is_ascii_token(error.message_key)) {
        return Result<ErrorInfo>::failure(
            validation_error(ErrorCode::invalid_dto, "dto.error"));
    }
    if (error.cause && error.cause->diagnostic_id != error.diagnostic_id) {
        return Result<ErrorInfo>::failure(
            validation_error(ErrorCode::invalid_dto, "dto.error"));
    }
    return Result<ErrorInfo>::success(std::move(error));
}

ErrorInfo wrap_error(ErrorInfo cause, std::string stage)
{
    ErrorInfo wrapped = cause;
    wrapped.stage = std::move(stage);
    wrapped.cause = std::make_shared<const ErrorInfo>(std::move(cause));
    return wrapped;
}

ErrorInfo map_current_exception(std::string stage) noexcept
{
    return make_error(ErrorCategory::internal, ErrorCode::internal_error, stage);
}

Result<TimelineSnapshot> validate_and_normalize_snapshot(TimelineSnapshot snapshot)
{
    constexpr std::string_view stage{"snapshot.validate"};
    if (snapshot.schema_version != schema_version
        || snapshot.core_contract_version != contract_version) {
        return Result<TimelineSnapshot>::failure(make_error(ErrorCategory::compatibility,
                                                            ErrorCode::unsupported_schema,
                                                            stage));
    }
    if (validate_id(snapshot.project_id, stage)) {
        return Result<TimelineSnapshot>::failure(
            validation_error(ErrorCode::invalid_identifier, stage));
    }
    std::ranges::sort(snapshot.tracks, [](const Track& lhs, const Track& rhs) {
        return std::tie(lhs.order_index, lhs.id) < std::tie(rhs.order_index, rhs.id);
    });
    std::set<TrackId> track_ids;
    for (std::size_t index = 0; index < snapshot.tracks.size(); ++index) {
        const auto& track = snapshot.tracks[index];
        if (validate_id(track.id, stage)) {
            return Result<TimelineSnapshot>::failure(
                validation_error(ErrorCode::invalid_identifier, stage));
        }
        if (!track_ids.insert(track.id).second) {
            return Result<TimelineSnapshot>::failure(
                validation_error(ErrorCode::duplicate_track_id, stage));
        }
        if (track.order_index != index) {
            return Result<TimelineSnapshot>::failure(
                validation_error(ErrorCode::invalid_track, stage));
        }
        if (const auto error = validate_extensions(track.extensions, stage)) {
            return Result<TimelineSnapshot>::failure(*error);
        }
    }
    std::set<EventId> event_ids;
    for (const auto& event : snapshot.events) {
        if (!event_ids.insert(event.id).second) {
            return Result<TimelineSnapshot>::failure(
                validation_error(ErrorCode::duplicate_event_id, stage));
        }
        if (const auto error = validate_event(event, track_ids, stage)) {
            return Result<TimelineSnapshot>::failure(*error);
        }
    }
    std::map<TrackId, std::uint32_t> track_order;
    for (const auto& track : snapshot.tracks) {
        track_order.emplace(track.id, track.order_index);
    }
    std::ranges::sort(snapshot.events, [&](const RhythmEvent& lhs, const RhythmEvent& rhs) {
        return std::tuple{lhs.time_ns, track_order.at(lhs.track_id), lhs.id.value}
            < std::tuple{rhs.time_ns, track_order.at(rhs.track_id), rhs.id.value};
    });
    if (const auto error = validate_extensions(snapshot.extensions, stage)) {
        return Result<TimelineSnapshot>::failure(*error);
    }
    return Result<TimelineSnapshot>::success(std::move(snapshot));
}

class Timeline::Impl final {
public:
    struct HistoryEntry {
        TransactionId transaction_id;
        TransactionOrigin origin{TransactionOrigin::user};
        std::shared_ptr<const TimelineSnapshot> before;
        std::shared_ptr<const TimelineSnapshot> after;
        std::optional<std::string> coalescing_key;
        std::set<std::string> entities;
        std::uint64_t savepoint_epoch{};
    };

    struct ReplayEntry {
        std::string fingerprint;
        TransactionResult result;
    };

    explicit Impl(TimelineSnapshot initial)
        : current(std::make_shared<const TimelineSnapshot>(std::move(initial))),
          saved(current)
    {
    }

    mutable std::mutex mutex;
    std::shared_ptr<const TimelineSnapshot> current;
    std::shared_ptr<const TimelineSnapshot> saved;
    std::vector<HistoryEntry> undo;
    std::vector<HistoryEntry> redo;
    std::unordered_map<std::string, ReplayEntry> replay;
    std::unordered_map<std::string, std::string> analysis_revisions;
    std::uint64_t savepoint_epoch{};
};

Timeline::Timeline(ProjectId project_id)
{
    TimelineSnapshot initial;
    initial.project_id = std::move(project_id);
    initial.tracks.push_back(Track{TrackId{"track-0"}, 0, std::nullopt, {}});
    auto validated = validate_and_normalize_snapshot(std::move(initial));
    if (!validated) {
        throw std::invalid_argument("invalid initial timeline");
    }
    impl_ = std::make_unique<Impl>(std::move(validated.value()));
}

Timeline::Timeline(TimelineSnapshot loaded_snapshot)
{
    auto validated = validate_and_normalize_snapshot(std::move(loaded_snapshot));
    if (!validated) {
        throw std::invalid_argument("invalid loaded timeline");
    }
    impl_ = std::make_unique<Impl>(std::move(validated.value()));
}

Timeline::~Timeline() = default;

std::shared_ptr<const TimelineSnapshot> Timeline::snapshot() const
{
    std::scoped_lock lock{impl_->mutex};
    return impl_->current;
}

TransactionResult Timeline::submit(const TransactionRequest& request,
                                   const CancellationToken* cancellation)
{
    std::scoped_lock lock{impl_->mutex};
    const auto before_revision = impl_->current->timeline_revision;
    if (validate_id(request.transaction_id, "transaction.validate")) {
        return rejected(request.transaction_id,
                        before_revision,
                        validation_error(ErrorCode::invalid_identifier,
                                         "transaction.validate"));
    }
    const auto fingerprint = canonical_request(request);
    if (const auto found = impl_->replay.find(request.transaction_id.value);
        found != impl_->replay.end()) {
        if (found->second.fingerprint == fingerprint) {
            return found->second.result;
        }
        return rejected(request.transaction_id,
                        before_revision,
                        conflict_error(ErrorCode::idempotency_conflict,
                                       "transaction.idempotency"));
    }

    const auto remember = [&](TransactionResult result) {
        impl_->replay.emplace(request.transaction_id.value,
                              Impl::ReplayEntry{fingerprint, result});
        return result;
    };
    if (request.operations.empty()) {
        return remember(rejected(request.transaction_id,
                                 before_revision,
                                 validation_error(ErrorCode::invalid_transaction,
                                                  "transaction.validate")));
    }
    if (request.operations.size() > max_transaction_operations) {
        return remember(rejected(
            request.transaction_id,
            before_revision,
            make_error(ErrorCategory::resource_limit,
                       ErrorCode::resource_limit,
                       "transaction.validate")));
    }
    if (request.coalescing_key && !validate_identifier(*request.coalescing_key)) {
        return remember(rejected(request.transaction_id,
                                 before_revision,
                                 validation_error(ErrorCode::invalid_identifier,
                                                  "transaction.validate")));
    }
    if (request.base_timeline_revision != before_revision) {
        return remember(rejected(request.transaction_id,
                                 before_revision,
                                 conflict_error(ErrorCode::stale_revision,
                                                "transaction.revision",
                                                true)));
    }
    if (request_cancelled(cancellation)) {
        return remember(cancelled(request.transaction_id, before_revision));
    }

    TimelineSnapshot working = *impl_->current;
    ApplyContext context{request.origin, {}, {}};
    for (const auto& operation : request.operations) {
        if (const auto error =
                apply_operation(working, operation, context, impl_->analysis_revisions)) {
            return remember(rejected(request.transaction_id, before_revision, *error));
        }
    }
    auto validated = validate_and_normalize_snapshot(std::move(working));
    if (!validated) {
        return remember(rejected(request.transaction_id,
                                 before_revision,
                                 validated.error()));
    }
    if (request_cancelled(cancellation)) {
        return remember(cancelled(request.transaction_id, before_revision));
    }
    if (semantic_equal(*impl_->current, validated.value())) {
        for (const auto& [revision, signature] : context.pending_analysis_revisions) {
            impl_->analysis_revisions.emplace(revision, signature);
        }
        auto result = no_change(request.transaction_id, before_revision);
        result.suppression_report = std::move(context.suppression);
        return remember(std::move(result));
    }
    if (before_revision == std::numeric_limits<TimelineRevision>::max()) {
        return remember(rejected(request.transaction_id,
                                 before_revision,
                                 conflict_error(ErrorCode::revision_exhausted,
                                                "transaction.commit")));
    }

    validated.value().timeline_revision = before_revision + 1;
    const auto next =
        std::make_shared<const TimelineSnapshot>(std::move(validated.value()));
    Impl::HistoryEntry entry{request.transaction_id,
                             request.origin,
                             impl_->current,
                             next,
                             request.coalescing_key,
                             affected_entities(request),
                             impl_->savepoint_epoch};
    const bool coalesce = request.origin == TransactionOrigin::user
        && request.coalescing_key && !impl_->undo.empty()
        && impl_->undo.back().origin == TransactionOrigin::user
        && impl_->undo.back().coalescing_key == request.coalescing_key
        && impl_->undo.back().entities == entry.entities
        && impl_->undo.back().savepoint_epoch == impl_->savepoint_epoch;
    if (coalesce) {
        impl_->undo.back().after = next;
    } else {
        if (impl_->undo.size() == max_history_entries) {
            impl_->undo.erase(impl_->undo.begin());
        }
        impl_->undo.push_back(std::move(entry));
    }
    impl_->redo.clear();
    const ChangeSummary summary{impl_->current->tracks.size(),
                                next->tracks.size(),
                                impl_->current->events.size(),
                                next->events.size()};
    impl_->current = next;
    for (const auto& [revision, signature] : context.pending_analysis_revisions) {
        impl_->analysis_revisions.emplace(revision, signature);
    }
    return remember(TransactionResult{request.transaction_id,
                                      TransactionStatus::committed,
                                      before_revision,
                                      next->timeline_revision,
                                      std::nullopt,
                                      summary,
                                      std::move(context.suppression)});
}

TransactionResult Timeline::undo(TimelineRevision expected_current_revision)
{
    std::scoped_lock lock{impl_->mutex};
    const auto revision = impl_->current->timeline_revision;
    const TransactionId transaction_id{"undo:" + std::to_string(revision)};
    if (expected_current_revision != revision) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::stale_revision, "history.undo", true));
    }
    if (impl_->undo.empty()) {
        return no_change(transaction_id, revision);
    }
    if (revision == std::numeric_limits<TimelineRevision>::max()) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::revision_exhausted, "history.undo"));
    }
    auto entry = impl_->undo.back();
    if (!semantic_equal(*impl_->current, *entry.after)) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::history_conflict, "history.undo"));
    }
    auto restored = *entry.before;
    restored.timeline_revision = revision + 1;
    const auto next = std::make_shared<const TimelineSnapshot>(std::move(restored));
    impl_->undo.pop_back();
    impl_->redo.push_back(entry);
    const ChangeSummary summary{impl_->current->tracks.size(),
                                next->tracks.size(),
                                impl_->current->events.size(),
                                next->events.size()};
    impl_->current = next;
    return TransactionResult{transaction_id,
                             TransactionStatus::committed,
                             revision,
                             next->timeline_revision,
                             std::nullopt,
                             summary,
                             {}};
}

TransactionResult Timeline::redo(TimelineRevision expected_current_revision)
{
    std::scoped_lock lock{impl_->mutex};
    const auto revision = impl_->current->timeline_revision;
    const TransactionId transaction_id{"redo:" + std::to_string(revision)};
    if (expected_current_revision != revision) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::stale_revision, "history.redo", true));
    }
    if (impl_->redo.empty()) {
        return no_change(transaction_id, revision);
    }
    if (revision == std::numeric_limits<TimelineRevision>::max()) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::revision_exhausted, "history.redo"));
    }
    auto entry = impl_->redo.back();
    if (!semantic_equal(*impl_->current, *entry.before)) {
        return rejected(transaction_id,
                        revision,
                        conflict_error(ErrorCode::history_conflict, "history.redo"));
    }
    auto restored = *entry.after;
    restored.timeline_revision = revision + 1;
    const auto next = std::make_shared<const TimelineSnapshot>(std::move(restored));
    impl_->redo.pop_back();
    impl_->undo.push_back(entry);
    const ChangeSummary summary{impl_->current->tracks.size(),
                                next->tracks.size(),
                                impl_->current->events.size(),
                                next->events.size()};
    impl_->current = next;
    return TransactionResult{transaction_id,
                             TransactionStatus::committed,
                             revision,
                             next->timeline_revision,
                             std::nullopt,
                             summary,
                             {}};
}

void Timeline::mark_saved()
{
    std::scoped_lock lock{impl_->mutex};
    impl_->saved = impl_->current;
    ++impl_->savepoint_epoch;
}

bool Timeline::is_dirty() const
{
    std::scoped_lock lock{impl_->mutex};
    return !semantic_equal(*impl_->current, *impl_->saved);
}

std::size_t Timeline::undo_depth() const
{
    std::scoped_lock lock{impl_->mutex};
    return impl_->undo.size();
}

std::size_t Timeline::redo_depth() const
{
    std::scoped_lock lock{impl_->mutex};
    return impl_->redo.size();
}

} // namespace space_rhythm::core
