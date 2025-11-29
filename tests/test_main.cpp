#include <catch2/catch_test_macros.hpp>
#include <pnq/pnq.h>

TEST_CASE("Version is defined", "[version]") {
    REQUIRE(pnq::version_major == 0);
    REQUIRE(pnq::version_minor == 1);
    REQUIRE(pnq::version_patch == 0);
}
