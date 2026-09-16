#include <space_rhythm/rendering/render_contract.hpp>

#include <algorithm>
#include <atomic>
#include <deque>
#include <intrin.h>
#include <limits>
#include <mutex>
#include <ranges>
#include <tuple>
#include <utility>

namespace space_rhythm::rendering {
namespace {

std::atomic_uint64_t next_diagnostic_id{1};

core::ErrorInfo make_error(core::ErrorCategory category,
                           core::ErrorCode code,
                           std::string_view stage,
                           bool retryable = false)
{
    const auto sequence = next_diagnostic_id.fetch_add(1, std::memory_order_relaxed);
    return core::ErrorInfo{core::schema_version,
                           category,
                           code,
                           std::string{stage},
                           "render:" + std::to_string(sequence),
                           retryable,
                           std::string{core::to_string(code)},
                           {},
                           {}};
}

core::ErrorInfo invalid(std::string_view stage,
                        core::ErrorCode code = core::ErrorCode::invalid_dto)
{
    return make_error(core::ErrorCategory::validation, code, stage);
}

core::ErrorInfo incompatible(std::string_view stage, core::ErrorCode code)
{
    return make_error(core::ErrorCategory::compatibility, code, stage);
}

core::ErrorInfo stale(std::string_view stage)
{
    return make_error(
        core::ErrorCategory::conflict, core::ErrorCode::stale_revision, stage, true);
}

bool is_identifier(std::string_view value)
{
    return static_cast<bool>(core::validate_identifier(value));
}

bool is_sha256(std::string_view value)
{
    return value.size() == 64
        && std::ranges::all_of(value, [](const unsigned char character) {
               return (character >= '0' && character <= '9')
                   || (character >= 'a' && character <= 'f');
           });
}

bool is_version(std::string_view value)
{
    if (value.empty()) {
        return false;
    }
    return std::ranges::all_of(value, [](const unsigned char character) {
        return (character >= '0' && character <= '9') || character == '.'
            || character == '-' || (character >= 'A' && character <= 'Z')
            || (character >= 'a' && character <= 'z');
    });
}

bool valid_time_range(const core::TimeRange& range)
{
    return range.start_ns >= 0 && range.end_ns > range.start_ns;
}

bool valid_output(const OutputSpec& output)
{
    return output.width_px > 0 && output.height_px > 0
        && output.pixel_format == PixelFormat::rgba8_unorm
        && output.color == ColorDescription{}
        && (output.alpha_mode == AlphaMode::straight
            || output.alpha_mode == AlphaMode::opaque)
        && output.row_order == RowOrder::top_down;
}

bool valid_feature_input(const FeatureInputRevision& input)
{
    return is_identifier(input.analysis_revision.value)
        && is_version(input.producer_contract_version)
        && input.feature_schema_version > 0 && is_sha256(input.input_fingerprint_sha256)
        && is_sha256(input.parameters_digest_sha256)
        && is_sha256(input.content_digest_sha256);
}

bool valid_template_parameters(const TemplateParameterBlock& parameters)
{
    if (parameters.schema_version != schema_version
        || !is_identifier(parameters.template_id)
        || !is_version(parameters.template_version)
        || !is_sha256(parameters.parameters_digest_sha256)) {
        return false;
    }
    for (const auto& [name, unused] : parameters.integer_parameters) {
        static_cast<void>(unused);
        if (!is_identifier(name)) {
            return false;
        }
    }
    const core::SchemaEnvelope envelope{parameters.schema_version,
                                        parameters.required_features,
                                        parameters.extensions};
    return static_cast<bool>(core::validate_schema_envelope(envelope,
                                                            supported_features()));
}

bool basic_frame_shape_valid(const RenderedFrame& frame)
{
    if (frame.schema_version != schema_version
        || frame.render_contract_version != contract_version || frame.device_generation == 0
        || !is_identifier(frame.snapshot_id.value) || frame.width_px == 0
        || frame.height_px == 0
        || frame.pixel_format != PixelFormat::rgba8_unorm
        || frame.color != ColorDescription{}
        || (frame.alpha_mode != AlphaMode::straight
            && frame.alpha_mode != AlphaMode::opaque)
        || frame.row_order != RowOrder::top_down || frame.bytes.empty()) {
        return false;
    }
    constexpr std::uint64_t bytes_per_pixel = 4;
    const auto minimum_stride = static_cast<std::uint64_t>(frame.width_px) * bytes_per_pixel;
    if (frame.stride_bytes < minimum_stride
        || frame.stride_bytes
            > std::numeric_limits<std::uint64_t>::max() / frame.height_px) {
        return false;
    }
    const auto expected_valid_bytes = frame.stride_bytes * frame.height_px;
    return frame.valid_bytes == expected_valid_bytes
        && frame.valid_bytes <= frame.bytes.size();
}

core::Result<std::int64_t> multiply_divide_floor(std::int64_t value,
                                                 std::int64_t multiplier,
                                                 std::int64_t divisor)
{
    if (value < 0 || multiplier <= 0 || divisor <= 0) {
        return core::Result<std::int64_t>::failure(invalid("render.coordinates"));
    }
    std::uint64_t high = 0;
    const auto low = _umul128(static_cast<std::uint64_t>(value),
                              static_cast<std::uint64_t>(multiplier),
                              &high);
    const auto unsigned_divisor = static_cast<std::uint64_t>(divisor);
    if (high >= unsigned_divisor) {
        return core::Result<std::int64_t>::failure(
            invalid("render.coordinates", core::ErrorCode::time_overflow));
    }
    std::uint64_t remainder = 0;
    const auto quotient = _udiv128(high, low, unsigned_divisor, &remainder);
    static_cast<void>(remainder);
    if (quotient > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        return core::Result<std::int64_t>::failure(
            invalid("render.coordinates", core::ErrorCode::time_overflow));
    }
    return core::Result<std::int64_t>::success(static_cast<std::int64_t>(quotient));
}

std::int32_t hit_kind_order(HitTargetKind kind)
{
    switch (kind) {
    case HitTargetKind::event:
        return 0;
    case HitTargetKind::series_sample:
        return 1;
    case HitTargetKind::track:
        return 2;
    }
    return 3;
}

} // namespace

const std::set<std::string>& supported_features()
{
    static const std::set<std::string> features{
        "render.hit-test-v1",
        "render.rgba8-srgb-v1",
        "render.snapshot-v1",
    };
    return features;
}

core::Result<ContractDescriptor> negotiate_contract(ContractDescriptor descriptor)
{
    if (descriptor.render_contract_version != contract_version) {
        return core::Result<ContractDescriptor>::failure(
            incompatible("render.compatibility", core::ErrorCode::unsupported_schema));
    }
    auto envelope = core::validate_schema_envelope(
        core::SchemaEnvelope{descriptor.schema_version,
                             descriptor.required_features,
                             descriptor.extensions},
        supported_features());
    if (!envelope) {
        return core::Result<ContractDescriptor>::failure(envelope.error());
    }
    descriptor.required_features = std::move(envelope.value().required_features);
    descriptor.extensions = std::move(envelope.value().extensions);
    return core::Result<ContractDescriptor>::success(std::move(descriptor));
}

core::Result<RenderRecipe> validate_render_recipe(RenderRecipe recipe)
{
    auto compatibility = negotiate_contract(ContractDescriptor{recipe.schema_version,
                                                                 recipe.render_contract_version,
                                                                 recipe.required_features,
                                                                 recipe.extensions});
    if (!compatibility) {
        return core::Result<RenderRecipe>::failure(compatibility.error());
    }
    if (!is_identifier(recipe.recipe_id.value) || !is_identifier(recipe.project_id.value)
        || !valid_time_range(recipe.time_range) || recipe.frame_rate.numerator == 0
        || recipe.frame_rate.denominator == 0 || !valid_output(recipe.output)
        || !valid_template_parameters(recipe.template_parameters)) {
        return core::Result<RenderRecipe>::failure(invalid("render.recipe"));
    }
    if (!std::ranges::all_of(recipe.feature_inputs, valid_feature_input)
        || !std::ranges::is_sorted(recipe.feature_inputs,
                                   {},
                                   [](const FeatureInputRevision& input) {
                                       return input.analysis_revision.value;
                                   })) {
        return core::Result<RenderRecipe>::failure(invalid("render.recipe.features"));
    }
    for (std::size_t index = 1; index < recipe.feature_inputs.size(); ++index) {
        if (recipe.feature_inputs[index - 1].analysis_revision
            == recipe.feature_inputs[index].analysis_revision) {
            return core::Result<RenderRecipe>::failure(
                invalid("render.recipe.features"));
        }
    }
    return core::Result<RenderRecipe>::success(std::move(recipe));
}

core::Result<core::TimeNs> frame_time_ns(const RenderRecipe& recipe,
                                         FrameIndex frame_index)
{
    if (frame_index > static_cast<FrameIndex>(std::numeric_limits<std::int64_t>::max())
        || recipe.frame_rate.numerator == 0 || recipe.frame_rate.denominator == 0
        || !valid_time_range(recipe.time_range)) {
        return core::Result<core::TimeNs>::failure(invalid("render.frame-time"));
    }
    auto offset = core::scale_ticks(
        static_cast<std::int64_t>(frame_index),
        core::TimeBase{recipe.frame_rate.denominator, recipe.frame_rate.numerator},
        core::RoundingMode::nearest_ties_to_even);
    if (!offset) {
        return core::Result<core::TimeNs>::failure(offset.error());
    }
    auto time = core::checked_add(recipe.time_range.start_ns, offset.value());
    if (!time) {
        return core::Result<core::TimeNs>::failure(time.error());
    }
    if (!recipe.time_range.contains(time.value())) {
        return core::Result<core::TimeNs>::failure(invalid("render.frame-time"));
    }
    return time;
}

RenderSnapshot::RenderSnapshot(RenderSnapshotId id,
                               RenderRecipe recipe,
                               core::TimelineSnapshot timeline,
                               std::vector<RenderSeries> series)
    : id_(std::move(id)),
      recipe_(std::move(recipe)),
      timeline_(std::move(timeline)),
      series_(std::move(series))
{
}

const RenderSnapshotId& RenderSnapshot::id() const noexcept
{
    return id_;
}

const RenderRecipe& RenderSnapshot::recipe() const noexcept
{
    return recipe_;
}

const core::TimelineSnapshot& RenderSnapshot::timeline() const noexcept
{
    return timeline_;
}

const std::vector<RenderSeries>& RenderSnapshot::series() const noexcept
{
    return series_;
}

core::Result<std::shared_ptr<const RenderSnapshot>> make_render_snapshot(
    RenderSnapshotId id,
    RenderRecipe recipe,
    core::TimelineSnapshot timeline,
    std::vector<RenderSeries> series)
{
    if (!is_identifier(id.value)) {
        return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
            invalid("render.snapshot", core::ErrorCode::invalid_identifier));
    }
    auto validated_recipe = validate_render_recipe(std::move(recipe));
    if (!validated_recipe) {
        return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
            validated_recipe.error());
    }
    auto validated_timeline = core::validate_and_normalize_snapshot(std::move(timeline));
    if (!validated_timeline) {
        return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
            validated_timeline.error());
    }
    if (validated_recipe.value().project_id != validated_timeline.value().project_id
        || validated_recipe.value().timeline_revision
            != validated_timeline.value().timeline_revision) {
        return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
            stale("render.snapshot.revision"));
    }

    const auto& recipe_inputs = validated_recipe.value().feature_inputs;
    std::ranges::sort(series, {}, &RenderSeries::id);
    for (std::size_t series_index = 0; series_index < series.size(); ++series_index) {
        auto& item = series[series_index];
        if (!is_identifier(item.id.value) || !is_identifier(item.analysis_revision.value)
            || !is_identifier(item.definition_id)
            || !is_version(item.source_contract_version) || item.source_schema_version == 0
            || !is_sha256(item.input_fingerprint_sha256)
            || !is_sha256(item.parameters_digest_sha256)
            || !is_sha256(item.content_digest_sha256)
            || (series_index > 0 && series[series_index - 1].id == item.id)) {
            return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
                invalid("render.snapshot.series"));
        }
        const auto input = std::ranges::find(recipe_inputs,
                                             item.analysis_revision,
                                             &FeatureInputRevision::analysis_revision);
        if (input == recipe_inputs.end()
            || input->producer_contract_version != item.source_contract_version
            || input->feature_schema_version != item.source_schema_version
            || input->input_fingerprint_sha256 != item.input_fingerprint_sha256
            || input->parameters_digest_sha256 != item.parameters_digest_sha256
            || input->content_digest_sha256 != item.content_digest_sha256) {
            return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
                stale("render.snapshot.features"));
        }
        std::ranges::sort(item.samples, {}, &RenderSeriesSample::time_ns);
        for (std::size_t sample_index = 0; sample_index < item.samples.size();
             ++sample_index) {
            const auto& sample = item.samples[sample_index];
            if (!validated_recipe.value().time_range.contains(sample.time_ns)
                || sample.primary_ppm > core::norm_ppm_max
                || (sample.secondary_ppm
                    && *sample.secondary_ppm > core::norm_ppm_max)
                || (sample_index > 0
                    && item.samples[sample_index - 1].time_ns == sample.time_ns)) {
                return core::Result<std::shared_ptr<const RenderSnapshot>>::failure(
                    invalid("render.snapshot.samples"));
            }
        }
    }

    auto snapshot = std::shared_ptr<const RenderSnapshot>(new RenderSnapshot{
        std::move(id),
        std::move(validated_recipe.value()),
        std::move(validated_timeline.value()),
        std::move(series)});
    return core::Result<std::shared_ptr<const RenderSnapshot>>::success(
        std::move(snapshot));
}

