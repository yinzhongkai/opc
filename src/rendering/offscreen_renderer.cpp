#include <space_rhythm/rendering/offscreen_renderer.hpp>

#include <space_rhythm/rendering/scene_graph_render_item.hpp>

#include <QCryptographicHash>
#include <QColorSpace>
#include <QEventLoop>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QQuickPaintedItem>
#include <QQuickGraphicsDevice>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QThread>
#include <QTimer>

#include <Windows.h>
#include <Psapi.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <limits>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>

namespace space_rhythm::rendering {
namespace {

using Microsoft::WRL::ComPtr;
using SteadyClock = std::chrono::steady_clock;

std::atomic_uint64_t next_diagnostic_sequence{1};
std::atomic_uint64_t next_device_generation{1};

std::string diagnostic_id(std::string_view prefix)
{
    return std::string{prefix} + ':'
        + std::to_string(next_diagnostic_sequence.fetch_add(
            1, std::memory_order_relaxed));
}

core::ErrorInfo make_error(core::ErrorCategory category,
                           core::ErrorCode code,
                           std::string_view stage,
                           bool retryable,
                           std::string detail = {})
{
    auto error = core::ErrorInfo{core::schema_version,
                                 category,
                                 code,
                                 std::string{stage},
                                 diagnostic_id("offscreen"),
                                 retryable,
                                 std::string{core::to_string(code)},
                                 {},
                                 {}};
    if (!detail.empty()) {
        error.context.emplace("detail", std::move(detail));
    }
    return error;
}

core::ErrorInfo invalid(std::string_view stage, std::string detail = {})
{
    return make_error(core::ErrorCategory::validation,
                      core::ErrorCode::invalid_dto,
                      stage,
                      false,
                      std::move(detail));
}

core::ErrorInfo unavailable(std::string_view stage, std::string detail = {})
{
    return make_error(core::ErrorCategory::compatibility,
                      core::ErrorCode::unsupported_feature,
                      stage,
                      true,
                      std::move(detail));
}

core::ErrorInfo cancelled(std::string_view stage)
{
    return make_error(core::ErrorCategory::cancelled,
                      core::ErrorCode::cancelled,
                      stage,
                      false);
}

core::ErrorInfo render_failed(std::string_view stage,
                              std::string detail,
                              bool retryable = true)
{
    return make_error(core::ErrorCategory::internal,
                      core::ErrorCode::internal_error,
                      stage,
                      retryable,
                      std::move(detail));
}

std::string hresult_text(HRESULT result)
{
    std::ostringstream text;
    text << "HRESULT=0x" << std::hex
         << static_cast<unsigned long>(result);
    return text.str();
}

std::uint64_t file_time_value(const FILETIME& value)
{
    ULARGE_INTEGER converted{};
    converted.LowPart = value.dwLowDateTime;
    converted.HighPart = value.dwHighDateTime;
    return converted.QuadPart;
}

std::optional<std::uint64_t> process_cpu_time_ns()
{
    FILETIME creation{};
    FILETIME exit{};
    FILETIME kernel{};
    FILETIME user{};
    if (!GetProcessTimes(GetCurrentProcess(),
                         &creation,
                         &exit,
                         &kernel,
                         &user)) {
        return std::nullopt;
    }
    const auto ticks = file_time_value(kernel) + file_time_value(user);
    if (ticks > std::numeric_limits<std::uint64_t>::max() / 100U) {
        return std::nullopt;
    }
    return ticks * 100U;
}

std::optional<std::uint64_t> process_working_set_bytes()
{
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            sizeof(counters))) {
        return std::nullopt;
    }
    return static_cast<std::uint64_t>(counters.WorkingSetSize);
}

UInt64Metric measured_metric(std::uint64_t value, std::string unit)
{
    return UInt64Metric{MetricAvailability::measured,
                        value,
                        std::move(unit),
                        {}};
}

UInt64Metric unavailable_metric(std::string unit, std::string reason)
{
    return UInt64Metric{MetricAvailability::unavailable,
                        0,
                        std::move(unit),
                        std::move(reason)};
}

std::uint64_t elapsed_ns(SteadyClock::time_point start,
                         SteadyClock::time_point end)
{
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
}

