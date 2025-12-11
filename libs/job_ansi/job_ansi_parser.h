#pragma once

#include <string>
#include <string_view>
#include <vector>


#include "utils/utf8_decoder.h"

#include "job_ansi_cell.h"
#include "job_ansi_screen.h"

#include "esc/csi/csi.h"
#include "esc/esc.h"
#include "esc/osc.h"

namespace job::ansi {
using job::ansi::utils::Utf8Decoder;
class ANSIParser {
public:
    explicit ANSIParser(Screen *screen);

    void parseInput(std::string_view data);
    void parseInput(const std::string &data);
    void parseInput(const char *data);
    void parseInput(const std::vector<char> &data);
    void parseInput(const std::vector<uint8_t> &data);

private:

    // Parser state machine
    enum class ParserState {
        Ground,     // Normal character processing
        Escape,     // After ESC (0x1B)
        CSI,        // Control Sequence Introducer ESC[
        OSC,        // Operating System Command ESC]
        Charset,    // Charset designator (e.g. ESC ( B)
    };

    Screen                  *m_screen = nullptr;
    Utf8Decoder             m_utf8;
    std::string             m_buffer;
    ParserState             m_state = ParserState::Ground;
    csi::DispatchCSI        m_dispatchCSI;
    csi::DispatchESC        m_dispatchESC;
    csi::DispatchOSC        m_dispatchOSC;
    char32_t                m_prevChar = U'\0';

    // Core character processing loop
    void handleCodepoint(char32_t ch);

    // State machine transitions
    void enterEscape();
    void enterCSI();
    void enterOSC();
    void enterCharset();

    void appendToBuffer(char32_t ch);
    void resetParserState();

    void processCSI(std::string_view buf);
    void processOSC(std::string_view buf);
    void processESC(std::string_view buf);
    void processCharset(std::string_view buf);

    std::vector<int> parseParams(std::string_view paramStr) const;
};
}
