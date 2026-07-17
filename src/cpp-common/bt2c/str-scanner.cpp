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

    /* Try each character of `escapeSeqStartList` */
    for (const auto escapeSeqStart : escapeSeqStartList) {
        if (_mAt[1] == '"' || _mAt[1] == '\\' || _mAt[1] == escapeSeqStart) {
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