core::Result<std::int64_t> time_to_item_x_sp(const CoordinateTransform& transform,
                                             core::TimeNs time_ns)
{
    if (!is_identifier(transform.snapshot_id.value)
        || transform.content_rect.width_sp <= 0 || transform.content_rect.height_sp <= 0
        || !valid_time_range(transform.time_range) || time_ns < transform.time_range.start_ns
        || time_ns > transform.time_range.end_ns) {
        return core::Result<std::int64_t>::failure(invalid("render.coordinates"));
    }
    auto duration = core::checked_subtract(transform.time_range.end_ns,
                                           transform.time_range.start_ns);
    auto delta = core::checked_subtract(time_ns, transform.time_range.start_ns);
    if (!duration || !delta) {
        return core::Result<std::int64_t>::failure(invalid("render.coordinates"));
    }
    auto offset = multiply_divide_floor(delta.value(),
                                        transform.content_rect.width_sp,
                                        duration.value());
    if (!offset) {
        return core::Result<std::int64_t>::failure(offset.error());
    }
    return core::checked_add(transform.content_rect.x_sp, offset.value());
}

core::Result<core::TimeNs> item_x_sp_to_time(const CoordinateTransform& transform,
                                             std::int64_t item_x_sp)
{
    if (!is_identifier(transform.snapshot_id.value)
        || transform.content_rect.width_sp <= 0 || transform.content_rect.height_sp <= 0
        || !valid_time_range(transform.time_range)) {
        return core::Result<core::TimeNs>::failure(invalid("render.coordinates"));
    }
    auto right = core::checked_add(transform.content_rect.x_sp,
                                   transform.content_rect.width_sp);
    if (!right || item_x_sp < transform.content_rect.x_sp || item_x_sp > right.value()) {
        return core::Result<core::TimeNs>::failure(invalid("render.coordinates"));
    }
    auto duration = core::checked_subtract(transform.time_range.end_ns,
                                           transform.time_range.start_ns);
    auto x_delta = core::checked_subtract(item_x_sp, transform.content_rect.x_sp);
    if (!duration || !x_delta) {
        return core::Result<core::TimeNs>::failure(invalid("render.coordinates"));
    }
    auto time_delta = multiply_divide_floor(x_delta.value(),
                                            duration.value(),
                                            transform.content_rect.width_sp);
    if (!time_delta) {
        return core::Result<core::TimeNs>::failure(time_delta.error());
    }
    return core::checked_add(transform.time_range.start_ns, time_delta.value());
}

