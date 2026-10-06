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

#include "muse_i18n.h"

#if CONFIG_MUSE_LANG_ARABIC

#include <stdio.h>
#include <string.h>

#include "muse_state.h"
#include "src/misc/lv_bidi_private.h"   /* lv_bidi_detect_base_dir */
#include "src/misc/lv_text_ap.h"

#if !LV_USE_BIDI || !LV_USE_ARABIC_PERSIAN_CHARS
#error "Arabic needs LV_USE_BIDI and LV_USE_ARABIC_PERSIAN_CHARS"
#endif

/* Noto Sans Arabic, made by tools/gen_arabic_font.sh. */
extern const lv_font_t muse_font_ar_12;
extern const lv_font_t muse_font_ar_16;
extern const lv_font_t muse_font_ar_20;
extern const lv_font_t muse_font_ar_28;

/* The firmware is in Arabic when built with it; the simulator only when asked. */
#if CONFIG_MUSE_BOARD_SIMULATOR
static bool s_arabic;
#else
static bool s_arabic = true;
#endif

/*
 * Arabic, without harakat: LVGL can't place combining marks (muse_text.c
 * drops them). Keys are the exact English the UI shows, format strings
 * included, so a "%s" here takes the same argument.
 */
static const struct {
    const char *en, *ar;
} WORDS[] = {
    /* Muse's face: the mode, and why it's idle */
    { "WAKING UP", "يبدأ التشغيل" },
    { "READY", "جاهز" },
    { "LISTENING", "أستمع" },
    { "THINKING", "أفكر" },
    { "SPEAKING", "أتحدث" },
    { "ERROR", "خطأ" },
    { "GOODBYE", "مع السلامة" },
    { "WI-FI OFF", "الواي فاي مطفأ" },
    { "SET UP WI-FI", "اضبط الواي فاي" },
    { "NO WI-FI", "لا يوجد واي فاي" },
    { "RECONNECTING", "يعيد الاتصال" },
    { "CONNECTING", "يتصل" },
    { "USB POWER", "طاقة USB" },
    { "CHARGING %d%%", "يشحن %d%%" },
    { "BATTERY %d%%", "البطارية %d%%" },
    { "TAP TO TAKE PHOTO", "المس لالتقاط صورة" },

    /* The pairing card */
    { "Pairing code", "رمز الاقتران" },
    { "Enter it on your phone", "أدخله في هاتفك" },
    { "Enter on phone", "أدخله بالهاتف" },
    { "Pair with Muse app", "الاقتران بتطبيق ميوز" },
    { "Muse app", "تطبيق ميوز" },
    { "Press button", "اضغط الزر" },
    { "Press", "اضغط" },
    { "Press the %s button", "اضغط الزر %s" },
    { "%s button", "الزر %s" },

    /* The buttons' names (muse_board_t) */
    { "yellow", "الأصفر" },
    { "blue", "الأزرق" },
    { "power", "الطاقة" },
    { "front", "الأمامي" },
    { "side", "الجانبي" },
    { "top", "العلوي" },
    { "wheel", "العجلة" },
    { "scroll", "التمرير" },
    { "boot", "BOOT" },

    /* Captions */
    { "WAKING UP...", "يبدأ التشغيل..." },
    { "LISTENING...", "أستمع..." },
    { "RECORDING...", "أسجل..." },
    { "SENDING VOICE NOTE", "أرسل الرسالة الصوتية" },
    { "NOTE SENT - WAITING FOR MUSE", "أرسلت الرسالة - بانتظار ميوز" },
    { "SET UP MUSE FIRST", "اضبط ميوز أولا" },
    { "COULDN'T SAVE THE NOTE", "تعذر حفظ الرسالة" },
    { "SENDING SAVED NOTE", "أرسل رسالة محفوظة" },
    { "SAVED NOTE SENT", "أرسلت الرسالة المحفوظة" },
    { "COULDN'T SEND A SAVED NOTE", "تعذر إرسال رسالة محفوظة" },
    { "SAVED NOTE: WILL TRY AGAIN", "رسالة محفوظة: سأحاول مرة أخرى" },
    { "NOTES STILL WAITING TO SEND", "رسائل بانتظار الإرسال" },
    { "HOLD LONGER TO TALK", "اضغط مدة أطول للتحدث" },
    { "AUDIO INIT FAILED", "فشل تشغيل الصوت" },
    { "GOODBYE!", "مع السلامة!" },
    { "COULDN'T POWER OFF", "تعذر الإطفاء" },
    { "PHONE SETUP ON", "إعداد الهاتف يعمل" },
    { "PHONE SETUP OFF", "إعداد الهاتف مطفأ" },
    { "HOLD TO POWER OFF", "اضغط مطولا للإطفاء" },
    { "RESETTING...", "يعيد الضبط..." },
    { "SPEAKER OFF", "السماعة مطفأة" },
    { "SPEAKER ON", "السماعة تعمل" },
    { "HOLD TO MUTE", "اضغط مطولا للكتم" },
    { "HOLD TO UNMUTE", "اضغط مطولا لإلغاء الكتم" },
    { "LOST CONNECTION TO MUSE", "انقطع الاتصال بميوز" },
    { "MUSE DIDN'T TAKE IT", "لم يستلمها ميوز" },
    { "MUSE REPLY ACCESS DENIED (403)", "رفض الوصول لرد ميوز (403)" },
    { "MUSE REPLY AUTH REQUIRED (401)", "رد ميوز يتطلب تسجيل الدخول (401)" },
    { "NO REPLY FROM MUSE", "لا رد من ميوز" },
    { "REPLY TOO LONG", "الرد طويل جدا" },
    { "REPLY BUFFER LIMIT - TRY AGAIN", "الرد تجاوز الحد - حاول مرة أخرى" },
    { "OUT OF MEMORY", "الذاكرة ممتلئة" },
    { "INTERRUPTED", "توقف" },
    { "CAN'T REACH MUSE", "تعذر الوصول إلى ميوز" },
    { "MUSE NOT SET UP", "ميوز غير معد" },
    { "MUSE STOPPED LISTENING", "توقف ميوز عن الاستماع" },
    { "MUSE COULDN'T LISTEN", "تعذر على ميوز الاستماع" },
    { "CAMERA FRAME TOO LARGE", "إطار الكاميرا كبير جدا" },
    { "HIMAX CAMERA NOT READY", "الكاميرا غير جاهزة" },
    { "CAMERA PREVIEW FAILED", "فشلت معاينة الكاميرا" },
    { "PHOTO CAPTURED", "التقطت الصورة" },
    { "CAMERA MEMORY ERROR", "خطأ في ذاكرة الكاميرا" },

    /* Settings: page titles and home */
    { "SETTINGS", "الإعدادات" },
    { "WI-FI", "الواي فاي" },
    { "MUSE", "ميوز" },
    { "BLUETOOTH", "البلوتوث" },
    { "SOUND", "الصوت" },
    { "AUTO-SLEEP", "السكون التلقائي" },
    { "BATTERY", "البطارية" },
    { "POWER", "الطاقة" },
    { "Wi-Fi", "واي فاي" },
    { "Muse", "ميوز" },
    { "Bluetooth", "بلوتوث" },
    { "Sound", "الصوت" },
    { "Sleep", "السكون" },
    { "Battery", "البطارية" },
    { "Power off", "إطفاء" },
    { "Off", "مطفأ" },
    { "On", "يعمل" },
    { "Not set", "غير محدد" },
    { "Joining", "ينضم" },
    { "Failed", "فشل" },
    { "Not nearby", "ليست قريبة" },
    { "Connected", "متصل" },
    { "Vol %d%%", "الصوت %d%%" },
    { "Muted", "مكتوم" },
    { "Muse %s  -  %s", "ميوز %s  -  %s" },
    { "offline", "غير متصل" },

    /* Text entry */
    { "Show", "إظهار" },
    { "Hide", "إخفاء" },
    { "Password", "كلمة المرور" },
    { "Empty if it's open", "فارغ إن كانت مفتوحة" },
    { "Other network", "شبكة أخرى" },
    { "Network name", "اسم الشبكة" },

    /* Wi-Fi */
    { "Saved networks", "الشبكات المحفوظة" },
    { "Tap to forget", "المس للحذف" },
    { "Hidden", "مخفية" },
    { "saved  ", "محفوظة " },
    { "open  ", "مفتوحة " },
    { "No networks found", "لم يعثر على شبكات" },
    { LV_SYMBOL_REFRESH "  Scan for networks", LV_SYMBOL_REFRESH "  البحث عن شبكات" },
    { "Scanning...", "يبحث..." },
    { "Other network...", "شبكة أخرى..." },
    { "MAC address", "عنوان MAC" },
    { "Muse remembers up to 8 networks and joins the strongest one in range. Tap a saved one twice to forget it.",
      "يتذكر ميوز حتى 8 شبكات وينضم إلى أقواها في النطاق. المس شبكة محفوظة مرتين لحذفها." },
    { "Wi-Fi is off", "الواي فاي مطفأ" },
    { "No saved networks. Scan and pick one.", "لا توجد شبكات محفوظة. ابحث واختر واحدة." },
    { "Joining %s\n%s", "ينضم إلى %s\n%s" },
    { "Connected to %s\n%s  -  %d dBm", "متصل بـ %s\nIP %s  -  %d dBm" },   /* a line can't start with a number */
    { "No saved network nearby\nLooking again within a minute", "لا توجد شبكة محفوظة قريبة\nسأبحث مرة أخرى خلال دقيقة" },
    { "Couldn't join %s\n%s", "تعذر الانضمام إلى %s\n%s" },

    /* Muse */
    { "Muse server", "خادم ميوز" },
    { "Empty for the default", "فارغ للافتراضي" },
    { "VM ID", "معرف VM" },
    { "Optional", "اختياري" },
    { "Device token", "رمز الجهاز" },
    { "Empty keeps the current one", "فارغ يبقي الحالي" },
    { "Resetting...", "يعيد الضبط..." },
    { "Tap again to reset", "المس مرة أخرى لإعادة الضبط" },
    { "Reset pairing", "إعادة ضبط الاقتران" },
    { "Server", "الخادم" },
    { "Test connection", "اختبار الاتصال" },
    { "Pair with the Muse app to use your account; a device token here overrides it, and a long one is "
      "easier to send over Bluetooth. The VM ID picks one of your VMs. "
      "Reset pairing forgets Wi-Fi and the app pairing, then restarts.",
      "اقترن بتطبيق ميوز لاستخدام حسابك. رمز الجهاز هنا يتقدم عليه، والرمز الطويل أسهل إرسالا عبر البلوتوث. "
      "معرف VM يختار أحد أجهزتك الافتراضية. إعادة ضبط الاقتران تحذف الواي فاي واقتران التطبيق ثم تعيد التشغيل." },
    { "Muse app: %s\n%s", "تطبيق ميوز: %s\n%s" },
    { "paired", "مقترن" },
    { "not paired", "غير مقترن" },
    { "Set (%u chars)", "محدد (%u حرفا)" },
    /* muse_link_state_name and muse_hatch_state_name */
    { "Starting", "يبدأ" },
    { "Ready to pair", "جاهز للاقتران" },
    { "App connected", "التطبيق متصل" },
    { "Confirm pairing", "أكد الاقتران" },
    { "Connecting", "يتصل" },
    { "Online", "متصل" },
    { "Offline", "غير متصل" },
    { "Error", "خطأ" },
    { "Not set up", "غير معد" },
    { "Saved", "محفوظ" },
    { "Can't connect", "تعذر الاتصال" },

    /* Bluetooth */
    { "Phone setup", "إعداد الهاتف" },
    { "Forget paired phones", "حذف الهواتف المقترنة" },
    { "When on, Muse is visible to phones nearby. Open tools/ble_setup.html in Chrome, "
      "connect, and enter the code Muse shows to pair.",
      "عند التشغيل يظهر ميوز للهواتف القريبة. افتح tools/ble_setup.html في Chrome، "
      "ثم اتصل وأدخل الرمز الذي يعرضه ميوز للاقتران." },
    { "Visible as %s", "ظاهر باسم %s" },
    { "Phone connected\n%s", "الهاتف متصل\n%s" },
    { "Paired", "مقترن" },
    { "Waiting for pairing", "بانتظار الاقتران" },

    /* Sound */
    { "Speaker", "السماعة" },
    { "Volume", "مستوى الصوت" },
    { "Mic gain", "تضخيم الميكروفون" },
    { "Mic level", "مستوى الميكروفون" },
    { "Talk at arm's length: the bar should reach green (-30 to -15 dBFS) without going orange.",
      "تحدث على بعد ذراع: يجب أن يصل الشريط إلى الأخضر (-30 إلى -15 dBFS) دون أن يصير برتقاليا." },
    { "Brightness", "السطوع" },

    /* Sleep */
    { "Turn the screen off after Muse has been idle for:", "أطفئ الشاشة بعد خمول ميوز لمدة:" },
    { "Never", "أبدا" },
    { "30 seconds", "30 ثانية" },
    { "1 minute", "دقيقة واحدة" },
    { "2 minutes", "دقيقتان" },
    { "5 minutes", "5 دقائق" },
    { "10 minutes", "10 دقائق" },
    { "Custom", "مخصص" },
    { LV_SYMBOL_EYE_CLOSE "  Sleep now", LV_SYMBOL_EYE_CLOSE "  أطفئ الشاشة الآن" },
    { "Tap the screen or press either button to wake.", "المس الشاشة أو اضغط أي زر للإيقاظ." },

    /* Battery */
    { "Used", "المستهلك" },
    { "A full charge", "شحنة كاملة" },
    { "Screen off", "الشاشة مطفأة" },
    { "Chip asleep", "الشريحة نائمة" },
    { "Wakes", "مرات الاستيقاظ" },
    { "CPU busy", "انشغال المعالج" },
    { LV_SYMBOL_REFRESH "  Start over", LV_SYMBOL_REFRESH "  البدء من جديد" },
    { "Measures from unplugging USB until it's plugged back in. The gauge moves in 1% steps, so give it a "
      "few hours. Chip asleep is time in light sleep; CPU busy is time a core was running a task.",
      "يقيس من فصل USB حتى إعادة توصيله. يتحرك المقياس بخطوات 1%، فامنحه بضع ساعات. "
      "الشريحة نائمة هو وقت السكون الخفيف، وانشغال المعالج هو وقت عمل إحدى النوى." },
    { "%d h %d min", "%d س %d د" },
    { "%d min", "%d د" },
    { "No battery", "لا توجد بطارية" },
    { "Unplug USB to start measuring.", "افصل USB لبدء القياس." },
    { "On battery for %s", "على البطارية منذ %s" },
    { "Last run: %s on battery", "آخر قياس: %s على البطارية" },
    { "None", "لا يوجد" },
    { "lasts ~%d h", "تدوم ~%d س" },
    { "%d%% so far", "%d%% حتى الآن" },
    { "measuring", "يقيس" },
    { "Also kept awake by: %s", "أبقاه مستيقظا أيضا: %s" },

    /* Power */
    { "Power Muse off completely?", "إطفاء ميوز بالكامل؟" },
    { LV_SYMBOL_POWER "  Power off", LV_SYMBOL_POWER "  إطفاء" },
    { "Cancel", "إلغاء" },
    { "Press the %s button to turn it back on. To just turn the screen off, press the %s button.",
      "اضغط الزر %s لتشغيله مرة أخرى. لإطفاء الشاشة فقط، اضغط الزر %s." },
};

