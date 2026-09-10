#include <gtest/gtest.h>

#include <space_rhythm/system/runtime.hpp>

#include <QFile>
#include <QTemporaryDir>

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace {

namespace core = space_rhythm::core;
namespace system = space_rhythm::system;

std::filesystem::path path(const QString& value)
{
    return std::filesystem::path{value.toStdWString()};
}

system::JobRequest request()
{
    return {"request-contract",
            "job-contract",
            "analyze",
            41,
            "asset-fingerprint",
            "parameters-digest",
            50,
            {},
            {}};
}

system::JobUpdate update(std::uint64_t sequence, system::JobUpdateType type)
{
    return {"request-contract",
            "job-contract",
            sequence,
            type,
            0,
            std::nullopt,
            std::nullopt,
            std::nullopt};
}

system::ProjectDocument document(core::TimelineRevision revision)
{
    core::TimelineSnapshot timeline;
    timeline.project_id = core::ProjectId{"project-contract"};
    timeline.timeline_revision = revision;
    return {system::project_schema_version,
            "0.1.0",
            std::move(timeline),
            {},
            {},
            {}};
}

void overwrite(const std::filesystem::path& target, const QByteArray& bytes)
{
    QFile file{QString::fromStdWString(target.wstring())};
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(bytes), bytes.size());
}

TEST(T021IpcContract, JobStateMachineRejectsSequenceGapsAndPreservesTerminalState)
{
    system::JobCoordinator coordinator;
    const auto submitted = coordinator.submit(request(), 1'000);
    ASSERT_TRUE(submitted);
    EXPECT_EQ(submitted.value().job.status, system::JobStatus::queued);
    EXPECT_EQ(submitted.value().job.deadline_ms, 1'050U);

    const auto accepted = coordinator.apply_update(
        update(1, system::JobUpdateType::accepted), 41);
    ASSERT_TRUE(accepted);
    EXPECT_EQ(accepted.value().job.status, system::JobStatus::running);

    auto out_of_order = update(3, system::JobUpdateType::progress);
    out_of_order.progress_ppm = 500'000;
    const auto rejected = coordinator.apply_update(out_of_order, 41);
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code, system::SystemErrorCode::out_of_order_message);

    auto progress = update(2, system::JobUpdateType::progress);
    progress.progress_ppm = 500'000;
    const auto progressed = coordinator.apply_update(progress, 41);
    ASSERT_TRUE(progressed);
    EXPECT_EQ(progressed.value().job.progress_ppm, 500'000U);

    const auto cancelling = coordinator.cancel("request-contract");
    ASSERT_TRUE(cancelling);
    EXPECT_EQ(cancelling.value().job.status, system::JobStatus::cancelling);

    const auto cancelled = coordinator.apply_update(
        update(3, system::JobUpdateType::cancelled), 41);
    ASSERT_TRUE(cancelled);
    EXPECT_EQ(cancelled.value().job.status, system::JobStatus::cancelled);
    ASSERT_TRUE(cancelled.value().job.error);
    EXPECT_EQ(cancelled.value().job.error->code, system::SystemErrorCode::cancelled);

    const auto replay = coordinator.apply_update(
        update(3, system::JobUpdateType::cancelled), 41);
    ASSERT_TRUE(replay);
    EXPECT_TRUE(replay.value().replayed);
    EXPECT_EQ(replay.value().job.status, system::JobStatus::cancelled);
}

TEST(T021IpcContract, ProtocolFrameRoundTripAndVersionFailureArePubliclyStable)
{
    system::ProtocolEnvelope envelope;
    envelope.message_id = "message-contract";
    envelope.request_id = "request-contract";
    envelope.job_id = "job-contract";
    envelope.sequence = 7;
    envelope.type = system::MessageType::job_update;
    envelope.payload = {{"state", "progress"}, {"progressPpm", "500000"}};

    const auto encoded = system::encode_protocol_frame(envelope);
    ASSERT_TRUE(encoded);
    const auto decoded = system::decode_protocol_frame(encoded.value());
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded.value(), envelope);

    envelope.protocol_version = system::ipc_protocol_version + 1;
    const auto incompatible = system::encode_protocol_frame(envelope);
    ASSERT_FALSE(incompatible);
    EXPECT_EQ(incompatible.error().code,
              system::SystemErrorCode::protocol_version_mismatch);
}

TEST(T021SchemaContract, MigratesPublishedV1ShapeAndRejectsFutureSchema)
{
    const system::ProjectStore store;
    const QByteArray v1 = R"json({
      "schemaVersion": 1,
      "appVersion": "0.0.9",
      "projectId": "legacy-contract",
      "timelineRevision": "17",
      "assetPath": "C:/contract/source.wav",
      "assetSize": "3",
      "assetFingerprint": "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
    })json";
    const std::vector<std::uint8_t> bytes(
        reinterpret_cast<const std::uint8_t*>(v1.constData()),
        reinterpret_cast<const std::uint8_t*>(v1.constData()) + v1.size());
    const auto migrated = store.deserialize(bytes);
    ASSERT_TRUE(migrated);
    ASSERT_TRUE(migrated.value().migrated_from_schema);
    EXPECT_EQ(*migrated.value().migrated_from_schema, 1U);
    EXPECT_EQ(migrated.value().document.schema_version,
              system::project_schema_version);
    EXPECT_EQ(migrated.value().document.timeline.project_id.value, "legacy-contract");
    EXPECT_EQ(migrated.value().document.timeline.timeline_revision, 17U);
    ASSERT_EQ(migrated.value().document.assets.size(), 1U);
    EXPECT_EQ(migrated.value().document.assets.front().size_bytes, 3U);

    const QByteArray future = R"json({"schemaVersion": 4294967295})json";
    const std::vector<std::uint8_t> future_bytes(
        reinterpret_cast<const std::uint8_t*>(future.constData()),
        reinterpret_cast<const std::uint8_t*>(future.constData()) + future.size());
    const auto rejected = store.deserialize(future_bytes);
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code,
              system::SystemErrorCode::unsupported_project_schema);
}