GraphicsCapability probe_d3d11()
{
    GraphicsCapability capability;
    capability.backend = GraphicsBackend::default_gpu_d3d11;
    capability.graphics_api = "Direct3D 11";
    capability.hardware_accelerated = true;
    capability.diagnostic_id = diagnostic_id("capability-d3d11");

    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL feature_level{};
    constexpr std::array feature_levels{D3D_FEATURE_LEVEL_11_1,
                                         D3D_FEATURE_LEVEL_11_0,
                                         D3D_FEATURE_LEVEL_10_1,
                                         D3D_FEATURE_LEVEL_10_0};
    auto result = D3D11CreateDevice(nullptr,
                                    D3D_DRIVER_TYPE_HARDWARE,
                                    nullptr,
                                    D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                    feature_levels.data(),
                                    static_cast<UINT>(feature_levels.size()),
                                    D3D11_SDK_VERSION,
                                    &device,
                                    &feature_level,
                                    &context);
    if (result == E_INVALIDARG) {
        result = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                   feature_levels.data() + 1,
                                   static_cast<UINT>(feature_levels.size() - 1),
                                   D3D11_SDK_VERSION,
                                   &device,
                                   &feature_level,
                                   &context);
    }
    if (FAILED(result)) {
        capability.detail = hresult_text(result);
        return capability;
    }

    ComPtr<IDXGIDevice> dxgi_device;
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC description{};
    if (SUCCEEDED(device.As(&dxgi_device))
        && SUCCEEDED(dxgi_device->GetAdapter(&adapter))
        && SUCCEEDED(adapter->GetDesc(&description))) {
        capability.adapter_name = QString::fromWCharArray(description.Description)
                                      .toStdString();
        capability.vendor_id = description.VendorId;
        capability.device_id = description.DeviceId;
        capability.dedicated_video_memory_capacity_bytes =
            static_cast<std::uint64_t>(description.DedicatedVideoMemory);
    }
    capability.availability = CapabilityAvailability::available;
    capability.detail = "D3D feature level "
        + std::to_string(static_cast<unsigned int>(feature_level));
    return capability;
}

const GraphicsCapability* find_capability(const GraphicsCapabilityReport& report,
                                          GraphicsBackend backend)
{
    const auto found = std::find_if(
        report.capabilities.begin(),
        report.capabilities.end(),
        [backend](const GraphicsCapability& item) { return item.backend == backend; });
    return found == report.capabilities.end() ? nullptr : &*found;
}

bool on_gui_thread()
{
    return QGuiApplication::instance() != nullptr
        && QThread::currentThread() == QGuiApplication::instance()->thread();
}

std::uint64_t reserve_generation(std::uint64_t minimum)
{
    auto candidate = next_device_generation.load(std::memory_order_relaxed);
    for (;;) {
        const auto selected = std::max(candidate, minimum);
        if (next_device_generation.compare_exchange_weak(
                candidate,
                selected + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            return selected;
        }
    }
}

std::vector<std::byte> image_to_straight_rgba(const QImage& source,
                                              std::uint64_t& stride)
{
    const auto converted = source.convertToFormat(QImage::Format_RGBA8888);
    stride = static_cast<std::uint64_t>(converted.width()) * 4U;
    std::vector<std::byte> bytes(
        static_cast<std::size_t>(stride) * static_cast<std::size_t>(converted.height()));
    for (int row = 0; row < converted.height(); ++row) {
        std::memcpy(bytes.data() + static_cast<std::size_t>(row) * stride,
                    converted.constScanLine(row),
                    static_cast<std::size_t>(stride));
    }
    return bytes;
}

RenderedFrame make_frame(std::shared_ptr<const RenderSnapshot> snapshot,
                         std::uint64_t generation,
                         const GeometryFrame& geometry,
                         std::vector<std::byte> bytes,
                         std::uint64_t stride)
{
    RenderedFrame frame;
    frame.snapshot_id = snapshot->id();
    frame.timeline_revision = snapshot->recipe().timeline_revision;
    frame.device_generation = generation;
    frame.frame_index = geometry.frame_index;
    frame.time_ns = geometry.frame_time_ns;
    frame.width_px = geometry.target_width_px;
    frame.height_px = geometry.target_height_px;
    frame.pixel_format = PixelFormat::rgba8_unorm;
    frame.color = snapshot->recipe().output.color;
    frame.alpha_mode = snapshot->recipe().output.alpha_mode;
    frame.row_order = RowOrder::top_down;
    frame.stride_bytes = stride;
    frame.valid_bytes = stride * geometry.target_height_px;
    frame.bytes = FrameLease{std::move(bytes)};
    return frame;
}

std::string sha256(const FrameLease& lease, std::uint64_t valid_bytes)
{
    const QByteArrayView view{reinterpret_cast<const char*>(lease.data()),
                              static_cast<qsizetype>(valid_bytes)};
    return QCryptographicHash::hash(view, QCryptographicHash::Sha256).toHex().toStdString();
}

class GeometryFrameSoftwareItem final : public QQuickPaintedItem {
public:
    explicit GeometryFrameSoftwareItem(QQuickItem* parent = nullptr)
        : QQuickPaintedItem(parent)
    {
        setRenderTarget(QQuickPaintedItem::Image);
        setAntialiasing(false);
    }

    void submit(std::shared_ptr<const GeometryFrame> frame)
    {
        frame_ = std::move(frame);
        update();
    }

    void paint(QPainter* painter) override
    {
        if (!frame_) {
            return;
        }
        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->setPen(Qt::NoPen);
        for (const auto& batch : frame_->batches) {
            for (std::size_t index = 0; index + 2 < batch.vertices.size();
                 index += 3) {
                const auto& first = batch.vertices[index];
                const auto& second = batch.vertices[index + 1];
                const auto& third = batch.vertices[index + 2];
                painter->setBrush(QColor{first.red,
                                         first.green,
                                         first.blue,
                                         first.alpha});
                const std::array<QPointF, 3> triangle{
                    QPointF{first.x, first.y},
                    QPointF{second.x, second.y},
                    QPointF{third.x, third.y}};
                painter->drawPolygon(triangle.data(),
                                     static_cast<int>(triangle.size()),
                                     Qt::OddEvenFill);
            }
        }
    }

private:
    std::shared_ptr<const GeometryFrame> frame_;
};

} // namespace

