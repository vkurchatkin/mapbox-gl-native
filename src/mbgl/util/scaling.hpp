#ifndef MBGL_UTIL_SCALING
#define MBGL_UTIL_SCALING

#include <mbgl/util/rect.hpp>
#include <mbgl/util/image.hpp>

#include <cstdint>

namespace mbgl {
namespace util {

void bilinearScale(const PremultipliedImage& src, const Rect<size_t>& srcPos,
                         PremultipliedImage& dst, const Rect<size_t>& dstPos,
                   bool wrap);

void nearestNeighborScale(const PremultipliedImage& src, const Rect<size_t>& srcPos,
                                PremultipliedImage& dst, const Rect<size_t>& dstPos);

} // namespace util
} // namespace mbgl

#endif
