#include "job_tui_mouse_area.h"

namespace job::tui {

JobTuiMouseArea::JobTuiMouseArea(JobTuiItem *parent) :
    JobTuiItem{parent}
{
    setItemHasContents(false);
    setAcceptsMouseEvents(true);
}

JobTuiMouseArea::Ptr JobTuiMouseArea::create(JobTuiItem *parent, bool fill)
{
    return JobTuiItem::createChild<JobTuiMouseArea>(parent, fill);
}

bool JobTuiMouseArea::mouseEvent(const Event &event)
{
    if (!isVisible())
        return false;

    const int ex = event.globalX();
    const int ey = event.globalY();
    const int x = this->x();
    const int y = this->y();
    const int w = this->width();
    const int h = this->height();

    if (ex >= x && ex < x + w && ey >= y && ey < y + h) {
        switch (event.type()) {
        case Event::EventType::MousePress:
            if (onClicked) onClicked(event);
            break;
        case Event::EventType::MouseRelease:
            if (onReleased) onReleased(event);
            break;
        case Event::EventType::MouseMove:
            if (onMoved) onMoved(event);
            break;
        default:
            break;
        }
        return true; // Handled
    }

    return false; // Outside bounds
}


}