GraphicsCapabilityReport probe_graphics_capabilities()
{
    GraphicsCapabilityReport report;
    report.capabilities.push_back(probe_d3d11());
    report.capabilities.push_back(GraphicsCapability{
        GraphicsBackend::qt_software,
        CapabilityAvailability::available,
        false,
        "Qt Quick Software",
        "Qt raster paint device",
        0,
        0,
        std::nullopt,
        diagnostic_id("capability-software"),
        "Public QQuickRenderTarget::fromPaintDevice path"});
    return report;
}

core::Result<BackendSelection> select_graphics_backend(
    BackendPreference preference,
    const GraphicsCapabilityReport& report)
{
    if (report.schema_version != offscreen_schema_version
        || report.contract_version != offscreen_contract_version) {
        return core::Result<BackendSelection>::failure(
            unavailable("offscreen.capability.version"));
    }
    const auto* gpu = find_capability(report, GraphicsBackend::default_gpu_d3d11);
    const auto* software = find_capability(report, GraphicsBackend::qt_software);
    const auto available = [](const GraphicsCapability* capability) {
        return capability != nullptr
            && capability->availability == CapabilityAvailability::available;
    };

    if (preference != BackendPreference::software_only && available(gpu)) {
        return core::Result<BackendSelection>::success(BackendSelection{
            GraphicsBackend::default_gpu_d3d11,
            false,
            gpu->diagnostic_id,
            gpu->detail});
    }
    if (preference == BackendPreference::default_gpu_only) {
        return core::Result<BackendSelection>::failure(
            unavailable("offscreen.capability.default-gpu",
                        gpu == nullptr ? "capability missing" : gpu->detail));
    }
    if (available(software)) {
        const bool fallback = preference
            == BackendPreference::default_gpu_with_software_fallback;
        return core::Result<BackendSelection>::success(BackendSelection{
            GraphicsBackend::qt_software,
            fallback,
            fallback ? diagnostic_id("backend-fallback") : software->diagnostic_id,
            fallback ? "Default GPU unavailable; selected Qt Quick Software"
                     : software->detail});
    }
    return core::Result<BackendSelection>::failure(
        unavailable("offscreen.capability.software",
                    software == nullptr ? "capability missing" : software->detail));
}

class OffscreenRenderSession::Impl final {
public:
    OffscreenSessionConfig config;
    BackendSelection selection;
    std::uint64_t generation{};
    BoundedFrameQueue queue{1, 4};
    std::optional<OffscreenDiagnostic> last_diagnostic;
    bool device_lost{false};
    bool initialized{false};

    std::unique_ptr<QQuickRenderControl> render_control;
    std::unique_ptr<QQuickWindow> window;
    GeometryFrameRenderItem* geometry_item{};
    GeometryFrameSoftwareItem* software_item{};
    QImage software_image;

    ComPtr<ID3D11Device> d3d_device;
    ComPtr<ID3D11DeviceContext> d3d_context;
    ComPtr<ID3D11Texture2D> d3d_target;
    ComPtr<ID3D11Texture2D> d3d_staging;

    Impl(OffscreenSessionConfig input,
         BackendSelection backend_selection,
         std::uint64_t device_generation,
         std::uint64_t maximum_bytes)
        : config(std::move(input))
        , selection(std::move(backend_selection))
        , generation(device_generation)
        , queue(config.max_outstanding_frames, maximum_bytes)
    {
    }

    ~Impl()
    {
        geometry_item = nullptr;
        software_item = nullptr;
        if (render_control && initialized
            && selection.backend == GraphicsBackend::default_gpu_d3d11) {
            render_control->invalidate();
        }
        window.reset();
        render_control.reset();
        d3d_staging.Reset();
        d3d_target.Reset();
        d3d_context.Reset();
        d3d_device.Reset();
    }

