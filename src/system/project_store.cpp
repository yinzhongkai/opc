#include <space_rhythm/system/runtime.hpp>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>

#include <algorithm>
#include <atomic>
#include <limits>
#include <ranges>
#include <set>
#include <utility>

namespace space_rhythm::system {
namespace {

constexpr qint64 maximum_project_bytes = 64LL * 1024LL * 1024LL;
std::atomic_uint64_t next_project_diagnostic{1};

SystemError make_project_error(core::ErrorCategory category,
                               SystemErrorCode code,
                               std::string_view stage,
                               bool retryable = false)
{
    const auto sequence = next_project_diagnostic.fetch_add(1, std::memory_order_relaxed);
    return SystemError{category,
                       code,
                       std::string{stage},
                       "project:" + std::to_string(sequence),
                       retryable,
                       std::string{to_string(code)},
                       {}};
}

QString path_string(const std::filesystem::path& path)
{
    return QString::fromStdWString(path.wstring());
}

std::filesystem::path native_path(const QString& path)
{
    return std::filesystem::path{path.toStdWString()};
}

bool has_only_keys(const QJsonObject& object,
                   std::initializer_list<QString> allowed_keys)
{
    const std::set<QString> allowed{allowed_keys};
    return std::ranges::all_of(object.keys(), [&](const QString& key) {
        return allowed.contains(key);
    });
}

QJsonArray strings_to_json(const std::vector<std::string>& values)
{
    QJsonArray array;
    for (const auto& value : values) {
        array.append(QString::fromStdString(value));
    }
    return array;
}

std::optional<std::vector<std::string>> strings_from_json(const QJsonValue& value)
{
    if (!value.isArray()) {
        return std::nullopt;
    }
    std::vector<std::string> values;
    std::set<std::string> unique;
    for (const auto& item : value.toArray()) {
        if (!item.isString()) {
            return std::nullopt;
        }
        auto text = item.toString().toStdString();
        if (!unique.insert(text).second) {
            return std::nullopt;
        }
        values.push_back(std::move(text));
    }
    return values;
}

QJsonObject string_map_to_json(const std::map<std::string, std::string>& values)
{
    QJsonObject object;
    for (const auto& [key, value] : values) {
        object.insert(QString::fromStdString(key), QString::fromStdString(value));
    }
    return object;
}

std::optional<std::map<std::string, std::string>> string_map_from_json(
    const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    std::map<std::string, std::string> values;
    const auto object = value.toObject();
    for (auto iterator = object.constBegin(); iterator != object.constEnd(); ++iterator) {
        if (!iterator.value().isString()) {
            return std::nullopt;
        }
        values.emplace(iterator.key().toStdString(), iterator.value().toString().toStdString());
    }
    return values;
}

QJsonObject opaque_to_json(const core::VersionedOpaqueObject& value)
{
    return {{QStringLiteral("owner"), QString::fromStdString(value.owner)},
            {QStringLiteral("schemaVersion"), static_cast<qint64>(value.schema_version)},
            {QStringLiteral("requiredFeatures"), strings_to_json(value.required_features)},
            {QStringLiteral("payload"), string_map_to_json(value.payload)}};
}

std::optional<core::VersionedOpaqueObject> opaque_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    if (!has_only_keys(object,
                       {QStringLiteral("owner"),
                        QStringLiteral("schemaVersion"),
                        QStringLiteral("requiredFeatures"),
                        QStringLiteral("payload")})
        || !object.value(QStringLiteral("owner")).isString()
        || !object.value(QStringLiteral("schemaVersion")).isDouble()) {
        return std::nullopt;
    }
    const auto features = strings_from_json(object.value(QStringLiteral("requiredFeatures")));
    const auto payload = string_map_from_json(object.value(QStringLiteral("payload")));
    const auto version = object.value(QStringLiteral("schemaVersion")).toInteger();
    if (!features || !payload || version <= 0
        || version > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return core::VersionedOpaqueObject{
        object.value(QStringLiteral("owner")).toString().toStdString(),
        static_cast<std::uint32_t>(version),
        *features,
        *payload};
}

QJsonObject opaque_map_to_json(
    const std::map<std::string, core::VersionedOpaqueObject>& values)
{
    QJsonObject object;
    for (const auto& [key, value] : values) {
        object.insert(QString::fromStdString(key), opaque_to_json(value));
    }
    return object;
}

std::optional<std::map<std::string, core::VersionedOpaqueObject>> opaque_map_from_json(
    const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    std::map<std::string, core::VersionedOpaqueObject> values;
    const auto object = value.toObject();
    for (auto iterator = object.constBegin(); iterator != object.constEnd(); ++iterator) {
        const auto opaque = opaque_from_json(iterator.value());
        if (!opaque) {
            return std::nullopt;
        }
        values.emplace(iterator.key().toStdString(), *opaque);
    }
    return values;
}

QString origin_token(core::EventOrigin origin)
{
    switch (origin) {
    case core::EventOrigin::user:
        return QStringLiteral("user");
    case core::EventOrigin::analysis:
        return QStringLiteral("analysis");
    case core::EventOrigin::import:
        return QStringLiteral("import");
    }
    return QStringLiteral("user");
}

std::optional<core::EventOrigin> parse_origin(const QString& token)
{
    if (token == QStringLiteral("user")) {
        return core::EventOrigin::user;
    }
    if (token == QStringLiteral("analysis")) {
        return core::EventOrigin::analysis;
    }
    if (token == QStringLiteral("import")) {
        return core::EventOrigin::import;
    }
    return std::nullopt;
}

QJsonObject source_to_json(const core::EventSource& source)
{
    QJsonArray candidate_ids;
    for (const auto& id : source.candidate_ids) {
        candidate_ids.append(QString::fromStdString(id.value));
    }
    QJsonObject object{{QStringLiteral("origin"), origin_token(source.origin)},
                       {QStringLiteral("producerId"),
                        QString::fromStdString(source.producer_id)},
                       {QStringLiteral("producerVersion"),
                        QString::fromStdString(source.producer_version)},
                       {QStringLiteral("candidateIds"), candidate_ids},
                       {QStringLiteral("extensions"),
                        opaque_map_to_json(source.extensions)}};
    if (source.input_fingerprint) {
        object.insert(QStringLiteral("inputFingerprint"),
                      QString::fromStdString(*source.input_fingerprint));
    }
    if (source.parameters_digest) {
        object.insert(QStringLiteral("parametersDigest"),
                      QString::fromStdString(*source.parameters_digest));
    }
    if (source.analysis_revision) {
        object.insert(QStringLiteral("analysisRevision"),
                      QString::fromStdString(source.analysis_revision->value));
    }
    return object;
}

std::optional<core::EventSource> source_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    if (!has_only_keys(object,
                       {QStringLiteral("origin"),
                        QStringLiteral("producerId"),
                        QStringLiteral("producerVersion"),
                        QStringLiteral("inputFingerprint"),
                        QStringLiteral("parametersDigest"),
                        QStringLiteral("analysisRevision"),
                        QStringLiteral("candidateIds"),
                        QStringLiteral("extensions")})
        || !object.value(QStringLiteral("origin")).isString()
        || !object.value(QStringLiteral("producerId")).isString()
        || !object.value(QStringLiteral("producerVersion")).isString()) {
        return std::nullopt;
    }
    const auto origin = parse_origin(object.value(QStringLiteral("origin")).toString());
    const auto extensions = opaque_map_from_json(object.value(QStringLiteral("extensions")));
    if (!origin || !extensions
        || !object.value(QStringLiteral("candidateIds")).isArray()) {
        return std::nullopt;
    }
    core::EventSource source;
    source.origin = *origin;
    source.producer_id = object.value(QStringLiteral("producerId")).toString().toStdString();
    source.producer_version =
        object.value(QStringLiteral("producerVersion")).toString().toStdString();
    if (object.contains(QStringLiteral("inputFingerprint"))) {
        if (!object.value(QStringLiteral("inputFingerprint")).isString()) {
            return std::nullopt;
        }
        source.input_fingerprint =
            object.value(QStringLiteral("inputFingerprint")).toString().toStdString();
    }
    if (object.contains(QStringLiteral("parametersDigest"))) {
        if (!object.value(QStringLiteral("parametersDigest")).isString()) {
            return std::nullopt;
        }
        source.parameters_digest =
            object.value(QStringLiteral("parametersDigest")).toString().toStdString();
    }
    if (object.contains(QStringLiteral("analysisRevision"))) {
        if (!object.value(QStringLiteral("analysisRevision")).isString()) {
            return std::nullopt;
        }
        source.analysis_revision = core::AnalysisRevision{
            object.value(QStringLiteral("analysisRevision")).toString().toStdString()};
    }
    for (const auto& id : object.value(QStringLiteral("candidateIds")).toArray()) {
        if (!id.isString()) {
            return std::nullopt;
        }
        source.candidate_ids.push_back(core::CandidateId{id.toString().toStdString()});
    }
    source.extensions = *extensions;
    return source;
}

template <typename Integer>
std::optional<Integer> parse_decimal(const QJsonValue& value)
{
    if (!value.isString()) {
        return std::nullopt;
    }
    bool ok = false;
    if constexpr (std::is_signed_v<Integer>) {
        const auto parsed = value.toString().toLongLong(&ok, 10);
        if (!ok || parsed < std::numeric_limits<Integer>::min()
            || parsed > std::numeric_limits<Integer>::max()) {
            return std::nullopt;
        }
        return static_cast<Integer>(parsed);
    } else {
        const auto text = value.toString();
        if (text.startsWith(QLatin1Char('-'))) {
            return std::nullopt;
        }
        const auto parsed = text.toULongLong(&ok, 10);
        if (!ok || parsed > std::numeric_limits<Integer>::max()) {
            return std::nullopt;
        }
        return static_cast<Integer>(parsed);
    }
}

QJsonObject timeline_to_json(const core::TimelineSnapshot& timeline)
{
    QJsonArray tracks;
    for (const auto& track : timeline.tracks) {
        QJsonObject item{{QStringLiteral("id"), QString::fromStdString(track.id.value)},
                         {QStringLiteral("orderIndex"),
                          static_cast<qint64>(track.order_index)},
                         {QStringLiteral("extensions"),
                          opaque_map_to_json(track.extensions)}};
        if (track.label) {
            item.insert(QStringLiteral("label"), QString::fromStdString(*track.label));
        }
        tracks.append(item);
    }
    QJsonArray events;
    for (const auto& event : timeline.events) {
        QJsonObject item{{QStringLiteral("id"), QString::fromStdString(event.id.value)},
                         {QStringLiteral("trackId"),
                          QString::fromStdString(event.track_id.value)},
                         {QStringLiteral("timeNs"),
                          QString::number(static_cast<qlonglong>(event.time_ns))},
                         {QStringLiteral("durationNs"),
                          QString::number(static_cast<qlonglong>(event.duration_ns))},
                         {QStringLiteral("kind"),
                          QString::fromUtf8(core::to_string(event.kind).data(),
                                            static_cast<qsizetype>(
                                                core::to_string(event.kind).size()))},
                         {QStringLiteral("source"), source_to_json(event.source)},
                         {QStringLiteral("strengthPpm"),
                          static_cast<qint64>(event.strength_ppm)},
                         {QStringLiteral("locked"), event.locked},
                         {QStringLiteral("userEdited"), event.user_edited},
                         {QStringLiteral("extensions"),
                          opaque_map_to_json(event.extensions)}};
        if (event.confidence_ppm) {
            item.insert(QStringLiteral("confidencePpm"),
                        static_cast<qint64>(*event.confidence_ppm));
        }
        if (event.sound_assignment) {
            item.insert(QStringLiteral("soundAssignment"),
                        opaque_to_json(*event.sound_assignment));
        }
        events.append(item);
    }
    return {{QStringLiteral("coreSchemaVersion"),
             static_cast<qint64>(timeline.schema_version)},
            {QStringLiteral("coreContractVersion"),
             QString::fromStdString(timeline.core_contract_version)},
            {QStringLiteral("projectId"),
             QString::fromStdString(timeline.project_id.value)},
            {QStringLiteral("timelineRevision"),
             QString::number(static_cast<qulonglong>(timeline.timeline_revision))},
            {QStringLiteral("tracks"), tracks},
            {QStringLiteral("events"), events},
            {QStringLiteral("extensions"),
             opaque_map_to_json(timeline.extensions)}};
}

std::optional<core::TimelineSnapshot> timeline_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    if (!has_only_keys(object,
                       {QStringLiteral("coreSchemaVersion"),
                        QStringLiteral("coreContractVersion"),
                        QStringLiteral("projectId"),
                        QStringLiteral("timelineRevision"),
                        QStringLiteral("tracks"),
                        QStringLiteral("events"),
                        QStringLiteral("extensions")})
        || !object.value(QStringLiteral("coreSchemaVersion")).isDouble()
        || !object.value(QStringLiteral("coreContractVersion")).isString()
        || !object.value(QStringLiteral("projectId")).isString()) {
        return std::nullopt;
    }
    const auto revision = parse_decimal<core::TimelineRevision>(
        object.value(QStringLiteral("timelineRevision")));
    const auto extensions = opaque_map_from_json(object.value(QStringLiteral("extensions")));
    const auto schema = object.value(QStringLiteral("coreSchemaVersion")).toInteger();
    if (!revision || !extensions || schema < 0
        || schema > std::numeric_limits<std::uint32_t>::max()
        || !object.value(QStringLiteral("tracks")).isArray()
        || !object.value(QStringLiteral("events")).isArray()) {
        return std::nullopt;
    }
    core::TimelineSnapshot timeline;
    timeline.schema_version = static_cast<std::uint32_t>(schema);
    timeline.core_contract_version =
        object.value(QStringLiteral("coreContractVersion")).toString().toStdString();
    timeline.project_id = core::ProjectId{
        object.value(QStringLiteral("projectId")).toString().toStdString()};
    timeline.timeline_revision = *revision;
    timeline.extensions = *extensions;
    for (const auto& track_value : object.value(QStringLiteral("tracks")).toArray()) {
        if (!track_value.isObject()) {
            return std::nullopt;
        }
        const auto track_object = track_value.toObject();
        if (!has_only_keys(track_object,
                           {QStringLiteral("id"),
                            QStringLiteral("orderIndex"),
                            QStringLiteral("label"),
                            QStringLiteral("extensions")})
            || !track_object.value(QStringLiteral("id")).isString()
            || !track_object.value(QStringLiteral("orderIndex")).isDouble()) {
            return std::nullopt;
        }
        const auto track_extensions =
            opaque_map_from_json(track_object.value(QStringLiteral("extensions")));
        const auto order = track_object.value(QStringLiteral("orderIndex")).toInteger(-1);
        if (!track_extensions || order < 0
            || order > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
        core::Track track{core::TrackId{
                              track_object.value(QStringLiteral("id")).toString().toStdString()},
                          static_cast<std::uint32_t>(order),
                          std::nullopt,
                          *track_extensions};
        if (track_object.contains(QStringLiteral("label"))) {
            if (!track_object.value(QStringLiteral("label")).isString()) {
                return std::nullopt;
            }
            track.label = track_object.value(QStringLiteral("label")).toString().toStdString();
        }
        timeline.tracks.push_back(std::move(track));
    }
    for (const auto& event_value : object.value(QStringLiteral("events")).toArray()) {
        if (!event_value.isObject()) {
            return std::nullopt;
        }
        const auto event_object = event_value.toObject();
        if (!has_only_keys(event_object,
                           {QStringLiteral("id"),
                            QStringLiteral("trackId"),
                            QStringLiteral("timeNs"),
                            QStringLiteral("durationNs"),
                            QStringLiteral("kind"),
                            QStringLiteral("source"),
                            QStringLiteral("strengthPpm"),
                            QStringLiteral("confidencePpm"),
                            QStringLiteral("locked"),
                            QStringLiteral("userEdited"),
                            QStringLiteral("soundAssignment"),
                            QStringLiteral("extensions")})
            || !event_object.value(QStringLiteral("id")).isString()
            || !event_object.value(QStringLiteral("trackId")).isString()
            || !event_object.value(QStringLiteral("kind")).isString()
            || !event_object.value(QStringLiteral("strengthPpm")).isDouble()
            || !event_object.value(QStringLiteral("locked")).isBool()
            || !event_object.value(QStringLiteral("userEdited")).isBool()) {
            return std::nullopt;
        }
        const auto time = parse_decimal<core::TimeNs>(
            event_object.value(QStringLiteral("timeNs")));
        const auto duration = parse_decimal<core::DurationNs>(
            event_object.value(QStringLiteral("durationNs")));
        const auto kind = core::parse_event_kind(
            event_object.value(QStringLiteral("kind")).toString().toStdString());
        const auto source = source_from_json(event_object.value(QStringLiteral("source")));
        const auto event_extensions =
            opaque_map_from_json(event_object.value(QStringLiteral("extensions")));
        const auto strength = event_object.value(QStringLiteral("strengthPpm")).toInteger(-1);
        if (!time || !duration || !kind || !source || !event_extensions || strength < 0
            || strength > std::numeric_limits<std::uint32_t>::max()) {
            return std::nullopt;
        }
        core::RhythmEvent event;
        event.id = core::EventId{
            event_object.value(QStringLiteral("id")).toString().toStdString()};
        event.track_id = core::TrackId{
            event_object.value(QStringLiteral("trackId")).toString().toStdString()};
        event.time_ns = *time;
        event.duration_ns = *duration;
        event.kind = kind.value();
        event.source = *source;
        event.strength_ppm = static_cast<std::uint32_t>(strength);
        if (event_object.contains(QStringLiteral("confidencePpm"))) {
            if (!event_object.value(QStringLiteral("confidencePpm")).isDouble()) {
                return std::nullopt;
            }
            const auto confidence =
                event_object.value(QStringLiteral("confidencePpm")).toInteger(-1);
            if (confidence < 0 || confidence > std::numeric_limits<std::uint32_t>::max()) {
                return std::nullopt;
            }
            event.confidence_ppm = static_cast<std::uint32_t>(confidence);
        }
        event.locked = event_object.value(QStringLiteral("locked")).toBool();
        event.user_edited = event_object.value(QStringLiteral("userEdited")).toBool();
        if (event_object.contains(QStringLiteral("soundAssignment"))) {
            event.sound_assignment =
                opaque_from_json(event_object.value(QStringLiteral("soundAssignment")));
            if (!event.sound_assignment) {
                return std::nullopt;
            }
        }
        event.extensions = *event_extensions;
        timeline.events.push_back(std::move(event));
    }
    return timeline;
}

QJsonObject asset_to_json(const AssetReference& asset)
{
    return {{QStringLiteral("assetId"), QString::fromStdString(asset.asset_id)},
            {QStringLiteral("storedPath"), path_string(asset.stored_path)},
            {QStringLiteral("sizeBytes"),
             QString::number(static_cast<qulonglong>(asset.size_bytes))},
            {QStringLiteral("fingerprintSha256"),
             QString::fromStdString(asset.fingerprint_sha256)}};
}

std::optional<AssetReference> asset_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    if (!has_only_keys(object,
                       {QStringLiteral("assetId"),
                        QStringLiteral("storedPath"),
                        QStringLiteral("sizeBytes"),
                        QStringLiteral("fingerprintSha256")})
        || !object.value(QStringLiteral("assetId")).isString()
        || !object.value(QStringLiteral("storedPath")).isString()
        || !object.value(QStringLiteral("fingerprintSha256")).isString()) {
        return std::nullopt;
    }
    const auto size = parse_decimal<std::uint64_t>(object.value(QStringLiteral("sizeBytes")));
    if (!size) {
        return std::nullopt;
    }
    return AssetReference{
        object.value(QStringLiteral("assetId")).toString().toStdString(),
        native_path(object.value(QStringLiteral("storedPath")).toString()),
        *size,
        object.value(QStringLiteral("fingerprintSha256")).toString().toStdString()};
}

