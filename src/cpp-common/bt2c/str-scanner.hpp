/*
 * Copyright (c) 2015-2024 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_STR_SCANNER_HPP
#define BABELTRACE_CPP_COMMON_BT2C_STR_SCANNER_HPP

#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include "common/assert.h"
#include "cpp-common/bt2c/logging.hpp"
#include "cpp-common/bt2c/regex.hpp"

#include "text-loc.hpp"

namespace bt2c {

/*!
@brief
    Abstract base string scanner.

@ingroup common-cpp-bt2c

A string scanner (lexer) wraps an input string view and scans specific
characters and sequences of characters, managing a current position.

This class is abstract: you inherit it to implement a concrete
scanner for a specific grammar.

When you call the various <code>tryScan*()</code> methods to try to scan
some contents, the methods advance the current position on success. They
also automatically skip leading "noise" first (see skipNoise()).

The supported base constructs are:

<table>
  <tr>
    <th>Construct
    <th>Scanning method
  <tr>
    <td>Constant unsigned decimal integer (up to 18,446,744,073,709,551,615)
    <td>tryScanConstUInt()
  <tr>
    <td>
      Constant signed decimal integer (-9,223,372,036,854,775,808 to
      9,223,372,036,854,775,807)
    <td>tryScanConstSInt()
  <tr>
    <td>Exact string
    <td>tryScanToken()
</table>

A concrete subclass adds its own <code>tryScan*()</code> methods on
top of the ones this class already offers, and has access to a few
protected primitives to help implement those methods without having
to reimplement position/line tracking:

- _tryScanAnyChar()
- _incrAt()
- _incrAtWithNewlineCheck()
- _decrAt()
- _tryScanLitStr()
- _tryScanConstReal()
- _skipWhitespaces()

A subclass overrides _skipNoise(), which this class calls before
scanning within each <code>tryScan*()</code> method, to skip whatever
"noise" (whitespace and/or, for example, comments) may appear between
scanned constructs in its own grammar. Such an override typically
calls _skipWhitespaces() as part of, or as the entirety of, its own
noise skipping logic. The default implementation of _skipNoise() does
nothing.

This class also supports backtracking with save(), accept(), and
reject(). Prefer StrScannerRejecter for RAII-based backtracking.

@note
    You must use this class in libbabeltrace2 context because it
    appends causes to the error of the current thread and throws
    on error.

@code{.cpp}
#include "cpp-common/bt2c/str-scanner.hpp"
@endcode
*/
class StrScanner
{
public:
    /*! @brief String view iterator. */
    using Iter = std::string_view::const_iterator;

    /*!
    @brief
        Destroys this string scanner.
    */
    virtual ~StrScanner() = 0;

    /*!
    @brief
        Current position within the wrapped string.

    @returns
        Current position within the wrapped string.
    */
    Iter at() const noexcept
    {
        return _mAt;
    }

    /*!
    @brief
        Sets the current position within the wrapped string
        to \bt_p{at}.

    @warning
        This may corrupt the current text location (loc()) if the
        string between at() and \bt_p{at} includes one or more
        newline characters.

    @param[in] at
        New position.

    @pre
        \bt_p{at} is within
        [<code>str().begin()</code>,&nbsp;<code>str().end()</code>].
    */
    void at(const Iter at) noexcept
    {
        BT_ASSERT_DBG(at >= _mStr.begin() && at <= _mStr.end());
        _mAt = at;
    }

    /*!
    @brief
        Wrapped string.

    @returns
        Wrapped string.
    */
    std::string_view str() const noexcept
    {
        return _mStr;
    }

    /*!
    @brief
        Number of characters left until <code>str().end()</code>.

    @returns
        Number of characters left until <code>str().end()</code>.
    */
    std::size_t charsLeft() const noexcept
    {
        return _mStr.end() - _mAt;
    }

    /*
     * Returns the current text location considering `_mBaseOffset`.
     */

    /*!
    @brief
        Current text location.

    This method uses the value of the \bt_p{baseOffset} parameter of
    StrScanner(std::string_view, std::size_t, const Logger&)
    when you built this string scanner as the base offset of the
    text location to build.

    @returns
        Current text location.
    */
    TextLoc loc() const noexcept
    {
        return TextLoc {_mBaseOffset + static_cast<std::size_t>(_mAt - _mStr.begin()), _mNbLines,
                        static_cast<std::size_t>(_mAt - _mLineBegin)};
    }

