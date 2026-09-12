#pragma once

#include <space_rhythm/audio/render.hpp>

#include <memory>
#include <cstdint>
#include <optional>
#include <string>

class QAudioDevice;

namespace space_rhythm::audio::render {

enum class PreviewError {
    none,
    invalid_pcm,
    device_unavailable,
    format_unsupported,
    sink_initialization_failed,
};

struct PreviewStartResult {
    bool started{false};
    PreviewError error{PreviewError::none};
    std::string diagnostic;
};

class QtAudioPreview final {
public:
    QtAudioPreview();
    ~QtAudioPreview();

    QtAudioPreview(const QtAudioPreview&) = delete;
    QtAudioPreview& operator=(const QtAudioPreview&) = delete;

    // QAudioSink is transport-only: the PCM must already have been produced by
    // DeterministicMixer. No device-side render, gain, clipping or resampling is used.
    [[nodiscard]] PreviewStartResult start_default(const RenderedPcm& pcm);
    [[nodiscard]] PreviewStartResult start(const RenderedPcm& pcm,
                                           const QAudioDevice& device);
    void stop() noexcept;
    [[nodiscard]] bool active() const noexcept;
    // Hardware-reported sink progress. This is the only preview value allowed
    // to drive T-019's audio-sample clock; wall-clock/QML animation is never
    // substituted while an audio device is active.
    [[nodiscard]] std::optional<std::uint64_t> processed_frames() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> implementation_;
};

[[nodiscard]] std::string_view to_string(PreviewError error) noexcept;

} // namespace space_rhythm::audio::render