bool valid_digest(std::string_view digest)
{
    return digest.size() == 64
        && std::ranges::all_of(digest, [](const unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f')
                   || (character >= 'A' && character <= 'F');
           });
}

Result<LoadedProject> invalid_project(SystemErrorCode code,
                                      std::string_view stage,
                                      core::ErrorCategory category =
                                          core::ErrorCategory::validation)
{
    return Result<LoadedProject>::failure(make_project_error(category, code, stage));
}

QJsonObject migrate_v1(QJsonObject object)
{
    object.insert(QStringLiteral("schemaVersion"),
                  static_cast<qint64>(project_schema_version));
    if (!object.contains(QStringLiteral("requiredFeatures"))) {
        object.insert(QStringLiteral("requiredFeatures"),
                      QJsonArray{QStringLiteral("core.timeline.v1"),
                                 QStringLiteral("project.assets.v1")});
    }
    if (!object.contains(QStringLiteral("settings"))) {
        object.insert(QStringLiteral("settings"), QJsonObject{});
    }
    if (!object.contains(QStringLiteral("extensions"))) {
        object.insert(QStringLiteral("extensions"), QJsonObject{});
    }
    if (!object.contains(QStringLiteral("timeline"))) {
        const auto project_id = object.value(QStringLiteral("projectId")).toString();
        const auto revision = object.value(QStringLiteral("timelineRevision")).isString()
            ? object.value(QStringLiteral("timelineRevision"))
            : QJsonValue{QStringLiteral("0")};
        QJsonArray tracks{
            QJsonObject{{QStringLiteral("id"), QStringLiteral("track-0")},
                        {QStringLiteral("orderIndex"), 0},
                        {QStringLiteral("extensions"), QJsonObject{}}}};
        object.insert(
            QStringLiteral("timeline"),
            QJsonObject{{QStringLiteral("coreSchemaVersion"), 1},
                        {QStringLiteral("coreContractVersion"),
                         QString::fromUtf8(core::contract_version.data(),
                                           static_cast<qsizetype>(
                                               core::contract_version.size()))},
                        {QStringLiteral("projectId"), project_id},
                        {QStringLiteral("timelineRevision"), revision},
                        {QStringLiteral("tracks"), tracks},
                        {QStringLiteral("events"), QJsonArray{}},
                        {QStringLiteral("extensions"), QJsonObject{}}});
    }
    if (!object.contains(QStringLiteral("assets"))) {
        QJsonArray assets;
        if (object.value(QStringLiteral("assetPath")).isString()) {
            assets.append(QJsonObject{
                {QStringLiteral("assetId"), QStringLiteral("asset-0")},
                {QStringLiteral("storedPath"),
                 object.value(QStringLiteral("assetPath")).toString()},
                {QStringLiteral("sizeBytes"),
                 object.value(QStringLiteral("assetSize")).isString()
                     ? object.value(QStringLiteral("assetSize"))
                     : QJsonValue{QStringLiteral("0")}},
                {QStringLiteral("fingerprintSha256"),
                 object.value(QStringLiteral("assetFingerprint")).toString()}});
        }
        object.insert(QStringLiteral("assets"), assets);
    }
    object.remove(QStringLiteral("projectId"));
    object.remove(QStringLiteral("timelineRevision"));
    object.remove(QStringLiteral("assetPath"));
    object.remove(QStringLiteral("assetSize"));
    object.remove(QStringLiteral("assetFingerprint"));
    return object;
}