    /*!
    @brief
        Whether or not the end of the wrapped string is reached.

    @returns
        \c true if the end of the wrapped string is reached.
    */
    bool isDone() const noexcept
    {
        return _mAt == _mStr.end();
    }

    /*!
    @brief
        Character at the current position.

    @pre
        isDone() returns \c false.

    @returns
        Character at the current position.
    */
    char curChar() const noexcept
    {
        BT_ASSERT_DBG(!this->isDone());
        return *_mAt;
    }

    /*!
    @brief
        Skips leading "noise" at the current position, updating the
        current position.

    Each <code>tryScan*()</code> method of this class already calls
    this method before scanning: you may still call it directly, for
    example to get an accurate loc() before such a call.

    @sa _skipNoise()
    */
    void skipNoise() noexcept
    {
        this->_skipNoise();
    }

    /*!
    @brief
        Resets this string scanner, setting the current position
        to <code>str().begin()</code>.
    */
    void reset();

    /*!
    @brief
        Pushes the current position on the position stack.

    Call this before calling one or more scanning methods of which
    the result could be reverted.

    You must remove this new entry from the stack by calling
    accept() or reject().

    @sa accept()
    @sa reject()
    @sa StrScannerRejecter
    */
    void save()
    {
        _mStack.push_back({_mAt, _mLineBegin, _mNbLines});
    }

    /*!
    @brief
        Accepts the content scanned since the latest call to save().

    This method removes an entry from the top of the position stack
    without changing the current position.

    @sa save()
    @sa reject()
    */
    void accept()
    {
        BT_ASSERT_DBG(!_mStack.empty());
        _mStack.pop_back();
    }

    /*!
    @brief
        Rejects the content scanned since the latest call to save().

    This method removes an entry from the top of the position stack,
    and also restores the current position and line information to the
    saved values.

    @sa save()
    @sa accept()
    */
    void reject();

    /*!
    @brief
        Tries to scan and decode a constant integer string, possibly
        negative if \bt_p{ValT} (either <code>unsigned long long</code>
        or <code>long long</code>) is signed.

    If \bt_p{AllowPrefixesV} is \c true, then the following prefixes
    are supported:

    <dl>
      <dt><code>0x</code> or <code>0X</code></dt>
      <dd>Hexadecimal</dd>

      <dt><code>0b</code> or <code>0B</code></dt>
      <dd>Binary</dd>

      <dt><code>0</code></dt>
      <dd>Octal</dd>
    </dl>

    When \bt_p{AllowPrefixesV} is \c false (the default), only decimal
    integers are supported.

    Valid examples:

    - <code>9283</code>
    - <code>-42</code>
    - <code>0</code>
    - When \bt_p{AllowPrefixesV} is <code>true</code>:
      - <code>0x1f3a</code>
      - <code>0b1101</code>
      - <code>0777</code>

    Calls _skipNoise() before scanning.

    Sets the current position to \em after this constant integer string
    on success.

    @returns
        Decoded constant integer on success, or std::nullopt
        if the method couldn't scan a constant integer.

    @sa tryScanConstUInt()
    @sa tryScanConstSInt()
    */
    template <typename ValT, bool AllowPrefixesV = false>
    std::optional<ValT> tryScanConstInt() noexcept;

    /*!
    @brief
        Alias of tryScanConstInt() with <code>unsigned long long</code>.

    @returns
        See tryScanConstInt().

    @sa tryScanConstSInt()
    */
    template <bool AllowPrefixesV = false>
    std::optional<unsigned long long> tryScanConstUInt() noexcept
    {
        return this->tryScanConstInt<unsigned long long, AllowPrefixesV>();
    }

    /*!
    @brief
        Alias of tryScanConstInt() with <code>long long</code>.

    @returns
        See tryScanConstInt().

    @sa tryScanConstUInt()
    */
    template <bool AllowPrefixesV = false>
    std::optional<long long> tryScanConstSInt() noexcept
    {
        return this->tryScanConstInt<long long, AllowPrefixesV>();
    }

    /*!
    @brief
        Tries to scan the exact string \bt_p{token}.

    Calls _skipNoise() before scanning.

    Sets the current position to \em after the token
    on success.

    @param[in] token
        Token to scan.

    @retval false
        Couldn't scan \bt_p{token}.
    @retval true
        Scanned \bt_p{token}.
    */
    bool tryScanToken(std::string_view token) noexcept;

private:
    /*
     * A frame of the position stack.
     */
    struct _tStackFrame final
    {
        Iter at;
        Iter lineBegin;
        std::size_t nbLines;
    };

