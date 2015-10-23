#ifndef QMAPBOXGL_P_H
#define QMAPBOXGL_P_H

#include <mbgl/map/map.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/platform/qt/qmapboxgl.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/util/run_loop.hpp>

#include <QSize>

#include <memory>

namespace mbgl {

class Map;
class SQLiteCache;

}  // namespace mbgl

class QMapboxGLPrivate : public QObject, public mbgl::View
{
    Q_OBJECT

public:
    explicit QMapboxGLPrivate(QMapboxGL *q);
    virtual ~QMapboxGLPrivate();

    // mbgl::View implementation.
    float getPixelRatio() const final;
    std::array<uint16_t, 2> getSize() const final;
    std::array<uint16_t, 2> getFramebufferSize() const final;

    void activate() final {}
    void deactivate() final {}
    void notify() final {}
    void invalidate() final;
    void beforeRender() final {}
    void afterRender() final {}
    void notifyMapChange(mbgl::MapChange change) final;

    QSize size;

    QMapboxGL *q_ptr = nullptr;

    mbgl::util::RunLoop loop;

    std::unique_ptr<mbgl::SQLiteCache> cacheObj;
    std::unique_ptr<mbgl::DefaultFileSource> fileSourceObj;
    std::unique_ptr<mbgl::Map> mapObj;

signals:
    void mapRegionDidChange();
    void needsRendering();
};

#endif // QMAPBOXGL_P_H
