#include <space_rhythm/system/runtime.hpp>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPointer>
#include <QTimer>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <limits>
#include <map>
#include <ranges>
#include <set>
#include <utility>

namespace space_rhythm::system {
namespace {

std::atomic_uint64_t next_ipc_diagnostic{1};

SystemError make_ipc_error(core::ErrorCategory category,
                           SystemErrorCode code,
                           std::string_view stage,
                           bool retryable = false)
{
    const auto sequence = next_ipc_diagnostic.fetch_add(1, std::memory_order_relaxed);
    return SystemError{category,
                       code,
                       std::string{stage},
                       "ipc:" + std::to_string(sequence),
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

bool valid_hex_digest(std::string_view value)
{
    return value.size() == 64
        && std::ranges::all_of(value, [](const unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f')
                   || (character >= 'A' && character <= 'F');
           });
}

bool valid_reference(const DataReference& reference)
{
    return !reference.locator.empty() && reference.locator.size() <= 2'048
        && valid_hex_digest(reference.sha256);
}

QString message_type_token(MessageType type)
{
    switch (type) {
    case MessageType::handshake:
        return QStringLiteral("handshake");
    case MessageType::handshake_acknowledged:
        return QStringLiteral("handshakeAcknowledged");
    case MessageType::submit_job:
        return QStringLiteral("submitJob");
    case MessageType::cancel_job:
        return QStringLiteral("cancelJob");
    case MessageType::job_update:
        return QStringLiteral("jobUpdate");
    case MessageType::protocol_error:
        return QStringLiteral("protocolError");
    }
    return QStringLiteral("protocolError");
}

std::optional<MessageType> parse_message_type(const QString& token)
{
    if (token == QStringLiteral("handshake")) {
        return MessageType::handshake;
    }
    if (token == QStringLiteral("handshakeAcknowledged")) {
        return MessageType::handshake_acknowledged;
    }
    if (token == QStringLiteral("submitJob")) {
        return MessageType::submit_job;
    }
    if (token == QStringLiteral("cancelJob")) {
        return MessageType::cancel_job;
    }
    if (token == QStringLiteral("jobUpdate")) {
        return MessageType::job_update;
    }
    if (token == QStringLiteral("protocolError")) {
        return MessageType::protocol_error;
    }
    return std::nullopt;
}

QString category_token(core::ErrorCategory category)
{
    return QString::fromUtf8(core::to_string(category).data(),
                             static_cast<qsizetype>(core::to_string(category).size()));
}

std::optional<core::ErrorCategory> parse_category(const QString& token)
{
    if (token == QStringLiteral("validation")) {
        return core::ErrorCategory::validation;
    }
    if (token == QStringLiteral("conflict")) {
        return core::ErrorCategory::conflict;
    }
    if (token == QStringLiteral("cancelled")) {
        return core::ErrorCategory::cancelled;
    }
    if (token == QStringLiteral("compatibility")) {
        return core::ErrorCategory::compatibility;
    }
    if (token == QStringLiteral("resource_limit")) {
        return core::ErrorCategory::resource_limit;
    }
    if (token == QStringLiteral("internal")) {
        return core::ErrorCategory::internal;
    }
    return std::nullopt;
}

std::optional<SystemErrorCode> parse_error_code(const QString& token)
{
    constexpr std::array codes{SystemErrorCode::invalid_request,
                               SystemErrorCode::invalid_transition,
                               SystemErrorCode::request_conflict,
                               SystemErrorCode::message_conflict,
                               SystemErrorCode::out_of_order_message,
                               SystemErrorCode::protocol_version_mismatch,
                               SystemErrorCode::message_too_large,
                               SystemErrorCode::forbidden_inline_data,
                               SystemErrorCode::request_timeout,
                               SystemErrorCode::worker_crashed,
                               SystemErrorCode::stale_revision,
                               SystemErrorCode::cancelled,
                               SystemErrorCode::unsupported_project_schema,
                               SystemErrorCode::invalid_project,
                               SystemErrorCode::corrupt_project,
                               SystemErrorCode::disk_full,
                               SystemErrorCode::partial_write,
                               SystemErrorCode::io_error,
                               SystemErrorCode::asset_not_found,
                               SystemErrorCode::fingerprint_mismatch,
                               SystemErrorCode::cache_corrupt,
                               SystemErrorCode::cache_miss};
    for (const auto code : codes) {
        if (token == QString::fromUtf8(to_string(code).data(),
                                      static_cast<qsizetype>(to_string(code).size()))) {
            return code;
        }
    }
    return std::nullopt;
}

QJsonObject reference_to_json(const DataReference& reference)
{
    return {{QStringLiteral("kind"),
             reference.kind == DataReferenceKind::file ? QStringLiteral("file")
                                                       : QStringLiteral("cache")},
            {QStringLiteral("locator"), QString::fromStdString(reference.locator)},
            {QStringLiteral("byteLength"),
             QString::number(static_cast<qulonglong>(reference.byte_length))},
            {QStringLiteral("sha256"), QString::fromStdString(reference.sha256)}};
}

std::optional<DataReference> reference_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    const auto kind = object.value(QStringLiteral("kind")).toString();
    if (kind != QStringLiteral("file") && kind != QStringLiteral("cache")) {
        return std::nullopt;
    }
    bool length_ok = false;
    const auto byte_length = object.value(QStringLiteral("byteLength"))
                                 .toString()
                                 .toULongLong(&length_ok);
    DataReference reference{kind == QStringLiteral("file") ? DataReferenceKind::file
                                                            : DataReferenceKind::cache,
                            object.value(QStringLiteral("locator")).toString().toStdString(),
                            static_cast<std::uint64_t>(byte_length),
                            object.value(QStringLiteral("sha256")).toString().toStdString()};
    if (!length_ok || !valid_reference(reference)) {
        return std::nullopt;
    }
    return reference;
}

QJsonObject error_to_json(const SystemError& error)
{
    QJsonObject context;
    for (const auto& [key, value] : error.context) {
        context.insert(QString::fromStdString(key), QString::fromStdString(value));
    }
    return {{QStringLiteral("category"), category_token(error.category)},
            {QStringLiteral("code"), QString::fromUtf8(to_string(error.code).data())},
            {QStringLiteral("stage"), QString::fromStdString(error.stage)},
            {QStringLiteral("diagnosticId"), QString::fromStdString(error.diagnostic_id)},
            {QStringLiteral("retryable"), error.retryable},
            {QStringLiteral("messageKey"), QString::fromStdString(error.message_key)},
            {QStringLiteral("context"), context}};
}

std::optional<SystemError> error_from_json(const QJsonValue& value)
{
    if (!value.isObject()) {
        return std::nullopt;
    }
    const auto object = value.toObject();
    const auto category = parse_category(object.value(QStringLiteral("category")).toString());
    const auto code = parse_error_code(object.value(QStringLiteral("code")).toString());
    if (!category || !code) {
        return std::nullopt;
    }
    SystemError error{*category,
                      *code,
                      object.value(QStringLiteral("stage")).toString().toStdString(),
                      object.value(QStringLiteral("diagnosticId")).toString().toStdString(),
                      object.value(QStringLiteral("retryable")).toBool(),
                      object.value(QStringLiteral("messageKey")).toString().toStdString(),
                      {}};
    const auto context = object.value(QStringLiteral("context")).toObject();
    for (auto iterator = context.constBegin(); iterator != context.constEnd(); ++iterator) {
        if (!iterator.value().isString()) {
            return std::nullopt;
        }
        error.context.emplace(iterator.key().toStdString(), iterator.value().toString().toStdString());
    }
    if (!valid_token(error.diagnostic_id) || error.stage.empty() || error.message_key.empty()) {
        return std::nullopt;
    }
    return error;
}

QJsonObject envelope_to_json(const ProtocolEnvelope& envelope)
{
    QJsonObject payload;
    for (const auto& [key, value] : envelope.payload) {
        payload.insert(QString::fromStdString(key), QString::fromStdString(value));
    }
    QJsonArray references;
    for (const auto& reference : envelope.references) {
        references.append(reference_to_json(reference));
    }
    QJsonObject object{{QStringLiteral("schemaVersion"),
                        static_cast<qint64>(envelope.schema_version)},
                       {QStringLiteral("protocolVersion"),
                        static_cast<qint64>(envelope.protocol_version)},
                       {QStringLiteral("messageId"), QString::fromStdString(envelope.message_id)},
                       {QStringLiteral("requestId"), QString::fromStdString(envelope.request_id)},
                       {QStringLiteral("sequence"),
                        QString::number(static_cast<qulonglong>(envelope.sequence))},
                       {QStringLiteral("type"), message_type_token(envelope.type)},
                       {QStringLiteral("payload"), payload},
                       {QStringLiteral("references"), references}};
    if (envelope.job_id) {
        object.insert(QStringLiteral("jobId"), QString::fromStdString(*envelope.job_id));
    }
    if (envelope.error) {
        object.insert(QStringLiteral("error"), error_to_json(*envelope.error));
    }
    return object;
}

std::uint32_t read_be32(const std::uint8_t* bytes)
{
    return (static_cast<std::uint32_t>(bytes[0]) << 24U)
        | (static_cast<std::uint32_t>(bytes[1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[2]) << 8U)
        | static_cast<std::uint32_t>(bytes[3]);
}

std::array<std::uint8_t, 4> write_be32(std::uint32_t value)
{
    return {static_cast<std::uint8_t>((value >> 24U) & 0xffU),
            static_cast<std::uint8_t>((value >> 16U) & 0xffU),
            static_cast<std::uint8_t>((value >> 8U) & 0xffU),
            static_cast<std::uint8_t>(value & 0xffU)};
}

constexpr std::array<std::string_view, 10> forbidden_payload_keys{
    "frameData", "frameBytes", "frames", "pcmData", "pcmBytes",
    "pcm",       "samples",    "binaryData", "blob",    "pixels"};

Result<ProtocolEnvelope> protocol_failure(SystemErrorCode code,
                                          std::string_view stage,
                                          core::ErrorCategory category =
                                              core::ErrorCategory::validation)
{
    return Result<ProtocolEnvelope>::failure(make_ipc_error(category, code, stage));
}

void send_envelope(QLocalSocket* socket, const ProtocolEnvelope& envelope)
{
    const auto encoded = encode_protocol_frame(envelope);
    if (!encoded) {
        return;
    }
    const auto& frame = encoded.value();
    socket->write(reinterpret_cast<const char*>(frame.data()),
                  static_cast<qint64>(frame.size()));
    socket->flush();
}

ProtocolEnvelope mock_update(const ProtocolEnvelope& request,
                             std::uint64_t sequence,
                             std::string_view state)
{
    return ProtocolEnvelope{ipc_schema_version,
                            ipc_protocol_version,
                            request.message_id + ":" + std::to_string(sequence),
                            request.request_id,
                            request.job_id,
                            sequence,
                            MessageType::job_update,
                            {{"state", std::string{state}}},
                            {},
                            std::nullopt};
}

std::string mock_request_fingerprint(const ProtocolEnvelope& request)
{
    std::string fingerprint;
    const auto append = [&](std::string_view value) {
        fingerprint += std::to_string(value.size());
        fingerprint.push_back(':');
        fingerprint.append(value);
        fingerprint.push_back(';');
    };
    append(request.job_id.value_or(std::string{}));
    for (const auto& [key, value] : request.payload) {
        append(key);
        append(value);
    }
    for (const auto& reference : request.references) {
        append(reference.kind == DataReferenceKind::file ? "file" : "cache");
        append(reference.locator);
        append(std::to_string(reference.byte_length));
        append(reference.sha256);
    }
    return fingerprint;
}

} // namespace

Result<ProtocolEnvelope> validate_protocol_envelope(ProtocolEnvelope envelope)
{
    if (envelope.schema_version != ipc_schema_version
        || envelope.protocol_version != ipc_protocol_version) {
        return protocol_failure(SystemErrorCode::protocol_version_mismatch,
                                "ipc.version",
                                core::ErrorCategory::compatibility);
    }
    if (!valid_token(envelope.message_id) || !valid_token(envelope.request_id)
        || (envelope.job_id && !valid_token(*envelope.job_id))
        || envelope.payload.size() > 128 || envelope.references.size() > 128) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.envelope");
    }
    std::size_t payload_bytes = 0;
    for (const auto& [key, value] : envelope.payload) {
        if (!valid_token(key) || value.size() > 4'096) {
            return protocol_failure(SystemErrorCode::invalid_request, "ipc.payload");
        }
        if (std::ranges::find(forbidden_payload_keys, key)
            != forbidden_payload_keys.end()) {
            return protocol_failure(SystemErrorCode::forbidden_inline_data,
                                    "ipc.payload");
        }
        payload_bytes += key.size() + value.size();
        if (payload_bytes > max_ipc_frame_bytes / 2U) {
            return protocol_failure(SystemErrorCode::message_too_large, "ipc.payload");
        }
    }
    if (!std::ranges::all_of(envelope.references, valid_reference)) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.reference");
    }
    if (envelope.error
        && (!valid_token(envelope.error->diagnostic_id) || envelope.error->stage.empty()
            || envelope.error->message_key.empty())) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.error");
    }
    const bool command_shape =
        (envelope.type == MessageType::handshake
         || envelope.type == MessageType::handshake_acknowledged)
        && !envelope.job_id && envelope.sequence == 0 && !envelope.error;
    const bool request_shape =
        (envelope.type == MessageType::submit_job
         || envelope.type == MessageType::cancel_job)
        && envelope.job_id && envelope.sequence == 0 && !envelope.error;
    const bool update_shape = envelope.type == MessageType::job_update
        && envelope.job_id && envelope.sequence > 0;
    const bool error_shape = envelope.type == MessageType::protocol_error
        && envelope.sequence == 0 && envelope.error.has_value();
    if (!command_shape && !request_shape && !update_shape && !error_shape) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.message_shape");
    }
    return Result<ProtocolEnvelope>::success(std::move(envelope));
}

