#include <mbgl/sprite/sprite_image.hpp>
#include <mbgl/util/exception.hpp>

namespace mbgl {

SpriteImage::SpriteImage(PremultipliedImage&& image_,
                         const float pixelRatio_,
                         bool sdf_)
    : image(std::move(image_)),
      pixelRatio(pixelRatio_),
      sdf(sdf_) {
    if (image.size() == 0 || pixelRatio == 0) {
        throw util::SpriteImageException("Sprite image dimensions may not be zero");
    }
}

} // namespace mbgl
