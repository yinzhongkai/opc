#include <gtest/gtest.h>

#include <space_rhythm/system/runtime.hpp>

#include <QFile>
#include <QTemporaryDir>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

namespace {

using namespace space_rhythm;

std::filesystem::path path(const QString& value)
{
    return std::filesystem::path{value.toStdWString()};
}

void write_file(const std::filesystem::path& target, const QByteArray& bytes)
{
    QFile file{QString::fromStdWString(target.wstring())};
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(bytes), bytes.size());
    file.close();
}

system::ProjectDocument project(core::TimelineRevision revision = 7)
{
    core::TimelineSnapshot timeline;
    timeline.project_id = core::ProjectId{"project-test"};
    timeline.timeline_revision = revision;
    timeline.tracks.push_back(core::Track{core::TrackId{"track-0"},
                                          0,
                                          std::string{"Main"},
                                          {}});
    core::RhythmEvent event;
    event.id = core::EventId{"event-0"};
    event.track_id = core::TrackId{"track-0"};
    event.time_ns = std::numeric_limits<core::TimeNs>::max() - 10;
    event.duration_ns = 10;
    event.kind = core::EventKind::manual;
    event.source = core::EventSource{core::EventOrigin::user,
                                     "user",
                                     "1.0",
                                     std::nullopt,
                                     std::nullopt,
                                     std::nullopt,
                                     {},
                                     {}};
    event.strength_ppm = 750'000;
    event.locked = true;
    event.user_edited = true;
    timeline.events.push_back(std::move(event));
    timeline.extensions.emplace(
        "space-rhythm.timeline-note",
        core::VersionedOpaqueObject{"space-rhythm.core",
                                    1,
                                    {"note.v1"},
                                    {{"text", "preserved"}}});

    return system::ProjectDocument{
        system::project_schema_version,
        "0.1.0",
        std::move(timeline),
        {system::AssetReference{"asset-0",
                                std::filesystem::path{L"C:/assets/source.mov"},
                                std::numeric_limits<std::uint64_t>::max(),
                                std::string(64, 'a')}},
        {{"theme", "dark"}},
        {{"vendor.future", "opaque-json-token"}}};
}

TEST(ProjectStore, RoundTripsExactInt64Uint64AndOpaqueFields)
{
    const system::ProjectStore store;
    const auto original = project(std::numeric_limits<core::TimelineRevision>::max());
    const auto serialized = store.serialize(original);
    ASSERT_TRUE(serialized);
    const std::string json(serialized.value().begin(), serialized.value().end());
    EXPECT_NE(json.find("\"timelineRevision\": \"18446744073709551615\""),
              std::string::npos);
    EXPECT_NE(json.find("\"timeNs\": \"9223372036854775797\""),
              std::string::npos);

    const auto loaded = store.deserialize(serialized.value());
    ASSERT_TRUE(loaded);
    EXPECT_FALSE(loaded.value().migrated_from_schema.has_value());
    EXPECT_EQ(loaded.value().document, original);
}

TEST(ProjectStore, MigratesV1AndRejectsUnsupportedFutureSchema)
{
    const system::ProjectStore store;
    const QByteArray legacy = R"json({
      "schemaVersion": 1,
      "appVersion": "0.0.9",
      "projectId": "legacy-project",
      "timelineRevision": "9",
      "assetPath": "C:/legacy/source.wav",
      "assetSize": "4",
      "assetFingerprint": "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
    })json";
    const auto migrated = store.deserialize(std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(legacy.constData()),
        reinterpret_cast<const std::uint8_t*>(legacy.constData()) + legacy.size()));
    ASSERT_TRUE(migrated);
    ASSERT_TRUE(migrated.value().migrated_from_schema);
    EXPECT_EQ(*migrated.value().migrated_from_schema, 1U);
    EXPECT_EQ(migrated.value().document.timeline.timeline_revision, 9U);
    ASSERT_EQ(migrated.value().document.assets.size(), 1U);
    EXPECT_EQ(migrated.value().document.assets.front().asset_id, "asset-0");

    const QByteArray future = R"json({"schemaVersion":99})json";
    const auto rejected = store.deserialize(std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(future.constData()),
        reinterpret_cast<const std::uint8_t*>(future.constData()) + future.size()));
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code,
              system::SystemErrorCode::unsupported_project_schema);
    EXPECT_EQ(rejected.error().category, core::ErrorCategory::compatibility);
}

TEST(ProjectStore, AtomicSavePreservesCommittedProjectAcrossWriteFailures)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto primary = path(directory.filePath(QStringLiteral("project.srp.json")));
    const system::ProjectStore store;
    ASSERT_TRUE(store.save(primary, project(1)));

    for (const auto fault : {system::SaveFault::disk_full,
                             system::SaveFault::partial_write,
                             system::SaveFault::before_commit}) {
        const auto failed = store.save(primary, project(2), fault);
        ASSERT_FALSE(failed);
        EXPECT_TRUE(failed.error().code == system::SystemErrorCode::disk_full
                    || failed.error().code == system::SystemErrorCode::partial_write);
        EXPECT_FALSE(failed.error().diagnostic_id.empty());
        const auto preserved = store.load(primary);
        ASSERT_TRUE(preserved);
        EXPECT_EQ(preserved.value().document.timeline.timeline_revision, 1U);
    }
}