Result<void> write_atomically(const std::filesystem::path& path,
                              const std::vector<std::uint8_t>& bytes,
                              SaveFault fault,
                              std::string_view stage)
{
    if (fault == SaveFault::disk_full) {
        return Result<void>::failure(make_project_error(core::ErrorCategory::resource_limit,
                                                        SystemErrorCode::disk_full,
                                                        stage,
                                                        true));
    }
    std::error_code filesystem_error;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, filesystem_error);
        if (filesystem_error) {
            return Result<void>::failure(make_project_error(core::ErrorCategory::internal,
                                                            SystemErrorCode::io_error,
                                                            stage,
                                                            true));
        }
    }
    QSaveFile file{path_string(path)};
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>::failure(make_project_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::io_error,
                                                        stage,
                                                        true));
    }
    const auto requested = fault == SaveFault::partial_write ? bytes.size() / 2U
                                                              : bytes.size();
    const auto written = file.write(reinterpret_cast<const char*>(bytes.data()),
                                    static_cast<qint64>(requested));
    if (written != static_cast<qint64>(requested)) {
        file.cancelWriting();
        return Result<void>::failure(make_project_error(core::ErrorCategory::resource_limit,
                                                        SystemErrorCode::disk_full,
                                                        stage,
                                                        true));
    }
    if (fault == SaveFault::partial_write || fault == SaveFault::before_commit) {
        file.cancelWriting();
        return Result<void>::failure(make_project_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::partial_write,
                                                        stage,
                                                        true));
    }
    if (!file.commit()) {
        return Result<void>::failure(make_project_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::io_error,
                                                        stage,
                                                        true));
    }
    return Result<void>::success();
}

