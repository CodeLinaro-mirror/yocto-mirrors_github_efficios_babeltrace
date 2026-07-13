/*
 * Copyright (c) 2015-2024 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <cctype>
#include <cmath>
#include <string_view>

#include "str-scanner.hpp"

namespace bt2c {

StrScanner::StrScanner(const std::string_view str, const std::size_t baseOffset,
                       const Logger& logger)
    : _mStr {str},
      _mAt {str.begin()},
      _mLineBegin {str.begin()},
      _mBaseOffset {baseOffset},
      _mLogger {logger, "STR-SCANNER"}
{
}

StrScanner::StrScanner(const std::string_view str, const Logger& logger)
    : StrScanner {str, 0, logger}
{
}

StrScanner::~StrScanner() = default;

void StrScanner::reset()
{
    this->at(_mStr.begin());
    _mNbLines = 0;
    _mLineBegin = _mStr.begin();
    _mStack.clear();
}

void StrScanner::reject()
{
    BT_ASSERT_DBG(!_mStack.empty());
    _mAt = _mStack.back().at;
    _mLineBegin = _mStack.back().lineBegin;
    _mNbLines = _mStack.back().nbLines;
    _mStack.pop_back();
}

void StrScanner::_skipWhitespaces() noexcept
{
    while (!this->isDone()) {
        switch (*_mAt) {
        case '\n':
            this->_incrAtWithNewlineCheck();
            break;
        case ' ':
        case '\t':
        case '\v':
        case '\r':
            this->_incrAt();
            break;
        default:
            return;
        }
    }
}

namespace {

/*
 * Returns the value of the hexadecimal digit `ch`.
 */
unsigned int hexDigitVal(const char ch) noexcept
{
    BT_ASSERT_DBG(std::isxdigit(static_cast<unsigned char>(ch)));

    if (ch >= '0' && ch <= '9') {
        return static_cast<unsigned int>(ch - '0');
    } else if (ch >= 'a' && ch <= 'f') {
        return static_cast<unsigned int>(ch - 'a') + 10;
    } else {
        return static_cast<unsigned int>(ch - 'A') + 10;
    }
}

} /* namespace */

void StrScanner::_appendEscapedUnicodeChar(const Iter at)
{
    /*
     * Validate the four hex characters, converting them to an integral
     * codepoint as we go.
     *
     * Because there are exactly four of them, `cp` can't be greater
     * than 0xffff.
     */
    auto cp = 0U;

    for (auto it = at; it != at + 4; ++it) {
        const auto ch = *it;

        if (!std::isxdigit(static_cast<unsigned char>(ch))) {
            BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
                Error, this->loc(), "In `\\u` escape sequence: unexpected character `{:c}`.", ch);
        }

        cp = (cp << 4) | hexDigitVal(ch);
    }

    /*
     * Append UTF-8 bytes from integral codepoint.
     *
     * See <https://en.wikipedia.org/wiki/UTF-8#Encoding>.
     */
    if (cp <= 0x7f) {
        _mStrBuf.push_back(static_cast<char>(cp));
    } else if (cp <= 0x7ff) {
        _mStrBuf.push_back(static_cast<char>((cp >> 6) + 0xc0));
        _mStrBuf.push_back(static_cast<char>((cp & 0x3f) + 0x80));
    } else if (cp > 0xd800 && cp <= 0xdfff) {
        /* Unsupported surrogate pairs */
        BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
            Error, this->loc(), "In `\\u` escape sequence: unsupported surrogate codepoint U+{:X}.",
            cp);
    } else {
        _mStrBuf.push_back(static_cast<char>((cp >> 12) + 0xe0));
        _mStrBuf.push_back(static_cast<char>(((cp >> 6) & 0x3f) + 0x80));
        _mStrBuf.push_back(static_cast<char>((cp & 0x3f) + 0x80));
    }
}

