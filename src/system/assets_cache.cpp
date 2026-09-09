#include <space_rhythm/system/runtime.hpp>

#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

#include <algorithm>
#include <array>
#include <atomic>
#include <limits>
#include <ranges>
#include <system_error>
#include <utility>

namespace space_rhythm::system {
namespace {

constexpr std::array<std::uint8_t, 8> cache_magic{'S', 'R', 'C', 'A', 'C', 'H', 'E', '1'};
constexpr std::uint32_t cache_format_version = 1;
constexpr qint64 maximum_asset_hash_bytes = std::numeric_limits<qint64>::max();
std::atomic_uint64_t next_storage_diagnostic{1};

SystemError make_storage_error(core::ErrorCategory category,
                               SystemErrorCode code,
                               std::string_view stage,
                               bool retryable = false)
{
    const auto sequence = next_storage_diagnostic.fetch_add(1, std::memory_order_relaxed);
    return SystemError{category,
                       code,
                       std::string{stage},
                       "storage:" + std::to_string(sequence),
                       retryable,
                       std::string{to_string(code)},
                       {}};
}

QString path_string(const std::filesystem::path& path)
{
    return QString::fromStdWString(path.wstring());
}

bool valid_hex(std::string_view text)
{
    return text.size() == 64
        && std::ranges::all_of(text, [](const unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f')
                   || (character >= 'A' && character <= 'F');
           });
}

bool equal_hex(std::string_view lhs, std::string_view rhs)
{
    return lhs.size() == rhs.size()
        && std::ranges::equal(lhs, rhs, [](unsigned char left, unsigned char right) {
               const auto lower = [](unsigned char value) {
                   return value >= 'A' && value <= 'F'
                       ? static_cast<unsigned char>(value - 'A' + 'a')
                       : value;
               };
               return lower(left) == lower(right);
           });
}

QByteArray sha256(const QByteArray& bytes)
{
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
}

void append_u32(QByteArray& bytes, std::uint32_t value)
{
    bytes.append(static_cast<char>((value >> 24U) & 0xffU));
    bytes.append(static_cast<char>((value >> 16U) & 0xffU));
    bytes.append(static_cast<char>((value >> 8U) & 0xffU));
    bytes.append(static_cast<char>(value & 0xffU));
}

void append_u64(QByteArray& bytes, std::uint64_t value)
{
    for (int shift = 56; shift >= 0; shift -= 8) {
        bytes.append(static_cast<char>((value >> static_cast<unsigned>(shift)) & 0xffU));
    }
}

std::uint32_t read_u32(const char* bytes)
{
    const auto* raw = reinterpret_cast<const unsigned char*>(bytes);
    return (static_cast<std::uint32_t>(raw[0]) << 24U)
        | (static_cast<std::uint32_t>(raw[1]) << 16U)
        | (static_cast<std::uint32_t>(raw[2]) << 8U)
        | static_cast<std::uint32_t>(raw[3]);
}

std::uint64_t read_u64(const char* bytes)
{
    std::uint64_t value = 0;
    const auto* raw = reinterpret_cast<const unsigned char*>(bytes);
    for (int index = 0; index < 8; ++index) {
        value = (value << 8U) | raw[index];
    }
    return value;
}

std::filesystem::path cache_path(const std::filesystem::path& root,
                                 std::string_view key)
{
    return root / (std::string{key} + ".srcache");
}

CacheLookup cache_failure(SystemErrorCode code,
                          std::string_view stage,
                          core::ErrorCategory category,
                          bool retryable)
{
    return CacheLookup{false,
                       true,
                       {},
                       make_storage_error(category, code, stage, retryable)};
}

} // namespace

Result<std::string> sha256_file(const std::filesystem::path& path)
{
    QFile file{path_string(path)};
    if (!file.open(QIODevice::ReadOnly) || file.size() < 0
        || file.size() > maximum_asset_hash_bytes) {
        auto error = make_storage_error(core::ErrorCategory::internal,
                                        SystemErrorCode::io_error,
                                        "asset.hash",
                                        true);
        error.context.emplace("path", path_string(path).toStdString());
        return Result<std::string>::failure(std::move(error));
    }
    QCryptographicHash hash{QCryptographicHash::Sha256};
    std::array<char, 64U * 1024U> buffer{};
    while (!file.atEnd()) {
        const auto read = file.read(buffer.data(), static_cast<qint64>(buffer.size()));
        if (read < 0 || (read == 0 && !file.atEnd())) {
            return Result<std::string>::failure(make_storage_error(
                core::ErrorCategory::internal,
                SystemErrorCode::io_error,
                "asset.hash",
                true));
        }
        hash.addData(QByteArrayView{buffer.data(), read});
    }
    return Result<std::string>::success(hash.result().toHex().toStdString());
}

Result<RelocationResult> relocate_asset(
    const AssetReference& asset,
    const std::vector<std::filesystem::path>& search_roots,
    std::size_t maximum_candidates)
{
    if (!core::validate_identifier(asset.asset_id) || !valid_hex(asset.fingerprint_sha256)
        || maximum_candidates == 0) {
        return Result<RelocationResult>::failure(make_storage_error(
            core::ErrorCategory::validation,
            SystemErrorCode::invalid_request,
            "asset.relocate"));
    }
    std::size_t checked = 0;
    bool fingerprint_mismatch = false;
    const auto inspect = [&](const std::filesystem::path& candidate)
        -> std::optional<Result<RelocationResult>> {
        if (checked >= maximum_candidates) {
            return std::nullopt;
        }
        std::error_code error;
        if (!std::filesystem::is_regular_file(candidate, error) || error) {
            return std::nullopt;
        }
        ++checked;
        const auto size = std::filesystem::file_size(candidate, error);
        if (error || size != asset.size_bytes) {
            if (candidate.filename() == asset.stored_path.filename()) {
                fingerprint_mismatch = true;
            }
            return std::nullopt;
        }
        const auto fingerprint = sha256_file(candidate);
        if (!fingerprint) {
            return std::nullopt;
        }
        if (equal_hex(fingerprint.value(), asset.fingerprint_sha256)) {
            return Result<RelocationResult>::success(RelocationResult{candidate, checked});
        }
        fingerprint_mismatch = true;
        return std::nullopt;
    };

    if (!asset.stored_path.empty()) {
        if (const auto found = inspect(asset.stored_path)) {
            return *found;
        }
    }
    for (const auto& root : search_roots) {
        QDirIterator iterator{path_string(root),
                              QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
                              QDirIterator::Subdirectories};
        while (iterator.hasNext() && checked < maximum_candidates) {
            const auto candidate =
                std::filesystem::path{iterator.next().toStdWString()};
            if (candidate == asset.stored_path) {
                continue;
            }
            if (const auto found = inspect(candidate)) {
                return *found;
            }
        }
    }
    auto error = make_storage_error(core::ErrorCategory::validation,
                                    fingerprint_mismatch
                                        ? SystemErrorCode::fingerprint_mismatch
                                        : SystemErrorCode::asset_not_found,
                                    "asset.relocate");
    error.context.emplace("candidatesChecked", std::to_string(checked));
    return Result<RelocationResult>::failure(std::move(error));
}

RebuildableCache::RebuildableCache(std::filesystem::path root)
    : root_(std::move(root))
{
}

std::string RebuildableCache::make_key(const CacheKeyInput& input)
{
    QByteArray canonical;
    const auto append = [&](std::string_view value) {
        append_u64(canonical, static_cast<std::uint64_t>(value.size()));
        canonical.append(value.data(), static_cast<qsizetype>(value.size()));
    };
    append(input.asset_fingerprint);
    append(input.algorithm_version);
    append(input.parameters_digest);
    append(input.tool_version);
    return sha256(canonical).toHex().toStdString();
}

Result<void> RebuildableCache::put(std::string_view key,
                                   const std::vector<std::uint8_t>& payload,
                                   SaveFault fault) const
{
    if (!valid_hex(key)) {
        return Result<void>::failure(make_storage_error(core::ErrorCategory::validation,
                                                        SystemErrorCode::invalid_request,
                                                        "cache.put"));
    }
    if (fault == SaveFault::disk_full) {
        return Result<void>::failure(make_storage_error(core::ErrorCategory::resource_limit,
                                                        SystemErrorCode::disk_full,
                                                        "cache.put",
                                                        true));
    }
    std::error_code filesystem_error;
    std::filesystem::create_directories(root_, filesystem_error);
    if (filesystem_error) {
        return Result<void>::failure(make_storage_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::io_error,
                                                        "cache.put",
                                                        true));
    }
    QByteArray body;
    body.reserve(static_cast<qsizetype>(cache_magic.size() + 4U + 8U + 32U
                                       + payload.size()));
    body.append(reinterpret_cast<const char*>(cache_magic.data()),
                static_cast<qsizetype>(cache_magic.size()));
    append_u32(body, cache_format_version);
    append_u64(body, static_cast<std::uint64_t>(payload.size()));
    const QByteArray payload_bytes{reinterpret_cast<const char*>(payload.data()),
                                   static_cast<qsizetype>(payload.size())};
    body.append(sha256(payload_bytes));
    body.append(payload_bytes);