TEST(T021StorageContract, FailedAtomicSaveNeverReplacesLastCommittedDocument)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto primary = path(directory.filePath(QStringLiteral("atomic.srp.json")));
    const system::ProjectStore store;
    ASSERT_TRUE(store.save(primary, document(10)));

    for (const auto fault : {system::SaveFault::disk_full,
                             system::SaveFault::partial_write,
                             system::SaveFault::before_commit}) {
        SCOPED_TRACE(static_cast<int>(fault));
        const auto failed = store.save(primary, document(11), fault);
        ASSERT_FALSE(failed);
        const auto loaded = store.load(primary);
        ASSERT_TRUE(loaded);
        EXPECT_EQ(loaded.value().document.timeline.timeline_revision, 10U);
    }
}

TEST(T021CacheContract, CorruptEntryIsQuarantinedAsRebuildRequired)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto root = path(directory.path());
    const system::RebuildableCache cache{root};
    const auto key = system::RebuildableCache::make_key(
        {"asset", "algorithm-v1", "parameters-v1", "tool-v1"});
    ASSERT_TRUE(cache.put(key, {1, 2, 3, 4}));
    ASSERT_TRUE(cache.get(key).hit);

    const auto cache_file = root / (key + ".srcache");
    overwrite(cache_file, QByteArrayLiteral("not-a-valid-cache-entry"));
    const auto corrupt = cache.get(key);
    EXPECT_FALSE(corrupt.hit);
    EXPECT_TRUE(corrupt.rebuild_required);
    ASSERT_TRUE(corrupt.diagnostic);
    EXPECT_EQ(corrupt.diagnostic->code, system::SystemErrorCode::cache_corrupt);
    EXPECT_FALSE(std::filesystem::exists(cache_file));
}

} // namespace