void StrScanner::_appendEscapedOctalChar()
{
    /* `_mAt[0]` is `\` and `_mAt[1]` is the first octal digit */
    auto val = static_cast<unsigned int>(_mAt[1] - '0');
    std::size_t consumed = 2;

    /* Try to consume up to two more octal digits */
    for (std::size_t i = 0; i < 2 && consumed < this->charsLeft(); ++i) {
        if (_mAt[consumed] >= '0' && _mAt[consumed] <= '7') {
            val = (val << 3) | static_cast<unsigned int>(_mAt[consumed] - '0');
            ++consumed;
        } else {
            break;
        }
    }

    if (val > 255) {
        BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
            Error, this->loc(), "Octal escape sequence value {:#o} is greater than 255.", val);
    }

    _mStrBuf.push_back(static_cast<char>(val));
    this->_incrAt(consumed);
}

void StrScanner::_appendEscapedHexChar()
{
    /* `_mAt[0]` is `\` and `_mAt[1]` is `x` or `X` */
    std::size_t consumed = 2;

    if (consumed >= this->charsLeft() || !std::isxdigit(_mAt[consumed])) {
        BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
            Error, this->loc(),
            "`\\{:c}` escape sequence: expecting at least one hexadecimal digit.", _mAt[1]);
    }

    auto val = 0U;

    while (consumed < this->charsLeft() && std::isxdigit(_mAt[consumed])) {
        val = (val << 4) | hexDigitVal(_mAt[consumed]);
        ++consumed;
    }

    if (val > 255) {
        BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
            Error, this->loc(), "`\\{:c}` escape sequence: value {:#x} is greater than 255.",
            _mAt[1], val);
    }

    _mStrBuf.push_back(static_cast<char>(val));
    this->_incrAt(consumed);
}

bool StrScanner::_tryAppendEscapedChar(const std::string_view escapeSeqStartList)
{
    if (this->charsLeft() < 2) {
        /* Need at least `\` and another character */
        return false;
    }

    if (_mAt[0] != '\\') {
        /* Not an escape sequence */
        return false;
    }

    /* `"` and `\` are always valid escape sequence starting characters */
    if (_mAt[1] == '"' || _mAt[1] == '\\') {
        _mStrBuf.push_back(_mAt[1]);
        this->_incrAt(2);
        return true;
    }

    /* Try each character of `escapeSeqStartList` */
    for (const auto escapeSeqStart : escapeSeqStartList) {
        if (escapeSeqStart == '0') {
            /* Octal: `\` followed by an octal digit */
            if (_mAt[1] >= '0' && _mAt[1] <= '7') {
                this->_appendEscapedOctalChar();
                return true;
            }

            continue;
        }

        if (_mAt[1] == escapeSeqStart) {
            /* Escape sequence detected */
            if (_mAt[1] == 'u') {
                /* `\u` escape sequence */
                if (this->charsLeft() < 6) {
                    /* Need `\u` + four hex characters */
                    BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
                        Error, this->loc(), "`\\u` escape sequence needs four hexadecimal digits.");
                }

                this->_appendEscapedUnicodeChar(_mAt + 2);
                this->_incrAt(6);
            } else if (_mAt[1] == 'x' || _mAt[1] == 'X') {
                /* Hexadecimal escape sequence */
                this->_appendEscapedHexChar();
            } else {
                /* Single-character escape sequence */
                switch (_mAt[1]) {
                case 'a':
                    _mStrBuf.push_back('\a');
                    break;
                case 'b':
                    _mStrBuf.push_back('\b');
                    break;
                case 'f':
                    _mStrBuf.push_back('\f');
                    break;
                case 'n':
                    _mStrBuf.push_back('\n');
                    break;
                case 'r':
                    _mStrBuf.push_back('\r');
                    break;
                case 't':
                    _mStrBuf.push_back('\t');
                    break;
                case 'v':
                    _mStrBuf.push_back('\v');
                    break;
                default:
                    /* As is */
                    _mStrBuf.push_back(_mAt[1]);
                    break;
                }

                this->_incrAt(2);
            }

            return true;
        }
    }

    return false;
}

