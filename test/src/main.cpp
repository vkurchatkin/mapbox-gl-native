#include <mbgl/test/util.hpp>

GTEST_API_ int main(int argc, char *argv[]) {
    auto server = std::make_unique<mbgl::test::Server>("test/storage/server.js");
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
