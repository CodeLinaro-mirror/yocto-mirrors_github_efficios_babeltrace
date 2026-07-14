/*
 * Copyright (c) 2022 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_ENDIAN_HPP
#define BABELTRACE_CPP_COMMON_BT2C_ENDIAN_HPP

/*!
@file

@brief
    C++ endianness conversion functions.

@ingroup common-cpp-bt2c

@code{.cpp}
#include "cpp-common/bt2c/endian.hpp"
@endcode

This file offers overloads of <code>%bt2c::littleEndianToNative()</code>
and <code>%bt2c::bigEndianToNative()</code> to deal with the endianness
of standard integer sizes, as well as the bt2c::Endianness enumerators
and the bt2c::nativeEndianness constant to know the endianness of the
host.

You may use them as such:

@code{.cpp}
template <typename IntT>
IntT fromBe(const IntT val) noexcept
{
    return bt2c::bigEndianToNative(val);
}
@endcode
*/

#include <cstdint>

#include "compat/endian.h" /* IWYU pragma: keep  */

namespace bt2c {

/*! @brief Endianness. */
enum class Endianness
{
    /*! @brief Little-endian. */
    Little,

    /*! @brief Big-endian. */
    Big,
};

/*! @brief Endianness of the host. */
constexpr auto nativeEndianness =
#if BYTE_ORDER == LITTLE_ENDIAN
    Endianness::Little;
#else
    Endianness::Big;
#endif

/*!
@brief
    Returns \bt_p{val}.

@param[in] val
    Value.

@returns
    \bt_p{val}
*/
inline std::uint8_t littleEndianToNative(const std::uint8_t val) noexcept
{
    return val;
}

/*!
@brief
    Returns \bt_p{val}.

@param[in] val
    Value.

@returns
    \bt_p{val}
*/
inline std::uint8_t bigEndianToNative(const std::uint8_t val) noexcept
{
    return val;
}

/*!
@brief
    Returns \bt_p{val}.

@param[in] val
    Value.

@returns
    \bt_p{val}
*/
inline std::int8_t littleEndianToNative(const std::int8_t val) noexcept
{
    return val;
}

/*!
@brief
    Returns \bt_p{val}.

@param[in] val
    Value.

@returns
    \bt_p{val}
*/
inline std::int8_t bigEndianToNative(const std::int8_t val) noexcept
{
    return val;
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::uint16_t littleEndianToNative(const std::uint16_t val) noexcept
{
    return static_cast<std::uint16_t>(le16toh(val));
}

inline std::uint16_t bigEndianToNative(const std::uint16_t val) noexcept
{
    return static_cast<std::uint16_t>(be16toh(val));
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int16_t littleEndianToNative(const std::int16_t val) noexcept
{
    return static_cast<std::int16_t>(littleEndianToNative(static_cast<std::uint16_t>(val)));
}

/*!
@brief
    Returns the native version of the big-endian \bt_p{val}.

@param[in] val
    Big-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int16_t bigEndianToNative(const std::int16_t val) noexcept
{
    return static_cast<std::int16_t>(bigEndianToNative(static_cast<std::uint16_t>(val)));
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::uint32_t littleEndianToNative(const std::uint32_t val) noexcept
{
    return static_cast<std::uint32_t>(le32toh(val));
}

/*!
@brief
    Returns the native version of the big-endian \bt_p{val}.

@param[in] val
    Big-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::uint32_t bigEndianToNative(const std::uint32_t val) noexcept
{
    return static_cast<std::uint32_t>(be32toh(val));
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int32_t littleEndianToNative(const std::int32_t val) noexcept
{
    return static_cast<std::int32_t>(littleEndianToNative(static_cast<std::uint32_t>(val)));
}

/*!
@brief
    Returns the native version of the big-endian \bt_p{val}.

@param[in] val
    Big-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int32_t bigEndianToNative(const std::int32_t val) noexcept
{
    return static_cast<std::int32_t>(bigEndianToNative(static_cast<std::uint32_t>(val)));
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::uint64_t littleEndianToNative(const std::uint64_t val) noexcept
{
    return static_cast<std::uint64_t>(le64toh(val));
}

/*!
@brief
    Returns the native version of the big-endian \bt_p{val}.

@param[in] val
    Big-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::uint64_t bigEndianToNative(const std::uint64_t val) noexcept
{
    return static_cast<std::uint64_t>(be64toh(val));
}

/*!
@brief
    Returns the native version of the little-endian \bt_p{val}.

@param[in] val
    Little-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int64_t littleEndianToNative(const std::int64_t val) noexcept
{
    return static_cast<std::int64_t>(littleEndianToNative(static_cast<std::uint64_t>(val)));
}

/*!
@brief
    Returns the native version of the big-endian \bt_p{val}.

@param[in] val
    Big-endian value.

@returns
    Native version of \bt_p{val}.
*/
inline std::int64_t bigEndianToNative(const std::int64_t val) noexcept
{
    return static_cast<std::int64_t>(bigEndianToNative(static_cast<std::uint64_t>(val)));
}

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_ENDIAN_HPP */