std::optional<std::string_view>
StrScanner::_tryScanLitStr(const std::string_view escapeSeqStartList)
{
    this->_skipNoise();

    /* Backup if we can't completely scan */
    const auto initAt = _mAt;
    const auto initLineBegin = _mLineBegin;
    const auto initNbLines = _mNbLines;

    /* First character: `"` or alpha */
    const auto c = this->_tryScanAnyChar();

    if (c < 0) {
        return std::nullopt;
    }

    if (c != '"') {
        /* Not a literal string */
        this->at(initAt);
        _mLineBegin = initLineBegin;
        _mNbLines = initNbLines;
        return std::nullopt;
    }

    /* Reset string buffer */
    _mStrBuf.clear();

    /*
     * Scan inner string, processing escape sequences during the
     * process.
     */
    while (!this->isDone()) {
        /* Check for illegal control character */
        if (std::iscntrl(*_mAt)) {
            BT_CPPLOGE_TEXT_LOC_APPEND_CAUSE_AND_THROW(
                Error, this->loc(), "Illegal control character {:#02x} in literal string.",
                static_cast<unsigned int>(*_mAt));
        }

        /* Try to append an escaped character first */
        if (this->_tryAppendEscapedChar(escapeSeqStartList)) {
            continue;
        }

        /* End of literal string? */
        if (*_mAt == '"') {
            /* Skip `"` */
            this->_incrAt();
            return std::string_view {_mStrBuf};
        }

        /* Append regular character and go to next one, checking for newline */
        _mStrBuf.push_back(*_mAt);
        this->_incrAtWithNewlineCheck();
    }

    /* Couldn't find end of string */
    this->at(initAt);
    _mLineBegin = initLineBegin;
    _mNbLines = initNbLines;
    return std::nullopt;
}

std::string_view StrScanner::_tryScanUnquotedStr(const std::string_view extraFirstChars,
                                                 const std::string_view extraOtherChars) noexcept
{
    this->_skipNoise();

    {
        /* First character: `_`, alpha, or one of `extraFirstChars` */
        const auto c = this->_tryScanAnyChar();

        if (c < 0) {
            return {};
        }

        /* Validate first char., then initialize `_mStrBuf` with it */
        const auto chr = static_cast<char>(c);

        if (chr != '_' && !std::isalpha(static_cast<unsigned char>(chr)) &&
            extraFirstChars.find(chr) == std::string_view::npos) {
            this->_decrAt();
            return {};
        }

        _mStrBuf.clear();
        _mStrBuf.push_back(chr);
    }

    /* Next characters: `_`, alphanum., or one of `extraOtherChars` */
    while (!this->isDone()) {
        const auto nextChr = *_mAt;

        if (nextChr != '_' && !std::isalnum(static_cast<unsigned char>(nextChr)) &&
            extraOtherChars.find(nextChr) == std::string_view::npos) {
            break;
        }

        _mStrBuf.push_back(nextChr);
        this->_incrAt();
    }

    return _mStrBuf;
}

bool StrScanner::tryScanToken(const std::string_view token) noexcept
{
    this->_skipNoise();

    /* Backup if we can't completely scan */
    const auto initAt = _mAt;

    /* Try to scan token completely */
    auto tokenAt = token.begin();

    while (tokenAt < token.end() && _mAt != _mStr.end()) {
        if (*_mAt != *tokenAt) {
            /* Mismatch */
            this->at(initAt);
            return false;
        }

        this->_incrAt();
        ++tokenAt;
    }

    if (tokenAt != token.end()) {
        /* Wrapped string ends before end of token */
        this->at(initAt);
        return false;
    }

    /* Success */
    return true;
}

std::optional<double> StrScanner::_tryScanConstReal(const bt2c::Regex& regex) noexcept
{
    this->_skipNoise();

    /*
     * Validate the constant real number format using `regex`.
     *
     * This is needed because std::strtod() accepts more formats than a
     * specific grammar may support.
     */
    if (!regex.match(_mStr.substr(_mAt - _mStr.begin()))) {
        return std::nullopt;
    }

    /* Parse */
    char *strEnd = nullptr;
    const auto val = std::strtod(&(*_mAt), &strEnd);

    if (val == HUGE_VAL || (val == 0 && &(*_mAt) == strEnd) || errno == ERANGE) {
        /* Couldn't parse */
        errno = 0;
        return std::nullopt;
    }

    /* Success: update character pointer and return value */
    this->at(_mStr.begin() + (strEnd - _mStr.data()));
    return val;
}

} /* namespace bt2c */
