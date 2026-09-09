#include <space_rhythm/system/runtime.hpp>

#include <algorithm>
#include <atomic>
#include <iomanip>
#include <limits>
#include <mutex>
#include <ranges>
#include <set>
#include <sstream>
#include <unordered_map>
#include <utility>

namespace space_rhythm::system {
namespace {

std::atomic_uint64_t next_diagnostic{1};

SystemError make_error(core::ErrorCategory category,
                       SystemErrorCode code,
                       std::string_view stage,
                       bool retryable = false)
{
    const auto sequence = next_diagnostic.fetch_add(1, std::memory_order_relaxed);
    return SystemError{category,
                       code,
                       std::string{stage},
                       "system:" + std::to_string(sequence),
                       retryable,
                       std::string{to_string(code)},
                       {}};
}

bool valid_token(std::string_view value)
{
    if (value.empty() || value.size() > 128) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z')
            || (character >= '0' && character <= '9') || character == '_'
            || character == '-' || character == '.' || character == ':';
    });
}

bool valid_digest(std::string_view value)
{
    return !value.empty() && value.size() <= 256
        && std::ranges::all_of(value, [](const unsigned char character) {
               return character >= 0x20U && character <= 0x7eU;
           });
}

bool valid_reference(const DataReference& reference)
{
    return !reference.locator.empty() && reference.locator.size() <= 2'048
        && reference.sha256.size() == 64
        && std::ranges::all_of(reference.sha256, [](const unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f')
                   || (character >= 'A' && character <= 'F');
           });
}

std::string request_fingerprint(const JobRequest& request)
{
    std::ostringstream stream;
    stream << std::quoted(request.request_id) << std::quoted(request.job_id)
           << std::quoted(request.operation) << request.base_timeline_revision
           << std::quoted(request.input_fingerprint)
           << std::quoted(request.parameters_digest) << request.timeout_ms;
    for (const auto& [key, value] : request.options) {
        stream << std::quoted(key) << std::quoted(value);
    }
    for (const auto& reference : request.inputs) {
        stream << static_cast<int>(reference.kind) << std::quoted(reference.locator)
               << reference.byte_length << std::quoted(reference.sha256);
    }
    return stream.str();
}

std::string update_fingerprint(const JobUpdate& update)
{
    std::ostringstream stream;
    stream << std::quoted(update.request_id) << std::quoted(update.job_id)
           << update.sequence << static_cast<int>(update.type) << update.progress_ppm
           << update.result_base_revision.has_value();
    if (update.result_base_revision) {
        stream << *update.result_base_revision;
    }
    stream << update.result.has_value();
    if (update.result) {
        stream << static_cast<int>(update.result->kind)
               << std::quoted(update.result->locator) << update.result->byte_length
               << std::quoted(update.result->sha256);
    }
    stream << update.error.has_value();
    if (update.error) {
        stream << static_cast<int>(update.error->category)
               << static_cast<int>(update.error->code)
               << std::quoted(update.error->diagnostic_id)
               << std::quoted(update.error->stage) << update.error->retryable
               << std::quoted(update.error->message_key);
        for (const auto& [key, value] : update.error->context) {
            stream << std::quoted(key) << std::quoted(value);
        }
    }
    return stream.str();
}

bool terminal(JobStatus status) noexcept
{
    return status == JobStatus::succeeded || status == JobStatus::failed
        || status == JobStatus::cancelled;
}

std::uint64_t deadline(std::uint64_t now_ms, std::uint64_t timeout_ms) noexcept
{
    if (timeout_ms > std::numeric_limits<std::uint64_t>::max() - now_ms) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return now_ms + timeout_ms;
}

} // namespace

std::string_view to_string(SystemErrorCode code) noexcept
{
    switch (code) {
    case SystemErrorCode::invalid_request:
        return "invalid_request";
    case SystemErrorCode::invalid_transition:
        return "invalid_transition";
    case SystemErrorCode::request_conflict:
        return "request_conflict";
    case SystemErrorCode::message_conflict:
        return "message_conflict";
    case SystemErrorCode::out_of_order_message:
        return "out_of_order_message";
    case SystemErrorCode::protocol_version_mismatch:
        return "protocol_version_mismatch";
    case SystemErrorCode::message_too_large:
        return "message_too_large";
    case SystemErrorCode::forbidden_inline_data:
        return "forbidden_inline_data";
    case SystemErrorCode::request_timeout:
        return "request_timeout";
    case SystemErrorCode::worker_crashed:
        return "worker_crashed";
    case SystemErrorCode::stale_revision:
        return "stale_revision";
    case SystemErrorCode::cancelled:
        return "cancelled";
    case SystemErrorCode::unsupported_project_schema:
        return "unsupported_project_schema";
    case SystemErrorCode::invalid_project:
        return "invalid_project";
    case SystemErrorCode::corrupt_project:
        return "corrupt_project";
    case SystemErrorCode::disk_full:
        return "disk_full";
    case SystemErrorCode::partial_write:
        return "partial_write";
    case SystemErrorCode::io_error:
        return "io_error";
    case SystemErrorCode::asset_not_found:
        return "asset_not_found";
    case SystemErrorCode::fingerprint_mismatch:
        return "fingerprint_mismatch";
    case SystemErrorCode::cache_corrupt:
        return "cache_corrupt";
    case SystemErrorCode::cache_miss:
        return "cache_miss";
    }
    return "io_error";
}