core::Result<HitTestRequest> validate_hit_test_request(
    HitTestRequest request,
    const RenderSnapshot& snapshot,
    const CoordinateTransform& transform)
{
    if (request.schema_version != schema_version || request.snapshot_id != snapshot.id()
        || request.snapshot_id != transform.snapshot_id
        || request.timeline_revision != snapshot.recipe().timeline_revision
        || request.timeline_revision != transform.timeline_revision) {
        return core::Result<HitTestRequest>::failure(
            stale("render.hit-test.revision"));
    }
    auto right = core::checked_add(transform.content_rect.x_sp,
                                   transform.content_rect.width_sp);
    auto bottom = core::checked_add(transform.content_rect.y_sp,
                                    transform.content_rect.height_sp);
    if (!right || !bottom || request.radius_sp < 0
        || request.item_x_sp < transform.content_rect.x_sp
        || request.item_x_sp >= right.value()
        || request.item_y_sp < transform.content_rect.y_sp
        || request.item_y_sp >= bottom.value()) {
        return core::Result<HitTestRequest>::failure(invalid("render.hit-test.request"));
    }
    return core::Result<HitTestRequest>::success(std::move(request));
}

core::Result<HitTestResult> validate_and_sort_hit_test_result(
    HitTestResult result,
    const HitTestRequest& request,
    const RenderSnapshot& snapshot)
{
    if (result.schema_version != schema_version || result.snapshot_id != snapshot.id()
        || result.snapshot_id != request.snapshot_id
        || result.timeline_revision != snapshot.recipe().timeline_revision
        || result.timeline_revision != request.timeline_revision) {
        return core::Result<HitTestResult>::failure(stale("render.hit-test.result"));
    }
    if (request.schema_version != schema_version || request.radius_sp < 0) {
        return core::Result<HitTestResult>::failure(invalid("render.hit-test.result"));
    }
    std::uint64_t radius_squared_high = 0;
    const auto unsigned_radius = static_cast<std::uint64_t>(request.radius_sp);
    const auto radius_squared = _umul128(
        unsigned_radius, unsigned_radius, &radius_squared_high);
    if (radius_squared_high != 0) {
        return core::Result<HitTestResult>::failure(invalid("render.hit-test.result"));
    }
    for (const auto& candidate : result.candidates) {
        if (!is_identifier(candidate.target_id.value)
            || !snapshot.recipe().time_range.contains(candidate.time_ns)
            || candidate.distance_squared_sp > radius_squared) {
            return core::Result<HitTestResult>::failure(
                invalid("render.hit-test.result"));
        }
    }
    std::ranges::sort(result.candidates, [](const HitCandidate& lhs,
                                            const HitCandidate& rhs) {
        if (lhs.z_order != rhs.z_order) {
            return lhs.z_order > rhs.z_order;
        }
        return std::tuple{lhs.distance_squared_sp,
                          hit_kind_order(lhs.kind),
                          lhs.time_ns,
                          lhs.target_id.value}
            < std::tuple{rhs.distance_squared_sp,
                         hit_kind_order(rhs.kind),
                         rhs.time_ns,
                         rhs.target_id.value};
    });
    return core::Result<HitTestResult>::success(std::move(result));
}

