#include <mbgl/layer/layer_impl.hpp>

namespace mbgl {

const std::string& Layer::Impl::bucketName() const {
    return ref.empty() ? id : ref;
}

bool Layer::Impl::hasRenderPass(RenderPass pass) const {
    return bool(passes & pass);
}

} // namespace mbgl