std::uint64_t modified_unix_ms(const std::filesystem::path& path)
{
    const auto milliseconds = QFileInfo{path_string(path)}.lastModified().toMSecsSinceEpoch();
    return milliseconds > 0 ? static_cast<std::uint64_t>(milliseconds) : 0;
}

} // namespace

Result<std::vector<std::uint8_t>> ProjectStore::serialize(
    const ProjectDocument& document) const
{
    if (document.schema_version != project_schema_version || document.app_version.empty()) {
        return Result<std::vector<std::uint8_t>>::failure(make_project_error(
            core::ErrorCategory::compatibility,
            SystemErrorCode::unsupported_project_schema,
            "project.serialize"));
    }
    const auto normalized = core::validate_and_normalize_snapshot(document.timeline);
    if (!normalized) {
        auto error = make_project_error(core::ErrorCategory::validation,
                                        SystemErrorCode::invalid_project,
                                        "project.serialize");
        error.context.emplace("coreError", std::string{core::to_string(normalized.error().code)});
        return Result<std::vector<std::uint8_t>>::failure(std::move(error));
    }
    std::set<std::string> asset_ids;
    for (const auto& asset : document.assets) {
        if (!core::validate_identifier(asset.asset_id) || asset.stored_path.empty()
            || !valid_digest(asset.fingerprint_sha256)
            || !asset_ids.insert(asset.asset_id).second) {
            return Result<std::vector<std::uint8_t>>::failure(make_project_error(
                core::ErrorCategory::validation,
                SystemErrorCode::invalid_project,
                "project.asset"));
        }
    }
    QJsonArray assets;
    for (const auto& asset : document.assets) {
        assets.append(asset_to_json(asset));
    }
    QJsonObject root{
        {QStringLiteral("schemaVersion"), static_cast<qint64>(document.schema_version)},
        {QStringLiteral("requiredFeatures"),
         QJsonArray{QStringLiteral("core.timeline.v1"),
                    QStringLiteral("project.assets.v1")}},
        {QStringLiteral("appVersion"), QString::fromStdString(document.app_version)},
        {QStringLiteral("timeline"), timeline_to_json(normalized.value())},
        {QStringLiteral("assets"), assets},
        {QStringLiteral("settings"), string_map_to_json(document.settings)},
        {QStringLiteral("extensions"), string_map_to_json(document.extensions)}};
    const auto bytes = QJsonDocument{root}.toJson(QJsonDocument::Indented);
    return Result<std::vector<std::uint8_t>>::success(std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(bytes.constData()),
        reinterpret_cast<const std::uint8_t*>(bytes.constData()) + bytes.size()));
}