    core::Result<BackendSelection> initialize()
    {
        if (!on_gui_thread()) {
            return core::Result<BackendSelection>::failure(
                invalid("offscreen.initialize.thread",
                        "QQuickRenderControl lifecycle requires the GUI thread"));
        }
        const auto api = selection.backend == GraphicsBackend::default_gpu_d3d11
            ? QSGRendererInterface::Direct3D11
            : QSGRendererInterface::Software;
        const auto current_api = QQuickWindow::graphicsApi();
        const bool current_is_software = QQuickWindow::sceneGraphBackend()
            == QStringLiteral("software");
        const bool compatible_existing_backend = current_api == api
            || current_api == QSGRendererInterface::Unknown
            || (api == QSGRendererInterface::Software && current_is_software);
        if (!QGuiApplication::allWindows().isEmpty()
            && !compatible_existing_backend) {
            return core::Result<BackendSelection>::failure(
                unavailable("offscreen.initialize.backend-scope",
                            "Software fallback requires an isolated worker process "
                            "when existing QQuickWindow instances use another backend"));
        }
        QQuickWindow::setGraphicsApi(api);

        render_control = std::make_unique<QQuickRenderControl>();
        window = std::make_unique<QQuickWindow>(render_control.get());
        const bool opaque_output = config.snapshot->recipe().output.alpha_mode
            == AlphaMode::opaque;
        window->setColor(opaque_output ? Qt::black : Qt::transparent);
        window->setGeometry(0,
                            0,
                            static_cast<int>(config.snapshot->recipe().output.width_px),
                            static_cast<int>(config.snapshot->recipe().output.height_px));
        window->contentItem()->setSize(window->size());
        if (selection.backend == GraphicsBackend::qt_software) {
            software_item = new GeometryFrameSoftwareItem(window->contentItem());
            software_item->setSize(window->size());
            software_image = QImage(window->size(), QImage::Format_ARGB32_Premultiplied);
            if (software_image.isNull()) {
                return core::Result<BackendSelection>::failure(
                    render_failed("offscreen.initialize.software-image",
                                  "QImage allocation failed"));
            }
            software_image.setColorSpace(QColorSpace::SRgb);
            software_image.fill(opaque_output ? Qt::black : Qt::transparent);
            auto target = QQuickRenderTarget::fromPaintDevice(&software_image);
            target.setDevicePixelRatio(1.0);
            window->setRenderTarget(target);
            initialized = true;
            return core::Result<BackendSelection>::success(selection);
        }

        geometry_item = new GeometryFrameRenderItem(window->contentItem());
        geometry_item->setSize(window->size());

        D3D_FEATURE_LEVEL feature_level{};
        constexpr std::array feature_levels{D3D_FEATURE_LEVEL_11_1,
                                             D3D_FEATURE_LEVEL_11_0,
                                             D3D_FEATURE_LEVEL_10_1,
                                             D3D_FEATURE_LEVEL_10_0};
        auto result = D3D11CreateDevice(nullptr,
                                        D3D_DRIVER_TYPE_HARDWARE,
                                        nullptr,
                                        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                        feature_levels.data(),
                                        static_cast<UINT>(feature_levels.size()),
                                        D3D11_SDK_VERSION,
                                        &d3d_device,
                                        &feature_level,
                                        &d3d_context);
        if (result == E_INVALIDARG) {
            result = D3D11CreateDevice(nullptr,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       nullptr,
                                       D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                                       feature_levels.data() + 1,
                                       static_cast<UINT>(feature_levels.size() - 1),
                                       D3D11_SDK_VERSION,
                                       &d3d_device,
                                       &feature_level,
                                       &d3d_context);
        }
        if (FAILED(result)) {
            return core::Result<BackendSelection>::failure(
                unavailable("offscreen.initialize.d3d11-device", hresult_text(result)));
        }

        D3D11_TEXTURE2D_DESC description{};
        description.Width = config.snapshot->recipe().output.width_px;
        description.Height = config.snapshot->recipe().output.height_px;
        description.MipLevels = 1;
        description.ArraySize = 1;
        description.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        description.SampleDesc.Count = 1;
        description.Usage = D3D11_USAGE_DEFAULT;
        description.BindFlags = D3D11_BIND_SHADER_RESOURCE
            | D3D11_BIND_RENDER_TARGET;
        result = d3d_device->CreateTexture2D(&description, nullptr, &d3d_target);
        if (FAILED(result)) {
            return core::Result<BackendSelection>::failure(
                render_failed("offscreen.initialize.d3d11-target",
                              hresult_text(result)));
        }
        description.Usage = D3D11_USAGE_STAGING;
        description.BindFlags = 0;
        description.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        result = d3d_device->CreateTexture2D(&description, nullptr, &d3d_staging);
        if (FAILED(result)) {
            return core::Result<BackendSelection>::failure(
                render_failed("offscreen.initialize.d3d11-staging",
                              hresult_text(result)));
        }

        window->setGraphicsDevice(QQuickGraphicsDevice::fromDeviceAndContext(
            d3d_device.Get(), d3d_context.Get()));
        if (!render_control->initialize()) {
            return core::Result<BackendSelection>::failure(
                render_failed("offscreen.initialize.render-control",
                              "QQuickRenderControl::initialize returned false"));
        }
        auto target = QQuickRenderTarget::fromD3D11Texture(
            d3d_target.Get(), window->size(), 1);
        target.setDevicePixelRatio(1.0);
        window->setRenderTarget(target);
        initialized = true;
        return core::Result<BackendSelection>::success(selection);
    }

