#include "job_ansi_parser.h"

#include <charconv>

namespace job::ansi {

ANSIParser::ANSIParser(Screen *screen):
    m_screen(screen),
    m_dispatchCSI(screen),
    m_dispatchESC(screen),
    m_dispatchOSC(screen)
{
    m_buffer.reserve(256);
}

void ANSIParser::parseInput(std::string_view data)
{
    // JOB_LOG_DEBUG("[ANSIParser] parseInput len=" << data.size() << "\n";

    for (char byte : data) {
        m_utf8.appendByte(byte);

        while (m_utf8.hasChar()) {
            char32_t ch = m_utf8.takeChar();
            handleCodepoint(ch);
        }
    }
    // JOB_LOG_DEBUG("[Parser] End input. Cursor at ( {} , {} )"
    //           , m_screen->cursor()->row()
    //           , m_screen->cursor()->col() );
}

void ANSIParser::parseInput(const std::string &data)
{
    parseInput(std::string_view(data));
}

void ANSIParser::parseInput(const char *data)
{
    parseInput(std::string_view(data));
}

void ANSIParser::parseInput(const std::vector<char> &data)
{
    parseInput(std::string_view(data.data(), data.size()));
}

void ANSIParser::parseInput(const std::vector<uint8_t> &data)
{
    parseInput(std::string_view(reinterpret_cast<const char *>(data.data()), data.size()));
}
void ANSIParser::handleCodepoint(char32_t ch)
{
    // auto chex = std::hex << static_cast<int>(ch);
    // JOB_LOG_DEBUG("[Parser] State={} ch= {} buffer={}"
    // , static_cast<int>(m_state)
    // , chex
    // , m_buffer);

    if (m_state != ParserState::Ground && m_buffer.size() > kMaxSequenceLength) {
        // std::cerr << "[Parser] Sequence too long, resetting.\n";
        resetParserState();
    }

    switch (m_state) {
    case ParserState::Ground:
        if (ch == 0x1B) {
            enterEscape();
            break;
        }

        switch (ch) {
        case U'\r': {
            if (m_screen && m_screen->cursor()) {
                m_screen->setCursor(m_screen->cursor()->row(), 0);
                // JOB_LOG_DEBUG("[Parser] Carriage return (\\r) at row={}", m_screen->cursor()->row());
            }
            break;
        }
        case U'\n': {
            if (m_screen && m_screen->cursor()) {
                int row = m_screen->cursor()->row();
                if (row == m_screen->scrollBottom() - 1) {
                    m_screen->scrollUp();
                    // JOB_LOG_DEBUG("[Parser] Scroll up at bottom row={}", row);;
                } else {
                    m_screen->moveCursor(1, 0);
                    // JOB_LOG_DEBUG("[Parser] Newline (\\n) at row={}", row );
                }
            }
            break;
        }
        case U'\b': {
            m_screen->moveCursor(0, -1);
            break;
        }
        case U'\t': {
            if (Cursor *cursor = m_screen->cursor()) {
                int col = cursor->column();
                int maxCol = m_screen->columnCount();
                do {
                    ++col;
                } while (col < maxCol - 1 && !m_screen->isTabStop(col));
                if (col >= maxCol) col = maxCol - 1;
                m_screen->setCursor(cursor->row(), col);
                JOB_LOG_DEBUG("[Parser] Tab (\\t) to col={}", col);
            }
            break;
        }
        default:
            if (ch >= 0x20) {
                char32_t resolved = m_screen->resolveCharsetChar(ch);
                m_dispatchCSI.setLastPrintedChar(resolved);
                m_screen->putChar(resolved);  // uses currentAttributes internally
            }
            break;
        }

        // Clamp cursor just in case
        if (m_screen && m_screen->cursor()) {
            int row = std::min(m_screen->cursor()->row(), m_screen->rowCount() - 1);
            int col = std::min(m_screen->cursor()->column(), m_screen->columnCount() - 1);
            m_screen->setCursor(row, col);
        }
        break;

    case ParserState::Escape:
        appendToBuffer(ch);
        switch (ch) {
        case '[':
            enterCSI();
            break;
        case ']':
            enterOSC();
            break;
        case '(':
        case ')':
        case '*':
        case '+': enterCharset();
            break;
        default:
            processESC(std::string_view(m_buffer).substr(1));
            resetParserState();
            break;
        }
        break;

    case ParserState::CSI:
        appendToBuffer(ch);
        if (ch >= '@' && ch <= '~') {
            processCSI(m_buffer.substr(2));  // skip ESC[
            resetParserState();
        }
        break;

    case ParserState::OSC:
        appendToBuffer(ch); // [ESC] [ ] [T] [i] [t] [l] [e] [BEL]
        if (ch == '\x07') {
            if (m_buffer.size() >= 3) {
                std::string_view content = std::string_view(m_buffer).substr(2, m_buffer.size() - 3);
                processOSC(content);
            }
            resetParserState();
        } else if (ch == '\\' && m_buffer.size() >= 2 && m_buffer[m_buffer.size() - 1] == '\\' && m_buffer[m_buffer.size() - 2] == '\x1B') {
            if (m_buffer.size() >= 4) {
                std::string_view content = std::string_view(m_buffer).substr(2, m_buffer.size() - 4);
                processOSC(content);
            }
            resetParserState();
        }
        break;

    case ParserState::Charset:
        appendToBuffer(ch);
        if (m_buffer.size() == 3) {
            processCharset(std::string_view(m_buffer).substr(1));
            resetParserState();
        }
        break;
    }

    m_prevChar = ch;
}


void ANSIParser::appendToBuffer(char32_t ch)
{
    if (ch <= 0x7F)
        m_buffer.push_back(static_cast<char>(ch));
    else
        ansi::utils::Utf8Decoder::encodeTo(ch, m_buffer);

}

void ANSIParser::resetParserState()
{
    m_buffer.clear();
    m_state = ParserState::Ground;
}

void ANSIParser::enterEscape()
{
    m_buffer.clear();
    m_buffer.push_back(0x1B);
    m_state = ParserState::Escape;
}

void ANSIParser::enterCSI()
{
    m_state = ParserState::CSI;
}

void ANSIParser::enterOSC()
{
    m_state = ParserState::OSC;
}

void ANSIParser::enterCharset()
{
    m_state = ParserState::Charset;
}

void ANSIParser::processCSI(std::string_view buf)
{
    if (buf.empty())
        return;

    bool privateMode = false;
    if (buf.front() == '?') {
        privateMode = true;
        buf.remove_prefix(1);
    }

    if (buf.empty())
        return;

    char code = buf.back();
    std::string_view paramStr = buf.substr(0, buf.size() - 1);

    // FIXME heap and could be else where
    std::vector<int> params = parseParams(paramStr);

    using job::ansi::utils::enums::CSI_CODE;
    CSI_CODE csiCode = ansi::utils::decodeCsiCode(code);

    m_dispatchCSI.setPrivateMode(privateMode);

    // Implicit conversion: vector<int> -> span<const int>
    m_dispatchCSI.dispatch(csiCode, params);
}

void ANSIParser::processOSC(std::string_view buf)
{
    size_t semi = buf.find(';');
    if (semi == std::string_view::npos) return;

    std::string_view paramStr = buf.substr(0, semi);
    std::string_view dataStr = buf.substr(semi + 1);

    int code = 0;
    std::from_chars(paramStr.data(), paramStr.data() + paramStr.size(), code);

    m_dispatchOSC.dispatch(static_cast<OSC_CODE>(code), std::string(dataStr));
}

void ANSIParser::processESC(std::string_view buf)
{
    if (buf.empty())
        return;

    char code = buf[0];
    CharsetDesignator designator = buf.size() > 1 ?
                                       static_cast<CharsetDesignator>(buf[1]) :
                                       CharsetDesignator::US_ASCII;

    m_dispatchESC.dispatch(static_cast<ESC_CODE>(code), designator);
}

void ANSIParser::processCharset(std::string_view buf)
{
    if (buf.size() != 2)
        return;

    char prefix = buf[0];
    char designatorChar = buf[1];

    // We assume G0/G1 switching only for now
    CharsetDesignator designator = static_cast<CharsetDesignator>(designatorChar);
    m_dispatchESC.dispatch(static_cast<ESC_CODE>(prefix), designator);
}

std::vector<int> ANSIParser::parseParams(std::string_view str) const
{
    std::vector<int> ret;
    const char *start = str.data();
    const char *end = start + str.size();

    while (start < end) {
        const char *next = std::find(start, end, ';');
        int value = 0;

        if (next != start) {
            std::from_chars_result result = std::from_chars(start, next, value);
            if (result.ec == std::errc())
                ret.push_back(value);
            else
                ret.push_back(0);
        } else {
            ret.push_back(0);
        }

        start = (next == end) ? end : next + 1;
    }
    return ret;
}


}
