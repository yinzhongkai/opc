#include <space_rhythm/rendering/scene_graph_render_item.hpp>

#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGMaterial>
#include <QSGVertexColorMaterial>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace space_rhythm::rendering {
namespace {

class GeometryBatchNode final : public QSGGeometryNode {
public:
    explicit GeometryBatchNode(GeometryBatchKind input_kind)
        : kind(input_kind)
    {
        auto* geometry = new QSGGeometry(
            QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        setGeometry(geometry);
        setFlag(QSGNode::OwnsGeometry, true);

        auto* material = new QSGVertexColorMaterial;
        material->setFlag(QSGMaterial::Blending, true);
        setMaterial(material);
        setFlag(QSGNode::OwnsMaterial, true);
        setFlag(QSGNode::OwnedByParent, true);
    }

    GeometryBatchKind kind;
};

class GeometryRootNode final : public QSGNode {
public:
    std::optional<GeometryBatchSyncState> sync_state;
};

void delete_children(QSGNode& root, std::uint32_t* removed_count)
{
    while (auto* child = root.firstChild()) {
        root.removeChildNode(child);
        delete child;
        if (removed_count != nullptr) {
            ++*removed_count;
        }
    }
}

GeometryBatchNode* create_batch_node(GeometryBatchKind kind)
{
    return new GeometryBatchNode{kind};
}

void upload_batch(GeometryBatchNode& node, const GeometryBatch& batch)
{
    node.kind = batch.kind;
    auto* geometry = node.geometry();
    geometry->allocate(static_cast<int>(batch.vertices.size()));
    auto* output = geometry->vertexDataAsColoredPoint2D();
    for (std::size_t index = 0; index < batch.vertices.size(); ++index) {
        const auto& input = batch.vertices[index];
        output[index].set(input.x,
                          input.y,
                          input.red,
                          input.green,
                          input.blue,
                          input.alpha);
    }
    geometry->markVertexDataDirty();
    node.markDirty(QSGNode::DirtyGeometry);
}

} // namespace

QSGNode* synchronize_geometry_nodes(QSGNode* old_node,
                                     const GeometryFrame& frame,
                                     SceneGraphSyncStats* stats)
{
    SceneGraphSyncStats local_stats;
    auto* root = dynamic_cast<GeometryRootNode*>(old_node);
    if (root == nullptr) {
        delete old_node;
        root = new GeometryRootNode;
        local_stats.root_created = true;
    }
    auto update_plan = plan_geometry_batch_update(root->sync_state, frame);
    if (!update_plan) {
        delete root;
        if (stats != nullptr) {
            *stats = local_stats;
        }
        return nullptr;
    }
    if (update_plan.value().rebuild_for_device_generation) {
        delete_children(*root, &local_stats.removed_node_count);
        local_stats.device_generation_rebuilt = true;
    }

    while (root->childCount() > static_cast<int>(frame.batches.size())) {
        auto* child = root->lastChild();
        root->removeChildNode(child);
        delete child;
        ++local_stats.removed_node_count;
    }
    for (std::size_t index = 0; index < frame.batches.size(); ++index) {
        const auto& batch = frame.batches[index];
        if (batch.vertices.size()
            > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            delete root;
            if (stats != nullptr) {
                *stats = local_stats;
            }
            return nullptr;
        }
        GeometryBatchNode* node = nullptr;
        if (index < static_cast<std::size_t>(root->childCount())) {
            node = static_cast<GeometryBatchNode*>(
                root->childAtIndex(static_cast<int>(index)));
            ++local_stats.reused_node_count;
        } else {
            node = create_batch_node(batch.kind);
            root->appendChildNode(node);
            ++local_stats.created_node_count;
        }
        upload_batch(*node, batch);
        local_stats.uploaded_vertex_count += batch.vertices.size();
    }
    root->sync_state = update_plan.value().next_state;
    if (stats != nullptr) {
        *stats = local_stats;
    }
    return root;
}

SceneGraphRenderItem::SceneGraphRenderItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
    QObject::connect(this,
                     &QQuickItem::windowChanged,
                     this,
                     [this](QQuickWindow* window) { bind_window(window); });
    bind_window(window());
}

SceneGraphRenderItem::~SceneGraphRenderItem()
{
    QObject::disconnect(initialized_connection_);
    QObject::disconnect(invalidated_connection_);
}

void SceneGraphRenderItem::submit_render_snapshot(
    std::shared_ptr<const RenderSnapshot> snapshot)
{
    {
        std::scoped_lock lock{pending_mutex_};
        pending_snapshot_ = std::move(snapshot);
    }
    update();
}

std::shared_ptr<const RenderSnapshot> SceneGraphRenderItem::submitted_snapshot() const
{
    std::scoped_lock lock{pending_mutex_};
    return pending_snapshot_;
}

void SceneGraphRenderItem::set_viewport_time_range(core::TimeRange viewport)
{
    {
        std::scoped_lock lock{pending_mutex_};
        pending_viewport_ = viewport;
    }
    update();
}

