/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <stdbool.h>
#include <string.h>

#include "lvgl.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The screen's language. With CONFIG_MUSE_LANG_ARABIC the UI's own text is
 * shown in Arabic, right to left, and every font can draw Arabic, so Arabic
 * replies from Muse show too. English text (network names, English replies)
 * still shows as it is. Without it, these do nothing and the UI is English.
 */
#if CONFIG_MUSE_LANG_ARABIC

/* Whether the UI is in Arabic. The firmware fixes it at build time; the
 * simulator picks it at start (muse_i18n_set_arabic). */
bool muse_i18n_arabic(void);
void muse_i18n_set_arabic(bool on);
/* Before the UI is built. */
void muse_i18n_init(void);

/* The UI's text in the screen's language: `en` itself if it has no
 * translation, or the UI is in English. Captions that end in a number or a
 * name ("LISTENING 2.1s") translate by their start. */
const char *muse_tr(const char *en);

/* Sets label `l` to `text`, readied for LVGL's bidi when it's Arabic: the
 * label runs right to left, and each number in it is marked to stay left to
 * right (see muse_i18n.c). */
void muse_label_set(lv_obj_t *l, const char *text);

/* `text` with its Arabic letters joined, for what LVGL draws without joining
 * them itself (a text area's placeholder); `text` itself in English. */
const char *muse_text_joined(const char *text);

/* Whether label `l` shows `text`, as muse_label_set left it. A label keeps
 * Arabic as the joined letters it draws, so a plain strcmp never matches. */
bool muse_label_shows(lv_obj_t *l, const char *text);

/* `font` with Arabic behind it, so it draws both, and lines tall enough for
 * Arabic's dots and hamzas; `font` itself in English. */
const lv_font_t *muse_font(const lv_font_t *font);

/* The alignment for `text` on `l`, a label `width` px wide: short text and
 * titles centred, Arabic that wraps over several lines to the right. English
 * keeps `en_align`. */
lv_text_align_t muse_text_align(lv_obj_t *l, const char *text, int width, lv_text_align_t en_align);

#else

static inline bool muse_i18n_arabic(void) { return false; }
static inline void muse_i18n_set_arabic(bool on) { (void)on; }
static inline void muse_i18n_init(void) {}
static inline const char *muse_tr(const char *en) { return en; }
static inline void muse_label_set(lv_obj_t *l, const char *text) { lv_label_set_text(l, text); }
static inline const char *muse_text_joined(const char *text) { return text; }
static inline bool muse_label_shows(lv_obj_t *l, const char *text) { return !strcmp(lv_label_get_text(l), text); }
static inline const lv_font_t *muse_font(const lv_font_t *font) { return font; }
static inline lv_text_align_t muse_text_align(lv_obj_t *l, const char *text, int width, lv_text_align_t en_align)
{
    (void)l;
    (void)text;
    (void)width;
    return en_align;
}

#endif

#ifdef __cplusplus
}
#endif
