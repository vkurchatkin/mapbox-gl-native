#include <mbgl/sprite/sprite_parser.hpp>
#include <mbgl/sprite/sprite_image.hpp>

#include <mbgl/platform/log.hpp>

#include <mbgl/util/image.hpp>
#include <mbgl/util/rapidjson.hpp>

#include <cmath>
#include <limits>
#include <sstream>

namespace mbgl {

SpriteImagePtr createSpriteImage(const PremultipliedImage& src,
                                 size_t x, size_t y, size_t w, size_t h,
                                 double ratio, bool sdf) {
    // Disallow invalid parameter configurations and avoid overflow.
    if (w == 0 || h == 0 || w > 1024 || h > 1024 || ratio <= 0 || ratio > 10 ||
        std::fmod(w, ratio) != 0 || std::fmod(h, ratio) != 0 ||
        w > src.width || h > src.height || x > src.width - w || y > src.height - h) {
        Log::Warning(Event::Sprite, "Can't create sprite with invalid metrics");
        return nullptr;
    }

    PremultipliedImage dst(w, h);

    const uint32_t* srcData = reinterpret_cast<const uint32_t*>(src.data.get()) + src.width * y + x;
          uint32_t* dstData = reinterpret_cast<      uint32_t*>(dst.data.get());

    for (size_t i = 0; i < h; ++i) {
        for (size_t j = 0; j < w; ++j) {
            *dstData++ = *srcData++;
        }
        srcData += src.width - w;
    }

    return std::make_unique<const SpriteImage>(std::move(dst), ratio, sdf);
}

namespace {

inline uint16_t getUInt16(const JSValue& value, const char* name, const uint16_t def = 0) {
    if (value.HasMember(name)) {
        auto& v = value[name];
        if (v.IsUint() && v.GetUint() <= std::numeric_limits<uint16_t>::max()) {
            return v.GetUint();
        } else {
            Log::Warning(Event::Sprite, "Value of '%s' must be an integer between 0 and 65535",
                         name);
        }
    }

    return def;
}

inline double getDouble(const JSValue& value, const char* name, const double def = 0) {
    if (value.HasMember(name)) {
        auto& v = value[name];
        if (v.IsNumber()) {
            return v.GetDouble();
        } else {
            Log::Warning(Event::Sprite, "Value of '%s' must be a number", name);
        }
    }

    return def;
}

inline bool getBoolean(const JSValue& value, const char* name, const bool def = false) {
    if (value.HasMember(name)) {
        auto& v = value[name];
        if (v.IsBool()) {
            return v.GetBool();
        } else {
            Log::Warning(Event::Sprite, "Value of '%s' must be a boolean", name);
        }
    }

    return def;
}

} // namespace

SpriteParseResult parseSprite(const std::string& image, const std::string& json) {
    Sprites sprites;
    PremultipliedImage raster;

    try {
        raster = decodeImage(image);
    } catch (...) {
        return std::string("Could not parse sprite image");
    }

    JSDocument doc;
    doc.Parse<0>(json.c_str());

    if (doc.HasParseError()) {
        std::stringstream message;
        message << "Failed to parse JSON: " << rapidjson::GetParseError_En(doc.GetParseError()) << " at offset " << doc.GetErrorOffset();
        return message.str();
    } else if (!doc.IsObject()) {
        return std::string("Sprite JSON root must be an object");
    } else {
        for (JSValue::ConstMemberIterator itr = doc.MemberBegin(); itr != doc.MemberEnd(); ++itr) {
            const std::string name = { itr->name.GetString(), itr->name.GetStringLength() };
            const JSValue& value = itr->value;

            if (value.IsObject()) {
                const uint16_t x = getUInt16(value, "x", 0);
                const uint16_t y = getUInt16(value, "y", 0);
                const uint16_t width = getUInt16(value, "width", 0);
                const uint16_t height = getUInt16(value, "height", 0);
                const double pixelRatio = getDouble(value, "pixelRatio", 1);
                const bool sdf = getBoolean(value, "sdf", false);

                auto sprite = createSpriteImage(raster, x, y, width, height, pixelRatio, sdf);
                if (sprite) {
                    sprites.emplace(name, sprite);
                }
            }
        }
    }

    return sprites;
}

} // namespace mbgl
