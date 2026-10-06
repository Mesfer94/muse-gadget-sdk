#!/usr/bin/env bash
# Copyright (c) Meta Platforms, Inc. and affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Regenerates the Arabic fonts in components/muse/fonts/ from Noto Sans Arabic
# and Noto Sans (SIL Open Font License 1.1) with LVGL's lv_font_conv.
#
# Usage: gen_arabic_font.sh [NotoSansArabic-Regular.ttf NotoSans-Regular.ttf]
#
# Without paths it downloads the fonts from the Noto project. Needs Node.js
# for npx (lv_font_conv is fetched on first use).
#
# The 12 and 16 px fonts also hold ASCII from Noto Sans, Noto Sans Arabic's
# Latin sibling: in Arabic, muse_i18n.c puts them in front of unscii on Muse's
# face, so a reply's English words and numbers are drawn in the same style as
# its Arabic. The 20 and 28 px fonts hold Arabic only and go behind Montserrat,
# which already draws Latin in a matching sans; the settings stay as they are.
#
# Only basic Arabic goes in, to keep them small: the 28 letters with hamza
# forms, taa marbuta and alef maksura, tatweel, Arabic punctuation and both
# sets of Arabic digits. LVGL joins letters by swapping in their presentation
# forms (LV_USE_ARABIC_PERSIAN_CHARS, src/misc/lv_text_ap.c), so the fonts
# carry Arabic Presentation Forms-B (U+FE70-FEFC) and, of Presentation Forms-A
# (U+FB50-FDFF), the forms lv_text_ap.c can produce: those of the Persian
# letters peh, tcheh, jeh, keheh, gaf and farsi yeh. Harakat aren't drawn
# (muse_text.c drops them), since LVGL doesn't place combining marks.
#
# No kerning: LVGL measures a line with its letters in the order they're
# stored and draws them in the order they're shown, which for Arabic is the
# other way round, so a kerning pair would size and draw the line
# differently. It also saves space.
set -euo pipefail

cd "$(dirname "$0")/.."
OUT=components/muse/fonts
NOTO=https://github.com/notofonts/notofonts.github.io/raw/main/fonts

TTF=${1:-}
LATIN_TTF=${2:-}
if [[ -z $TTF || -z $LATIN_TTF ]]; then
    TMP=$(mktemp -d)
    trap 'rm -rf "$TMP"' EXIT
    TTF=$TMP/NotoSansArabic-Regular.ttf
    LATIN_TTF=$TMP/NotoSans-Regular.ttf
    echo "Downloading Noto Sans Arabic and Noto Sans" >&2
    curl -fsSL -o "$TTF" "$NOTO/NotoSansArabic/unhinted/ttf/NotoSansArabic-Regular.ttf"
    curl -fsSL -o "$LATIN_TTF" "$NOTO/NotoSans/unhinted/ttf/NotoSans-Regular.ttf"
fi

RANGES=(
    0x060C 0x061B 0x061F           # comma, semicolon, question mark
    0x0621-0x063A 0x0640-0x064A    # letters, tatweel
    0x0660-0x066C                  # Arabic-Indic digits, percent, decimal and thousands separators
    0x067E 0x0686 0x0698 0x06A9 0x06AF 0x06CC   # the Persian letters lv_text_ap.c knows
    0x06F0-0x06F9                  # Persian digits
    0xFB56-0xFB59 0xFB7A-0xFB7D 0xFB8A-0xFB8B 0xFB8E-0xFB95 0xFBFC-0xFBFF
    0xFE70-0xFEFC                  # Presentation Forms-B, lam-alef included
)
RANGE_ARG=$(IFS=,; echo "${RANGES[*]}")

# Sizes match the UI's fonts: 12 for unscii 8 and Montserrat 12, 16 for
# unscii 16 and Montserrat 14 and 16, then 20 and 28.
mkdir -p "$OUT"
for size in 12 16 20 28; do
    name=muse_font_ar_$size
    latin=()
    if [[ $size == 12 || $size == 16 ]]; then
        latin=(--font "$LATIN_TTF" -r 0x20-0x7E)
    fi
    npx --yes lv_font_conv@1.5.3 --font "$TTF" -r "$RANGE_ARG" ${latin[@]+"${latin[@]}"} \
        --size "$size" --bpp 4 --no-compress --no-kerning --format lvgl \
        --lv-include lvgl.h --lv-font-name "$name" -o "$OUT/$name.c"
    # The header records the options; keep the fonts' paths out of it.
    sed -i.bak -e "s|--font $TTF|--font $(basename "$TTF")|" -e "s|--font $LATIN_TTF|--font $(basename "$LATIN_TTF")|" \
        "$OUT/$name.c" && rm -f "$OUT/$name.c.bak"
    echo "$OUT/$name.c" >&2
done
