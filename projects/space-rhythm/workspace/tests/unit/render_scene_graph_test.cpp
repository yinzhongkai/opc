#include <space_rhythm/rendering/scene_graph_render_item.hpp>

#include <QSGGeometryNode>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <utility>

namespace {

namespace rendering = space_rhythm::rendering;

rendering::GeometryFrame make_frame(std::uint64_t generation,
                                    std::uint32_t batch_count,
                                    float offset = 0.0F)
{
    rendering::GeometryFrame frame;
    frame.snapshot_id = rendering::RenderSnapshotId{"snapshot-qsg"};
    frame.timeline_revision = 1;
    frame.device_generation = generation;
    frame.viewport_time_range = {0, 1'000'000'000};
    frame.target_width_px = 128;
    frame.target_height_px = 96;
    frame.template_id = "space-rhythm.waveform-oscilloscope";
    frame.template_version = "1.0.0";
    for (std::uint32_t index = 0; index < batch_count; ++index) {
        const auto x = offset + static_cast<float>(index);
        rendering::GeometryBatch batch;
        batch.kind = rendering::GeometryBatchKind::waveform;
        batch.vertices = {
            {x, 0.0F, 0x36, 0xd8, 0xff, 0xff},
            {x + 1.0F, 0.0F, 0x36, 0xd8, 0xff, 0xff},
            {x + 1.0F, 1.0F, 0x36, 0xd8, 0xff, 0xff},
            {x, 0.0F, 0x36, 0xd8, 0xff, 0xff},
            {x + 1.0F, 1.0F, 0x36, 0xd8, 0xff, 0xff},
            {x, 1.0F, 0x36, 0xd8, 0xff, 0xff},
        };
        frame.batches.push_back(std::move(batch));
    }
    frame.stats.batch_count = batch_count;
    return frame;
}

TEST(RenderSceneGraph, RGV_QSG_001_BatchesReuseNodesAndRebuildOnDeviceGeneration)
{
    const auto generation_one = make_frame(1, 2);
    rendering::SceneGraphSyncStats first_stats;
    QSGNode* root = rendering::synchronize_geometry_nodes(
        nullptr, generation_one, &first_stats);
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(first_stats.root_created);
    EXPECT_EQ(root->childCount(), 2);
    EXPECT_EQ(first_stats.created_node_count, 2U);

    rendering::SceneGraphSyncStats reuse_stats;
    QSGNode* reused = rendering::synchronize_geometry_nodes(
        root, generation_one, &reuse_stats);
    ASSERT_EQ(reused, root);
    EXPECT_FALSE(reuse_stats.device_generation_rebuilt);
    EXPECT_EQ(reuse_stats.reused_node_count, 2U);

    const auto generation_two = make_frame(2, 1);
    rendering::SceneGraphSyncStats rebuild_stats;
    root = rendering::synchronize_geometry_nodes(
        root, generation_two, &rebuild_stats);
    ASSERT_NE(root, nullptr);
    EXPECT_TRUE(rebuild_stats.device_generation_rebuilt);
    EXPECT_EQ(rebuild_stats.removed_node_count, 2U);
    EXPECT_EQ(rebuild_stats.created_node_count, 1U);
    delete root;
}

TEST(RenderSceneGraph, RGV_QSG_002_DynamicVertexBuffersUpdateInPlace)
{
    const auto first = make_frame(1, 1);
    QSGNode* root = rendering::synchronize_geometry_nodes(nullptr, first);
    ASSERT_NE(root, nullptr);
    auto* original_child = root->firstChild();

    const auto later = make_frame(1, 1, 10.0F);
    rendering::SceneGraphSyncStats update_stats;
    QSGNode* updated = rendering::synchronize_geometry_nodes(
        root, later, &update_stats);
    ASSERT_EQ(updated, root);
    EXPECT_EQ(root->firstChild(), original_child);
    EXPECT_EQ(update_stats.reused_node_count, 1U);
    EXPECT_EQ(update_stats.uploaded_vertex_count, 6U);
    const auto* geometry_node = static_cast<const QSGGeometryNode*>(root->firstChild());
    ASSERT_NE(geometry_node, nullptr);
    EXPECT_EQ(geometry_node->geometry()->vertexCount(), 6);
    delete root;
}

} // namespace
