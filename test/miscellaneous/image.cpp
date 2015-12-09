#include "../fixtures/util.hpp"

#include <mbgl/util/premultiply.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/scaling.hpp>

using namespace mbgl;

TEST(Image, PNGRoundTrip) {
    PremultipliedImage rgba { 1, 1 };
    rgba.data[0] = 128;
    rgba.data[1] = 0;
    rgba.data[2] = 0;
    rgba.data[3] = 255;

    PremultipliedImage image = decodeImage(encodePNG(rgba));
    EXPECT_EQ(128, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(255, image.data[3]);
}

TEST(Image, PNGRoundTripAlpha) {
    PremultipliedImage rgba { 1, 1 };
    rgba.data[0] = 128;
    rgba.data[1] = 0;
    rgba.data[2] = 0;
    rgba.data[3] = 128;

    PremultipliedImage image = decodeImage(encodePNG(rgba));
    EXPECT_EQ(128, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(128, image.data[3]);
}

TEST(Image, PNGReadNoProfile) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/no_profile.png"));
    EXPECT_EQ(128, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(255, image.data[3]);
}

TEST(Image, PNGReadNoProfileAlpha) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/no_profile_alpha.png"));
    EXPECT_EQ(64, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(128, image.data[3]);
}

TEST(Image, PNGReadProfile) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/profile.png"));
    EXPECT_EQ(128, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(255, image.data[3]);
}

TEST(Image, PNGReadProfileAlpha) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/profile_alpha.png"));
    EXPECT_EQ(64, image.data[0]);
    EXPECT_EQ(0, image.data[1]);
    EXPECT_EQ(0, image.data[2]);
    EXPECT_EQ(128, image.data[3]);
}

TEST(Image, PNGTile) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/tile.png"));
    EXPECT_EQ(256, image.width);
    EXPECT_EQ(256, image.height);
}

TEST(Image, JPEGTile) {
    PremultipliedImage image = decodeImage(util::read_file("test/fixtures/image/tile.jpeg"));
    EXPECT_EQ(256, image.width);
    EXPECT_EQ(256, image.height);
}

TEST(Image, Premultiply) {
    UnassociatedImage rgba { 1, 1 };
    rgba.data[0] = 255;
    rgba.data[1] = 254;
    rgba.data[2] = 253;
    rgba.data[3] = 128;

    PremultipliedImage image = util::premultiply(std::move(rgba));
    EXPECT_EQ(128, image.data[0]);
    EXPECT_EQ(127, image.data[1]);
    EXPECT_EQ(127, image.data[2]);
    EXPECT_EQ(128, image.data[3]);
}

TEST(Image, BilinearScale) {
    PremultipliedImage src = decodeImage(util::read_file("test/fixtures/image/sprite.png"));
    PremultipliedImage dst { 128, 128 };
    uint32_t *dstData = reinterpret_cast<uint32_t*>(dst.data.get());
    std::fill(dstData, dstData + 128 * 128, 0xFFFF00FF);

    util::bilinearScale(src, { 0, 0, 24, 24 }, dst, { 8, 8, 24, 24 }, false);
    util::bilinearScale(src, { 26, 0, 24, 24 }, dst, { 0, 40, 48, 48 }, false);
    util::bilinearScale(src, { 26, 26, 24, 24 }, dst, { 52, 40, 36, 36 }, false);
    util::bilinearScale(src, { 26, 26, 24, 24 }, dst, { 52, 40, 36, 36 }, false);
    util::bilinearScale(src, { 104, 0, 24, 24 }, dst, { 96, 0, 48, 48 }, false);
    util::bilinearScale(src, { 52, 260, 24, 24 }, dst, { 108, 108, 38, 38 }, false);
    util::bilinearScale(src, { 380, 0, 24, 24 }, dst, { 36, 0, 24, 24 }, false);
    util::bilinearScale(src, { 396, 396, 24, 24 }, dst, { 0, 0, 50, 50 }, false);
    util::bilinearScale(src, { 380, 182, 12, 12 }, dst, { 52, 80, 24, 24 }, false);

    // From the bottom
    util::bilinearScale(src, { 252, 380, 12, 12 }, dst, { 0, 90, 12, 12 }, false);
    util::bilinearScale(src, { 252, 380, 12, 12 }, dst, { 18, 90, 24, 24 }, false);

    test::checkImage("test/fixtures/image/bilinear_scale", dst);
}
