# Arabic fonts

`muse_font_ar_12.c`, `_16`, `_20` and `_28` are Noto Sans Arabic Regular,
with Noto Sans Regular's ASCII in the 12 and 16 px ones, converted to LVGL
fonts by `tools/gen_arabic_font.sh`. Run it again to change
the sizes or the characters; don't edit them by hand. They're built only with
`CONFIG_MUSE_LANG_ARABIC` (`components/muse/muse_i18n.h`).

Noto Sans Arabic and Noto Sans are Copyright 2022 The Noto Project Authors and
licensed under the SIL Open Font License 1.1, in [`OFL.txt`](OFL.txt). The Apache License
doesn't cover these files.
