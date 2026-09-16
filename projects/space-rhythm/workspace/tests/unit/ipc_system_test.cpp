#include <gtest/gtest.h>

#include <space_rhythm/system/runtime.hpp>

#include <QCoreApplication>
#include <QProcess>
#include <QThread>
#include <QUuid>

#include <array>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

using namespace space_rhythm;

QCoreApplication& application()
{
    static int argument_count = 1;
    static std::array<char, 24> executable_name{
        's', 'p', 'a', 'c', 'e', '-', 'r', 'h', 'y', 't', 'h', 'm', '-', 't', 'e', 's', 't'};
    static char* arguments[] = {executable_name.data(), nullptr};
    static QCoreApplication instance{argument_count, arguments};
    return instance;
}

system::ProtocolEnvelope envelope(system::MessageType type,
                                  std::string message_id = "message-1",
                                  std::string request_id = "request-1")
{
    return system::ProtocolEnvelope{system::ipc_schema_version,
                                    system::ipc_protocol_version,
                                    std::move(message_id),
                                    std::move(request_id),
                                    std::nullopt,
                                    0,
                                    type,
                                    {},
                                    {},
                                    std::nullopt};
}

class MockWorkerProcess final {
public:
    MockWorkerProcess()
    {
        static_cast<void>(application());
        server_name_ =
            QStringLiteral("space-rhythm-test-")
                + QUuid::createUuid().toString(QUuid::WithoutBraces);
        process_.setProgram(QString::fromUtf8(SPACE_RHYTHM_WORKER_EXE));
        process_.setArguments({QStringLiteral("--mock-worker"), server_name_});
        process_.setProcessChannelMode(QProcess::MergedChannels);
        bool started = false;
        for (int attempt = 0; attempt < 10 && !started; ++attempt) {
            process_.start();
            started = process_.waitForStarted(5'000);
            if (!started) {
                process_.close();
                QThread::msleep(100);
            }
        }
        EXPECT_TRUE(started) << process_.errorString().toStdString();
        QByteArray output;
        for (int attempt = 0; attempt < 20
             && !output.contains("SPACE_RHYTHM_MOCK_WORKER_READY");
             ++attempt) {
            process_.waitForReadyRead(250);
            output.append(process_.readAll());
        }
        ready_ = started && output.contains("SPACE_RHYTHM_MOCK_WORKER_READY");
        EXPECT_TRUE(ready_)
            << output.toStdString();
    }

    ~MockWorkerProcess()
    {
        if (process_.state() != QProcess::NotRunning) {
            process_.kill();
            process_.waitForFinished(5'000);
        }
    }

    [[nodiscard]] std::string server_name() const
    {
        return server_name_.toStdString();
    }

    [[nodiscard]] QProcess& process() { return process_; }
    [[nodiscard]] bool ready() const noexcept { return ready_; }

private:
    QString server_name_;
    QProcess process_;
    bool ready_{false};
};

TEST(VersionedIpc, RoundTripsStructuredEnvelopeAndRejectsInlineBulkData)
{
    auto original = envelope(system::MessageType::job_update);
    original.job_id = "job-1";
    original.sequence = std::numeric_limits<std::uint64_t>::max();
    original.payload = {{"state", "failed"}};
    original.references.push_back(system::DataReference{
        system::DataReferenceKind::file,
        "C:/scratch/result.bin",
        std::numeric_limits<std::uint64_t>::max(),
        std::string(64, 'f')});
    original.error = system::SystemError{core::ErrorCategory::resource_limit,
                                         system::SystemErrorCode::request_timeout,
                                         "worker.execute",
                                         "worker:123",
                                         true,
                                         "request_timeout",
                                         {{"operation", "analyze"}}};

    const auto encoded = system::encode_protocol_frame(original);
    ASSERT_TRUE(encoded);
    ASSERT_GE(encoded.value().size(), 5U);
    const auto decoded = system::decode_protocol_frame(encoded.value());
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded.value(), original);

    auto inline_frame = envelope(system::MessageType::submit_job);
    inline_frame.payload.emplace("frameData", "base64-data");
    const auto rejected_frame = system::encode_protocol_frame(inline_frame);
    ASSERT_FALSE(rejected_frame);
    EXPECT_EQ(rejected_frame.error().code,
              system::SystemErrorCode::forbidden_inline_data);

    auto inline_pcm = envelope(system::MessageType::submit_job);
    inline_pcm.payload.emplace("pcmBytes", "raw-data");
    const auto rejected_pcm = system::encode_protocol_frame(inline_pcm);
    ASSERT_FALSE(rejected_pcm);
    EXPECT_EQ(rejected_pcm.error().code,
              system::SystemErrorCode::forbidden_inline_data);

    auto oversized = envelope(system::MessageType::submit_job);
    for (int index = 0; index < 10; ++index) {
        oversized.payload.emplace("option-" + std::to_string(index),
                                  std::string(4'096, 'x'));
    }
    const auto rejected_size = system::encode_protocol_frame(oversized);
    ASSERT_FALSE(rejected_size);
    EXPECT_EQ(rejected_size.error().code,
              system::SystemErrorCode::message_too_large);
}

TEST(VersionedIpc, RejectsMalformedLengthAndProtocolVersion)
{
    auto wrong_version = envelope(system::MessageType::handshake);
    wrong_version.protocol_version = system::ipc_protocol_version + 1;
    const auto rejected = system::validate_protocol_envelope(wrong_version);
    ASSERT_FALSE(rejected);
    EXPECT_EQ(rejected.error().code,
              system::SystemErrorCode::protocol_version_mismatch);

    const std::vector<std::uint8_t> truncated{0, 0, 0, 5, '{', '}'};
    const auto malformed = system::decode_protocol_frame(truncated);
    ASSERT_FALSE(malformed);
    EXPECT_EQ(malformed.error().code, system::SystemErrorCode::invalid_request);
}

TEST(MockWorkerIpc, PerformsHandshakeProgressAndOutOfBandResultReference)
{
    MockWorkerProcess worker;
    ASSERT_TRUE(worker.ready());
    system::LocalIpcClient client;
    ASSERT_TRUE(client.connect_to(worker.server_name(), 5'000));

    auto handshake = envelope(system::MessageType::handshake, "handshake-1", "session-1");
    handshake.payload = {{"minVersion", "1"}, {"maxVersion", "1"}};
    ASSERT_TRUE(client.send(handshake));
    const auto acknowledged = client.receive(5'000);
    ASSERT_TRUE(acknowledged);
    EXPECT_EQ(acknowledged.value().type,
              system::MessageType::handshake_acknowledged);
    EXPECT_EQ(acknowledged.value().payload.at("selectedVersion"), "1");

    auto request = envelope(system::MessageType::submit_job, "submit-1", "request-echo");
    request.job_id = "job-echo";
    request.payload = {{"operation", "echo"}, {"baseRevision", "19"}};
    request.references.push_back(system::DataReference{
        system::DataReferenceKind::file,
        "C:/scratch/input.media",
        123,
        std::string(64, '1')});
    ASSERT_TRUE(client.send(request));

    const auto accepted = client.receive(5'000);
    const auto progress = client.receive(5'000);
    const auto succeeded = client.receive(5'000);
    ASSERT_TRUE(accepted);
    ASSERT_TRUE(progress);
    ASSERT_TRUE(succeeded);
    EXPECT_EQ(accepted.value().payload.at("state"), "accepted");
    EXPECT_EQ(progress.value().payload.at("state"), "progress");
    EXPECT_EQ(progress.value().payload.at("progressPpm"), "500000");
    EXPECT_EQ(succeeded.value().payload.at("state"), "succeeded");
    EXPECT_EQ(succeeded.value().payload.at("baseRevision"), "19");
    ASSERT_EQ(succeeded.value().references.size(), 1U);
    EXPECT_EQ(succeeded.value().references.front().kind,
              system::DataReferenceKind::cache);
    client.close();
}

TEST(MockWorkerIpc, NegotiatesVersionAndHandlesIdempotentCancellation)
{
    MockWorkerProcess worker;
    ASSERT_TRUE(worker.ready());
    system::LocalIpcClient client;
    ASSERT_TRUE(client.connect_to(worker.server_name(), 5'000));

    auto incompatible = envelope(system::MessageType::handshake,
                                 "handshake-incompatible",
                                 "session-incompatible");
    incompatible.payload = {{"minVersion", "2"}, {"maxVersion", "2"}};
    ASSERT_TRUE(client.send(incompatible));
    const auto protocol_error = client.receive(5'000);
    ASSERT_TRUE(protocol_error);
    EXPECT_EQ(protocol_error.value().type, system::MessageType::protocol_error);
    ASSERT_TRUE(protocol_error.value().error);
    EXPECT_EQ(protocol_error.value().error->code,
              system::SystemErrorCode::protocol_version_mismatch);
    EXPECT_FALSE(protocol_error.value().error->diagnostic_id.empty());

    auto compatible = envelope(system::MessageType::handshake,
                               "handshake-compatible",
                               "session-compatible");
    compatible.payload = {{"minVersion", "1"}, {"maxVersion", "1"}};
    ASSERT_TRUE(client.send(compatible));
    const auto compatible_ack = client.receive(5'000);
    ASSERT_TRUE(compatible_ack);
    EXPECT_EQ(compatible_ack.value().type,
              system::MessageType::handshake_acknowledged);

    auto delayed = envelope(system::MessageType::submit_job,
                            "submit-delay",
                            "request-delay");
    delayed.job_id = "job-delay";
    delayed.payload = {{"operation", "delay"}, {"baseRevision", "3"}};
    ASSERT_TRUE(client.send(delayed));
    const auto accepted = client.receive(5'000);
    ASSERT_TRUE(accepted);
    EXPECT_EQ(accepted.value().sequence, 1U);

    auto conflicting = delayed;
    conflicting.message_id = "submit-delay-conflict";
    conflicting.payload.at("operation") = "echo";
    ASSERT_TRUE(client.send(conflicting));
    const auto idempotency_conflict = client.receive(5'000);
    ASSERT_TRUE(idempotency_conflict);
    EXPECT_EQ(idempotency_conflict.value().type,
              system::MessageType::protocol_error);
    ASSERT_TRUE(idempotency_conflict.value().error);
    EXPECT_EQ(idempotency_conflict.value().error->code,
              system::SystemErrorCode::request_conflict);

    auto cancel = envelope(system::MessageType::cancel_job,
                           "cancel-delay",
                           "request-delay");
    cancel.job_id = "job-delay";
    ASSERT_TRUE(client.send(cancel));
    const auto cancel_ack = client.receive(5'000);
    const auto cancelled = client.receive(5'000);
    ASSERT_TRUE(cancel_ack);
    ASSERT_TRUE(cancelled);
    EXPECT_EQ(cancel_ack.value().payload.at("state"),
              "cancellationAcknowledged");
    EXPECT_EQ(cancelled.value().payload.at("state"), "cancelled");
    EXPECT_LT(cancel_ack.value().sequence, cancelled.value().sequence);

    ASSERT_TRUE(client.send(cancel));
    const auto replay_ack = client.receive(5'000);
    const auto replay_cancelled = client.receive(5'000);
    ASSERT_TRUE(replay_ack);
    ASSERT_TRUE(replay_cancelled);
    EXPECT_EQ(replay_ack.value().sequence, cancel_ack.value().sequence);
    EXPECT_EQ(replay_cancelled.value().sequence, cancelled.value().sequence);
}

TEST(MockWorkerIpc, SurfacesTimeoutAndWorkerCrash)
{
    {
        MockWorkerProcess worker;
        ASSERT_TRUE(worker.ready());
        system::LocalIpcClient client;
        ASSERT_TRUE(client.connect_to(worker.server_name(), 5'000));
        auto handshake = envelope(system::MessageType::handshake,
                                  "handshake-timeout",
                                  "session-timeout");
        handshake.payload = {{"minVersion", "1"}, {"maxVersion", "1"}};
        ASSERT_TRUE(client.send(handshake));
        ASSERT_TRUE(client.receive(5'000));
        auto request = envelope(system::MessageType::submit_job,
                                "submit-timeout",
                                "request-timeout");
        request.job_id = "job-timeout";
        request.payload = {{"operation", "timeout"}, {"baseRevision", "1"}};
        ASSERT_TRUE(client.send(request));
        ASSERT_TRUE(client.receive(5'000));
        const auto timed_out = client.receive(25);
        ASSERT_FALSE(timed_out);
        EXPECT_EQ(timed_out.error().code,
                  system::SystemErrorCode::request_timeout);
    }

    MockWorkerProcess worker;
    ASSERT_TRUE(worker.ready());
    system::LocalIpcClient client;
    ASSERT_TRUE(client.connect_to(worker.server_name(), 5'000));
    auto handshake = envelope(system::MessageType::handshake,
                              "handshake-crash",
                              "session-crash");
    handshake.payload = {{"minVersion", "1"}, {"maxVersion", "1"}};
    ASSERT_TRUE(client.send(handshake));
    ASSERT_TRUE(client.receive(5'000));
    auto crash = envelope(system::MessageType::submit_job,
                          "submit-crash",
                          "request-crash");
    crash.job_id = "job-crash";
    crash.payload = {{"operation", "crash"}, {"baseRevision", "1"}};
    ASSERT_TRUE(client.send(crash));
    ASSERT_TRUE(client.receive(5'000));
    ASSERT_TRUE(worker.process().waitForFinished(5'000));
    EXPECT_NE(worker.process().exitCode(), 0);
    const auto disconnected = client.receive(100);
    ASSERT_FALSE(disconnected);
    EXPECT_EQ(disconnected.error().code,
              system::SystemErrorCode::worker_crashed);
}

} // namespace