    /*
     * Tries to negate `ullVal` as a signed integer value if `ValT` is
     * signed and `negate` is true, returning `std::nullopt` if it
     * can't.
     *
     * Always succeeds when `ValT` is unsigned.
     */
    template <typename ValT>
    static std::optional<ValT> _tryNegateConstInt(unsigned long long ullVal, bool negate) noexcept;

    /*
     * Tries to scan a binary integer (after the `0b`/`0B` prefix has
     * already been consumed), returning `std::nullopt` if not
     * possible.
     */
    template <typename ValT>
    std::optional<ValT> _tryScanConstBinInt(bool negate) noexcept;

    /*
     * Tries to scan a constant integer in base `BaseV` (after any
     * prefix has already been consumed), returning `std::nullopt` if
     * not possible.
     */
    template <typename ValT, int BaseV>
    std::optional<ValT> _tryScanConstIntWithBase(bool negate) noexcept;

    /*
     * Handles a `\u` escape sequence, appending the UTF-8-encoded
     * Unicode character to `_mStrBuf` on success, or throwing `Error`
     * on error.
     *
     * `at` is the position of the first hexadecimal character
     * after `\u`.
     */
    void _appendEscapedUnicodeChar(Iter at);

    /*
     * Handles an octal escape sequence (`\` followed by one to three
     * octal digits), appending the decoded byte to `_mStrBuf` on
     * success, or throwing `Error` on error.
     *
     * `_mAt[0]` is `\` and `_mAt[1]` is the first octal digit.
     */
    void _appendEscapedOctalChar();

    /*
     * Handles a `\x` or `\X` hexadecimal escape sequence, appending
     * the decoded byte to `_mStrBuf` on success, or throwing `Error`
     * on error.
     *
     * `_mAt[0]` is `\` and `_mAt[1]` is `x` or `X`.
     */
    void _appendEscapedHexChar();

    /*
     * Tries to append an escaped character to `_mStrBuf` from the
     * escape sequence characters at the current positin, considering
     * the characters of `escapeSeqStartList`, `\`, and `"` as escape
     * sequence starting characters.
     */
    bool _tryAppendEscapedChar(std::string_view escapeSeqStartList);

protected:
    /*!
    @brief
        Builds a string scanner, wrapping the string \bt_p{str}.

    When the created string scanner logs or appends a cause to the
    error of the current thread, it uses \bt_p{baseOffset} to format
    the \link TextLoc text location\endlink part of the error message.

    The created string scanner remains valid as long as \bt_p{str}
    isn't modified.

    A subclass constructor uses this to initialize its `StrScanner`
    base.

    @param[in] str
        String to wrap.
    @param[in] baseOffset
        Base offset to use to format a text location for an
        error message.
    @param[in] logger
        Logger to use on error.
    */
    explicit StrScanner(std::string_view str, std::size_t baseOffset, const Logger& logger);

    /*!
    @brief
        Like StrScanner(std::string_view, std::size_t, const Logger&),
        but with \bt_p{baseOffset} set to&nbsp;0.

    @param[in] str
        See StrScanner(std::string_view, std::size_t, const Logger&).
    @param[in] logger
        See StrScanner(std::string_view, std::size_t, const Logger&).
    */
    explicit StrScanner(std::string_view str, const Logger& logger);

