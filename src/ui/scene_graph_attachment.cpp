#include <space_rhythm/ui/scene_graph_attachment.hpp>

#include <space_rhythm/rendering/scene_graph_render_item.hpp>

#include <QQuickItem>

namespace space_rhythm::ui {

rendering::SceneGraphRenderItem* attach_scene_graph_render_item(QQuickItem& host)
{
    auto* render_item = new rendering::SceneGraphRenderItem(&host);
    render_item->setObjectName(QStringLiteral("sceneGraphRenderItem"));
    render_item->setParentItem(&host);
    render_item->setSize(host.size());
    render_item->setVisible(host.isVisible());

    QObject::connect(&host,
                     &QQuickItem::widthChanged,
                     render_item,
                     [render_item, &host]() { render_item->setWidth(host.width()); });
    QObject::connect(&host,
                     &QQuickItem::heightChanged,
                     render_item,
                     [render_item, &host]() { render_item->setHeight(host.height()); });
    QObject::connect(&host,
                     &QQuickItem::visibleChanged,
                     render_item,
                     [render_item, &host]() { render_item->setVisible(host.isVisible()); });
    return render_item;
}

} // namespace space_rhythm::ui