Result<LoadedProject> ProjectStore::deserialize(
    const std::vector<std::uint8_t>& bytes) const
{
    if (bytes.empty() || bytes.size() > static_cast<std::size_t>(maximum_project_bytes)) {
        return invalid_project(SystemErrorCode::corrupt_project,
                               "project.deserialize",
                               core::ErrorCategory::resource_limit);
    }
    QJsonParseError parse_error;
    const auto json = QJsonDocument::fromJson(
        QByteArray{reinterpret_cast<const char*>(bytes.data()),
                   static_cast<qsizetype>(bytes.size())},
        &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !json.isObject()) {
        return invalid_project(SystemErrorCode::corrupt_project,
                               "project.deserialize");
    }
    auto root = json.object();
    if (!root.value(QStringLiteral("schemaVersion")).isDouble()) {
        return invalid_project(SystemErrorCode::invalid_project, "project.schema");
    }
    const auto schema = root.value(QStringLiteral("schemaVersion")).toInteger(-1);
    if (schema < 1 || schema > project_schema_version) {
        return invalid_project(SystemErrorCode::unsupported_project_schema,
                               "project.schema",
                               core::ErrorCategory::compatibility);
    }
    std::optional<std::uint32_t> migrated;
    if (schema == 1) {
        root = migrate_v1(std::move(root));
        migrated = 1;
    }
    if (!has_only_keys(root,
                       {QStringLiteral("schemaVersion"),
                        QStringLiteral("requiredFeatures"),
                        QStringLiteral("appVersion"),
                        QStringLiteral("timeline"),
                        QStringLiteral("assets"),
                        QStringLiteral("settings"),
                        QStringLiteral("extensions")})
        || !root.value(QStringLiteral("appVersion")).isString()) {
        return invalid_project(SystemErrorCode::invalid_project, "project.shape");
    }
    const auto features = strings_from_json(root.value(QStringLiteral("requiredFeatures")));
    const std::set<std::string> supported{"core.timeline.v1", "project.assets.v1"};
    if (!features
        || std::ranges::any_of(*features,
                              [&](const std::string& feature) {
                                  return !supported.contains(feature);
                              })) {
        return invalid_project(SystemErrorCode::unsupported_project_schema,
                               "project.features",
                               core::ErrorCategory::compatibility);
    }
    const auto timeline = timeline_from_json(root.value(QStringLiteral("timeline")));
    const auto settings = string_map_from_json(root.value(QStringLiteral("settings")));
    const auto extensions = string_map_from_json(root.value(QStringLiteral("extensions")));
    if (!timeline || !settings || !extensions
        || !root.value(QStringLiteral("assets")).isArray()) {
        return invalid_project(SystemErrorCode::invalid_project, "project.shape");
    }
    ProjectDocument document;
    document.schema_version = project_schema_version;
    document.app_version = root.value(QStringLiteral("appVersion")).toString().toStdString();
    document.timeline = *timeline;
    document.settings = *settings;
    document.extensions = *extensions;
    for (const auto& item : root.value(QStringLiteral("assets")).toArray()) {
        const auto asset = asset_from_json(item);
        if (!asset) {
            return invalid_project(SystemErrorCode::invalid_project, "project.asset");
        }
        document.assets.push_back(*asset);
    }
    const auto canonical = serialize(document);
    if (!canonical) {
        return Result<LoadedProject>::failure(canonical.error());
    }
    return Result<LoadedProject>::success(LoadedProject{std::move(document), migrated});
}

