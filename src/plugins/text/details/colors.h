/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2019 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_TEXT_DETAILS_COLORS_H
#define BABELTRACE_PLUGINS_TEXT_DETAILS_COLORS_H

#include "common/common.h"

#include "write.h"

static inline
const char *color_reset(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.reset;
}

static inline
const char *color_bold(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bold;
}

static inline
const char *color_fg_default(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_default;
}

static inline
const char *color_fg_red(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_red;
}

static inline
const char *color_fg_green(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_green;
}

static inline
const char *color_fg_yellow(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_yellow;
}

static inline
const char *color_fg_blue(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_blue;
}

static inline
const char *color_fg_magenta(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_magenta;
}

static inline
const char *color_fg_cyan(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_cyan;
}

static inline
const char *color_fg_light_gray(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_light_gray;
}

static inline
const char *color_fg_bright_red(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_red;
}

static inline
const char *color_fg_bright_green(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_green;
}

static inline
const char *color_fg_bright_yellow(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_yellow;
}

static inline
const char *color_fg_bright_blue(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_blue;
}

static inline
const char *color_fg_bright_magenta(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_magenta;
}

static inline
const char *color_fg_bright_cyan(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_cyan;
}

static inline
const char *color_fg_bright_light_gray(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.fg_bright_light_gray;
}

static inline
const char *color_bg_default(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_default;
}

static inline
const char *color_bg_red(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_red;
}

static inline
const char *color_bg_green(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_green;
}

static inline
const char *color_bg_yellow(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_yellow;
}

static inline
const char *color_bg_blue(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_blue;
}

static inline
const char *color_bg_magenta(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_magenta;
}

static inline
const char *color_bg_cyan(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_cyan;
}

static inline
const char *color_bg_light_gray(struct details_write_ctx *ctx)
{
	return ctx->details_comp->cfg.color_codes.bg_light_gray;
}

#endif /* BABELTRACE_PLUGINS_TEXT_DETAILS_COLORS_H */
