#include "job_ansi_utils.h"
#include <cctype>
#include <string>

namespace job::ansi::utils {

// CSI = ESC [ ... terminated by '@'–'~'
bool isCsiTerminator(char ch)
{
    return ch >= '@' && ch <= '~';
}

// ESC final = alphabetic or symbol
bool isEscFinal(char ch)
{
    return std::isalpha(static_cast<unsigned char>(ch)) || ch == '\\';
}

// OSC = ESC ] ... terminated by BEL (\a) or ST (ESC \)
bool isOscTerminator(const std::string &buffer)
{
    return buffer.ends_with('\a') || isStSequence(buffer);
}

// ST = ESC
bool isStSequence(const std::string &buffer)
{
    return buffer.size() >= 2 &&
           static_cast<unsigned char>(buffer[buffer.size() - 2]) == 0x1B &&
           buffer.back() == '\\';
}

}