    /*!
    @brief
        Tries to scan a double-quoted literal string, considering the
        characters of \bt_p{escapeSeqStartList}, <code>\\</code>,
        and <code>&quot;</code> as escape sequence starting characters.

    If \bt_p{escapeSeqStartList} includes \c u, then a <code>\\u</code>
    escape sequence is interpreted as in
    <a href="https://www.json.org/">JSON</a>: four hexadecimal
    characters which represent the value of a single Unicode codepoint.

    If \bt_p{escapeSeqStartList} includes <code>0</code>, then a
    <code>\\</code> followed by one to three octal digits (\c 0 to
    <code>7</code>) is an octal escape sequence; the resulting byte
    value must be less than&nbsp;256.

    If \bt_p{escapeSeqStartList} includes \c x or \c X, then a
    <code>\\x</code> or <code>\\X</code> followed by one or more
    hexadecimal digits is a hexadecimal escape sequence; the resulting
    byte value must be less than&nbsp;256.

    Valid examples:

    - <code>&quot;salut!&quot;</code>
    - <code>&quot;en circulation\\nYves?&quot;</code>
    - <code>&quot;\\u03c9 often represents angular velocity in physics&quot;</code>
    - <code>&quot;\\101\\x42&quot;</code> (<code>AB</code>)

    Calls _skipNoise() before scanning.

    Sets the current position to \em after the closing double
    quote on success.

    Logs and appends a cause to the error of the current thread,
    throwing bt2c::Error, if the scanning method finds an invalid escape
    sequence or an illegal control character.

    A subclass can use this method to implement its own
    <code>tryScanLitStr()</code> method (with or without parameters),
    matching its own grammar.

    @param[in] escapeSeqStartList
        List of characters to consider as escape sequence
        starting characters.

    @returns
        @parblock
        View of the escaped string, \em without beginning/end
        double quotes, on success, or \c std::nullopt if there's no
        double-quoted literal string (or if the method reaches
        <code>str().end()</code> before a closing <code>&quot;</code>).

        The returned string view remains valid as long as you don't call
        any method of this string scanner.
        @endparblock
    */
    std::optional<std::string_view> _tryScanLitStr(std::string_view escapeSeqStartList);

    /*!
    @brief
        Skips "noise" at the current position, updating the current
        position.

    Each <code>tryScan*()</code> method of this class calls this
    method before scanning.

    The default implementation does nothing.

    A subclass whose grammar needs to skip more than whitespaces (for
    example, comments) between scanned constructs overrides this
    method, typically calling _skipWhitespaces() as part of its own
    noise skipping logic.

    @sa skipNoise()
    */
    virtual void _skipNoise() noexcept
    {
    }

    /*!
    @brief
        Skips all the following whitespaces, updating the current
        position accordingly.

    A concrete _skipNoise() implementation can use this method to skip
    whitespaces as part of, or as the entirety of, its own
    noise-skipping logic.
    */
    void _skipWhitespaces() noexcept;

    /*!
    @brief
        Tries to scan any character, returning it and advancing the
        current position on success.

    @returns
        Scanned character on success, or -1 if the current position
        is <code>str().end()</code>.
    */
    int _tryScanAnyChar() noexcept
    {
        if (this->isDone()) {
            return -1;
        }

        const auto c = *_mAt;

        this->_incrAt();
        return c;
    }

    /*!
    @brief
        Increments the current position by \bt_p{count}.

    @warning
        This method does \em not check for newline characters amongst
        the skipped ones: it doesn't update the line count or the line
        beginning position. Use _incrAtWithNewlineCheck() instead when
        the single character you're advancing over may be a newline and
        you need loc() to remain accurate.

    @param[in] count
        Number of characters to advance the current position by.

    @pre
        What <code>charsLeft()<code> returns is greater than or equal
        to \bt_p{count}.
    */
    void _incrAt(const std::size_t count = 1) noexcept
    {
        _mAt += count;
        BT_ASSERT_DBG(_mAt <= _mStr.end());
    }

    /*!
    @brief
        Increments the current position by one character, first
        checking whether that character is a newline and, if so,
        updating the line count and line beginning position
        accordingly.

    A subclass can use this method instead of _incrAt() to advance the
    current position by one character while scanning some contents
    which may contain newline characters, such as the inner
    characters of a literal string, so that loc() remains accurate.

    @pre
        isDone() returns \c false.
    */
    void _incrAtWithNewlineCheck() noexcept
    {
        BT_ASSERT_DBG(!this->isDone());

        if (*_mAt == '\n') {
            ++_mNbLines;
            _mLineBegin = _mAt + 1;
        }

        this->_incrAt();
    }

    /*!
    @brief
        Decrements the current position by \bt_p{count}.

    @warning
        This method does \em not update the line count or the line
        beginning position: it only moves the current position back,
        assuming you're only rewinding over characters which can't have
        updated this line tracking state in the first place (typically
        to backtrack after a failed multi-character lookahead), \em not
        over previously consumed newline characters.

    @param[in] count
        Number of characters to move the current position back by.

    @pre
        The current position is at least \bt_p{count} characters
        after <code>str().begin()</code>.
    */
    void _decrAt(const std::size_t count = 1) noexcept
    {
        _mAt -= count;
        BT_ASSERT_DBG(_mAt >= _mStr.begin());
    }