SceneGraphEpoch SceneGraphLifecycle::current() const noexcept
{
    return current_;
}

core::Result<SceneGraphEpoch> SceneGraphLifecycle::apply(SceneGraphEvent event)
{
    bool accepted = false;
    switch (current_.state) {
    case SceneGraphState::detached:
        if (event == SceneGraphEvent::attach_window) {
            current_.state = SceneGraphState::awaiting_initialization;
            accepted = true;
        }
        break;
    case SceneGraphState::awaiting_initialization:
        if (event == SceneGraphEvent::initialize_scene_graph) {
            if (current_.device_generation == std::numeric_limits<std::uint64_t>::max()) {
                return core::Result<SceneGraphEpoch>::failure(
                    invalid("render.lifecycle.generation",
                            core::ErrorCode::resource_limit));
            }
            ++current_.device_generation;
            current_.state = SceneGraphState::ready;
            accepted = true;
        } else if (event == SceneGraphEvent::invalidate_scene_graph) {
            current_.state = SceneGraphState::invalidated;
            accepted = true;
        } else if (event == SceneGraphEvent::detach_window) {
            current_.state = SceneGraphState::detached;
            accepted = true;
        }
        break;
    case SceneGraphState::ready:
        if (event == SceneGraphEvent::request_cleanup) {
            current_.state = SceneGraphState::cleanup_pending;
            accepted = true;
        } else if (event == SceneGraphEvent::invalidate_scene_graph) {
            current_.state = SceneGraphState::invalidated;
            accepted = true;
        }
        break;
    case SceneGraphState::cleanup_pending:
        if (event == SceneGraphEvent::complete_cleanup) {
            current_.state = SceneGraphState::awaiting_initialization;
            accepted = true;
        } else if (event == SceneGraphEvent::invalidate_scene_graph) {
            current_.state = SceneGraphState::invalidated;
            accepted = true;
        }
        break;
    case SceneGraphState::invalidated:
        if (event == SceneGraphEvent::initialize_scene_graph) {
            if (current_.device_generation == std::numeric_limits<std::uint64_t>::max()) {
                return core::Result<SceneGraphEpoch>::failure(
                    invalid("render.lifecycle.generation",
                            core::ErrorCode::resource_limit));
            }
            ++current_.device_generation;
            current_.state = SceneGraphState::ready;
            accepted = true;
        } else if (event == SceneGraphEvent::detach_window) {
            current_.state = SceneGraphState::detached;
            accepted = true;
        }
        break;
    }
    if (!accepted) {
        return core::Result<SceneGraphEpoch>::failure(invalid("render.lifecycle.event"));
    }
    return core::Result<SceneGraphEpoch>::success(current_);
}