    core::Result<std::vector<std::byte>> render(
        std::shared_ptr<const GeometryFrame> geometry,
        std::uint64_t& stride)
    {
        if (selection.backend == GraphicsBackend::qt_software) {
            software_item->submit(std::move(geometry));
            software_image.fill(
                config.snapshot->recipe().output.alpha_mode == AlphaMode::opaque
                    ? Qt::black
                    : Qt::transparent);
            render_control->polishItems();
            if (!render_control->sync()) {
                return core::Result<std::vector<std::byte>>::failure(
                    render_failed("offscreen.render.software-sync",
                                  "QQuickRenderControl::sync returned false"));
            }
            render_control->render();
            return core::Result<std::vector<std::byte>>::success(
                image_to_straight_rgba(software_image, stride));
        }

        geometry_item->submit_geometry_frame(std::move(geometry));

        render_control->polishItems();
        render_control->beginFrame();
        if (!render_control->sync()) {
            render_control->endFrame();
            return core::Result<std::vector<std::byte>>::failure(
                render_failed("offscreen.render.gpu-sync",
                              "QQuickRenderControl::sync returned false"));
        }
        render_control->render();
        render_control->endFrame();
        d3d_context->CopyResource(d3d_staging.Get(), d3d_target.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        const auto result = d3d_context->Map(
            d3d_staging.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(result)) {
            const auto removed = d3d_device->GetDeviceRemovedReason();
            const auto detail = FAILED(removed) ? hresult_text(removed)
                                                : hresult_text(result);
            auto error = render_failed("offscreen.render.gpu-readback", detail);
            if (FAILED(removed)) {
                error.context.emplace("device_lost", "true");
            }
            return core::Result<std::vector<std::byte>>::failure(std::move(error));
        }
        stride = static_cast<std::uint64_t>(
            config.snapshot->recipe().output.width_px) * 4U;
        const auto height = config.snapshot->recipe().output.height_px;
        std::vector<std::byte> bytes(static_cast<std::size_t>(stride) * height);
        for (std::uint32_t row = 0; row < height; ++row) {
            const auto* input = static_cast<const std::uint8_t*>(mapped.pData)
                + static_cast<std::size_t>(row) * mapped.RowPitch;
            auto* output = reinterpret_cast<std::uint8_t*>(
                bytes.data() + static_cast<std::size_t>(row) * stride);
            std::memcpy(output, input, static_cast<std::size_t>(stride));
            if (config.snapshot->recipe().output.alpha_mode == AlphaMode::straight) {
                for (std::uint32_t column = 0;
                     column < config.snapshot->recipe().output.width_px;
                     ++column) {
                    auto* pixel = output + static_cast<std::size_t>(column) * 4U;
                    const auto alpha = pixel[3];
                    if (alpha == 0) {
                        pixel[0] = pixel[1] = pixel[2] = 0;
                    } else if (alpha < 255) {
                        for (std::size_t channel = 0; channel < 3; ++channel) {
                            pixel[channel] = static_cast<std::uint8_t>(std::min(
                                255U,
                                (static_cast<unsigned int>(pixel[channel]) * 255U
                                 + alpha / 2U)
                                    / alpha));
                        }
                    }
                }
            }
        }
        d3d_context->Unmap(d3d_staging.Get(), 0);
        return core::Result<std::vector<std::byte>>::success(std::move(bytes));
    }
};

OffscreenRenderSession::OffscreenRenderSession(std::unique_ptr<Impl> impl)
    : impl_(std::move(impl))
{
}

OffscreenRenderSession::~OffscreenRenderSession() = default;

core::Result<std::unique_ptr<OffscreenRenderSession>>
OffscreenRenderSession::create(OffscreenSessionConfig config)
{
    if (config.schema_version != offscreen_schema_version || !config.snapshot
        || config.expected_snapshot_id != config.snapshot->id()
        || config.expected_timeline_revision
            != config.snapshot->recipe().timeline_revision
        || config.max_outstanding_frames == 0
        || config.minimum_device_generation == 0
        || config.viewport_time_range.start_ns
            < config.snapshot->recipe().time_range.start_ns
        || config.viewport_time_range.end_ns
            > config.snapshot->recipe().time_range.end_ns
        || config.viewport_time_range.end_ns
            <= config.viewport_time_range.start_ns) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            invalid("offscreen.session.config"));
    }
    const auto width = config.snapshot->recipe().output.width_px;
    const auto height = config.snapshot->recipe().output.height_px;
    if (width == 0 || height == 0
        || width > std::numeric_limits<std::uint64_t>::max() / 4U
        || static_cast<std::uint64_t>(width) * 4U
            > std::numeric_limits<std::uint64_t>::max() / height) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            invalid("offscreen.session.extent"));
    }
    const auto frame_bytes = static_cast<std::uint64_t>(width) * 4U * height;
    auto maximum_bytes = config.max_outstanding_bytes;
    if (maximum_bytes == 0) {
        if (frame_bytes > std::numeric_limits<std::uint64_t>::max()
                / config.max_outstanding_frames) {
            return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
                invalid("offscreen.session.queue-capacity"));
        }
        maximum_bytes = frame_bytes * config.max_outstanding_frames;
    }
    if (maximum_bytes < frame_bytes) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            invalid("offscreen.session.queue-capacity"));
    }

    auto selection = select_graphics_backend(config.backend_preference,
                                             probe_graphics_capabilities());
    if (!selection) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            selection.error());
    }
    const auto generation = reserve_generation(config.minimum_device_generation);
    auto impl = std::make_unique<Impl>(std::move(config),
                                      selection.value(),
                                      generation,
                                      maximum_bytes);
    auto initialized = impl->initialize();
    if (!initialized
        && impl->selection.backend == GraphicsBackend::default_gpu_d3d11
        && impl->config.backend_preference
            == BackendPreference::default_gpu_with_software_fallback) {
        auto fallback_config = impl->config;
        fallback_config.backend_preference = BackendPreference::software_only;
        impl.reset();
        auto fallback = create(std::move(fallback_config));
        if (fallback) {
            fallback.value()->impl_->selection.used_fallback = true;
            fallback.value()->impl_->selection.diagnostic_id =
                diagnostic_id("backend-fallback-initialize");
            fallback.value()->impl_->selection.detail =
                "Default GPU initialization failed; selected Qt Quick Software";
        }
        return fallback;
    }
    if (!initialized) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            initialized.error());
    }
    return core::Result<std::unique_ptr<OffscreenRenderSession>>::success(
        std::unique_ptr<OffscreenRenderSession>{
            new OffscreenRenderSession{std::move(impl)}});
}

