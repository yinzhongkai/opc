#pragma once

#include <space_rhythm/rendering/geometry_core.hpp>

#include <QMetaObject>
#include <QQuickItem>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

QT_BEGIN_NAMESPACE
class QQuickWindow;
class QSGNode;
QT_END_NAMESPACE

namespace space_rhythm::rendering {

struct SceneGraphSyncStats {
    bool root_created{false};
    bool device_generation_rebuilt{false};
    std::uint32_t created_node_count{};
    std::uint32_t reused_node_count{};
    std::uint32_t removed_node_count{};
    std::uint64_t uploaded_vertex_count{};

    bool operator==(const SceneGraphSyncStats&) const = default;
};

// Render-thread only. The returned node is owned by the QQuickItem scene graph.
QSGNode* synchronize_geometry_nodes(QSGNode* old_node,
                                     const GeometryFrame& frame,
                                     SceneGraphSyncStats* stats = nullptr);

class SceneGraphRenderItem final : public QQuickItem {
public:
    explicit SceneGraphRenderItem(QQuickItem* parent = nullptr);
    ~SceneGraphRenderItem() override;

    void submit_render_snapshot(std::shared_ptr<const RenderSnapshot> snapshot);
    [[nodiscard]] std::shared_ptr<const RenderSnapshot> submitted_snapshot() const;

    void set_viewport_time_range(core::TimeRange viewport);
    void clear_viewport_time_range();
    [[nodiscard]] std::optional<core::TimeRange> viewport_time_range() const;

    void set_frame_index(FrameIndex frame_index);
    [[nodiscard]] FrameIndex frame_index() const;
    [[nodiscard]] std::uint64_t device_generation() const noexcept;

protected:
    QSGNode* updatePaintNode(QSGNode* old_node,
                             UpdatePaintNodeData* data) override;
    void releaseResources() override;

private:
    void bind_window(QQuickWindow* window);

    mutable std::mutex pending_mutex_;
    std::shared_ptr<const RenderSnapshot> pending_snapshot_;
    std::optional<core::TimeRange> pending_viewport_;
    FrameIndex pending_frame_index_{};
    std::atomic_uint64_t device_generation_{};
    std::atomic_bool resources_invalidated_{true};
    QMetaObject::Connection initialized_connection_;
    QMetaObject::Connection invalidated_connection_;
};

} // namespace space_rhythm::rendering