struct FrameLease::State final {
    explicit State(std::vector<std::byte> input)
        : bytes(std::move(input))
    {
    }

    std::vector<std::byte> bytes;
    std::shared_ptr<void> lifetime_guard;
};

FrameLease::FrameLease() = default;

FrameLease::FrameLease(std::vector<std::byte> bytes)
    : state_(std::make_shared<State>(std::move(bytes)))
{
}

const std::byte* FrameLease::data() const noexcept
{
    return state_ && !state_->bytes.empty() ? state_->bytes.data() : nullptr;
}

std::size_t FrameLease::size() const noexcept
{
    return state_ ? state_->bytes.size() : 0;
}

bool FrameLease::empty() const noexcept
{
    return size() == 0;
}

std::size_t FrameLease::use_count() const noexcept
{
    return state_ ? state_.use_count() : 0;
}

bool FrameLease::attach_lifetime_guard(std::shared_ptr<void> guard)
{
    if (!state_ || state_.use_count() != 1 || state_->lifetime_guard || !guard) {
        return false;
    }
    state_->lifetime_guard = std::move(guard);
    return true;
}

core::Result<RenderedFrame> validate_rendered_frame(RenderedFrame frame,
                                                    const RenderSnapshot& snapshot)
{
    if (!basic_frame_shape_valid(frame) || frame.snapshot_id != snapshot.id()
        || frame.timeline_revision != snapshot.recipe().timeline_revision
        || frame.width_px != snapshot.recipe().output.width_px
        || frame.height_px != snapshot.recipe().output.height_px
        || frame.pixel_format != snapshot.recipe().output.pixel_format
        || frame.color != snapshot.recipe().output.color
        || frame.alpha_mode != snapshot.recipe().output.alpha_mode
        || frame.row_order != snapshot.recipe().output.row_order) {
        return core::Result<RenderedFrame>::failure(invalid("render.frame"));
    }
    auto expected_time = frame_time_ns(snapshot.recipe(), frame.frame_index);
    if (!expected_time || frame.time_ns != expected_time.value()) {
        return core::Result<RenderedFrame>::failure(
            invalid("render.frame.time", core::ErrorCode::timestamp_mismatch));
    }
    return core::Result<RenderedFrame>::success(std::move(frame));
}

