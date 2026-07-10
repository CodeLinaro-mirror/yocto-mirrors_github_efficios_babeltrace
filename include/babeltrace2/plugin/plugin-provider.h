/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2026 EfficiOS Inc. and Linux Foundation
 */

#ifndef BABELTRACE2_PLUGIN_PLUGIN_PROVIDER_H
#define BABELTRACE2_PLUGIN_PLUGIN_PROVIDER_H

#ifndef __BT_IN_BABELTRACE_H
# error "Please include <babeltrace2/babeltrace.h> instead."
#endif

#include <stdint.h>
#include <stddef.h>

#include <babeltrace2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*!
@defgroup api-plugin-provider Plugin provider

@brief
    A plugin provider.

Plugin providers load libbabeltrace2 plugins.

Borrow the set of plugin providers loaded by libbabeltrace2 with
bt_plugin_provider_set_borrow(). This function loads the plugin providers
if libbabeltrace2 didn't already do so, reporting any loading error.

With the borrowed plugin provider set, query the number of plugin
providers it contains with
bt_plugin_provider_set_get_plugin_provider_count(), and borrow a specific
plugin provider using
bt_plugin_provider_set_borrow_plugin_provider_by_index().

See \ref api-plugin-provider-loading-def-dirs "Automatic plugin provider loading"
to learn how libbabeltrace2 automatically loads plugin providers.

@sa \ref guide-write-plugin-provider

<h1>Plugin provider properties</h1>

A plugin provider has the following properties:

<dl>
  <dt>
    \anchor api-plugin-provider-prop-name
    Name
  </dt>
  <dd>
    Name of the plugin provider.

    The name of a plugin provider is not related to its file name. For
    example, a plugin provider found in the file \c fabienk.so can
    be named <code>Angine</code>.

    A plugin provider has a mandatory name property.

    Use bt_plugin_provider_get_name().
  </dd>

  <dt>
    \anchor api-plugin-provider-prop-descr
    \bt_dt_opt Description
  </dt>
  <dd>
    Description of the plugin provider.

    Use bt_plugin_provider_get_description().
  </dd>

  <dt>
    \anchor api-plugin-provider-prop-author
    \bt_dt_opt Author name(s)
  </dt>
  <dd>
    Name(s) of the plugin provider's author(s).

    Use bt_plugin_provider_get_author().
  </dd>

  <dt>
    \anchor api-plugin-provider-prop-license
    \bt_dt_opt License
  </dt>
  <dd>
    License or license name of the plugin provider.

    Use bt_plugin_provider_get_license().
  </dd>

  <dt>
    \anchor api-plugin-provider-prop-path
    \bt_dt_opt Path
  </dt>
  <dd>
    Path of the file which contains the plugin provider.

    A static plugin provider has no path property.

    Use bt_plugin_provider_get_path().
  </dd>

  <dt>
    \anchor api-plugin-provider-prop-version
    \bt_dt_opt Version
  </dt>
  <dd>
    Version of the plugin provider (major, minor, patch, and extra
    information).

    The version of a plugin provider is completely user-defined: the library
    does not use this property in any way to verify the compatibility of the
    plugin provider.

    Use bt_plugin_provider_get_version().
  </dd>
</dl>
*/

/*! @{ */

/*!
@name Types
@{

@typedef struct bt_plugin_provider bt_plugin_provider;

@brief
    Plugin provider.

@typedef struct bt_plugin_provider_set bt_plugin_provider_set;

@brief
    Set of plugin providers loaded by libbabeltrace2.

@}
*/

/*!
@name Plugin provider set access
@{
*/

/*!
@brief
    Status codes for bt_plugin_provider_set_borrow().
*/
typedef enum bt_plugin_provider_set_borrow_status {
	/*!
	@brief
	    Success.
	*/
	BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_OK		= __BT_FUNC_STATUS_OK,

	/*!
	@brief
	    Out of memory.
	*/
	BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_MEMORY_ERROR = __BT_FUNC_STATUS_MEMORY_ERROR,

	/*!
	@brief
	    Error.
	*/
	BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_ERROR	= __BT_FUNC_STATUS_ERROR,
} bt_plugin_provider_set_borrow_status;

/*!
@brief
    Borrows the set of plugin providers loaded by libbabeltrace2,
    setting \bt_p{*plugin_provider_set} to it.

This function attempts to load plugin providers when libbabeltrace2
didn't already do so. Any error which occurs while loading the plugin
providers is reported through the returned status code and the
\ref api-error "current thread's error".

@param[out] plugin_provider_set
    <strong>On success</strong>, \bt_p{*plugin_provider_set} is the set
    of plugin providers loaded by libbabeltrace2.

    The returned pointer remains valid until libbabeltrace2 is unloaded.

@retval #BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_OK
    Success.
@retval #BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_MEMORY_ERROR
    Out of memory.
@retval #BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_ERROR
    Error.

@bt_pre_not_null{plugin_provider_set}
*/
extern bt_plugin_provider_set_borrow_status bt_plugin_provider_set_borrow(
		const bt_plugin_provider_set **plugin_provider_set)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the number of plugin providers contained in
    \bt_p{plugin_provider_set}.

@param[in] plugin_provider_set
    Plugin provider set of which to get the number of contained plugin
    providers.

@returns
    Number of plugin providers contained in
    \bt_p{plugin_provider_set}.

@bt_pre_not_null{plugin_provider_set}
*/
extern uint64_t bt_plugin_provider_set_get_plugin_provider_count(
		const bt_plugin_provider_set *plugin_provider_set)
		__BT_NOEXCEPT;

