#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>

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
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stopwatch.stop();

        stopwatch.reset();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken == 0.0);
    }

    SECTION("Test single measurement") {
        // Measure some work
        stopwatch.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stopwatch.stop();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken >= 0.05);
        REQUIRE(time_taken < 0.1);
    }

    SECTION("Test multiple measurements") {
        // Measure some work
        stopwatch.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stopwatch.stop();

        // Do some other work
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Measure some work again
        stopwatch.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stopwatch.stop();

        double time_taken = stopwatch.get_time_taken_in_seconds();
        REQUIRE(time_taken >= 0.15);
        REQUIRE(time_taken < 0.25);
    }
}
