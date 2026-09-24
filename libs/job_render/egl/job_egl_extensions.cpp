#include "job_egl_extensions.h"

namespace job::render {

bool JobEglExtensions::queryClient()
{
    return query(EGL_NO_DISPLAY);
}

bool JobEglExtensions::queryDisplay(EGLDisplay display)
{
    if (display == EGL_NO_DISPLAY) {
        clear();
        return false;
    }

    return query(display);
}

bool JobEglExtensions::query(EGLDisplay display)
{
    clear();
    const char *extensions = eglQueryString(display, EGL_EXTENSIONS);
    if (!extensions)
        return false;

    const std::string_view extensionString{extensions};

    std::size_t begin = 0;

    while (begin < extensionString.size()) {
        while (begin < extensionString.size() && extensionString[begin] == ' ') {
            ++begin;
        }

        if (begin == extensionString.size())
            break;

        const std::size_t end = extensionString.find(' ', begin);
        if (end == std::string_view::npos) {
            m_extensions.emplace(extensionString.substr(begin));
            break;
        }

        m_extensions.emplace(extensionString.substr(begin, end - begin));
        begin = end + 1;
    }

    return true;
}

}