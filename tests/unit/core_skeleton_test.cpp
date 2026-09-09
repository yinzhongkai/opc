#include <gtest/gtest.h>

#include <space_rhythm/build/core_skeleton.hpp>
#include <space_rhythm/media/media_adapter_skeleton.hpp>

TEST(BuildSkeleton, LinksCoreAndMediaAdapterTargets)
{
    space_rhythm::build::core_link_anchor();
    space_rhythm::media::media_adapter_link_anchor();
    EXPECT_EQ(space_rhythm::build::target_architecture, "x64");
    EXPECT_EQ(space_rhythm::build::runtime_linkage, "dynamic");
}