Result<void> ProjectStore::save(const std::filesystem::path& path,
                                const ProjectDocument& document,
                                SaveFault fault) const
{
    const auto bytes = serialize(document);
    if (!bytes) {
        return Result<void>::failure(bytes.error());
    }
    return write_atomically(path, bytes.value(), fault, "project.save");
}

Result<LoadedProject> ProjectStore::load(const std::filesystem::path& path) const
{
    QFile file{path_string(path)};
    if (!file.open(QIODevice::ReadOnly) || file.size() <= 0
        || file.size() > maximum_project_bytes) {
        auto error = make_project_error(core::ErrorCategory::internal,
                                        SystemErrorCode::io_error,
                                        "project.load",
                                        true);
        error.context.emplace("path", path_string(path).toStdString());
        return Result<LoadedProject>::failure(std::move(error));
    }
    const auto raw = file.readAll();
    return deserialize(std::vector<std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(raw.constData()),
        reinterpret_cast<const std::uint8_t*>(raw.constData()) + raw.size()));
}

Result<void> ProjectStore::save_autosave(const std::filesystem::path& primary_path,
                                         const ProjectDocument& document,
                                         SaveFault fault) const
{
    const auto bytes = serialize(document);
    if (!bytes) {
        return Result<void>::failure(bytes.error());
    }
    return write_atomically(autosave_path(primary_path),
                            bytes.value(),
                            fault,
                            "project.autosave");
}

