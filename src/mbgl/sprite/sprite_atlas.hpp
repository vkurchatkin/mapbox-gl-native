#ifndef MBGL_SPRITE_ATLAS
#define MBGL_SPRITE_ATLAS

#include <mbgl/geometry/binpack.hpp>
#include <mbgl/platform/gl.hpp>
#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/image.hpp>

#include <mapbox/optional.hpp>

#include <string>
#include <map>
#include <mutex>
#include <atomic>
#include <set>
#include <array>

namespace mbgl {

class SpriteStore;

class SpriteAtlasImage {
public:
    Rect<uint16_t> rect;
    float width;
    float height;
    bool sdf;
    std::array<float, 2> tl;
    std::array<float, 2> br;
};

class SpriteAtlas : public util::noncopyable {
public:
    SpriteAtlas(uint16_t width, uint16_t height, float pixelRatio, SpriteStore&);
    ~SpriteAtlas();

    mapbox::util::optional<SpriteAtlasImage> getPattern(const std::string&);
    mapbox::util::optional<SpriteAtlasImage> getIcon(const std::string&);

    // Binds the atlas texture to the GPU, and uploads data if it is out of date.
    void bind(bool linear = false);

    // Uploads the texture to the GPU to be available when we need it. This is a lazy operation;
    // the texture is only bound when the data is out of date (=dirty).
    void upload();

    uint16_t getWidth() const { return width; }
    uint16_t getHeight() const { return height; }
    float getPixelRatio() const { return pixelRatio; }

	// Only for use in tests.
    const PremultipliedImage* getImage() const { return image.get(); }

private:
    using ImageMap = std::map<std::string, SpriteAtlasImage>;

    mapbox::util::optional<SpriteAtlasImage> getImage(const std::string& name, ImageMap&, bool pattern);

    const uint16_t width;
    const uint16_t height;
    const float pixelRatio;
    std::recursive_mutex mtx;
    SpriteStore& store;
    BinPack<uint16_t> bin;
    std::map<std::string, SpriteAtlasImage> icons;
    std::map<std::string, SpriteAtlasImage> patterns;
    std::set<std::string> uninitialized;
    std::unique_ptr<PremultipliedImage> image;
    std::atomic<bool> dirty;
    bool fullUploadRequired = true;
    GLuint texture = 0;
    uint32_t filter = 0;
};

} // namespace mbgl

#endif