core::Result<OffscreenRenderResult> OffscreenRenderSession::render_frame(
    FrameIndex frame_index,
    const core::CancellationToken* cancellation)
{
    if (!on_gui_thread()) {
        return core::Result<OffscreenRenderResult>::failure(
            invalid("offscreen.render.thread"));
    }
    if (impl_->device_lost) {
        return core::Result<OffscreenRenderResult>::failure(
            render_failed("offscreen.render.device-lost",
                          impl_->last_diagnostic->diagnostic_id));
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        impl_->queue.cancel();
        return core::Result<OffscreenRenderResult>::failure(
            cancelled("offscreen.render.cancelled-before-build"));
    }

    const auto total_start = SteadyClock::now();
    const auto cpu_start = process_cpu_time_ns();
    const auto geometry_start = SteadyClock::now();
    GeometryBuildRequest request;
    request.snapshot = impl_->config.snapshot;
    request.expected_snapshot_id = impl_->config.expected_snapshot_id;
    request.expected_timeline_revision = impl_->config.expected_timeline_revision;
    request.viewport_time_range = impl_->config.viewport_time_range;
    request.target_width_px = impl_->config.snapshot->recipe().output.width_px;
    request.target_height_px = impl_->config.snapshot->recipe().output.height_px;
    request.frame_index = frame_index;
    request.device_generation = impl_->generation;
    auto geometry_result = build_geometry_frame(request);
    const auto geometry_end = SteadyClock::now();
    if (!geometry_result) {
        return core::Result<OffscreenRenderResult>::failure(geometry_result.error());
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        impl_->queue.cancel();
        return core::Result<OffscreenRenderResult>::failure(
            cancelled("offscreen.render.cancelled-after-build"));
    }

    auto geometry = std::make_shared<const GeometryFrame>(
        std::move(geometry_result.value()));
    const auto render_start = SteadyClock::now();
    std::uint64_t stride = 0;
    auto bytes = impl_->render(geometry, stride);
    const auto render_end = SteadyClock::now();
    if (!bytes) {
        const auto identifier = bytes.error().diagnostic_id;
        const auto detail = bytes.error().context.contains("detail")
            ? bytes.error().context.at("detail")
            : "render/readback failed";
        if (bytes.error().context.contains("device_lost")) {
            notify_device_lost(identifier, detail);
        } else {
            impl_->last_diagnostic = OffscreenDiagnostic{
                offscreen_schema_version,
                identifier,
                FrameFailureCode::render_failed,
                impl_->selection.backend,
                impl_->generation,
                true,
                bytes.error().stage,
                detail};
            impl_->queue.fail(FrameFailure{FrameFailureCode::render_failed,
                                           impl_->generation,
                                           identifier});
        }
        return core::Result<OffscreenRenderResult>::failure(bytes.error());
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        impl_->queue.cancel();
        return core::Result<OffscreenRenderResult>::failure(
            cancelled("offscreen.render.cancelled-before-publish"));
    }

    auto frame = make_frame(impl_->config.snapshot,
                            impl_->generation,
                            *geometry,
                            std::move(bytes.value()),
                            stride);
    auto valid_frame = validate_rendered_frame(std::move(frame),
                                               *impl_->config.snapshot);
    if (!valid_frame) {
        return core::Result<OffscreenRenderResult>::failure(valid_frame.error());
    }
    const auto publish_status = impl_->queue.publish(
        std::move(valid_frame.value()));
    const auto total_end = SteadyClock::now();
    const auto cpu_end = process_cpu_time_ns();
    const auto working_set = process_working_set_bytes();

    std::uint64_t upload_vertices = 0;
    for (const auto& batch : geometry->batches) {
        upload_vertices += batch.vertices.size();
    }
    const auto upload_bytes = upload_vertices
        * static_cast<std::uint64_t>(sizeof(QSGGeometry::ColoredPoint2D));

    FrameMeasurements measurements;
    measurements.total_frame_time_ns = measured_metric(
        elapsed_ns(total_start, total_end), "ns");
    measurements.geometry_build_time_ns = measured_metric(
        elapsed_ns(geometry_start, geometry_end), "ns");
    measurements.polish_sync_render_readback_time_ns = measured_metric(
        elapsed_ns(render_start, render_end), "ns");
    measurements.uploaded_vertices = measured_metric(upload_vertices, "vertices");
    measurements.uploaded_bytes = measured_metric(upload_bytes, "bytes");
    measurements.process_cpu_time_ns = cpu_start && cpu_end && *cpu_end >= *cpu_start
        ? measured_metric(*cpu_end - *cpu_start, "ns")
        : unavailable_metric("ns", "GetProcessTimes unavailable");
    measurements.process_working_set_bytes = working_set
        ? measured_metric(*working_set, "bytes")
        : unavailable_metric("bytes", "GetProcessMemoryInfo unavailable");
    measurements.gpu_frame_time_ns = unavailable_metric(
        "ns", "Qt public API exposes no per-frame GPU timestamp for this path");
    measurements.gpu_memory_usage_bytes = unavailable_metric(
        "bytes", "Qt/D3D11 public path exposes adapter capacity, not allocation usage");

    return core::Result<OffscreenRenderResult>::success(OffscreenRenderResult{
        publish_status,
        impl_->generation,
        frame_index,
        geometry->frame_time_ns,
        geometry->stats,
        std::move(measurements)});
}

