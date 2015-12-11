#include "../fixtures/util.hpp"

#include <mbgl/layer/layer_impl.hpp>
#include <mbgl/layer/background_layer.hpp>

using namespace mbgl;

TEST(Layer::Impl, Create) {
    std::unique_ptr<Layer::Impl> layer = std::make_unique<BackgroundLayer>();
    EXPECT_TRUE(reinterpret_cast<BackgroundLayer*>(layer.get()));
}

TEST(Layer::Impl, Clone) {
    std::unique_ptr<Layer::Impl> layer = std::make_unique<BackgroundLayer>();
    std::unique_ptr<Layer::Impl> clone = layer->clone();
    EXPECT_NE(layer.get(), clone.get());
    EXPECT_TRUE(reinterpret_cast<BackgroundLayer*>(layer.get()));
}

TEST(Layer::Impl, CloneCopiesBaseProperties) {
    std::unique_ptr<BackgroundLayer> layer = std::make_unique<BackgroundLayer>();
    layer->id = "test";
    EXPECT_EQ("test", layer->clone()->id);
}