struct BoundedFrameQueue::Impl final {
    Impl(std::size_t frame_limit, std::uint64_t byte_limit)
        : max_frames(frame_limit), max_bytes(byte_limit)
    {
    }

    const std::size_t max_frames;
    const std::uint64_t max_bytes;
    mutable std::mutex mutex;
    FrameQueueState state{FrameQueueState::open};
    std::size_t outstanding_frames{};
    std::uint64_t outstanding_bytes{};
    std::deque<RenderedFrame> frames;
    std::optional<FrameFailure> failure;
};

BoundedFrameQueue::BoundedFrameQueue(std::size_t max_outstanding_frames,
                                     std::uint64_t max_outstanding_bytes)
    : impl_(std::make_shared<Impl>(max_outstanding_frames, max_outstanding_bytes))
{
}

BoundedFrameQueue::~BoundedFrameQueue() = default;
BoundedFrameQueue::BoundedFrameQueue(BoundedFrameQueue&&) noexcept = default;
BoundedFrameQueue& BoundedFrameQueue::operator=(BoundedFrameQueue&&) noexcept = default;

FramePublishStatus BoundedFrameQueue::publish(RenderedFrame frame)
{
    if (!impl_ || impl_->max_frames == 0 || impl_->max_bytes == 0
        || !basic_frame_shape_valid(frame) || frame.bytes.use_count() != 1) {
        return FramePublishStatus::invalid;
    }
    std::scoped_lock lock{impl_->mutex};
    switch (impl_->state) {
    case FrameQueueState::open:
        break;
    case FrameQueueState::draining:
    case FrameQueueState::ended:
        return FramePublishStatus::closed;
    case FrameQueueState::cancelled:
        return FramePublishStatus::cancelled;
    case FrameQueueState::failed:
        return FramePublishStatus::failed;
    }
    if (impl_->outstanding_frames >= impl_->max_frames
        || frame.valid_bytes > impl_->max_bytes - impl_->outstanding_bytes) {
        return FramePublishStatus::would_block;
    }

    const auto bytes = frame.valid_bytes;
    const std::weak_ptr<Impl> weak_impl{impl_};
    auto guard = std::shared_ptr<void>(new std::byte{}, [weak_impl, bytes](void* pointer) {
        delete static_cast<std::byte*>(pointer);
        if (const auto queue = weak_impl.lock()) {
            std::scoped_lock guard_lock{queue->mutex};
            --queue->outstanding_frames;
            queue->outstanding_bytes -= bytes;
            if (queue->state == FrameQueueState::draining
                && queue->outstanding_frames == 0) {
                queue->state = FrameQueueState::ended;
            }
        }
    });
    if (!frame.bytes.attach_lifetime_guard(std::move(guard))) {
        return FramePublishStatus::invalid;
    }
    ++impl_->outstanding_frames;
    impl_->outstanding_bytes += bytes;
    impl_->frames.push_back(std::move(frame));
    return FramePublishStatus::accepted;
}