    /*!
    @brief
        Tries to scan and decode a constant real number string whose
        textual form, anchored at the current position, matches
        \bt_p{regex}, returning \c std::nullopt if it doesn't match, or
        if <code>std::strtod()</code> fails to parse it.

    This is needed, instead of simply calling <code>std::strtod()</code>
    directly, because <code>std::strtod()</code> accepts more formats
    than a specific grammar may support.

    A subclass can use this method to implement its own
    <code>tryScanConstReal()</code> method, matching its own grammar,
    without reimplementing the <code>std::strtod()</code> parsing and
    error handling logic.

    Calls _skipNoise() before scanning.

    Sets the current position to \em after the scanned constant real
    number string on success.

    @param[in] regex
        Regular expression which the constant real number string,
        anchored at the current position, must match.

    @returns
        Decoded constant real number on success, or std::nullopt
        if the method couldn't scan a constant real number.

    @pre
        \bt_p{regex} cannot match a string containing a newline
        character: on success, this method jumps the current position
        to \em after the match using at() directly, which doesn't
        update the line count or the line beginning position (see the
        warning of at()).
    */
    std::optional<double> _tryScanConstReal(const bt2c::Regex& regex) noexcept;

private:
    /* Viewed string, given by user */
    std::string_view _mStr;

    /* Current position within `_mStr` */
    Iter _mAt;

    /* Beginning of the current line */
    Iter _mLineBegin;

    /* Number of lines scanned so far */
    std::size_t _mNbLines = 0;

    /* String buffer, used by _tryScanLitStr() */
    std::string _mStrBuf;

    /* Base offset for error messages */
    std::size_t _mBaseOffset;

    /* Logging configuration */
    Logger _mLogger;

    /* Position stack for backtracking */
    std::vector<_tStackFrame> _mStack;
};

template <typename ValT>
std::optional<ValT> StrScanner::_tryNegateConstInt(const unsigned long long ullVal,
                                                   const bool negate) noexcept
{
    /* Check for overflow */
    if constexpr (std::is_signed_v<ValT>) {
        constexpr auto llMaxAsUll =
            static_cast<unsigned long long>(std::numeric_limits<long long>::max());

        if (negate) {
            if (ullVal > llMaxAsUll + 1) {
                return std::nullopt;
            }
        } else {
            if (ullVal > llMaxAsUll) {
                return std::nullopt;
            }
        }
    }

    /* Success: cast and negate if needed */
    auto val = static_cast<ValT>(ullVal);

    if (negate) {
        val *= static_cast<ValT>(-1);
    }

    return val;
}

template <typename ValT>
std::optional<ValT> StrScanner::_tryScanConstBinInt(const bool negate) noexcept
{
    const auto initAt = _mAt;

    /* Accumulate `0` and `1` characters */
    auto ullVal = 0ULL;
    auto nbBits = 0;

    while (!this->isDone()) {
        if (*_mAt != '0' && *_mAt != '1') {
            break;
        }

        if (nbBits >= 64) {
            /* Too many bits */
            this->at(initAt);
            return std::nullopt;
        }

        ullVal = (ullVal << 1) | static_cast<unsigned long long>(*_mAt - '0');
        ++nbBits;
        this->_incrAt();
    }

    if (nbBits == 0) {
        /* `0b`/`0B` not followed by `0` or `1` */
        this->at(initAt);
        return std::nullopt;
    }

    const auto val = StrScanner::_tryNegateConstInt<ValT>(ullVal, negate);

    if (!val) {
        this->at(initAt);
    }

    return val;
}

template <typename ValT, int BaseV>
std::optional<ValT> StrScanner::_tryScanConstIntWithBase(const bool negate) noexcept
{
    /*
     * Reject a `0x`/`0X` prefix at this level: std::strtoull() with
     * base 16 would happily accept it, but the caller already handled
     * such a prefix.
     */
    if constexpr (BaseV == 16) {
        if (this->charsLeft() >= 2 && _mAt[0] == '0' && (_mAt[1] == 'x' || _mAt[1] == 'X')) {
            return std::nullopt;
        }
    }

    if (this->isDone() || !std::isxdigit(*_mAt)) {
        return std::nullopt;
    }

    /* Parse */
    char *strEnd = nullptr;
    const auto ullVal = std::strtoull(&(*_mAt), &strEnd, BaseV);

    if ((ullVal == 0 && &(*_mAt) == strEnd) || errno == ERANGE) {
        /* Couldn't parse */
        errno = 0;
        return std::nullopt;
    }

    /* Negate if needed */
    const auto val = StrScanner::_tryNegateConstInt<ValT>(ullVal, negate);

    if (val) {
        /* Success: update current position */
        this->at(_mStr.begin() + (strEnd - _mStr.data()));
    }

    return val;
}

