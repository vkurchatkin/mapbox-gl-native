#include <mbgl/text/shaping.hpp>
#include <mbgl/layer/symbol_layer.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>

namespace mbgl {

PositionedIcon shapeIcon(const SpriteAtlasImage& element, const SymbolLayoutProperties& layout) {
    float dx = layout.icon.offset.value[0];
    float dy = layout.icon.offset.value[1];
    float x1 = dx - element.width / 2.0f;
    float x2 = x1 + element.width;
    float y1 = dy - element.height / 2.0f;
    float y2 = y1 + element.height;

    return PositionedIcon(element.rect, y1, y2, x1, x2);
}

} // namespace mbgl
