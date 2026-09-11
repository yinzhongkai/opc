#pragma once

class QQuickItem;

namespace space_rhythm::rendering {
class SceneGraphRenderItem;
}

namespace space_rhythm::ui {

// Creates the rendering-owned item as a child of a QML layout host. QML never
// receives RenderSnapshot, FrameIndex, geometry or render-thread resources.
[[nodiscard]] rendering::SceneGraphRenderItem* attach_scene_graph_render_item(
    QQuickItem& host);

} // namespace space_rhythm::ui