Result<void> ProjectStore::mark_session_clean(const std::filesystem::path& primary_path,
                                              bool clean) const
{
    const std::string text = clean ? "clean\n" : "unclean\n";
    return write_atomically(session_marker_path(primary_path),
                            std::vector<std::uint8_t>(text.begin(), text.end()),
                            SaveFault::none,
                            "project.marker");
}

Result<RecoveryResult> ProjectStore::recover(
    const std::filesystem::path& primary_path) const
{
    bool unclean = false;
    QFile marker{path_string(session_marker_path(primary_path))};
    if (marker.open(QIODevice::ReadOnly)) {
        unclean = marker.readAll().trimmed() == QByteArrayLiteral("unclean");
    }
    const auto primary = load(primary_path);
    const auto autosave = load(autosave_path(primary_path));
    if (unclean && autosave) {
        return Result<RecoveryResult>::success(
            RecoveryResult{autosave.value(),
                           RecoverySource::autosave,
                           modified_unix_ms(autosave_path(primary_path)),
                           std::nullopt});
    }
    if (primary) {
        std::optional<SystemError> ignored;
        if (!autosave && std::filesystem::exists(autosave_path(primary_path))) {
            ignored = autosave.error();
        }
        return Result<RecoveryResult>::success(
            RecoveryResult{primary.value(),
                           RecoverySource::primary,
                           modified_unix_ms(primary_path),
                           ignored});
    }
    if (autosave) {
        return Result<RecoveryResult>::success(
            RecoveryResult{autosave.value(),
                           RecoverySource::autosave,
                           modified_unix_ms(autosave_path(primary_path)),
                           primary.error()});
    }
    return Result<RecoveryResult>::failure(primary.error());
}

std::filesystem::path ProjectStore::autosave_path(
    const std::filesystem::path& primary_path)
{
    auto path = primary_path;
    path += L".autosave";
    return path;
}

std::filesystem::path ProjectStore::session_marker_path(
    const std::filesystem::path& primary_path)
{
    auto path = primary_path;
    path += L".session";
    return path;
}

} // namespace space_rhythm::system