Result<std::vector<std::uint8_t>> encode_protocol_frame(
    const ProtocolEnvelope& envelope)
{
    const auto validated = validate_protocol_envelope(envelope);
    if (!validated) {
        return Result<std::vector<std::uint8_t>>::failure(validated.error());
    }
    const auto body = QJsonDocument{envelope_to_json(validated.value())}
                          .toJson(QJsonDocument::Compact);
    if (body.isEmpty() || static_cast<std::size_t>(body.size()) > max_ipc_frame_bytes) {
        return Result<std::vector<std::uint8_t>>::failure(make_ipc_error(
            core::ErrorCategory::resource_limit,
            SystemErrorCode::message_too_large,
            "ipc.encode"));
    }
    const auto prefix = write_be32(static_cast<std::uint32_t>(body.size()));
    std::vector<std::uint8_t> frame;
    frame.reserve(prefix.size() + static_cast<std::size_t>(body.size()));
    frame.insert(frame.end(), prefix.begin(), prefix.end());
    frame.insert(frame.end(), reinterpret_cast<const std::uint8_t*>(body.constData()),
                 reinterpret_cast<const std::uint8_t*>(body.constData()) + body.size());
    return Result<std::vector<std::uint8_t>>::success(std::move(frame));
}

Result<ProtocolEnvelope> decode_protocol_frame(const std::vector<std::uint8_t>& frame)
{
    if (frame.size() < 4U) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.frame");
    }
    const auto body_size = read_be32(frame.data());
    if (body_size == 0 || body_size > max_ipc_frame_bytes) {
        return protocol_failure(SystemErrorCode::message_too_large,
                                "ipc.frame",
                                core::ErrorCategory::resource_limit);
    }
    if (frame.size() != static_cast<std::size_t>(body_size) + 4U) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.frame");
    }
    QJsonParseError parse_error;
    const auto document = QJsonDocument::fromJson(
        QByteArray{reinterpret_cast<const char*>(frame.data() + 4U),
                   static_cast<qsizetype>(body_size)},
        &parse_error);
    if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.json");
    }
    const auto object = document.object();
    const auto type = parse_message_type(object.value(QStringLiteral("type")).toString());
    bool sequence_ok = false;
    const auto sequence = object.value(QStringLiteral("sequence"))
                              .toString()
                              .toULongLong(&sequence_ok);
    if (!type || !sequence_ok) {
        return protocol_failure(SystemErrorCode::invalid_request, "ipc.json");
    }
    ProtocolEnvelope envelope;
    envelope.schema_version = static_cast<std::uint32_t>(
        object.value(QStringLiteral("schemaVersion")).toInteger());
    envelope.protocol_version = static_cast<std::uint32_t>(
        object.value(QStringLiteral("protocolVersion")).toInteger());
    envelope.message_id = object.value(QStringLiteral("messageId")).toString().toStdString();
    envelope.request_id = object.value(QStringLiteral("requestId")).toString().toStdString();
    if (object.contains(QStringLiteral("jobId"))) {
        envelope.job_id = object.value(QStringLiteral("jobId")).toString().toStdString();
    }
    envelope.sequence = static_cast<std::uint64_t>(sequence);
    envelope.type = *type;
    const auto payload = object.value(QStringLiteral("payload")).toObject();
    for (auto iterator = payload.constBegin(); iterator != payload.constEnd(); ++iterator) {
        if (!iterator.value().isString()) {
            return protocol_failure(SystemErrorCode::invalid_request, "ipc.payload");
        }
        envelope.payload.emplace(iterator.key().toStdString(),
                                 iterator.value().toString().toStdString());
    }
    const auto references = object.value(QStringLiteral("references")).toArray();
    for (const auto& item : references) {
        const auto reference = reference_from_json(item);
        if (!reference) {
            return protocol_failure(SystemErrorCode::invalid_request, "ipc.reference");
        }
        envelope.references.push_back(*reference);
    }
    if (object.contains(QStringLiteral("error"))) {
        envelope.error = error_from_json(object.value(QStringLiteral("error")));
        if (!envelope.error) {
            return protocol_failure(SystemErrorCode::invalid_request, "ipc.error");
        }
    }
    return validate_protocol_envelope(std::move(envelope));
}