void SceneGraphRenderItem::clear_viewport_time_range()
{
    {
        std::scoped_lock lock{pending_mutex_};
        pending_viewport_.reset();
    }
    update();
}

std::optional<core::TimeRange> SceneGraphRenderItem::viewport_time_range() const
{
    std::scoped_lock lock{pending_mutex_};
    return pending_viewport_;
}

void SceneGraphRenderItem::set_frame_index(FrameIndex frame_index)
{
    {
        std::scoped_lock lock{pending_mutex_};
        pending_frame_index_ = frame_index;
    }
    update();
}

FrameIndex SceneGraphRenderItem::frame_index() const
{
    std::scoped_lock lock{pending_mutex_};
    return pending_frame_index_;
}

std::uint64_t SceneGraphRenderItem::device_generation() const noexcept
{
    return device_generation_.load(std::memory_order_acquire);
}

QSGNode* SceneGraphRenderItem::updatePaintNode(QSGNode* old_node,
                                               UpdatePaintNodeData* data)
{
    static_cast<void>(data);
    std::shared_ptr<const RenderSnapshot> snapshot;
    std::optional<core::TimeRange> viewport;
    FrameIndex frame_index = 0;
    {
        std::scoped_lock lock{pending_mutex_};
        snapshot = pending_snapshot_;
        viewport = pending_viewport_;
        frame_index = pending_frame_index_;
    }
    const auto generation = device_generation();
    if (!snapshot || resources_invalidated_.load(std::memory_order_acquire)
        || generation == 0 || !std::isfinite(width()) || !std::isfinite(height())
        || width() <= 0.0 || height() <= 0.0) {
        delete old_node;
        return nullptr;
    }
    const auto rounded_width = std::ceil(width());
    const auto rounded_height = std::ceil(height());
    if (rounded_width > static_cast<qreal>(std::numeric_limits<std::uint32_t>::max())
        || rounded_height
            > static_cast<qreal>(std::numeric_limits<std::uint32_t>::max())) {
        delete old_node;
        return nullptr;
    }
    GeometryBuildRequest request;
    request.snapshot = snapshot;
    request.expected_snapshot_id = snapshot->id();
    request.expected_timeline_revision = snapshot->recipe().timeline_revision;
    request.viewport_time_range = viewport.value_or(snapshot->recipe().time_range);
    request.target_width_px = static_cast<std::uint32_t>(rounded_width);
    request.target_height_px = static_cast<std::uint32_t>(rounded_height);
    request.frame_index = frame_index;
    request.device_generation = generation;
    auto frame = build_geometry_frame(request);
    if (!frame) {
        delete old_node;
        return nullptr;
    }
    return synchronize_geometry_nodes(old_node, frame.value());
}

void SceneGraphRenderItem::releaseResources()
{
    // All current resources are QSGNode-owned and Qt destroys them on the render
    // thread. Out-of-tree GPU resources are intentionally not introduced in T-034.
}

void SceneGraphRenderItem::bind_window(QQuickWindow* window)
{
    QObject::disconnect(initialized_connection_);
    QObject::disconnect(invalidated_connection_);
    resources_invalidated_.store(true, std::memory_order_release);
    if (window == nullptr) {
        return;
    }
    initialized_connection_ = QObject::connect(
        window,
        &QQuickWindow::sceneGraphInitialized,
        this,
        [this]() {
            device_generation_.fetch_add(1, std::memory_order_acq_rel);
            resources_invalidated_.store(false, std::memory_order_release);
        },
        Qt::DirectConnection);
    invalidated_connection_ = QObject::connect(
        window,
        &QQuickWindow::sceneGraphInvalidated,
        this,
        [this]() {
            resources_invalidated_.store(true, std::memory_order_release);
        },
        Qt::DirectConnection);
    if (window->isSceneGraphInitialized()) {
        device_generation_.fetch_add(1, std::memory_order_acq_rel);
        resources_invalidated_.store(false, std::memory_order_release);
    }
}

GeometryFrameRenderItem::GeometryFrameRenderItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setFlag(QQuickItem::ItemHasContents, true);
}

GeometryFrameRenderItem::~GeometryFrameRenderItem() = default;

void GeometryFrameRenderItem::submit_geometry_frame(
    std::shared_ptr<const GeometryFrame> frame)
{
    {
        std::scoped_lock lock{pending_mutex_};
        pending_frame_ = std::move(frame);
    }
    update();
}

std::shared_ptr<const GeometryFrame>
GeometryFrameRenderItem::submitted_geometry_frame() const
{
    std::scoped_lock lock{pending_mutex_};
    return pending_frame_;
}

QSGNode* GeometryFrameRenderItem::updatePaintNode(QSGNode* old_node,
                                                  UpdatePaintNodeData* data)
{
    static_cast<void>(data);
    std::shared_ptr<const GeometryFrame> frame;
    {
        std::scoped_lock lock{pending_mutex_};
        frame = pending_frame_;
    }
    if (!frame) {
        delete old_node;
        return nullptr;
    }
    return synchronize_geometry_nodes(old_node, *frame);
}

} // namespace space_rhythm::rendering
