#pragma once
#include <bitset>
#include <memory>
#include "job_ansi_cursor.h"
namespace job::ansi {

struct SavedBufferState {
    std::unique_ptr<Cursor> cursor;
    int scrollTop;
    int scrollBottom;
    std::bitset<512> tabStops;
};

}