TEST(ProjectStore, RecoversAutosaveAndIgnoresCorruptAutosaveWhenPrimaryIsValid)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto primary = path(directory.filePath(QStringLiteral("project.srp.json")));
    const system::ProjectStore store;
    ASSERT_TRUE(store.save(primary, project(1)));
    ASSERT_TRUE(store.save_autosave(primary, project(2)));
    ASSERT_TRUE(store.mark_session_clean(primary, false));

    const auto recovered = store.recover(primary);
    ASSERT_TRUE(recovered);
    EXPECT_EQ(recovered.value().source, system::RecoverySource::autosave);
    EXPECT_GT(recovered.value().source_modified_unix_ms, 0U);
    EXPECT_EQ(recovered.value().project.document.timeline.timeline_revision, 2U);

    write_file(system::ProjectStore::autosave_path(primary), QByteArrayLiteral("{broken"));
    const auto fallback = store.recover(primary);
    ASSERT_TRUE(fallback);
    EXPECT_EQ(fallback.value().source, system::RecoverySource::primary);
    EXPECT_EQ(fallback.value().project.document.timeline.timeline_revision, 1U);
    ASSERT_TRUE(fallback.value().ignored_recovery_error);
    EXPECT_FALSE(fallback.value().ignored_recovery_error->diagnostic_id.empty());

    ASSERT_TRUE(store.save_autosave(primary, project(3)));
    write_file(primary, QByteArrayLiteral("partial"));
    const auto autosave_fallback = store.recover(primary);
    ASSERT_TRUE(autosave_fallback);
    EXPECT_EQ(autosave_fallback.value().source, system::RecoverySource::autosave);
    EXPECT_EQ(autosave_fallback.value().project.document.timeline.timeline_revision, 3U);
}

TEST(AssetRelocation, RequiresSizeAndFingerprintInsteadOfFilenameAlone)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto root = path(directory.path());
    const auto wrong_directory = root / L"a";
    const auto correct_directory = root / L"b";
    std::filesystem::create_directories(wrong_directory);
    std::filesystem::create_directories(correct_directory);
    const auto wrong = wrong_directory / L"source.media";
    const auto correct = correct_directory / L"renamed.media";
    write_file(wrong, QByteArrayLiteral("wrong"));
    write_file(correct, QByteArrayLiteral("right"));
    const auto fingerprint = system::sha256_file(correct);
    ASSERT_TRUE(fingerprint);
    system::AssetReference asset{"asset-1",
                                 root / L"missing" / L"source.media",
                                 5,
                                 fingerprint.value()};

    const auto relocated = system::relocate_asset(asset, {root});
    ASSERT_TRUE(relocated);
    EXPECT_EQ(std::filesystem::weakly_canonical(relocated.value().resolved_path),
              std::filesystem::weakly_canonical(correct));
    EXPECT_GE(relocated.value().candidates_checked, 2U);

    ASSERT_TRUE(QFile::remove(QString::fromStdWString(correct.wstring())));
    const auto rejected = system::relocate_asset(asset, {root});
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code,
              system::SystemErrorCode::fingerprint_mismatch);
}

TEST(RebuildableCache, ValidatesContentAndTreatsCorruptionAsRebuildable)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const auto root = path(directory.path());
    const system::RebuildableCache cache{root};
    const system::CacheKeyInput key_input{"asset-fingerprint",
                                          "algorithm-v1",
                                          "parameters",
                                          "tool-v1"};
    const auto key = system::RebuildableCache::make_key(key_input);
    EXPECT_EQ(key, system::RebuildableCache::make_key(key_input));
    ASSERT_EQ(key.size(), 64U);

    const std::vector<std::uint8_t> original{1, 2, 3, 4};
    ASSERT_TRUE(cache.put(key, original));
    const auto hit = cache.get(key);
    EXPECT_TRUE(hit.hit);
    EXPECT_FALSE(hit.rebuild_required);
    EXPECT_EQ(hit.payload, original);

    const std::vector<std::uint8_t> replacement{9, 8, 7};
    const auto partial = cache.put(key, replacement, system::SaveFault::partial_write);
    ASSERT_FALSE(partial);
    const auto still_original = cache.get(key);
    ASSERT_TRUE(still_original.hit);
    EXPECT_EQ(still_original.payload, original);

    const auto cache_file = root / (key + ".srcache");
    write_file(cache_file, QByteArrayLiteral("corrupt-cache"));
    const auto corrupt = cache.get(key);
    EXPECT_FALSE(corrupt.hit);
    EXPECT_TRUE(corrupt.rebuild_required);
    ASSERT_TRUE(corrupt.diagnostic);
    EXPECT_EQ(corrupt.diagnostic->code, system::SystemErrorCode::cache_corrupt);
    EXPECT_FALSE(std::filesystem::exists(cache_file));

    const auto miss = cache.get(key);
    EXPECT_FALSE(miss.hit);
    EXPECT_TRUE(miss.rebuild_required);
    ASSERT_TRUE(miss.diagnostic);
    EXPECT_EQ(miss.diagnostic->code, system::SystemErrorCode::cache_miss);
}

TEST(RebuildableCache, PrunesOldestEntriesToQuota)
{
    QTemporaryDir directory;
    ASSERT_TRUE(directory.isValid());
    const system::RebuildableCache cache{path(directory.path())};
    const auto key_a = system::RebuildableCache::make_key(
        {"asset-a", "algo", "params", "tool"});
    const auto key_b = system::RebuildableCache::make_key(
        {"asset-b", "algo", "params", "tool"});
    ASSERT_TRUE(cache.put(key_a, std::vector<std::uint8_t>(128, 1)));
    ASSERT_TRUE(cache.put(key_b, std::vector<std::uint8_t>(128, 2)));
    const auto pruned = cache.prune(0);
    ASSERT_TRUE(pruned);
    EXPECT_GT(pruned.value(), 0U);
    EXPECT_TRUE(cache.get(key_a).rebuild_required);
    EXPECT_TRUE(cache.get(key_b).rebuild_required);
}

} // namespace
