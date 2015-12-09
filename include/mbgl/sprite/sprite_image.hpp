#ifndef MBGL_SPRITE_IMAGE
#define MBGL_SPRITE_IMAGE

#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/image.hpp>

namespace mbgl {

class SpriteImage : private util::noncopyable {
public:
    SpriteImage(PremultipliedImage&&, float pixelRatio, bool sdf = false);

    const PremultipliedImage image;
    const float pixelRatio;
    const bool sdf;
};

} // namespace mbgl

#endif