/* Captions built around a number or a name, by how they start. */
static const struct {
    const char *en, *ar;
} STARTS[] = {
    { "LISTENING ", "أستمع " },
    { "RECORDING ", "أسجل " },
    { "PHONE SETUP: ", "إعداد الهاتف: " },
    { "MUSE REPLY ERROR (HTTP ", "خطأ في رد ميوز (HTTP " },
};

bool muse_i18n_arabic(void)
{
    return s_arabic;
}

void muse_i18n_set_arabic(bool on)
{
    s_arabic = on;
}

void muse_i18n_init(void)
{
    if (s_arabic) {
        /* LVGL's bidi is a short take on Unicode's: a neutral between a number
         * and Arabic takes the Arabic's direction, so "70%" showed as "%70" and
         * "3:30" as "30:3". Separators and signs that sit inside numbers,
         * addresses and file names ("%.,:/-") aren't neutral here, which keeps
         * them with what they're in. Spaces, quotes and brackets still follow
         * the text around them. */
        lv_bidi_set_custom_neutrals_static(" \t\n\r\"'`!?=()[]{}<>@#&$|;");
    }
}

const char *muse_tr(const char *en)
{
    if (!s_arabic || !en || !en[0]) {
        return en;
    }
    for (size_t i = 0; i < sizeof(WORDS) / sizeof(WORDS[0]); i++) {
        if (!strcmp(en, WORDS[i].en)) {
            return WORDS[i].ar;
        }
    }
    /* One caption at a time, so one buffer does. */
    static char started[MUSE_CAPTION_MAX];
    for (size_t i = 0; i < sizeof(STARTS) / sizeof(STARTS[0]); i++) {
        size_t n = strlen(STARTS[i].en);
        if (!strncmp(en, STARTS[i].en, n)) {
            snprintf(started, sizeof(started), "%s%s", STARTS[i].ar, en + n);
            return started;
        }
    }
    return en;
}

