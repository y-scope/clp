#include <unistd.h>

#include <Catch2/single_include/catch2/catch.hpp>

#include "../src/clp/Stopwatch.hpp"

TEST_CASE("Stopwatch", "[Stopwatch]") {
    clp::Stopwatch stopwatch;

    SECTION("Test if initialized with 0.0") {
        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken == 0.0);
    }

    SECTION("Test reset()") {
        // Measure some work
        stopwatch.start();
        sleep(1);
        stopwatch.stop();

        stopwatch.reset();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken == 0.0);
    }

    SECTION("Test single measurement") {
        // Measure some work
        stopwatch.start();
        sleep(1);
        stopwatch.stop();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken >= 1.0);
        REQUIRE(time_taken < 1.1);
    }

    SECTION("Test multiple measurements") {
        // Measure some work
        stopwatch.start();
        sleep(1);
        stopwatch.stop();

        // Do some other work
        sleep(1);

        // Measure some work again
        stopwatch.start();
        sleep(2);
        stopwatch.stop();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken >= 3.0);
        REQUIRE(time_taken < 3.1);
    }
}
