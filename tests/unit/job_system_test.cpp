#include <gtest/gtest.h>

#include <space_rhythm/system/runtime.hpp>

#include <limits>
#include <string>

namespace {

using namespace space_rhythm;

system::DataReference valid_reference()
{
    return system::DataReference{system::DataReferenceKind::cache,
                                 "cache/result.bin",
                                 32,
                                 std::string(64, 'a')};
}

system::JobRequest valid_request(std::string request_id = "request-1",
                                 std::string job_id = "job-1",
                                 core::TimelineRevision revision = 7)
{
    return system::JobRequest{std::move(request_id),
                              std::move(job_id),
                              "analyze",
                              revision,
                              "input-fingerprint",
                              "parameters-digest",
                              100,
                              {},
                              {valid_reference()}};
}

system::JobUpdate update(const system::JobRequest& request,
                         std::uint64_t sequence,
                         system::JobUpdateType type)
{
    return system::JobUpdate{request.request_id,
                             request.job_id,
                             sequence,
                             type,
                             0,
                             std::nullopt,
                             std::nullopt,
                             std::nullopt};
}

TEST(JobCoordinator, SupportsEveryStateAndIdempotentCancellation)
{
    system::JobCoordinator coordinator;
    const auto request = valid_request();
    const auto submitted = coordinator.submit(request, 1'000);
    ASSERT_TRUE(submitted);
    EXPECT_EQ(submitted.value().job.status, system::JobStatus::queued);

    const auto accepted = coordinator.apply_update(
        update(request, 1, system::JobUpdateType::accepted), 7);
    ASSERT_TRUE(accepted);
    EXPECT_EQ(accepted.value().job.status, system::JobStatus::running);

    auto progress = update(request, 2, system::JobUpdateType::progress);
    progress.progress_ppm = 250'000;
    const auto progressed = coordinator.apply_update(progress, 7);
    ASSERT_TRUE(progressed);
    EXPECT_EQ(progressed.value().job.progress_ppm, 250'000U);

    const auto cancelling = coordinator.cancel(request.request_id);
    ASSERT_TRUE(cancelling);
    EXPECT_EQ(cancelling.value().job.status, system::JobStatus::cancelling);
    const auto repeated_cancel = coordinator.cancel(request.request_id);
    ASSERT_TRUE(repeated_cancel);
    EXPECT_TRUE(repeated_cancel.value().replayed);

    const auto acknowledged = coordinator.apply_update(
        update(request, 3, system::JobUpdateType::cancellation_acknowledged), 7);
    ASSERT_TRUE(acknowledged);
    EXPECT_EQ(acknowledged.value().job.status, system::JobStatus::cancelling);

    const auto cancelled = coordinator.apply_update(
        update(request, 4, system::JobUpdateType::cancelled), 7);
    ASSERT_TRUE(cancelled);
    EXPECT_EQ(cancelled.value().job.status, system::JobStatus::cancelled);
    ASSERT_TRUE(cancelled.value().job.error);
    EXPECT_EQ(cancelled.value().job.error->category, core::ErrorCategory::cancelled);
    EXPECT_FALSE(cancelled.value().job.error->diagnostic_id.empty());

    const auto queued_request = valid_request("request-queued", "job-queued");
    ASSERT_TRUE(coordinator.submit(queued_request, 1'000));
    const auto cancelled_queued = coordinator.cancel(queued_request.request_id);
    ASSERT_TRUE(cancelled_queued);
    EXPECT_EQ(cancelled_queued.value().job.status, system::JobStatus::cancelled);
}

TEST(JobCoordinator, SupportsSuccessFailureAndStaleRevisionProtection)
{
    system::JobCoordinator coordinator;
    const auto success_request = valid_request("success-request", "success-job", 10);
    ASSERT_TRUE(coordinator.submit(success_request, 0));
    ASSERT_TRUE(coordinator.apply_update(
        update(success_request, 1, system::JobUpdateType::accepted), 10));
    auto success = update(success_request, 2, system::JobUpdateType::succeeded);
    success.result_base_revision = 10;
    success.result = valid_reference();
    const auto succeeded = coordinator.apply_update(success, 10);
    ASSERT_TRUE(succeeded);
    EXPECT_EQ(succeeded.value().job.status, system::JobStatus::succeeded);
    EXPECT_EQ(succeeded.value().job.progress_ppm, core::norm_ppm_max);

    const auto failure_request = valid_request("failure-request", "failure-job", 10);
    ASSERT_TRUE(coordinator.submit(failure_request, 0));
    ASSERT_TRUE(coordinator.apply_update(
        update(failure_request, 1, system::JobUpdateType::accepted), 10));
    auto failure = update(failure_request, 2, system::JobUpdateType::failed);
    failure.error = system::SystemError{core::ErrorCategory::internal,
                                        system::SystemErrorCode::worker_crashed,
                                        "worker.run",
                                        "worker:42",
                                        true,
                                        "worker_crashed",
                                        {}};
    const auto failed = coordinator.apply_update(failure, 10);
    ASSERT_TRUE(failed);
    EXPECT_EQ(failed.value().job.status, system::JobStatus::failed);
    auto conflicting_failure = failure;
    conflicting_failure.error->message_key = "different_worker_failure";
    const auto rejected_duplicate = coordinator.apply_update(conflicting_failure, 10);
    ASSERT_FALSE(rejected_duplicate);
    EXPECT_EQ(rejected_duplicate.error().code,
              system::SystemErrorCode::message_conflict);

    const auto stale_request = valid_request("stale-request", "stale-job", 10);
    ASSERT_TRUE(coordinator.submit(stale_request, 0));
    ASSERT_TRUE(coordinator.apply_update(
        update(stale_request, 1, system::JobUpdateType::accepted), 10));
    auto stale = update(stale_request, 2, system::JobUpdateType::succeeded);
    stale.result_base_revision = 10;
    stale.result = valid_reference();
    const auto rejected = coordinator.apply_update(stale, 11);
    ASSERT_TRUE(rejected);
    EXPECT_EQ(rejected.value().job.status, system::JobStatus::failed);
    ASSERT_TRUE(rejected.value().job.error);
    EXPECT_EQ(rejected.value().job.error->code,
              system::SystemErrorCode::stale_revision);
    EXPECT_FALSE(rejected.value().job.result.has_value());
}

TEST(JobCoordinator, RejectsConflictingDuplicateAndOutOfOrderMessages)
{
    system::JobCoordinator coordinator;
    const auto request = valid_request();
    ASSERT_TRUE(coordinator.submit(request, 0));

    const auto replayed_submit = coordinator.submit(request, 99);
    ASSERT_TRUE(replayed_submit);
    EXPECT_TRUE(replayed_submit.value().replayed);

    auto conflicting_request = request;
    conflicting_request.parameters_digest = "other-parameters";
    const auto conflict = coordinator.submit(conflicting_request, 0);
    ASSERT_FALSE(conflict);
    EXPECT_EQ(conflict.error().code, system::SystemErrorCode::request_conflict);
    EXPECT_FALSE(conflict.error().diagnostic_id.empty());

    const auto accepted_message =
        update(request, 1, system::JobUpdateType::accepted);
    ASSERT_TRUE(coordinator.apply_update(accepted_message, 7));
    const auto duplicate = coordinator.apply_update(accepted_message, 7);
    ASSERT_TRUE(duplicate);
    EXPECT_TRUE(duplicate.value().replayed);

    auto conflicting_message = accepted_message;
    conflicting_message.type = system::JobUpdateType::progress;
    const auto message_conflict = coordinator.apply_update(conflicting_message, 7);
    ASSERT_FALSE(message_conflict);
    EXPECT_EQ(message_conflict.error().code,
              system::SystemErrorCode::message_conflict);

    auto sequence_gap = update(request, 3, system::JobUpdateType::progress);
    sequence_gap.progress_ppm = 1;
    const auto gap = coordinator.apply_update(sequence_gap, 7);
    ASSERT_FALSE(gap);
    EXPECT_EQ(gap.error().code, system::SystemErrorCode::out_of_order_message);

    auto progress = update(request, 2, system::JobUpdateType::progress);
    progress.progress_ppm = 10;
    ASSERT_TRUE(coordinator.apply_update(progress, 7));
    auto regressed = update(request, 3, system::JobUpdateType::progress);
    regressed.progress_ppm = 9;
    const auto regression = coordinator.apply_update(regressed, 7);
    ASSERT_FALSE(regression);
    EXPECT_EQ(regression.error().code,
              system::SystemErrorCode::out_of_order_message);
}

TEST(JobCoordinator, ConvertsTimeoutAndWorkerDisconnectToTerminalFailures)
{
    system::JobCoordinator coordinator;
    ASSERT_TRUE(coordinator.submit(valid_request("timeout", "timeout-job"), 1'000));
    ASSERT_TRUE(coordinator.submit(valid_request("crash", "crash-job"), 1'000));

    const auto before_deadline = coordinator.expire(1'099);
    EXPECT_TRUE(before_deadline.empty());
    const auto expired = coordinator.expire(1'100);
    ASSERT_EQ(expired.size(), 2U);
    for (const auto& job : expired) {
        EXPECT_EQ(job.status, system::JobStatus::failed);
        ASSERT_TRUE(job.error);
        EXPECT_EQ(job.error->code, system::SystemErrorCode::request_timeout);
        EXPECT_TRUE(job.error->retryable);
        EXPECT_FALSE(job.error->diagnostic_id.empty());
    }

    system::JobCoordinator disconnected;
    const auto active = valid_request("active", "active-job");
    ASSERT_TRUE(disconnected.submit(active, 0));
    ASSERT_TRUE(disconnected.apply_update(
        update(active, 1, system::JobUpdateType::accepted), 7));
    const auto crashed = disconnected.worker_disconnected();
    ASSERT_EQ(crashed.size(), 1U);
    ASSERT_TRUE(crashed.front().error);
    EXPECT_EQ(crashed.front().error->code,
              system::SystemErrorCode::worker_crashed);
}

} // namespace
