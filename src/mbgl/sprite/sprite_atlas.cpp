#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/sprite/sprite_store.hpp>
#include <mbgl/platform/gl.hpp>
#include <mbgl/platform/log.hpp>
#include <mbgl/util/gl_object_store.hpp>
#include <mbgl/util/thread_context.hpp>

#include <cmath>

namespace mbgl {

SpriteAtlas::SpriteAtlas(uint16_t width_, uint16_t height_, float pixelRatio_, SpriteStore& store_)
    : width(width_),
      height(height_),
      pixelRatio(pixelRatio_),
      store(store_),
      bin(width_, height_),
      dirty(true) {
}

mapbox::util::optional<SpriteAtlasImage> SpriteAtlas::getIcon(const std::string& name) {
    return getImage(name, icons, false);
}

mapbox::util::optional<SpriteAtlasImage> SpriteAtlas::getPattern(const std::string& name) {
    return getImage(name, patterns, true);
}

mapbox::util::optional<SpriteAtlasImage> SpriteAtlas::getImage(const std::string& name, ImageMap& images, bool pattern) {
    std::lock_guard<std::recursive_mutex> lock(mtx);

    auto it = images.find(name);
    if (it != images.end()) {
        return it->second;
    }

    std::shared_ptr<const SpriteImage> src = store.getSprite(name);
    if (!src) {
        return {};
    }

    float w = src->image.width / src->pixelRatio,
          h = src->image.height / src->pixelRatio;

    // Pad icons to prevent them from polluting neighbours during linear interpolation.
    uint16_t padding = 1,
             paddedWidth = std::ceil(w) + 2 * padding,
             paddedHeight = std::ceil(h) + 2 * padding;

    // Increase to next number divisible by 4, but at least 1. This is so we can scale
    // down the texture coordinates and pack them into 2 bytes rather than 4 bytes.
    uint16_t packedWidth = paddedWidth + ((4 - paddedWidth % 4) % 4),
             packedHeight = paddedHeight + ((4 - paddedHeight % 4) % 4);

    Rect<uint16_t> dst = bin.allocate(packedWidth, packedHeight);
    if (dst.w == 0) {
        Log::Warning(Event::Sprite, "sprite atlas bitmap overflow");
        return {};
    }

    uint16_t dstX = dst.x + padding,
             dstY = dst.y + padding;

    SpriteAtlasImage& icon = images[name];

    icon.rect = dst;
    icon.width = w,
    icon.height = h;
    icon.sdf = src->sdf;
    icon.tl = std::array<float, 2> {{ float(dstX) / width,
                                      float(dstY) / height }};
    icon.br = std::array<float, 2> {{ float(dstX + w) / width,
                                      float(dstY + h) / height }};

    if (!image) {
        image = std::make_unique<PremultipliedImage>(std::ceil(width * pixelRatio),
                                                     std::ceil(height * pixelRatio));
    }

    dirty = true;

    const uint32_t* srcData = reinterpret_cast<const uint32_t*>(src->image.data.get());
          uint32_t* dstData = reinterpret_cast<      uint32_t*>(image->data.get());

    uint16_t srcStride = src->image.width,
             dstStride = std::ceil(width * pixelRatio),
             srcI = 0,
             dstI = (dstY * dstStride + dstX) * pixelRatio;

    if (pattern) {
        // Add 1 pixel wrapped padding on each side of the icon.
        dstI -= dstStride;
        for (int y = -1; y <= int(src->image.height); y++, srcI = ((y + src->image.height) % src->image.height) * srcStride, dstI += dstStride) {
            for (int x = -1; x <= int(src->image.width); x++) {
                dstData[dstI + x] = srcData[srcI + ((x + src->image.width) % src->image.width)];
            }
        }
        
    } else {
        for (size_t y = 0; y < src->image.height; y++, srcI += srcStride, dstI += dstStride) {
            for (size_t x = 0; x < src->image.width; x++) {
                dstData[dstI + x] = srcData[srcI + x];
            }
        }
    }

    return icon;
}

void SpriteAtlas::upload() {
    if (dirty) {
        bind();
    }
}

void SpriteAtlas::bind(bool linear) {
    if (!image) {
        return; // Empty atlas
    }

    if (!texture) {
        MBGL_CHECK_ERROR(glGenTextures(1, &texture));
        MBGL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
#ifndef GL_ES_VERSION_2_0
        MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0));
#endif
        // We are using clamp to edge here since OpenGL ES doesn't allow GL_REPEAT on NPOT textures.
        // We use those when the pixelRatio isn't a power of two, e.g. on iPhone 6 Plus.
        MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        fullUploadRequired = true;
    } else {
        MBGL_CHECK_ERROR(glBindTexture(GL_TEXTURE_2D, texture));
    }

    GLuint filter_val = linear ? GL_LINEAR : GL_NEAREST;
    if (filter_val != filter) {
        MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter_val));
        MBGL_CHECK_ERROR(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_val));
        filter = filter_val;
    }

    if (dirty) {
        std::lock_guard<std::recursive_mutex> lock(mtx);
        
        if (fullUploadRequired) {
            MBGL_CHECK_ERROR(glTexImage2D(
                GL_TEXTURE_2D, // GLenum target
                0, // GLint level
                GL_RGBA, // GLint internalformat
                GLsizei(image->width),
                GLsizei(image->height),
                0, // GLint border
                GL_RGBA, // GLenum format
                GL_UNSIGNED_BYTE, // GLenum type
                image->data.get() // const GLvoid * data
            ));
            fullUploadRequired = false;
        } else {
            MBGL_CHECK_ERROR(glTexSubImage2D(
                GL_TEXTURE_2D, // GLenum target
                0, // GLint level
                0, // GLint xoffset
                0, // GLint yoffset
                GLsizei(image->width),
                GLsizei(image->height),
                GL_RGBA, // GLenum format
                GL_UNSIGNED_BYTE, // GLenum type
                image->data.get() // const GLvoid *pixels
            ));
        }

        dirty = false;
    }
};

SpriteAtlas::~SpriteAtlas() {
    std::lock_guard<std::recursive_mutex> lock(mtx);
    if (texture) {
        mbgl::util::ThreadContext::getGLObjectStore()->abandonTexture(texture);
        texture = 0;
    }
}

} // namespace mbgl
