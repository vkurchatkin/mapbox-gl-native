#include <mbgl/util/scaling.hpp>
#include <mbgl/util/vec.hpp>

namespace {

using namespace mbgl;

inline uint8_t bilinearInterpolate(uint8_t tl, uint8_t tr, uint8_t bl, uint8_t br, double dx, double dy) {
    const double t = dx * (tr - tl) + tl;
    const double b = dx * (br - bl) + bl;
    return t + dy * (b - t);
}

template <size_t i>
inline const uint8_t& b(const uint32_t& w) {
    return reinterpret_cast<const uint8_t*>(&w)[i];
}

template <size_t i>
inline uint8_t& b(uint32_t& w) {
    return reinterpret_cast<uint8_t*>(&w)[i];
}

vec2<double> getFactor(const Rect<size_t>& srcPos, const Rect<size_t>& dstPos) {
    return {
        double(srcPos.w) / dstPos.w,
        double(srcPos.h) / dstPos.h
    };
}

vec2<size_t> getBounds(const vec2<size_t>& srcSize, const Rect<size_t>& srcPos,
                       const vec2<size_t>& dstSize, const Rect<size_t>& dstPos,
                       const vec2<double>& factor) {
    if (srcPos.x > srcSize.x || srcPos.y > srcSize.y ||
        dstPos.x > dstSize.x || dstPos.y > dstSize.y) {
        // Source or destination position is out of range.
        return { 0, 0 };
    }

    // Make sure we don't read/write values out of range.
    return { std::min(size_t(double(srcSize.x - srcPos.x) / factor.x),
                      std::min(dstSize.x - dstPos.x, dstPos.w)),
             std::min(size_t(double(srcSize.y - srcPos.y) / factor.y),
                      std::min(dstSize.y - dstPos.y, dstPos.h)) };
}
} // namespace

namespace mbgl {
namespace util {

void bilinearScale(const PremultipliedImage& src, const Rect<size_t>& srcPos,
                         PremultipliedImage& dst, const Rect<size_t>& dstPos,
                   bool wrap) {

    const uint32_t* srcData = reinterpret_cast<const uint32_t*>(src.data.get());
          uint32_t* dstData = reinterpret_cast<      uint32_t*>(dst.data.get());

    const vec2<size_t> srcSize { src.width, src.height };
    const vec2<size_t> dstSize { dst.width, dst.height };

    const auto factor = getFactor(srcPos, dstPos);
    const auto bounds = getBounds(srcSize, srcPos, dstSize, dstPos, factor);

    size_t x, y;
    size_t i = dstSize.x * dstPos.y + dstPos.x;
    for (y = 0; y < bounds.y; y++) {
        const double fractY = y * factor.y;
        const size_t Y0 = fractY;
        const size_t Y1 = wrap ? (Y0 + 1) % srcPos.h : (Y0 + 1);
        const size_t srcY0 = srcPos.y + Y0;
        const size_t srcY1 = std::min(srcPos.y + Y1, srcSize.y - 1);
        for (x = 0; x < bounds.x; x++) {
            const double fractX = x * factor.x;
            const size_t X0 = fractX;
            const size_t X1 = wrap ? (X0 + 1) % srcPos.w : (X0 + 1);
            const size_t srcX0 = srcPos.x + X0;
            const size_t srcX1 = std::min(srcPos.x + X1, srcSize.x - 1);

            const uint32_t tl = srcData[srcSize.x * srcY0 + srcX0];
            const uint32_t tr = srcData[srcSize.x * srcY0 + srcX1];
            const uint32_t bl = srcData[srcSize.x * srcY1 + srcX0];
            const uint32_t br = srcData[srcSize.x * srcY1 + srcX1];

            const double dx = fractX - X0;
            const double dy = fractY - Y0;
            uint32_t& dest = dstData[i + x];
            b<0>(dest) = bilinearInterpolate(b<0>(tl), b<0>(tr), b<0>(bl), b<0>(br), dx, dy);
            b<1>(dest) = bilinearInterpolate(b<1>(tl), b<1>(tr), b<1>(bl), b<1>(br), dx, dy);
            b<2>(dest) = bilinearInterpolate(b<2>(tl), b<2>(tr), b<2>(bl), b<2>(br), dx, dy);
            b<3>(dest) = bilinearInterpolate(b<3>(tl), b<3>(tr), b<3>(bl), b<3>(br), dx, dy);
        }
        i += dstSize.x;
    }
}

void nearestNeighborScale(const PremultipliedImage& src, const Rect<size_t>& srcPos,
                                PremultipliedImage& dst, const Rect<size_t>& dstPos) {
    const uint32_t* srcData = reinterpret_cast<const uint32_t*>(src.data.get());
          uint32_t* dstData = reinterpret_cast<      uint32_t*>(dst.data.get());

    const vec2<size_t> srcSize { src.width, src.height };
    const vec2<size_t> dstSize { dst.width, dst.height };

    const auto factor = getFactor(srcPos, dstPos);
    const auto bounds = getBounds(srcSize, srcPos, dstSize, dstPos, factor);

    double fractSrcY = srcPos.y;
    double fractSrcX;
    size_t i = dstSize.x * dstPos.y + dstPos.x;
    size_t srcY;
    size_t x, y;
    for (y = 0; y < bounds.y; y++) {
        fractSrcX = srcPos.x;
        srcY = srcSize.x * uint32_t(fractSrcY);
        for (x = 0; x < bounds.x; x++) {
            dstData[i + x] = srcData[srcY + uint32_t(fractSrcX)];
            fractSrcX += factor.x;
        }
        i += dstSize.x;
        fractSrcY += factor.y;
    }
}

} // namespace util
} // namespace mbgl
