#include <space_rhythm/audio/preview_qt.hpp>

#include <QAudioDevice>
#include <QAudioFormat>
#include <QAudioSink>
#include <QBuffer>
#include <QByteArray>
#include <QMediaDevices>

#include <algorithm>
#include <cstring>
#include <cmath>
#include <limits>
#include <utility>

namespace space_rhythm::audio::render {

struct QtAudioPreview::Impl final {
    QByteArray bytes;
    std::unique_ptr<QBuffer> buffer;
    std::unique_ptr<QAudioSink> sink;
};

QtAudioPreview::QtAudioPreview() : implementation_(std::make_unique<Impl>()) {}
QtAudioPreview::~QtAudioPreview() = default;

PreviewStartResult QtAudioPreview::start_default(const RenderedPcm& pcm)
{
    return start(pcm, QMediaDevices::defaultAudioOutput());
}

PreviewStartResult QtAudioPreview::start(const RenderedPcm& pcm,
                                         const QAudioDevice& device)
{
    stop();
    if ((pcm.channel_count != 1U && pcm.channel_count != 2U)
        || pcm.sample_rate != render_sample_rate
        || pcm.frame_count > std::numeric_limits<std::size_t>::max() / pcm.channel_count
        || pcm.interleaved_f32.size()
            != static_cast<std::size_t>(pcm.frame_count * pcm.channel_count)
        || !std::ranges::all_of(pcm.interleaved_f32, [](const float sample) {
               return std::isfinite(sample) && sample >= -1.0F && sample <= 1.0F;
           })) {
        return {false, PreviewError::invalid_pcm,
                "preview requires finite 48 kHz mono/stereo mixer PCM"};
    }
    if (device.isNull() || device.mode() != QAudioDevice::Output) {
        return {false, PreviewError::device_unavailable, "audio output device is unavailable"};
    }
    QAudioFormat format;
    format.setSampleRate(static_cast<int>(pcm.sample_rate));
    format.setChannelCount(static_cast<int>(pcm.channel_count));
    format.setSampleFormat(QAudioFormat::Float);
    if (!device.isFormatSupported(format)) {
        return {false, PreviewError::format_unsupported,
                "device does not support deterministic f32 interleaved PCM"};
    }

    const auto bytes = pcm_f32le_bytes(pcm);
    implementation_->bytes.resize(static_cast<qsizetype>(bytes.size()));
    if (!bytes.empty()) {
        std::memcpy(implementation_->bytes.data(), bytes.data(), bytes.size());
    }
    implementation_->buffer = std::make_unique<QBuffer>(&implementation_->bytes);
    if (!implementation_->buffer->open(QIODevice::ReadOnly)) {
        stop();
        return {false, PreviewError::sink_initialization_failed,
                "preview PCM buffer could not be opened"};
    }
    implementation_->sink = std::make_unique<QAudioSink>(device, format);
    if (implementation_->sink->isNull()) {
        stop();
        return {false, PreviewError::sink_initialization_failed,
                "QAudioSink could not be created"};
    }
    implementation_->sink->setVolume(1.0);
    implementation_->sink->start(implementation_->buffer.get());
    if (implementation_->sink->error() == QtAudio::OpenError) {
        stop();
        return {false, PreviewError::sink_initialization_failed,
                "QAudioSink failed to open the audio device"};
    }
    return {true, PreviewError::none, {}};
}

void QtAudioPreview::stop() noexcept
{
    if (implementation_->sink) {
        implementation_->sink->stop();
    }
    implementation_->sink.reset();
    if (implementation_->buffer) {
        implementation_->buffer->close();
    }
    implementation_->buffer.reset();
    implementation_->bytes.clear();
}

bool QtAudioPreview::active() const noexcept
{
    return implementation_->sink != nullptr
        && implementation_->sink->state() != QtAudio::StoppedState;
}

std::string_view to_string(const PreviewError error) noexcept
{
    switch (error) {
    case PreviewError::none:
        return "none";
    case PreviewError::invalid_pcm:
        return "invalid_pcm";
    case PreviewError::device_unavailable:
        return "device_unavailable";
    case PreviewError::format_unsupported:
        return "format_unsupported";
    case PreviewError::sink_initialization_failed:
        return "sink_initialization_failed";
    }
    return "unknown";
}

} // namespace space_rhythm::audio::render