class LocalIpcClient::Impl final {
public:
    QLocalSocket socket;
    QByteArray receive_buffer;
};

LocalIpcClient::LocalIpcClient()
    : impl_(std::make_unique<Impl>())
{
}

LocalIpcClient::~LocalIpcClient() = default;

Result<void> LocalIpcClient::connect_to(std::string_view server_name,
                                        std::uint32_t timeout_ms)
{
    if (server_name.empty() || timeout_ms == 0) {
        return Result<void>::failure(make_ipc_error(core::ErrorCategory::validation,
                                                    SystemErrorCode::invalid_request,
                                                    "ipc.connect"));
    }
    impl_->socket.abort();
    impl_->receive_buffer.clear();
    impl_->socket.connectToServer(QString::fromUtf8(server_name.data(),
                                                    static_cast<qsizetype>(server_name.size())));
    if (!impl_->socket.waitForConnected(static_cast<int>(std::min<std::uint32_t>(
            timeout_ms, static_cast<std::uint32_t>(std::numeric_limits<int>::max()))))) {
        auto error = make_ipc_error(core::ErrorCategory::internal,
                                    SystemErrorCode::worker_crashed,
                                    "ipc.connect",
                                    true);
        error.context.emplace("socketError", impl_->socket.errorString().toStdString());
        return Result<void>::failure(std::move(error));
    }
    return Result<void>::success();
}

