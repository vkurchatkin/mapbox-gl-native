#include <mbgl/platform/platform.hpp>

#include <QCoreApplication>

namespace mbgl {
namespace platform {

const std::string& assetRoot() {
    static const std::string root = QCoreApplication::applicationDirPath().toStdString();
    return root;
}

}
}
