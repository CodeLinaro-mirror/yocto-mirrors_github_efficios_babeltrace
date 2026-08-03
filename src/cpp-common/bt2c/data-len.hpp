/*
 * Copyright (c) 2022 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_DATA_LEN_HPP
#define BABELTRACE_CPP_COMMON_BT2C_DATA_LEN_HPP

/*!
@file

@brief
    Data length class and related functions.

@ingroup common-cpp-bt2c

@code{.cpp}
#include "cpp-common/bt2c/data-len.hpp"
@endcode

This file mainly offers the bt2c::DataLen class as well as related
functions: operators and useful user-defined literals under the
\c bt2c::literals::datalen namespace, for example:

@code{.cpp}
using namespace bt2c::literals::datalen;

auto len = 588_KiBytes + myOtherLen;

len -= 64_bits;
@endcode
*/

#include "safe-ops.hpp"

namespace bt2c {

/*!
@brief
    Data length.

A data length is a quantity of binary data (bits). You may also use it
for an offset which is just a number of bits from a given position.

This class can make some code clearer and safer because its
constructor is private so that you need to call DataLen::fromBits()
or DataLen::fromBytes() to create an instance.

Get the quantity in bits with bits() or operator*().

Get the quantity in bytes (floored) with bytes().

Get the number of extra bits with extraBitCount().

Check whether or not a data length is a power of two with isPowOfTwo().

You may add to and subtract from a data length. All the comparison
operators are also implemented.

A DataLen instance is lightweight: it only contains an unsigned integer.
Pass and return by copy.
*/
class DataLen final
{
private:
    constexpr explicit DataLen(const unsigned long long lenBits) noexcept
        : _mLenBits {lenBits}
    {
    }

public:
    /*!
    @brief
        Builds and returns a data length of \bt_p{lenBits} bits.

    @param[in] lenBits
        Length of the created data length (bits).

    @returns
        Data length of \bt_p{lenBits} bits.

    @sa fromBytes()
    @sa
        The \c _bits user-defined literal under the \c bt2c::literals::datalen
        namespace.
    */
    static constexpr DataLen fromBits(const unsigned long long lenBits) noexcept
    {
        return DataLen {lenBits};
    }

    /*!
    @brief
        Builds and returns a data length of \bt_p{lenBytes} bytes.

    @param[in] lenBytes
        Length of the created data length (bytes).

    @returns
        Data length of \bt_p{lenBytes} bytes.

    @sa fromBits()
    @sa
        The \c _bytes user-defined literal under the \c bt2c::literals::datalen
        namespace.
    */
    static DataLen fromBytes(const unsigned long long lenBytes) noexcept
    {
        return DataLen {safeMul(lenBytes, 8ULL)};
    }

    /*!
    @brief
        Alias of bits().

    @returns
        See bits().
    */
    constexpr unsigned long long operator*() const noexcept
    {
        return _mLenBits;
    }

    /*!
    @brief
        Number of bits.

    @returns
        Number of bits.
    */
    constexpr unsigned long long bits() const noexcept
    {
        return _mLenBits;
    }

    /*!
    @brief
        Number of bytes (floored).

    @returns
        Number of bytes (floored).
    */
    constexpr unsigned long long bytes() const noexcept
    {
        return _mLenBits / 8;
    }

    /*!
    @brief
        Whether or not this data length is not a multiple
        of 8&nbsp;bits.

    @retval false
        This data length is a multiple of 8&nbsp;bits.
    @retval true
        This data length is \em not a multiple of 8&nbsp;bits.
    */
    constexpr bool hasExtraBits() const noexcept
    {
        return this->extraBitCount() > 0;
    }

    /*!
    @brief
        Remainder of this data length, in bits, divided by eight.

    @returns
        Remainder of this data length, in bits, divided by eight.
    */
    constexpr unsigned int extraBitCount() const noexcept
    {
        return _mLenBits & 7;
    }

    /*!
    @brief
        Whether or not this data length, in bits, is a power of two.

    @returns
        \c true if this data length is a power of two.
    */
    constexpr bool isPowOfTwo() const noexcept
    {
        return ((_mLenBits & (_mLenBits - 1)) == 0) && _mLenBits > 0;
    }

    /*!
    @brief
        Returns whether or not this data length is equal
        to \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length is equal to \bt_p{other}.
    */
    constexpr bool operator==(const DataLen& other) const noexcept
    {
        return _mLenBits == other._mLenBits;
    }

    /*!
    @brief
        Returns whether or not this data length isn't equal
        to \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length isn't equal to \bt_p{other}.
    */
    constexpr bool operator!=(const DataLen& other) const noexcept
    {
        return !(*this == other);
    }

    /*!
    @brief
        Returns whether or not this data length is less than
        \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length is less than \bt_p{other}.
    */
    constexpr bool operator<(const DataLen& other) const noexcept
    {
        return _mLenBits < other._mLenBits;
    }

    /*!
    @brief
        Returns whether or not this data length is less than or equal to
        \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length is less than or equal
        to \bt_p{other}.
    */
    constexpr bool operator<=(const DataLen& other) const noexcept
    {
        return (*this == other) || (*this < other);
    }

    /*!
    @brief
        Returns whether or not this data length is greater than
        \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length is greater than \bt_p{other}.
    */
    constexpr bool operator>(const DataLen& other) const noexcept
    {
        return !(*this <= other);
    }

    /*!
    @brief
        Returns whether or not this data length is greater than or
        equal to \bt_p{other}.

    @param[in] other
        Other data length to compare to.

    @returns
        \c true if this data length is greater than or equal
        to \bt_p{other}.
    */
    constexpr bool operator>=(const DataLen& other) const noexcept
    {
        return (*this > other) || (*this == other);
    }