/*
 * Each UI font paired with an Arabic one at about the same visual size. In
 * front of Montserrat (settings) the Arabic font only fills in Arabic.
 * unscii's pixel letters (Muse's face, its replies) would sit oddly beside
 * smooth Arabic, so there the Arabic font, which has Noto Sans's ASCII too,
 * goes in front, and unscii only fills in what it lacks.
 */
static struct {
    const lv_font_t *ui, *arabic;
    bool arabic_first;
    lv_font_t both;
} s_fonts[] = {
#if LV_FONT_UNSCII_8
    { .ui = &lv_font_unscii_8, .arabic = &muse_font_ar_12, .arabic_first = true },
#endif
#if LV_FONT_UNSCII_16
    { .ui = &lv_font_unscii_16, .arabic = &muse_font_ar_16, .arabic_first = true },
#endif
#if LV_FONT_MONTSERRAT_12
    { .ui = &lv_font_montserrat_12, .arabic = &muse_font_ar_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { .ui = &lv_font_montserrat_14, .arabic = &muse_font_ar_16 },
#endif
#if LV_FONT_MONTSERRAT_16
    { .ui = &lv_font_montserrat_16, .arabic = &muse_font_ar_16 },
#endif
#if LV_FONT_MONTSERRAT_20
    { .ui = &lv_font_montserrat_20, .arabic = &muse_font_ar_20 },
#endif
#if LV_FONT_MONTSERRAT_28
    { .ui = &lv_font_montserrat_28, .arabic = &muse_font_ar_28 },
#endif
};

const lv_font_t *muse_font(const lv_font_t *font)
{
    if (!s_arabic) {
        return font;
    }
    for (size_t i = 0; i < sizeof(s_fonts) / sizeof(s_fonts[0]); i++) {
        if (s_fonts[i].ui == font) {
            if (!s_fonts[i].both.get_glyph_dsc) {
                /* Room above and below the baseline for the taller of the two. */
                const lv_font_t *ar = s_fonts[i].arabic;
                int32_t above = LV_MAX(font->line_height - font->base_line, ar->line_height - ar->base_line);
                int32_t below = LV_MAX(font->base_line, ar->base_line);
                s_fonts[i].both = s_fonts[i].arabic_first ? *ar : *font;
                s_fonts[i].both.fallback = s_fonts[i].arabic_first ? font : ar;
                s_fonts[i].both.line_height = above + below;
                s_fonts[i].both.base_line = below;
            }
            return &s_fonts[i].both;
        }
    }
    return font;
}

#define LRM "\xE2\x80\x8E"   /* U+200E LEFT-TO-RIGHT MARK */
#define MARKED_MAX 768

/* Whether `text` has a letter LVGL runs right to left and joins: Arabic,
 * U+0600-06FF, or its presentation forms (not Hebrew's, nor the BOM). */
static bool has_arabic(const char *text)
{
    for (const unsigned char *p = (const unsigned char *)text; *p;) {
        uint32_t cp = *p;
        int n = cp >= 0xF0 ? 4 : cp >= 0xE0 ? 3 : cp >= 0xC0 ? 2 : 1;
        cp &= n == 1 ? 0x7F : 0x3F >> (n - 1);
        for (int i = 1; i < n; i++) {
            if ((p[i] & 0xC0) != 0x80) {
                n = i;   /* broken: skip what's there */
                break;
            }
            cp = cp << 6 | (p[i] & 0x3F);
        }
        p += n;
        if ((cp >= 0x0600 && cp <= 0x06FF) || (cp >= 0xFB50 && cp <= 0xFDFF) || (cp >= 0xFE70 && cp <= 0xFEFE)) {
            return true;
        }
    }
    return false;
}

/* Part of a number, with the digits: "3:30", "72%", "41.5", "-15". */
static bool in_number(char c)
{
    return (c >= '0' && c <= '9') || strchr(".,:%/-+", c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/*
 * LVGL's bidi reads digits at the start of a right-to-left line wrongly: up to
 * the first letter they're reversed with what follows, so a line starting
 * "3:30" showed ":303". A left-to-right mark before each number makes the
 * number a left-to-right run like any English word. The mark draws nothing.
 */
static const char *mark_numbers(const char *text)
{
    static char marked[MARKED_MAX];
    if (!s_arabic || !has_arabic(text)) {
        return text;
    }
    size_t o = 0;
    for (const char *p = text; *p; p++) {
        bool starts = *p >= '0' && *p <= '9' && (p == text || !in_number(p[-1]));
        if (o + (starts ? 3 : 0) + 2 > sizeof(marked)) {
            return text;   /* too long to mark: shown as it is */
        }
        if (starts) {
            memcpy(marked + o, LRM, 3);
            o += 3;
        }
        marked[o++] = *p;
    }
    marked[o] = '\0';
    return marked;
}

void muse_label_set(lv_obj_t *l, const char *text)
{
    if (s_arabic) {
        /* Arabic runs right to left even when a number comes first; anything
         * else goes by its first letter. */
        bool rtl = has_arabic(text) && lv_bidi_detect_base_dir(text) == LV_BASE_DIR_RTL;
        lv_obj_set_style_base_dir(l, rtl ? LV_BASE_DIR_RTL : LV_BASE_DIR_AUTO, 0);
        text = mark_numbers(text);
    }
    lv_label_set_text(l, text);
}

const char *muse_text_joined(const char *text)
{
    static char joined[MARKED_MAX];
    if (!s_arabic || !has_arabic(text) || lv_text_ap_calc_bytes_count(text) > sizeof(joined)) {
        return text;
    }
    lv_text_ap_proc(text, joined);
    return joined;
}

bool muse_label_shows(lv_obj_t *l, const char *text)
{
    const char *shown = lv_label_get_text(l);
    if (!has_arabic(text)) {
        return !strcmp(shown, text);
    }
    /* Joined even in English (an Arabic network name): LVGL joins it anyway. */
    text = mark_numbers(text);
    static char joined[MARKED_MAX * 2];
    if (lv_text_ap_calc_bytes_count(text) > sizeof(joined)) {
        return false;   /* too long to check: set it again */
    }
    lv_text_ap_proc(text, joined);
    return !strcmp(shown, joined);
}

lv_text_align_t muse_text_align(lv_obj_t *l, const char *text, int width, lv_text_align_t en_align)
{
    if (!s_arabic || !text || lv_bidi_detect_base_dir(text) != LV_BASE_DIR_RTL) {
        return en_align;
    }
    if (width <= 0) {
        return LV_TEXT_ALIGN_CENTER;
    }
    /* Wider than the label once on one line, as it was before it was split
     * into lines: it wraps, so it reads from the right. */
    static char joined[MARKED_MAX];
    if (strlcpy(joined, text, sizeof(joined)) >= sizeof(joined)) {
        return LV_TEXT_ALIGN_RIGHT;
    }
    for (char *p = joined; *p; p++) {
        if (*p == '\n') {
            *p = ' ';
        }
    }
    lv_point_t size;
    lv_text_get_size(&size, joined, lv_obj_get_style_text_font(l, LV_PART_MAIN),
                     lv_obj_get_style_text_letter_space(l, LV_PART_MAIN), 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
    return size.x > width ? LV_TEXT_ALIGN_RIGHT : LV_TEXT_ALIGN_CENTER;
}

#endif /* CONFIG_MUSE_LANG_ARABIC */
