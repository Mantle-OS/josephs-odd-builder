#pragma once
#include "job_tui_item.h"
namespace job::tui {
struct JobTuiAnchors
{
    using Ptr = std::shared_ptr<JobTuiAnchors>;
    JobTuiItem *top                 = nullptr;
    JobTuiItem *bottom              = nullptr;
    JobTuiItem *left                = nullptr;
    JobTuiItem *right               = nullptr;
    JobTuiItem *horizontalCenter    = nullptr;
    JobTuiItem *verticalCenter      = nullptr;
    JobTuiItem *fill                = nullptr;
    JobTuiItem *centerIn            = nullptr;
    int topMargin                   = 0;
    int bottomMargin                = 0;
    int leftMargin                  = 0;
    int rightMargin                 = 0;

    void setMargins(int margin)
    {
        topMargin = margin;
        bottomMargin = margin;
        leftMargin = margin;
        rightMargin = margin;
    }

    bool isAnchored() const
    {
        return top || bottom || left || right ||
               fill || centerIn ||
               horizontalCenter || verticalCenter;
    }

    void clear()
    {
        top = bottom = left = right = nullptr;
        horizontalCenter = verticalCenter = nullptr;
        fill = centerIn = nullptr;
        topMargin = bottomMargin = leftMargin = rightMargin = 0;
    }
};
} // namespace job::tui
//