/*!
@brief
    Borrows the plugin provider at index \bt_p{index} from
    \bt_p{plugin_provider_set}.

@param[in] plugin_provider_set
    Plugin provider set from which to borrow the plugin provider at index
    \bt_p{index}.
@param[in] index
    Index of the plugin provider to borrow.

@returns
    @parblock
    \em Borrowed reference of the plugin provider at index \bt_p{index}.

    The returned pointer remains valid until libbabeltrace2 is unloaded.
    @endparblock

@bt_pre_not_null{plugin_provider_set}
@pre
    \bt_p{index} is less than the number of plugin providers contained
    in \bt_p{plugin_provider_set}, as returned by
    bt_plugin_provider_set_get_plugin_provider_count().
*/
extern const bt_plugin_provider *
bt_plugin_provider_set_borrow_plugin_provider_by_index(
		const bt_plugin_provider_set *plugin_provider_set,
		uint64_t index) __BT_NOEXCEPT;

/*! @} */

/*!
@name Plugin provider properties
@{
*/

/*!
@brief
    Returns the name property of \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-name "name" property.

@param[in] plugin_provider
    Plugin provider of which to get the name property.

@returns
    @parblock
    Name property of \bt_p{plugin_provider}.

    The returned pointer remains valid as long as
    \bt_p{plugin_provider} exists.
    @endparblock

@bt_pre_not_null{plugin_provider}
*/
extern const char *bt_plugin_provider_get_name(
		const bt_plugin_provider *plugin_provider)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the description property of \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-descr "description" property.

@param[in] plugin_provider
    Plugin provider of which to get the description property.

@returns
    @parblock
    Description property of \bt_p{plugin_provider}, or \c NULL if
    not available.

    The returned pointer remains valid as long as
    \bt_p{plugin_provider} exists.
    @endparblock

@bt_pre_not_null{plugin_provider}
*/

extern const char *bt_plugin_provider_get_description(
		const bt_plugin_provider *plugin_provider)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the name(s) of the author(s) of \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-author "author name(s)"
property.

@param[in] plugin_provider
    Plugin provider of which to get the author name(s) property.

@returns
    @parblock
    Author name(s) of \bt_p{plugin_provider}, or \c NULL
    if not available.

    The returned pointer remains valid as long as
    \bt_p{plugin_provider} exists.
    @endparblock

@bt_pre_not_null{plugin_provider}
*/
extern const char *bt_plugin_provider_get_author(
		const bt_plugin_provider *plugin_provider)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the license text or the license name of
    \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-license "license" property.

@param[in] plugin_provider
    Plugin provider of which to get the license property.

@returns
    @parblock
    License of \bt_p{plugin_provider}, or \c NULL
    if not available.

    The returned pointer remains valid as long as
    \bt_p{plugin_provider} exists.
    @endparblock

@bt_pre_not_null{plugin_provider}
*/
extern const char *bt_plugin_provider_get_license(
		const bt_plugin_provider *plugin_provider)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the path of the file which contains \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-path "path" property.

This function returns \c NULL if \bt_p{plugin_provider} is a static
plugin provider because a static plugin provider has no path property.

@param[in] plugin_provider
    Plugin provider of which to get the containing file path.

@returns
    @parblock
    Path of the file which contains \bt_p{plugin_provider}, or \c NULL
    if not available.

    The returned pointer remains valid as long as
    \bt_p{plugin_provider} exists.
    @endparblock

@bt_pre_not_null{plugin_provider}
*/
extern const char *bt_plugin_provider_get_path(
		const bt_plugin_provider *plugin_provider)
		__BT_NOEXCEPT;

/*!
@brief
    Returns the version of \bt_p{plugin_provider}.

See the \ref api-plugin-provider-prop-version "version" property.

@param[in] plugin_provider
    Plugin provider of which to get the version property.
@param[out] major
    <strong>If not \c NULL and this function returns
    #BT_PROPERTY_AVAILABILITY_AVAILABLE</strong>, \bt_p{*major} is the
    major version of \bt_p{plugin_provider}.
@param[out] minor
    <strong>If not \c NULL and this function returns
    #BT_PROPERTY_AVAILABILITY_AVAILABLE</strong>, \bt_p{*minor} is the
    minor version of \bt_p{plugin_provider}.
@param[out] patch
    <strong>If not \c NULL and this function returns
    #BT_PROPERTY_AVAILABILITY_AVAILABLE</strong>, \bt_p{*patch} is the
    patch version of \bt_p{plugin_provider}.
@param[out] extra
    @parblock
    <strong>If not \c NULL and this function returns
    #BT_PROPERTY_AVAILABILITY_AVAILABLE</strong>, \bt_p{*extra} is the
    extra information of the version of \bt_p{plugin_provider}.

    \bt_p{*extra} can be \c NULL if the version of the plugin provider has no
    extra information.

    \bt_p{*extra} remains valid as long as \bt_p{plugin_provider}
    exists.
    @endparblock

@retval #BT_PROPERTY_AVAILABILITY_AVAILABLE
    The version property of \bt_p{plugin_provider} is available.
@retval #BT_PROPERTY_AVAILABILITY_NOT_AVAILABLE
    The version property of \bt_p{plugin_provider} is not available.

@bt_pre_not_null{plugin_provider}
*/
extern bt_property_availability bt_plugin_provider_get_version(
		const bt_plugin_provider *plugin_provider,
		unsigned int *major, unsigned int *minor,
		unsigned int *patch, const char **extra) __BT_NOEXCEPT;

/*! @} */

/*! @} */


#ifdef __cplusplus
}
#endif

#endif /* BABELTRACE2_PLUGIN_PLUGIN_PROVIDER_H */