    QSaveFile file{path_string(cache_path(root_, key))};
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly)) {
        return Result<void>::failure(make_storage_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::io_error,
                                                        "cache.put",
                                                        true));
    }
    const auto size = fault == SaveFault::partial_write ? body.size() / 2 : body.size();
    if (file.write(body.constData(), size) != size) {
        file.cancelWriting();
        return Result<void>::failure(make_storage_error(core::ErrorCategory::resource_limit,
                                                        SystemErrorCode::disk_full,
                                                        "cache.put",
                                                        true));
    }
    if (fault == SaveFault::partial_write || fault == SaveFault::before_commit) {
        file.cancelWriting();
        return Result<void>::failure(make_storage_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::partial_write,
                                                        "cache.put",
                                                        true));
    }
    if (!file.commit()) {
        return Result<void>::failure(make_storage_error(core::ErrorCategory::internal,
                                                        SystemErrorCode::io_error,
                                                        "cache.put",
                                                        true));
    }
    return Result<void>::success();
}

CacheLookup RebuildableCache::get(std::string_view key) const
{
    if (!valid_hex(key)) {
        return cache_failure(SystemErrorCode::cache_miss,
                             "cache.get",
                             core::ErrorCategory::validation,
                             false);
    }
    const auto path = cache_path(root_, key);
    QFile file{path_string(path)};
    if (!file.open(QIODevice::ReadOnly)) {
        return cache_failure(SystemErrorCode::cache_miss,
                             "cache.get",
                             core::ErrorCategory::internal,
                             true);
    }
    const auto bytes = file.readAll();
    constexpr qsizetype header_size = 8 + 4 + 8 + 32;
    bool valid = bytes.size() >= header_size
        && std::equal(cache_magic.begin(),
                      cache_magic.end(),
                      reinterpret_cast<const std::uint8_t*>(bytes.constData()))
        && read_u32(bytes.constData() + 8) == cache_format_version;
    std::uint64_t payload_size = 0;
    if (valid) {
        payload_size = read_u64(bytes.constData() + 12);
        valid = payload_size <= static_cast<std::uint64_t>(
                                  std::numeric_limits<qsizetype>::max())
            && bytes.size() == header_size + static_cast<qsizetype>(payload_size);
    }
    if (valid) {
        const auto expected = QByteArray{bytes.constData() + 20, 32};
        const auto payload = bytes.sliced(header_size);
        valid = sha256(payload) == expected;
        if (valid) {
            return CacheLookup{
                true,
                false,
                std::vector<std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(payload.constData()),
                    reinterpret_cast<const std::uint8_t*>(payload.constData())
                        + payload.size()),
                std::nullopt};
        }
    }
    file.close();
    QFile::remove(path_string(path));
    return cache_failure(SystemErrorCode::cache_corrupt,
                         "cache.get",
                         core::ErrorCategory::internal,
                         true);
}

Result<std::uint64_t> RebuildableCache::prune(std::uint64_t maximum_bytes) const
{
    QDir directory{path_string(root_)};
    if (!directory.exists()) {
        return Result<std::uint64_t>::success(0);
    }
    auto files = directory.entryInfoList({QStringLiteral("*.srcache")},
                                         QDir::Files | QDir::NoSymLinks,
                                         QDir::Time | QDir::Reversed);
    std::uint64_t total = 0;
    for (const auto& file : files) {
        total += static_cast<std::uint64_t>(std::max<qint64>(file.size(), 0));
    }
    std::uint64_t removed = 0;
    for (const auto& file : files) {
        if (total <= maximum_bytes) {
            break;
        }
        const auto size = static_cast<std::uint64_t>(std::max<qint64>(file.size(), 0));
        if (!QFile::remove(file.absoluteFilePath())) {
            return Result<std::uint64_t>::failure(make_storage_error(
                core::ErrorCategory::internal,
                SystemErrorCode::io_error,
                "cache.prune",
                true));
        }
        total -= size;
        removed += size;
    }
    return Result<std::uint64_t>::success(removed);
}

} // namespace space_rhythm::system
