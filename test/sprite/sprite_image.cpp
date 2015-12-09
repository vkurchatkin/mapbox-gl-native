#include "../fixtures/util.hpp"

#include <mbgl/sprite/sprite_image.hpp>
#include <mbgl/util/exception.hpp>

using namespace mbgl;

TEST(Sprite, SpriteImageZeroWidth) {
    try {
        SpriteImage(PremultipliedImage(0, 16), 2);
        FAIL() << "Expected exception";
    } catch (util::SpriteImageException& ex) {
        EXPECT_STREQ("Sprite image dimensions may not be zero", ex.what());
    }
}

TEST(Sprite, SpriteImageZeroHeight) {
    try {
        SpriteImage(PremultipliedImage(16, 0), 2);
        FAIL() << "Expected exception";
    } catch (util::SpriteImageException& ex) {
        EXPECT_STREQ("Sprite image dimensions may not be zero", ex.what());
    }
}

TEST(Sprite, SpriteImageZeroRatio) {
    try {
        SpriteImage(PremultipliedImage(16, 16), 0);
        FAIL() << "Expected exception";
    } catch (util::SpriteImageException& ex) {
        EXPECT_STREQ("Sprite image dimensions may not be zero", ex.what());
    }
}

TEST(Sprite, SpriteImage) {
    SpriteImage sprite(PremultipliedImage(32, 24), 2);
    EXPECT_EQ(32, sprite.image.width);
    EXPECT_EQ(24, sprite.image.height);
    EXPECT_EQ(2, sprite.pixelRatio);
    EXPECT_EQ(32u * 24 * 4, sprite.image.size());
}

TEST(Sprite, SpriteImageFractionalRatio) {
    SpriteImage sprite(PremultipliedImage(20, 12), 1.5);
    EXPECT_EQ(20, sprite.image.width);
    EXPECT_EQ(12, sprite.image.height);
    EXPECT_EQ(1.5, sprite.pixelRatio);
    EXPECT_EQ(20u * 12 * 4, sprite.image.size());
}