    /*!
    @brief
        Adds \bt_p{*len} bits to this data length.

    @param[in] len
        Quantity to add to this data length.

    @returns
        <code>*this</code>
    */
    DataLen& operator+=(const DataLen len) noexcept
    {
        _mLenBits = safeAdd(_mLenBits, len._mLenBits);
        return *this;
    }

    /*!
    @brief
        Subtract \bt_p{*len} bits from this data length.

    @param[in] len
        Quantity to subtract from this data length.

    @returns
        <code>*this</code>
    */
    DataLen& operator-=(const DataLen len) noexcept
    {
        _mLenBits = safeSub(_mLenBits, len._mLenBits);
        return *this;
    }

    /*!
    @brief
        Multiplies this data length by \bt_p{mul}.

    @param[in] mul
        Multiplier.

    @returns
        <code>*this</code>
    */
    DataLen& operator*=(const unsigned long long mul) noexcept
    {
        _mLenBits = safeMul(_mLenBits, mul);
        return *this;
    }

private:
    unsigned long long _mLenBits = 0;
};

/*!
@brief
    Returns a data length of
    (\bt_p{*lenA}&nbsp;+&nbsp;\bt_p{*lenB})&nbsp;bits.

@param[in] lenA
    First term.
@param[in] lenB
    Second term.

@returns
    Data length of
    (\bt_p{*lenA}&nbsp;+&nbsp;\bt_p{*lenB})&nbsp;bits.
*/
inline DataLen operator+(const DataLen lenA, const DataLen lenB) noexcept
{
    return DataLen::fromBits(safeAdd(*lenA, *lenB));
}

/*!
@brief
    Returns a data length of
    (\bt_p{*lenA}&nbsp;−&nbsp;\bt_p{*lenB})&nbsp;bits.

@param[in] lenA
    Minuend.
@param[in] lenB
    Subtrahend.

@returns
    Data length of
    (\bt_p{*lenA}&nbsp;−&nbsp;\bt_p{*lenB})&nbsp;bits.
*/
inline DataLen operator-(const DataLen lenA, const DataLen lenB) noexcept
{
    return DataLen::fromBits(safeSub(*lenA, *lenB));
}

/*!
@brief
    Returns a data length of
    (\bt_p{*len}&nbsp;×&nbsp;\bt_p{mul})&nbsp;bits.

@param[in] len
    Length to multiply.
@param[in] mul
    Multiplier.

@returns
    Data length of
    (\bt_p{*len}&nbsp;×&nbsp;\bt_p{mul})&nbsp;bits.
*/
inline DataLen operator*(const DataLen len, const unsigned long long mul) noexcept
{
    return DataLen::fromBits(safeMul(*len, mul));
}

/*
 * Use this namespace to access handy data length user literals, for
 * example:
 *
 *     using namespace bt2c::literals::datalen;
 *
 *     const auto bufSize = 64_MiBytes + 8_KiBits;
 */
namespace literals::datalen {

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;bits.

@param[in] val
    Number of bits.

@returns
    Data length of \bt_p{val}&nbsp;bits.
*/
inline DataLen operator"" _bits(const unsigned long long val) noexcept
{
    return DataLen::fromBits(val);
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;kibibits.

@param[in] val
    Number of kibibits.

@returns
    Data length of \bt_p{val}&nbsp;kibibits.
*/
inline DataLen operator"" _KiBits(const unsigned long long val) noexcept
{
    return DataLen::fromBits(safeMul(val, 1024ULL));
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;mibibits.

@param[in] val
    Number of mibibits.

@returns
    Data length of \bt_p{val}&nbsp;mibibits.
*/
inline DataLen operator"" _MiBits(const unsigned long long val) noexcept
{
    return DataLen::fromBits(safeMul(val, 1024ULL * 1024));
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;gibibits.

@param[in] val
    Number of gibibits.

@returns
    Data length of \bt_p{val}&nbsp;gibibits.
*/
inline DataLen operator"" _GiBits(const unsigned long long val) noexcept
{
    return DataLen::fromBits(safeMul(val, 1024ULL * 1024 * 1024));
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;bytes.

@param[in] val
    Number of bytes.

@returns
    Data length of \bt_p{val}&nbsp;bytes.
*/
inline DataLen operator"" _bytes(const unsigned long long val) noexcept
{
    return DataLen::fromBytes(val);
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;kibibytes.

@param[in] val
    Number of kibibytes.

@returns
    Data length of \bt_p{val}&nbsp;kibibytes.
*/
inline DataLen operator"" _KiBytes(const unsigned long long val) noexcept
{
    return DataLen::fromBytes(safeMul(val, 1024ULL));
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;mibibytes.

@param[in] val
    Number of mibibytes.

@returns
    Data length of \bt_p{val}&nbsp;mibibytes.
*/
inline DataLen operator"" _MiBytes(const unsigned long long val) noexcept
{
    return DataLen::fromBytes(safeMul(val, 1024ULL * 1024));
}

/*!
@brief
    User-defined literal which returns a data length
    of \bt_p{val}&nbsp;gibibytes.

@param[in] val
    Number of gibibytes.

@returns
    Data length of \bt_p{val}&nbsp;gibibytes.
*/
inline DataLen operator"" _GiBytes(const unsigned long long val) noexcept
{
    return DataLen::fromBytes(safeMul(val, 1024ULL * 1024 * 1024));
}

} /* namespace literals::datalen */
} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_DATA_LEN_HPP */
