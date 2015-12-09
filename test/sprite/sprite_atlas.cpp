#include "../fixtures/util.hpp"
#include "../fixtures/fixture_log_observer.hpp"

#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/sprite/sprite_store.hpp>
#include <mbgl/sprite/sprite_parser.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/image.hpp>

using namespace mbgl;

TEST(Sprite, SpriteAtlas) {
    FixtureLog log;

    auto spriteParseResult = parseSprite(util::read_file("test/fixtures/sprites/emerald.png"),
                                         util::read_file("test/fixtures/sprites/emerald.json"));

    SpriteStore store(1);
    store.setSprites(spriteParseResult.get<Sprites>());

    SpriteAtlas atlas(63, 112, 1, store);

    EXPECT_EQ(1.0f, atlas.getPixelRatio());
    EXPECT_EQ(63, atlas.getWidth());
    EXPECT_EQ(112, atlas.getHeight());

    // Image hasn't been created yet.
    EXPECT_FALSE(atlas.getImage());

    ASSERT_TRUE(atlas.getIcon("metro").operator bool());
    SpriteAtlasImage metro = *atlas.getIcon("metro");

    EXPECT_EQ(0, metro.rect.x);
    EXPECT_EQ(0, metro.rect.y);
    EXPECT_EQ(20, metro.rect.w);
    EXPECT_EQ(20, metro.rect.h);
    EXPECT_EQ(18, metro.width);
    EXPECT_EQ(18, metro.height);
    EXPECT_DOUBLE_EQ(1.0f / 63, metro.tl[0]);
    EXPECT_DOUBLE_EQ(1.0f / 112, metro.tl[1]);
    EXPECT_DOUBLE_EQ(19.0f / 63, metro.br[0]);
    EXPECT_DOUBLE_EQ(19.0f / 112, metro.br[1]);

    EXPECT_TRUE(atlas.getImage());

    EXPECT_FALSE(atlas.getIcon("doesnotexist").operator bool());
    EXPECT_EQ(1u, log.count({
                      EventSeverity::Info,
                      Event::Sprite,
                      int64_t(-1),
                      "Can't find sprite named 'doesnotexist'",
                  }));

    // Different wrapping mode produces different image.
    ASSERT_TRUE(atlas.getPattern("metro").operator bool());
    SpriteAtlasImage metro2 = *atlas.getPattern("metro");

    EXPECT_EQ(20, metro2.rect.x);
    EXPECT_EQ(0, metro2.rect.y);
    EXPECT_EQ(20, metro2.rect.w);
    EXPECT_EQ(20, metro2.rect.h);
    EXPECT_EQ(18, metro2.width);
    EXPECT_EQ(18, metro2.height);

    test::checkImage("test/fixtures/sprites/sprite_atlas", *atlas.getImage());
}

TEST(Sprite, SpriteAtlasSize) {
    auto spriteParseResult = parseSprite(util::read_file("test/fixtures/sprites/emerald.png"),
                                         util::read_file("test/fixtures/sprites/emerald.json"));

    SpriteStore store(1);
    store.setSprites(spriteParseResult.get<Sprites>());

    SpriteAtlas atlas(63, 112, 1.4, store);

    EXPECT_DOUBLE_EQ(1.4f, atlas.getPixelRatio());
    EXPECT_EQ(63, atlas.getWidth());
    EXPECT_EQ(112, atlas.getHeight());

    ASSERT_TRUE(atlas.getIcon("metro").operator bool());
    SpriteAtlasImage metro = *atlas.getIcon("metro");

    EXPECT_EQ(0, metro.rect.x);
    EXPECT_EQ(0, metro.rect.y);
    EXPECT_EQ(20, metro.rect.w);
    EXPECT_EQ(20, metro.rect.h);
    EXPECT_EQ(18, metro.width);
    EXPECT_EQ(18, metro.height);

    test::checkImage("test/fixtures/sprites/sprite_atlas_size", *atlas.getImage());
}

//TEST(Sprite, SpriteAtlasUpdates) {
//    SpriteStore store(1);
//
//    SpriteAtlas atlas(32, 32, 1, store);
//
//    EXPECT_EQ(1.0f, atlas.getPixelRatio());
//    EXPECT_EQ(32, atlas.getWidth());
//    EXPECT_EQ(32, atlas.getHeight());
//
//    store.setSprite("one", std::make_shared<SpriteImage>(PremultipliedImage(16, 12), 1));
//
//    ASSERT_TRUE(atlas.getImage("one", false).operator bool());
//    SpriteAtlasElement one = *atlas.getImage("one", false);
//
//    EXPECT_EQ(0, one.pos.x);
//    EXPECT_EQ(0, one.pos.y);
//    EXPECT_EQ(20, one.pos.w);
//    EXPECT_EQ(16, one.pos.h);
//    EXPECT_EQ(16, one.originalW);
//    EXPECT_EQ(12, one.originalH);
//
//    const auto hash = test::crc64(reinterpret_cast<const char*>(atlas.getImage()->data.get()), atlas.getImage()->size());
//    EXPECT_EQ(0x0000000000000000u, hash) << std::hex << hash;
//
//    // Update sprite
//    auto newSprite = std::make_shared<SpriteImage>(PremultipliedImage(16, 12), 1); // 0xFF
//    store.setSprite("one", newSprite);
//    ASSERT_EQ(newSprite, store.getSprite("one"));
//
//    // Atlas texture hasn't changed yet.
//    const auto hash2 = test::crc64(reinterpret_cast<const char*>(atlas.getImage()->data.get()), atlas.getImage()->size());
//    EXPECT_EQ(0x0000000000000000u, hash2) << std::hex << hash2;
//
//    atlas.updateDirty();
//
//    test::checkImage("test/fixtures/sprites/sprite_atlas_updates", *atlas.getImage());
//}