FrameTakeResult OffscreenRenderSession::try_take()
{
    return impl_->queue.try_take();
}

void OffscreenRenderSession::finish()
{
    impl_->queue.drain();
}

void OffscreenRenderSession::cancel()
{
    impl_->queue.cancel();
}

void OffscreenRenderSession::notify_device_lost(std::string identifier,
                                                std::string detail)
{
    if (impl_->device_lost) {
        return;
    }
    if (identifier.empty()) {
        identifier = diagnostic_id("device-lost");
    }
    impl_->device_lost = true;
    impl_->last_diagnostic = OffscreenDiagnostic{offscreen_schema_version,
                                                 identifier,
                                                 FrameFailureCode::device_lost,
                                                 impl_->selection.backend,
                                                 impl_->generation,
                                                 true,
                                                 "offscreen.device-lost",
                                                 std::move(detail)};
    impl_->queue.fail(FrameFailure{FrameFailureCode::device_lost,
                                   impl_->generation,
                                   std::move(identifier)});
    if (impl_->render_control && impl_->initialized
        && impl_->selection.backend == GraphicsBackend::default_gpu_d3d11) {
        impl_->render_control->invalidate();
        impl_->initialized = false;
    }
}

core::Result<std::unique_ptr<OffscreenRenderSession>>
OffscreenRenderSession::rebuild() const
{
    if (!impl_->device_lost) {
        return core::Result<std::unique_ptr<OffscreenRenderSession>>::failure(
            invalid("offscreen.rebuild.state",
                    "Old session must be terminated by device loss first"));
    }
    auto config = impl_->config;
    config.minimum_device_generation = impl_->generation + 1;
    return create(std::move(config));
}

GraphicsBackend OffscreenRenderSession::backend() const noexcept
{
    return impl_->selection.backend;
}

std::uint64_t OffscreenRenderSession::device_generation() const noexcept
{
    return impl_->generation;
}

FrameQueueState OffscreenRenderSession::queue_state() const
{
    return impl_->queue.state();
}

const BackendSelection& OffscreenRenderSession::backend_selection() const noexcept
{
    return impl_->selection;
}

const std::optional<OffscreenDiagnostic>&
OffscreenRenderSession::last_diagnostic() const noexcept
{
    return impl_->last_diagnostic;
}

core::Result<PixelDifferenceMeasurement> measure_pixel_difference(
    const RenderedFrame& reference,
    const RenderedFrame& candidate)
{
    if (reference.width_px != candidate.width_px
        || reference.height_px != candidate.height_px
        || reference.pixel_format != PixelFormat::rgba8_unorm
        || candidate.pixel_format != PixelFormat::rgba8_unorm
        || reference.color != candidate.color
        || reference.alpha_mode != candidate.alpha_mode
        || reference.row_order != RowOrder::top_down
        || candidate.row_order != RowOrder::top_down
        || reference.stride_bytes < static_cast<std::uint64_t>(reference.width_px) * 4U
        || candidate.stride_bytes < static_cast<std::uint64_t>(candidate.width_px) * 4U
        || reference.bytes.size() < reference.valid_bytes
        || candidate.bytes.size() < candidate.valid_bytes) {
        return core::Result<PixelDifferenceMeasurement>::failure(
            invalid("offscreen.pixel-difference.frame"));
    }

    PixelDifferenceMeasurement measurement;
    measurement.reference_sha256 = sha256(reference.bytes, reference.valid_bytes);
    measurement.candidate_sha256 = sha256(candidate.bytes, candidate.valid_bytes);
    measurement.exact_hash_match = measurement.reference_sha256
        == measurement.candidate_sha256;
    measurement.compared_pixel_count = static_cast<std::uint64_t>(reference.width_px)
        * reference.height_px;
    for (std::uint32_t row = 0; row < reference.height_px; ++row) {
        const auto* reference_row = reinterpret_cast<const std::uint8_t*>(
            reference.bytes.data() + static_cast<std::size_t>(row)
                * reference.stride_bytes);
        const auto* candidate_row = reinterpret_cast<const std::uint8_t*>(
            candidate.bytes.data() + static_cast<std::size_t>(row)
                * candidate.stride_bytes);
        for (std::uint32_t column = 0; column < reference.width_px; ++column) {
            bool pixel_differs = false;
            for (std::size_t channel = 0; channel < 4; ++channel) {
                const auto offset = static_cast<std::size_t>(column) * 4U + channel;
                const auto difference = static_cast<std::uint8_t>(
                    std::abs(static_cast<int>(reference_row[offset])
                             - static_cast<int>(candidate_row[offset])));
                measurement.maximum_channel_difference = std::max(
                    measurement.maximum_channel_difference, difference);
                pixel_differs = pixel_differs || difference != 0;
            }
            if (pixel_differs) {
                ++measurement.different_pixel_count;
            }
        }
    }
    return core::Result<PixelDifferenceMeasurement>::success(
        std::move(measurement));
}