class JobCoordinator::Impl final {
public:
    struct Runtime {
        JobSnapshot snapshot;
        std::string request_fingerprint;
        std::map<std::uint64_t, std::string> updates;
    };

    mutable std::mutex mutex;
    std::unordered_map<std::string, Runtime> requests;
    std::unordered_map<std::string, std::string> jobs;
};

JobCoordinator::JobCoordinator()
    : impl_(std::make_unique<Impl>())
{
}

JobCoordinator::~JobCoordinator() = default;

Result<JobAction> JobCoordinator::submit(const JobRequest& request, std::uint64_t now_ms)
{
    std::scoped_lock lock{impl_->mutex};
    if (!valid_token(request.request_id) || !valid_token(request.job_id)
        || !valid_token(request.operation) || !valid_digest(request.input_fingerprint)
        || !valid_digest(request.parameters_digest) || request.timeout_ms == 0
        || request.options.size() > 128 || request.inputs.size() > 128) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::validation,
                                                     SystemErrorCode::invalid_request,
                                                     "job.submit"));
    }
    for (const auto& [key, value] : request.options) {
        if (!valid_token(key) || value.size() > 4'096) {
            return Result<JobAction>::failure(make_error(
                core::ErrorCategory::validation,
                SystemErrorCode::invalid_request,
                "job.submit"));
        }
    }
    if (!std::ranges::all_of(request.inputs, valid_reference)) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::validation,
                                                     SystemErrorCode::invalid_request,
                                                     "job.submit"));
    }
    const auto fingerprint = request_fingerprint(request);
    if (const auto existing = impl_->requests.find(request.request_id);
        existing != impl_->requests.end()) {
        if (existing->second.request_fingerprint == fingerprint) {
            return Result<JobAction>::success(JobAction{existing->second.snapshot, true});
        }
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::request_conflict,
                                                     "job.idempotency"));
    }
    if (impl_->jobs.contains(request.job_id)) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::request_conflict,
                                                     "job.idempotency"));
    }
    JobSnapshot snapshot{request,
                         JobStatus::queued,
                         0,
                         0,
                         deadline(now_ms, request.timeout_ms),
                         std::nullopt,
                         std::nullopt};
    impl_->jobs.emplace(request.job_id, request.request_id);
    impl_->requests.emplace(
        request.request_id,
        Impl::Runtime{snapshot, std::move(fingerprint), {}});
    return Result<JobAction>::success(JobAction{std::move(snapshot), false});
}

Result<JobAction> JobCoordinator::apply_update(
    const JobUpdate& update,
    core::TimelineRevision current_timeline_revision)
{
    std::scoped_lock lock{impl_->mutex};
    const auto found = impl_->requests.find(update.request_id);
    if (found == impl_->requests.end() || found->second.snapshot.request.job_id != update.job_id
        || update.sequence == 0 || update.progress_ppm > core::norm_ppm_max) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::validation,
                                                     SystemErrorCode::invalid_request,
                                                     "job.update"));
    }
    auto& runtime = found->second;
    const auto fingerprint = update_fingerprint(update);
    if (const auto replay = runtime.updates.find(update.sequence);
        replay != runtime.updates.end()) {
        if (replay->second == fingerprint) {
            return Result<JobAction>::success(JobAction{runtime.snapshot, true});
        }
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::message_conflict,
                                                     "job.update"));
    }
    if (update.sequence != runtime.snapshot.last_sequence + 1) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::out_of_order_message,
                                                     "job.sequence",
                                                     true));
    }
    if (terminal(runtime.snapshot.status)) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::invalid_transition,
                                                     "job.transition"));
    }

    auto next = runtime.snapshot;
    switch (update.type) {
    case JobUpdateType::accepted:
        if (next.status != JobStatus::queued) {
            return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                         SystemErrorCode::invalid_transition,
                                                         "job.transition"));
        }
        next.status = JobStatus::running;
        break;
    case JobUpdateType::progress:
        if ((next.status != JobStatus::running && next.status != JobStatus::cancelling)
            || update.progress_ppm < next.progress_ppm) {
            return Result<JobAction>::failure(make_error(
                core::ErrorCategory::conflict,
                SystemErrorCode::out_of_order_message,
                "job.progress"));
        }
        next.progress_ppm = update.progress_ppm;
        break;
    case JobUpdateType::cancellation_acknowledged:
        if (next.status != JobStatus::cancelling) {
            return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                         SystemErrorCode::invalid_transition,
                                                         "job.transition"));
        }
        break;
    case JobUpdateType::succeeded:
        if (next.status != JobStatus::running || !update.result_base_revision
            || !update.result || !valid_reference(*update.result)) {
            return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                         SystemErrorCode::invalid_transition,
                                                         "job.transition"));
        }
        if (*update.result_base_revision != next.request.base_timeline_revision
            || current_timeline_revision != next.request.base_timeline_revision) {
            next.status = JobStatus::failed;
            next.error = make_error(core::ErrorCategory::conflict,
                                    SystemErrorCode::stale_revision,
                                    "job.commit",
                                    true);
            break;
        }
        next.status = JobStatus::succeeded;
        next.progress_ppm = core::norm_ppm_max;
        next.result = update.result;
        break;
    case JobUpdateType::failed:
        if ((next.status != JobStatus::running && next.status != JobStatus::cancelling)
            || !update.error) {
            return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                         SystemErrorCode::invalid_transition,
                                                         "job.transition"));
        }
        next.status = JobStatus::failed;
        next.error = update.error;
        break;
    case JobUpdateType::cancelled:
        if (next.status != JobStatus::cancelling && next.status != JobStatus::queued) {
            return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                         SystemErrorCode::invalid_transition,
                                                         "job.transition"));
        }
        next.status = JobStatus::cancelled;
        next.error = update.error.value_or(make_error(core::ErrorCategory::cancelled,
                                                      SystemErrorCode::cancelled,
                                                      "job.cancel"));
        break;
    }
    next.last_sequence = update.sequence;
    runtime.updates.emplace(update.sequence, std::move(fingerprint));
    runtime.snapshot = next;
    return Result<JobAction>::success(JobAction{std::move(next), false});
}

