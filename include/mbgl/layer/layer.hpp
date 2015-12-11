#ifndef MBGL_LAYER
#define MBGL_LAYER

#include <mbgl/util/noncopyable.hpp>

#include <memory>

namespace mbgl {

class Layer : public mbgl::util::noncopyable {
public:
    class Impl;

    Layer(std::unique_ptr<Impl> impl_)
        : impl(std::move(impl_)) {}

    virtual ~Layer() = default;

    const std::unique_ptr<Impl> impl;
};

} // namespace mbgl

#endif
