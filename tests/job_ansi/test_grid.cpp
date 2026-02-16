#include <catch2/catch_test_macros.hpp>

#ifdef JOB_TEST_BENCHMARKS
#include <catch2/benchmark/catch_benchmark.hpp>
#endif

#include <job_ansi_grid.h>

using namespace job::ansi;

TEST_CASE("Grid basic usage and access", "[ansi][grid][usage]")
{
    // Initialize a standard terminal size
    Grid grid(24, 80);

    SECTION("Grid dimensions are correct on init") {
        REQUIRE(grid.rows() == 24);
        REQUIRE(grid.columns() == 80);
    }

    SECTION("Accessing cells works as expected") {
        // Grab a pointer to a specific cell
        Cell* cell = grid.at(5, 10);
        REQUIRE(cell != nullptr);

        // Modify it
        cell->setChar(U'X');

        // Verify we can read it back via const access
        const Grid& constGrid = grid;
        const Cell* readBack = constGrid.at(5, 10);

        REQUIRE(readBack != nullptr);
        REQUIRE(readBack->getChar() == U'X');
    }

    SECTION("Row views provide span access") {
        // "I walk the line..." - Johnny Cash
        auto rowSpan = grid.row(0);
        REQUIRE(rowSpan.size() == 80);

        // Modify via span
        rowSpan[0].setChar(U'A');

        // Check backing store
        REQUIRE(grid.at(0, 0)->getChar() == U'A');
    }
}

TEST_CASE("Grid line operations", "[ansi][grid][ops]")
{
    Grid grid(4, 4);
    Cell fillChar(U'#');

    // Setup:
    // A A A A
    // B B B B
    // C C C C
    // D D D D
    for(int r=0; r<4; ++r) {
        char32_t ch = U'A' + r;
        for(int c=0; c<4; ++c) grid.at(r,c)->setChar(ch);
    }

    SECTION("insertLines pushes content down") {
        // Insert 1 line at row 1.
        // Expected:
        // A A A A
        // # # # #  <-- Inserted
        // B B B B  <-- Moved down
        // C C C C  <-- Moved down (D is lost)

        grid.insertLines(1, 1, fillChar);

        CHECK(grid.at(0, 0)->getChar() == U'A');
        CHECK(grid.at(1, 0)->getChar() == U'#');
        CHECK(grid.at(2, 0)->getChar() == U'B');
        CHECK(grid.at(3, 0)->getChar() == U'C');
    }

    SECTION("deleteLines pulls content up") {
        // Delete 1 line at row 1.
        // Expected:
        // A A A A
        // C C C C  <-- Pulled up
        // D D D D  <-- Pulled up
        // # # # #  <-- Filled at bottom

        grid.deleteLines(1, 1, fillChar);

        CHECK(grid.at(0, 0)->getChar() == U'A');
        CHECK(grid.at(1, 0)->getChar() == U'C');
        CHECK(grid.at(2, 0)->getChar() == U'D');
        CHECK(grid.at(3, 0)->getChar() == U'#');
    }
}


TEST_CASE("Grid edge cases and boundaries", "[ansi][grid][edge]")
{
    SECTION("Zero or negative dimensions enforce minimums") {
        // "You can't have a 0x0 terminal, Dave."
        Grid grid(0, 0);
        CHECK(grid.rows() >= 1);
        CHECK(grid.columns() >= 1);
    }

    SECTION("Out of bounds access returns nullptr") {
        Grid grid(10, 10);
        CHECK(grid.at(-1, 0) == nullptr);
        CHECK(grid.at(0, -1) == nullptr);
        CHECK(grid.at(10, 5) == nullptr); // Row 10 is OOB (0-9)
        CHECK(grid.at(5, 10) == nullptr); // Col 10 is OOB (0-9)
    }

    SECTION("Row view throws on invalid index") {
        Grid grid(5, 5);
        CHECK_THROWS_AS(grid.row(99), std::out_of_range);
        CHECK_THROWS_AS(grid.row(-1), std::out_of_range);
    }

    SECTION("Operations with zero count do nothing") {
        Grid grid(4, 4);
        grid.at(0,0)->setChar(U'A');

        Cell fill(U'X');
        grid.insertLines(0, 0, fill); // Should do nothing
        grid.deleteLines(0, 0, fill); // Should do nothing

        CHECK(grid.at(0,0)->getChar() == U'A');
    }
}


#ifdef JOB_TEST_BENCHMARKS

TEST_CASE("Grid performance benchmarks", "[ansi][grid][benchmark]")
{
    // Simulate a full 1080p terminal buffer
    int rows = 60;
    int cols = 200;
    Grid grid(rows, cols);
    Cell fillChar(U' ');

    BENCHMARK("Clear Grid") {
        grid.reset(fillChar);
    };

    BENCHMARK("Scroll Up (delete top line)") {
        // This is the most common operation in a terminal (scrolling text)
        grid.deleteLines(0, 1, fillChar);
    };

    BENCHMARK("Insert Line Middle") {
        // Vim-style editing
        grid.insertLines(rows / 2, 1, fillChar);
    };
}

#endif