core::Result<RenderedFrame> render_onscreen_reference(
    std::shared_ptr<const RenderSnapshot> snapshot,
    core::TimeRange viewport_time_range,
    FrameIndex frame_index,
    std::uint64_t device_generation,
    GraphicsBackend backend,
    const core::CancellationToken* cancellation)
{
    if (!on_gui_thread() || !snapshot || device_generation == 0) {
        return core::Result<RenderedFrame>::failure(
            invalid("onscreen-reference.request"));
    }
    if (cancellation != nullptr && cancellation->is_cancelled()) {
        return core::Result<RenderedFrame>::failure(
            cancelled("onscreen-reference.cancelled"));
    }
    if (QGuiApplication::allWindows().isEmpty()) {
        QQuickWindow::setGraphicsApi(
            backend == GraphicsBackend::qt_software
                ? QSGRendererInterface::Software
                : QSGRendererInterface::Direct3D11);
    }
    GeometryBuildRequest request;
    request.snapshot = snapshot;
    request.expected_snapshot_id = snapshot->id();
    request.expected_timeline_revision = snapshot->recipe().timeline_revision;
    request.viewport_time_range = viewport_time_range;
    request.target_width_px = snapshot->recipe().output.width_px;
    request.target_height_px = snapshot->recipe().output.height_px;
    request.frame_index = frame_index;
    request.device_generation = device_generation;
    auto geometry = build_geometry_frame(request);
    if (!geometry) {
        return core::Result<RenderedFrame>::failure(geometry.error());
    }

    QQuickWindow window;
    window.setColor(snapshot->recipe().output.alpha_mode == AlphaMode::opaque
                        ? Qt::black
                        : Qt::transparent);
    const auto device_pixel_ratio = std::max<qreal>(window.devicePixelRatio(), 1.0);
    const auto logical_width = static_cast<int>(std::ceil(
        static_cast<qreal>(request.target_width_px) / device_pixel_ratio));
    const auto logical_height = static_cast<int>(std::ceil(
        static_cast<qreal>(request.target_height_px) / device_pixel_ratio));
    window.setGeometry(0,
                       0,
                       logical_width,
                       logical_height);
    window.contentItem()->setSize(window.size());
    QQuickItem* rendered_item = nullptr;
    if (backend == GraphicsBackend::qt_software) {
        auto* item = new GeometryFrameSoftwareItem(window.contentItem());
        rendered_item = item;
        item->submit(std::make_shared<const GeometryFrame>(geometry.value()));
    } else {
        auto* item = new GeometryFrameRenderItem(window.contentItem());
        rendered_item = item;
        item->submit_geometry_frame(
            std::make_shared<const GeometryFrame>(geometry.value()));
    }
    rendered_item->setSize(QSizeF{static_cast<qreal>(request.target_width_px),
                                  static_cast<qreal>(request.target_height_px)});
    rendered_item->setTransformOrigin(QQuickItem::TopLeft);
    rendered_item->setScale(1.0 / device_pixel_ratio);

    bool rendered = false;
    QEventLoop event_loop;
    QObject::connect(&window,
                     &QQuickWindow::afterRendering,
                     &event_loop,
                     [&]() {
                         rendered = true;
                         event_loop.quit();
                     },
                     Qt::QueuedConnection);
    QTimer::singleShot(5'000, &event_loop, &QEventLoop::quit);
    window.show();
    window.requestUpdate();
    event_loop.exec();
    if (!rendered) {
        window.hide();
        return core::Result<RenderedFrame>::failure(
            render_failed("onscreen-reference.wait",
                          "No frame completed within 5000 ms"));
    }
    auto image = window.grabWindow();
    window.hide();
    if (image.isNull()
        || image.width() < static_cast<int>(request.target_width_px)
        || image.height() < static_cast<int>(request.target_height_px)) {
        return core::Result<RenderedFrame>::failure(
            render_failed("onscreen-reference.grab",
                          "QQuickWindow::grabWindow image="
                              + std::to_string(image.width()) + 'x'
                              + std::to_string(image.height()) + " window="
                              + std::to_string(window.width()) + 'x'
                              + std::to_string(window.height()) + " dpr="
                              + std::to_string(window.devicePixelRatio())));
    }
    if (image.width() != static_cast<int>(request.target_width_px)
        || image.height() != static_cast<int>(request.target_height_px)) {
        image = image.copy(0,
                           0,
                           static_cast<int>(request.target_width_px),
                           static_cast<int>(request.target_height_px));
    }
    std::uint64_t stride = 0;
    auto bytes = image_to_straight_rgba(image, stride);
    auto frame = make_frame(snapshot,
                            device_generation,
                            geometry.value(),
                            std::move(bytes),
                            stride);
    return validate_rendered_frame(std::move(frame), *snapshot);
}

} // namespace space_rhythm::rendering
