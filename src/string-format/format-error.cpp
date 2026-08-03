/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright EfficiOS, Inc.
 */

#include "cpp-common/bt2/wrap.hpp"
#include "cpp-common/bt2c/glib-up.hpp"
#include "string-format/format-plugin-comp-cls-name.hpp"

#include "format-error.h"
#include "format-error.hpp"

namespace {

bt2c::Logger createLogger(const bt2c::Logger& parentLogger)
{
    return bt2c::Logger {parentLogger, "COMMON/FORMAT-ERROR"};
}

static std::string formatBtErrorCauseInt(const bt2::ConstErrorCause cause,
                                         const unsigned int columns, const bt2c::Logger& logger,
                                         const bt_common_color_when useColors)
{
    std::string str = "[";
    const auto codes = bt_common_color_get_codes(useColors);

    /* Print actor name */
    switch (cause.actorType()) {
    case bt2::ErrorCauseActorType::Unknown:
        fmt::format_to(std::back_inserter(str), "{}{}{}", codes.bold, cause.moduleName(),
                       codes.reset);
        break;

    case bt2::ErrorCauseActorType::Component:
    {
        const auto compCause = cause.asComponent();

        fmt::format_to(std::back_inserter(str), "{}{}{}: {}", codes.bold, compCause.componentName(),
                       codes.reset,
                       formatPluginCompClsOpt(compCause.pluginName(),
                                              compCause.componentClassName(),
                                              compCause.componentClassType(), useColors));
        break;
    }

    case bt2::ErrorCauseActorType::ComponentClass:
    {
        const auto compClsCause = cause.asComponentClass();

        str += formatPluginCompClsOpt(compClsCause.pluginName(), compClsCause.componentClassName(),
                                      compClsCause.componentClassType(), useColors);
        break;
    }

    case bt2::ErrorCauseActorType::MessageIterator:
    {
        const auto msgIterCause = cause.asMessageIterator();

        fmt::format_to(std::back_inserter(str), "{}{}{} ({}{}{}): {}", codes.bold,
                       msgIterCause.componentName(), codes.reset, codes.bold,
                       msgIterCause.componentOutputPortName(), codes.reset,
                       formatPluginCompClsOpt(msgIterCause.pluginName(),
                                              msgIterCause.componentClassName(),
                                              msgIterCause.componentClassType(), useColors));
        break;
    }

    default:
        bt_common_abort();
    }

    /* Print file name and line number */
    fmt::format_to(std::back_inserter(str), "] ({}{}{}{}:{}{}{})\n", codes.bold,
                   codes.fg_bright_magenta, cause.fileName(), codes.reset, codes.fg_green,
                   cause.lineNumber(), codes.reset);

    /* Print message */
    const bt2c::GStringUP folded {bt_common_fold(cause.message(), columns, 2)};

    if (folded) {
        str += folded->str;
    } else {
        BT_CPPLOGW_SPEC(logger, "Could not fold string.");
        str += cause.message();
    }

    return str;
}

} /* namespace */

std::string formatBtErrorCause(const bt2::ConstErrorCause cause, const unsigned int columns,
                               const bt2c::Logger& parentLogger,
                               const bt_common_color_when useColors)
{
    return formatBtErrorCauseInt(cause, columns, createLogger(parentLogger), useColors);
}

std::string formatBtError(const bt2::ConstError error, const unsigned int columns,
                          const bt2c::Logger& parentLogger, const bt_common_color_when useColors)
{
    BT_ASSERT(error.length() > 0);

    const auto logger = createLogger(parentLogger);
    std::string str;
    const auto codes = bt_common_color_get_codes(useColors);

    /* Reverse order: deepest (root) cause printed at the end */
    for (std::int64_t i = error.length() - 1; i >= 0; --i) {
        /* Format prefix */
        if (i < error.length() - 1) {
            fmt::format_to(std::back_inserter(str), "{}{}CAUSED BY{} {}", codes.bold,
                           codes.fg_bright_red, codes.reset,
                           formatBtErrorCauseInt(error[i], columns, logger, useColors));
        } else {
            fmt::format_to(std::back_inserter(str), "{}{}ERROR{}:    {}", codes.bold,
                           codes.fg_bright_red, codes.reset,
                           formatBtErrorCauseInt(error[i], columns, logger, useColors));
        }

        /*
         * Don't append a newline at the end, since that is used to
         * generate the Python __str__, which doesn't need a newline
         * at the end.
         */
        if (i > 0) {
            str += '\n';
        }
    }

    return str;
}

gchar *format_bt_error_cause(const bt_error_cause * const errorCause, const unsigned int columns,
                             const bt_logging_level logLevel, const bt_common_color_when useColors)
{
    try {
        return g_strdup(formatBtErrorCause(
                            bt2::wrap(errorCause), columns,
                            createLogger(bt2c::Logger {"BT2C", "DUMMY-PARENT-LOGGER",
                                                       static_cast<bt2c::Logger::Level>(logLevel)}),
                            useColors)
                            .c_str());
    } catch (const bt2c::MemoryError&) {
        return nullptr;
    }
}

gchar *format_bt_error(const bt_error * const error, const unsigned int columns,
                       const bt_logging_level logLevel, const bt_common_color_when useColors)
{
    try {
        return g_strdup(
            formatBtError(bt2::wrap(error), columns,
                          createLogger(bt2c::Logger {"BT2C", "DUMMY-PARENT-LOGGER",
                                                     static_cast<bt2c::Logger::Level>(logLevel)}),
                          useColors)
                .c_str());
    } catch (const bt2c::MemoryError&) {
        return nullptr;
    }
}