Result<void> LocalIpcClient::send(const ProtocolEnvelope& envelope)
{
    const auto encoded = encode_protocol_frame(envelope);
    if (!encoded) {
        return Result<void>::failure(encoded.error());
    }
    if (impl_->socket.state() != QLocalSocket::ConnectedState) {
        return Result<void>::failure(make_ipc_error(core::ErrorCategory::internal,
                                                    SystemErrorCode::worker_crashed,
                                                    "ipc.send",
                                                    true));
    }
    const auto& frame = encoded.value();
    const auto written = impl_->socket.write(reinterpret_cast<const char*>(frame.data()),
                                             static_cast<qint64>(frame.size()));
    if (written != static_cast<qint64>(frame.size())
        || !impl_->socket.waitForBytesWritten(5'000)) {
        auto error = make_ipc_error(core::ErrorCategory::internal,
                                    SystemErrorCode::io_error,
                                    "ipc.send",
                                    true);
        error.context.emplace("socketError", impl_->socket.errorString().toStdString());
        return Result<void>::failure(std::move(error));
    }
    return Result<void>::success();
}

Result<ProtocolEnvelope> LocalIpcClient::receive(std::uint32_t timeout_ms)
{
    QElapsedTimer timer;
    timer.start();
    const auto remaining = [&]() {
        const auto elapsed = static_cast<std::uint64_t>(timer.elapsed());
        return elapsed >= timeout_ms ? 0
                                     : static_cast<int>(std::min<std::uint64_t>(
                                           timeout_ms - elapsed,
                                           static_cast<std::uint64_t>(
                                               std::numeric_limits<int>::max())));
    };
    while (true) {
        if (impl_->receive_buffer.size() >= 4) {
            const auto body_size = read_be32(reinterpret_cast<const std::uint8_t*>(
                impl_->receive_buffer.constData()));
            if (body_size == 0 || body_size > max_ipc_frame_bytes) {
                impl_->receive_buffer.clear();
                return protocol_failure(SystemErrorCode::message_too_large,
                                        "ipc.receive",
                                        core::ErrorCategory::resource_limit);
            }
            const auto frame_size = static_cast<qsizetype>(body_size) + 4;
            if (impl_->receive_buffer.size() >= frame_size) {
                const auto bytes = impl_->receive_buffer.first(frame_size);
                impl_->receive_buffer.remove(0, frame_size);
                return decode_protocol_frame(std::vector<std::uint8_t>(
                    reinterpret_cast<const std::uint8_t*>(bytes.constData()),
                    reinterpret_cast<const std::uint8_t*>(bytes.constData())
                        + bytes.size()));
            }
        }
        impl_->receive_buffer.append(impl_->socket.readAll());
        if ((impl_->receive_buffer.size() < 4
             || (impl_->receive_buffer.size() >= 4
                 && impl_->receive_buffer.size()
                     < static_cast<qsizetype>(read_be32(
                           reinterpret_cast<const std::uint8_t*>(
                               impl_->receive_buffer.constData())))
                         + 4))
            && remaining() > 0 && impl_->socket.waitForReadyRead(remaining())) {
            continue;
        }
        impl_->receive_buffer.append(impl_->socket.readAll());
        if (remaining() == 0) {
            return protocol_failure(SystemErrorCode::request_timeout,
                                    "ipc.receive",
                                    core::ErrorCategory::resource_limit);
        }
        if (impl_->socket.state() != QLocalSocket::ConnectedState) {
            return protocol_failure(SystemErrorCode::worker_crashed,
                                    "ipc.receive",
                                    core::ErrorCategory::internal);
        }
    }
}

void LocalIpcClient::close()
{
    impl_->socket.disconnectFromServer();
    if (impl_->socket.state() != QLocalSocket::UnconnectedState) {
        impl_->socket.waitForDisconnected(1'000);
    }
    impl_->receive_buffer.clear();
}

int run_mock_worker(std::string_view server_name)
{
    if (server_name.empty() || QCoreApplication::instance() == nullptr) {
        return 2;
    }
    auto* application = QCoreApplication::instance();
    auto* server = new QLocalServer{application};
    server->setSocketOptions(QLocalServer::UserAccessOption);
    const auto name = QString::fromUtf8(server_name.data(),
                                        static_cast<qsizetype>(server_name.size()));
    QLocalServer::removeServer(name);
    if (!server->listen(name)) {
        return 3;
    }
    std::fprintf(stdout, "SPACE_RHYTHM_MOCK_WORKER_READY %s\n", name.toUtf8().constData());
    std::fflush(stdout);

    QObject::connect(server, &QLocalServer::newConnection, application, [server]() {
        while (auto* socket = server->nextPendingConnection()) {
            auto* buffer = new QByteArray;
            auto* sequences = new std::map<std::string, std::uint64_t>;
            auto* request_fingerprints = new std::map<std::string, std::string>;
            auto* negotiated = new bool{false};
            QObject::connect(socket, &QLocalSocket::disconnected, socket,
                             [buffer,
                              sequences,
                              request_fingerprints,
                              negotiated,
                              socket]() {
                                 delete buffer;
                                 delete sequences;
                                 delete request_fingerprints;
                                 delete negotiated;
                                 socket->deleteLater();
                             });
            QObject::connect(socket, &QLocalSocket::readyRead, socket,
                             [socket,
                              buffer,
                              sequences,
                              request_fingerprints,
                              negotiated]() {
                buffer->append(socket->readAll());
                while (buffer->size() >= 4) {
                    const auto body_size = read_be32(
                        reinterpret_cast<const std::uint8_t*>(buffer->constData()));
                    if (body_size == 0 || body_size > max_ipc_frame_bytes) {
                        socket->abort();
                        return;
                    }
                    const auto frame_size = static_cast<qsizetype>(body_size) + 4;
                    if (buffer->size() < frame_size) {
                        return;
                    }
                    const auto raw = buffer->first(frame_size);
                    buffer->remove(0, frame_size);
                    const auto decoded = decode_protocol_frame(std::vector<std::uint8_t>(
                        reinterpret_cast<const std::uint8_t*>(raw.constData()),
                        reinterpret_cast<const std::uint8_t*>(raw.constData())
                            + raw.size()));
                    if (!decoded) {
                        ProtocolEnvelope response{ipc_schema_version,
                                                  ipc_protocol_version,
                                                  "mock:error",
                                                  "mock:error",
                                                  std::nullopt,
                                                  0,
                                                  MessageType::protocol_error,
                                                  {},
                                                  {},
                                                  decoded.error()};
                        send_envelope(socket, response);
                        continue;
                    }
                    const auto request = decoded.value();
                    if (request.type == MessageType::handshake) {
                        bool min_ok = false;
                        bool max_ok = false;
                        const auto minimum = QString::fromStdString(
                            request.payload.contains("minVersion")
                                ? request.payload.at("minVersion")
                                : std::string{})
                                                 .toUInt(&min_ok);
                        const auto maximum = QString::fromStdString(
                            request.payload.contains("maxVersion")
                                ? request.payload.at("maxVersion")
                                : std::string{})
                                                 .toUInt(&max_ok);
                        if (!min_ok || !max_ok || minimum > ipc_protocol_version
                            || maximum < ipc_protocol_version) {
                            auto error = make_ipc_error(core::ErrorCategory::compatibility,
                                                        SystemErrorCode::protocol_version_mismatch,
                                                        "worker.handshake");
                            send_envelope(socket,
                                          ProtocolEnvelope{ipc_schema_version,
                                                           ipc_protocol_version,
                                                           request.message_id + ":error",
                                                           request.request_id,
                                                           std::nullopt,
                                                           0,
                                                           MessageType::protocol_error,
                                                           {},
                                                           {},
                                                           std::move(error)});
                        } else {
                            *negotiated = true;
                            send_envelope(socket,
                                          ProtocolEnvelope{ipc_schema_version,
                                                           ipc_protocol_version,
                                                           request.message_id + ":ack",
                                                           request.request_id,
                                                           std::nullopt,
                                                           0,
                                                           MessageType::handshake_acknowledged,
                                                           {{"selectedVersion", "1"},
                                                            {"worker", "mock"}},
                                                           {},
                                                           std::nullopt});
                        }
                        continue;
                    }
                    if (!*negotiated) {
                        auto error = make_ipc_error(core::ErrorCategory::compatibility,
                                                    SystemErrorCode::protocol_version_mismatch,
                                                    "worker.handshake_required");
                        send_envelope(socket,
                                      ProtocolEnvelope{ipc_schema_version,
                                                       ipc_protocol_version,
                                                       request.message_id + ":error",
                                                       request.request_id,
                                                       request.job_id,
                                                       0,
                                                       MessageType::protocol_error,
                                                       {},
                                                       {},
                                                       std::move(error)});
                        continue;
                    }
                    if (request.type == MessageType::submit_job && request.job_id) {
                        const auto operation = request.payload.contains("operation")
                            ? request.payload.at("operation")
                            : std::string{};
                        const auto fingerprint = mock_request_fingerprint(request);
                        if (const auto known = request_fingerprints->find(request.request_id);
                            known != request_fingerprints->end()
                            && known->second != fingerprint) {
                            auto error = make_ipc_error(core::ErrorCategory::conflict,
                                                        SystemErrorCode::request_conflict,
                                                        "worker.idempotency");
                            send_envelope(socket,
                                          ProtocolEnvelope{ipc_schema_version,
                                                           ipc_protocol_version,
                                                           request.message_id + ":conflict",
                                                           request.request_id,
                                                           request.job_id,
                                                           0,
                                                           MessageType::protocol_error,
                                                           {},
                                                           {},
                                                           std::move(error)});
                            continue;
                        }
                        request_fingerprints->emplace(request.request_id, fingerprint);
                        (*sequences)[request.request_id] = 1;
                        send_envelope(socket, mock_update(request, 1, "accepted"));
                        if (operation == "crash") {
                            QTimer::singleShot(25, []() { QCoreApplication::exit(86); });
                        } else if (operation == "timeout" || operation == "delay") {
                            continue;
                        } else {
                            auto progress = mock_update(request, 2, "progress");
                            progress.payload.emplace("progressPpm", "500000");
                            send_envelope(socket, progress);
                            auto success = mock_update(request, 3, "succeeded");
                            success.payload.emplace(
                                "baseRevision",
                                request.payload.contains("baseRevision")
                                    ? request.payload.at("baseRevision")
                                    : std::string{"0"});
                            success.references.push_back(DataReference{
                                DataReferenceKind::cache,
                                "mock/output.bin",
                                16,
                                std::string(64, '0')});
                            send_envelope(socket, success);
                            (*sequences)[request.request_id] = 3;
                        }
                        continue;
                    }
                    if (request.type == MessageType::cancel_job && request.job_id) {
                        auto& sequence = (*sequences)[request.request_id];
                        const auto acknowledged = sequence >= 3 ? 2 : ++sequence;
                        send_envelope(socket,
                                      mock_update(request,
                                                  acknowledged,
                                                  "cancellationAcknowledged"));
                        const auto cancelled = sequence >= 3 ? 3 : ++sequence;
                        send_envelope(socket, mock_update(request, cancelled, "cancelled"));
                    }
                }
            });
        }
    });
    return application->exec();
}

} // namespace space_rhythm::system
