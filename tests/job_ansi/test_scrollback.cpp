#include <catch2/catch_test_macros.hpp>
#include "job_ansi_scrollback.h"

using namespace job::ansi;

TEST_CASE("ScrollbackBuffer basic FIFO usage", "[ansi][scrollback][usage]")
{
    // Create a history of size 3
    ScrollbackBuffer history(3);

    SECTION("Pushing lines retains order") {
        // Push 1, 2, 3
        Cell::Line line1 = { Cell(U'1') };
        Cell::Line line2 = { Cell(U'2') };
        Cell::Line line3 = { Cell(U'3') };

        CHECK_FALSE(history.push_back(std::move(line1))); // Not full yet
        CHECK_FALSE(history.push_back(std::move(line2)));
        CHECK_FALSE(history.push_back(std::move(line3)));

        REQUIRE(history.size() == 3);

        // Access: 0 is oldest (1), 2 is newest (3)
        CHECK(history.at(0)[0].getChar() == U'1');
        CHECK(history.back()[0].getChar() == U'3');
    }

    SECTION("Overflow drops oldest line") {
        // "Out with the old, in with the new"
        history.push_back({ Cell(U'A') });
        history.push_back({ Cell(U'B') });
        history.push_back({ Cell(U'C') });

        // Push 'D', should drop 'A'
        Cell::Line lineD = { Cell(U'D') };
        bool dropped = history.push_back(std::move(lineD));

        REQUIRE(dropped == true);
        REQUIRE(history.size() == 3); // Max size maintained

        // Oldest should now be B
        CHECK(history.at(0)[0].getChar() == U'B');
        // Newest should be D
        CHECK(history.back()[0].getChar() == U'D');
    }
}

TEST_CASE("ScrollbackBuffer edge cases", "[ansi][scrollback][edge]")
{
    SECTION("Zero capacity buffer") {
        ScrollbackBuffer tiny(0);
        Cell::Line line = { Cell(U'X') };
        bool dropped = tiny.push_back(std::move(line));

        CHECK(tiny.size() == 0);
        CHECK(dropped == false);
    }

    SECTION("Resizing smaller truncates history") {
        ScrollbackBuffer history(10);
        for(int i=0; i<5; ++i)
            history.push_back({ Cell(U'A') }); // Fill 5

        REQUIRE(history.size() == 5);

        history.setMaxLines(2);

        REQUIRE(history.size() == 2);
        REQUIRE(history.maxLines() == 2);
    }

    SECTION("Accessing empty buffer throws") {
        ScrollbackBuffer empty(10);
        CHECK_THROWS_AS(empty.at(0), std::out_of_range);
        CHECK_THROWS_AS(empty.back(), std::out_of_range);
    }
}