Result<JobAction> JobCoordinator::cancel(std::string_view request_id)
{
    std::scoped_lock lock{impl_->mutex};
    const auto found = impl_->requests.find(std::string{request_id});
    if (found == impl_->requests.end()) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::validation,
                                                     SystemErrorCode::invalid_request,
                                                     "job.cancel"));
    }
    auto& snapshot = found->second.snapshot;
    if (snapshot.status == JobStatus::cancelling
        || snapshot.status == JobStatus::cancelled) {
        return Result<JobAction>::success(JobAction{snapshot, true});
    }
    if (terminal(snapshot.status)) {
        return Result<JobAction>::failure(make_error(core::ErrorCategory::conflict,
                                                     SystemErrorCode::invalid_transition,
                                                     "job.cancel"));
    }
    if (snapshot.status == JobStatus::queued) {
        snapshot.status = JobStatus::cancelled;
        snapshot.error = make_error(core::ErrorCategory::cancelled,
                                    SystemErrorCode::cancelled,
                                    "job.cancel");
    } else {
        snapshot.status = JobStatus::cancelling;
    }
    return Result<JobAction>::success(JobAction{snapshot, false});
}

std::vector<JobSnapshot> JobCoordinator::expire(std::uint64_t now_ms)
{
    std::scoped_lock lock{impl_->mutex};
    std::vector<JobSnapshot> expired;
    for (auto& [request_id, runtime] : impl_->requests) {
        static_cast<void>(request_id);
        if (!terminal(runtime.snapshot.status) && now_ms >= runtime.snapshot.deadline_ms) {
            runtime.snapshot.status = JobStatus::failed;
            runtime.snapshot.error = make_error(core::ErrorCategory::resource_limit,
                                                SystemErrorCode::request_timeout,
                                                "job.timeout",
                                                true);
            expired.push_back(runtime.snapshot);
        }
    }
    return expired;
}

std::vector<JobSnapshot> JobCoordinator::worker_disconnected()
{
    std::scoped_lock lock{impl_->mutex};
    std::vector<JobSnapshot> failed;
    for (auto& [request_id, runtime] : impl_->requests) {
        static_cast<void>(request_id);
        if (!terminal(runtime.snapshot.status)) {
            runtime.snapshot.status = JobStatus::failed;
            runtime.snapshot.error = make_error(core::ErrorCategory::internal,
                                                SystemErrorCode::worker_crashed,
                                                "worker.disconnect",
                                                true);
            failed.push_back(runtime.snapshot);
        }
    }
    return failed;
}

Result<JobSnapshot> JobCoordinator::snapshot(std::string_view request_id) const
{
    std::scoped_lock lock{impl_->mutex};
    const auto found = impl_->requests.find(std::string{request_id});
    if (found == impl_->requests.end()) {
        return Result<JobSnapshot>::failure(make_error(core::ErrorCategory::validation,
                                                       SystemErrorCode::invalid_request,
                                                       "job.snapshot"));
    }
    return Result<JobSnapshot>::success(found->second.snapshot);
}

} // namespace space_rhythm::system