template <typename ValT, bool AllowPrefixesV>
std::optional<ValT> StrScanner::tryScanConstInt() noexcept
{
    static_assert(std::is_same_v<ValT, long long> || std::is_same_v<ValT, unsigned long long>,
                  "`ValT` is `long long` or `unsigned long long`.");

    this->_skipNoise();

    /* Backup if we can't scan completely */
    const auto initAt = _mAt;

    /* Scan initial character */
    const auto c = this->_tryScanAnyChar();

    if (c < 0) {
        /* Nothing left */
        return std::nullopt;
    }

    /* Check for negation */
    const bool negate = (c == '-');

    if constexpr (!std::is_signed_v<ValT>) {
        if (negate) {
            /* Can't negate an unsigned integer */
            this->at(initAt);
            return std::nullopt;
        }
    }

    if (!negate) {
        /* No negation: rewind */
        this->_decrAt();
    }

    /*
     * Only allow a digit at this point: std::strtoull() below supports
     * an initial `+`, but this scanner doesn't.
     */
    if (this->isDone() || !std::isdigit(*_mAt)) {
        this->at(initAt);
        return std::nullopt;
    }

    /* Check for a radix prefix */
    if constexpr (AllowPrefixesV) {
        if (*_mAt == '0' && this->charsLeft() >= 2) {
            if (_mAt[1] == 'b' || _mAt[1] == 'B') {
                /* Binary */
                this->_incrAt(2);

                const auto val = this->_tryScanConstBinInt<ValT>(negate);

                if (!val) {
                    this->at(initAt);
                }

                return val;
            } else if (_mAt[1] == 'x' || _mAt[1] == 'X') {
                /* Hexadecimal */
                this->_incrAt(2);

                const auto val = this->_tryScanConstIntWithBase<ValT, 16>(negate);

                if (!val) {
                    this->at(initAt);
                }

                return val;
            } else if (_mAt[1] >= '0' && _mAt[1] <= '7') {
                /* Octal (the leading `0` is part of the octal digits) */
                const auto val = this->_tryScanConstIntWithBase<ValT, 8>(negate);

                if (!val) {
                    this->at(initAt);
                }

                return val;
            }
        }
    }

    /* Decimal */
    const auto val = this->_tryScanConstIntWithBase<ValT, 10>(negate);

    if (!val) {
        this->at(initAt);
    }

    return val;
}

/*!
@brief
    String scanner rejecter (RAII).

@ingroup common-cpp-bt2c

Automatically calls StrScanner::save() on construction and
StrScanner::reject() on destruction, unless you call accept() or
reject() manually first.
*/
class StrScannerRejecter final
{
public:
    /*!
    @brief
        Builds a string scanner rejecter, managing the string
        scanner \bt_p{ss}.

    Immediately calls <code>ss.save()</code>.

    @param[in] ss
        String scanner to manage.
    */
    explicit StrScannerRejecter(StrScanner& ss) noexcept
        : _mSs {&ss}
    {
        _mSs->save();
    }

    /*!
    @brief
        Destroys this string scanner rejecter.

    If neither accept() nor reject() was called, then this destructor
    calls StrScanner::reject() on the managed string scanner.
    */
    ~StrScannerRejecter()
    {
        if (_mReject) {
            _mSs->reject();
        }
    }

    /* Disable copy/move operations to simplify */
    StrScannerRejecter(const StrScannerRejecter&) = delete;
    StrScannerRejecter& operator=(const StrScannerRejecter&) = delete;
    StrScannerRejecter(StrScannerRejecter&&) = delete;
    StrScannerRejecter& operator=(StrScannerRejecter&&) = delete;

    /*!
    @brief
        Accepts the content scanned since construction.

    Calls StrScanner::accept() on the managed string scanner and
    inhibits a future rejection by this rejecter.
    */
    void accept()
    {
        _mSs->accept();
        _mReject = false;
    }

    /*!
    @brief
        Rejects the content scanned since construction.

    Calls StrScanner::reject() on the managed string scanner and
    inhibits a future rejection by this rejecter.
    */
    void reject()
    {
        _mSs->reject();
        _mReject = false;
    }

private:
    bool _mReject = true;
    StrScanner *_mSs;
};

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_STR_SCANNER_HPP */