FrameTakeResult BoundedFrameQueue::try_take()
{
    if (!impl_) {
        return FrameTakeResult{FrameTakeStatus::ended, std::nullopt, std::nullopt};
    }
    std::scoped_lock lock{impl_->mutex};
    if (!impl_->frames.empty()) {
        RenderedFrame frame = std::move(impl_->frames.front());
        impl_->frames.pop_front();
        return FrameTakeResult{FrameTakeStatus::frame,
                               std::move(frame),
                               std::nullopt};
    }
    switch (impl_->state) {
    case FrameQueueState::open:
    case FrameQueueState::draining:
        return FrameTakeResult{FrameTakeStatus::empty, std::nullopt, std::nullopt};
    case FrameQueueState::ended:
        return FrameTakeResult{FrameTakeStatus::ended, std::nullopt, std::nullopt};
    case FrameQueueState::cancelled:
        return FrameTakeResult{FrameTakeStatus::cancelled, std::nullopt, std::nullopt};
    case FrameQueueState::failed:
        return FrameTakeResult{FrameTakeStatus::failed, std::nullopt, impl_->failure};
    }
    return FrameTakeResult{FrameTakeStatus::failed, std::nullopt, impl_->failure};
}

void BoundedFrameQueue::drain()
{
    if (!impl_) {
        return;
    }
    std::scoped_lock lock{impl_->mutex};
    if (impl_->state == FrameQueueState::open) {
        impl_->state = impl_->outstanding_frames == 0 ? FrameQueueState::ended
                                                      : FrameQueueState::draining;
    }
}

void BoundedFrameQueue::cancel()
{
    if (!impl_) {
        return;
    }
    std::deque<RenderedFrame> discarded;
    {
        std::scoped_lock lock{impl_->mutex};
        if (impl_->state == FrameQueueState::open
            || impl_->state == FrameQueueState::draining) {
            impl_->state = FrameQueueState::cancelled;
            discarded.swap(impl_->frames);
        }
    }
}

void BoundedFrameQueue::fail(FrameFailure failure)
{
    if (!impl_) {
        return;
    }
    std::deque<RenderedFrame> discarded;
    {
        std::scoped_lock lock{impl_->mutex};
        if ((impl_->state == FrameQueueState::open
             || impl_->state == FrameQueueState::draining)
            && failure.device_generation > 0 && is_identifier(failure.diagnostic_id)) {
            impl_->state = FrameQueueState::failed;
            impl_->failure = std::move(failure);
            discarded.swap(impl_->frames);
        }
    }
}

FrameQueueState BoundedFrameQueue::state() const
{
    if (!impl_) {
        return FrameQueueState::ended;
    }
    std::scoped_lock lock{impl_->mutex};
    return impl_->state;
}

std::size_t BoundedFrameQueue::outstanding_frames() const
{
    if (!impl_) {
        return 0;
    }
    std::scoped_lock lock{impl_->mutex};
    return impl_->outstanding_frames;
}

std::uint64_t BoundedFrameQueue::outstanding_bytes() const
{
    if (!impl_) {
        return 0;
    }
    std::scoped_lock lock{impl_->mutex};
    return impl_->outstanding_bytes;
}

} // namespace space_rhythm::rendering
