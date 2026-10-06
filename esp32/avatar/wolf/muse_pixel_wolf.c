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

/*
 * The wolf avatar (CONFIG_MUSE_AVATAR_WOLF), in place of avatar/muse_pixel.c.
 *
 * It draws the wolf's own frames (wolf_sprites.c, from the GIFs in source/)
 * and moves them for each state: the head is the logo while Muse boots, then
 * the wolf hops in. Idle, it breathes and blinks; listening, its glow follows
 * your voice; thinking, it sways under thought dots; speaking, its mouth
 * opens with the reply; on an error it shakes beside a "!"; petted, it hops
 * with hearts; powering off, it shuts its eyes and dims.
 */

#include "muse_pixel.h"

#include <math.h>
#include <string.h>

#include "wolf_sprites.h"

#define W MUSE_PX_W
#define H MUSE_PX_H
#define TAU 6.2831853f

_Static_assert(WOLF_GRID == W && WOLF_GRID == H, "the wolf's frames are the avatar's grid");

/* After the frames' own colours (0 is the background): the effects'. */
enum {
    C_BG = 0,
    C_AURA1 = WOLF_COLOURS,
    C_AURA2,
    C_ACC,
    C_G0,
    C_SPK,
    C_SHADOW,
    C_HEART,
    C_WHITE,
    C_MOUTH,
    C_TONGUE,
    C_COUNT,
};

/* Where the standing wolf's face is on the grid (from its frame). */
#define MOUTH_X 27
#define MOUTH_Y 34
#define HEAD_TOP 10

typedef struct {
    float r, g, b;
} rgb_t;

/* The UI's colours for each state, as the default avatar has them. */
typedef struct {
    uint32_t glow, acc;
} scheme_t;

static const scheme_t SCHEMES[MUSE_MODE_COUNT] = {
    [MUSE_MODE_BOOT]      = { 0xcfe0ff, 0xa9c0ff },
    [MUSE_MODE_IDLE]      = { 0xc7a4ff, 0xa77dff },
    [MUSE_MODE_LISTENING] = { 0x8fdcff, 0x5cb8ff },
    [MUSE_MODE_THINKING]  = { 0xff9cf0, 0xe07bff },
    [MUSE_MODE_SPEAKING]  = { 0x9ff5cf, 0x6ff0bf },
    [MUSE_MODE_ERROR]     = { 0xff6b6b, 0xff5c5c },
    [MUSE_MODE_OFF]       = { 0x8f86d9, 0x7c72d0 },
};

static uint8_t s_fb[W * H];
static uint16_t s_pal[C_COUNT];
static uint16_t s_pal_dim[C_COUNT];
static rgb_t s_glow, s_acc;
static bool s_scheme_init;

static const uint8_t BAYER4[4][4] = {
    { 0, 8, 2, 10 },
    { 12, 4, 14, 6 },
    { 3, 11, 1, 9 },
    { 15, 7, 13, 5 },
};

static inline float bayer(int x, int y)
{
    return (BAYER4[y & 3][x & 3] + 0.5f) / 16.0f;
}

static inline rgb_t hex_rgb(uint32_t c)
{
    return (rgb_t){ (c >> 16 & 0xff) / 255.0f, (c >> 8 & 0xff) / 255.0f, (c & 0xff) / 255.0f };
}

static inline rgb_t mix(rgb_t a, rgb_t b, float t)
{
    return (rgb_t){ a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}

static inline rgb_t scale_rgb(rgb_t a, float k)
{
    return (rgb_t){ a.r * k, a.g * k, a.b * k };
}

static inline uint16_t to565(rgb_t c)
{
    int r = (int)(c.r * 31.0f + 0.5f), g = (int)(c.g * 63.0f + 0.5f), b = (int)(c.b * 31.0f + 0.5f);
    r = r < 0 ? 0 : r > 31 ? 31 : r;
    g = g < 0 ? 0 : g > 63 ? 63 : g;
    b = b < 0 ? 0 : b > 31 ? 31 : b;
    return (uint16_t)(r << 11 | g << 5 | b);
}

static inline float clampf(float v, float lo, float hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline float fracf(float v)
{
    return v - floorf(v);
}

static inline int iround(float v)
{
    return (int)floorf(v + 0.5f);
}

static uint32_t s_rng = 0x9e3779b9u;
static float frand(void)
{
    s_rng ^= s_rng << 13;
    s_rng ^= s_rng >> 17;
    s_rng ^= s_rng << 5;
    return (float)(s_rng & 0xffffff) / (float)0x1000000;
}

uint32_t muse_pixel_accent(muse_mode_t mode)
{
    return SCHEMES[mode < MUSE_MODE_COUNT ? mode : MUSE_MODE_IDLE].acc;
}

/*
 * The state's colours ease in. `tint` reddens the wolf (an error), `light`
 * dims everything (powering off).
 */
static void update_palette(muse_mode_t mode, float dt, float tint, float light)
{
    rgb_t glow = hex_rgb(SCHEMES[mode].glow), acc = hex_rgb(SCHEMES[mode].acc);
    float k = s_scheme_init ? 1.0f - expf(-dt * 7.0f) : 1.0f;
    s_glow = mix(s_glow, glow, k);
    s_acc = mix(s_acc, acc, k);
    s_scheme_init = true;

    rgb_t pal[C_COUNT];
    rgb_t red = hex_rgb(0xff4a5a);
    for (int i = 0; i < WOLF_COLOURS; i++) {
        pal[i] = mix(hex_rgb(wolf_palette[i]), red, i ? tint : 0);
    }
    pal[C_AURA1] = scale_rgb(s_acc, 0.2f);
    pal[C_AURA2] = scale_rgb(s_acc, 0.4f);
    pal[C_ACC] = s_acc;
    pal[C_G0] = s_glow;
    pal[C_SPK] = mix(s_acc, hex_rgb(0xffffff), 0.45f);
    pal[C_SHADOW] = hex_rgb(0x16101f);
    pal[C_HEART] = hex_rgb(0xff4f8b);
    pal[C_WHITE] = hex_rgb(0xffffff);
    pal[C_MOUTH] = hex_rgb(0x2a1820);
    pal[C_TONGUE] = hex_rgb(0xe86a7a);
    for (int i = 0; i < C_COUNT; i++) {
        rgb_t c = scale_rgb(pal[i], light);
        s_pal[i] = to565(c);
        /* The block edge shade gives the enlarged pixels a faint grid texture. */
        s_pal_dim[i] = to565(scale_rgb(c, 0.72f));
    }
}

/* ---------------------------------------------------------------------------
 * Drawing
 * ------------------------------------------------------------------------- */

static inline void px(int x, int y, uint8_t c)
{
    if ((unsigned)x < W && (unsigned)y < H) {
        s_fb[y * W + x] = c;
    }
}

static inline uint8_t get_px(int x, int y)
{
    return (unsigned)x < W && (unsigned)y < H ? s_fb[y * W + x] : C_BG;
}

static void stamp(const char *const *rows, int nrows, int x0, int y0, uint8_t fill, uint8_t alt)
{
    for (int r = 0; r < nrows; r++) {
        for (int c = 0; rows[r][c]; c++) {
            if (rows[r][c] == '#') {
                px(x0 + c, y0 + r, fill);
            } else if (rows[r][c] == 'o') {
                px(x0 + c, y0 + r, alt);
            }
        }
    }
}

/*
 * One of the wolf's frames, moved by (dx, dy) and scaled by `scale` about the
 * grid's centre (nearest neighbour, so the pixels stay pixels).
 */
static void draw_frame(int frame, int dx, int dy, float scale)
{
    const uint8_t *f = wolf_frames[frame];
    if (scale > 0.99f && scale < 1.01f) {
        for (int y = 0; y < H; y++) {
            for (int x = 0; x < W; x++) {
                uint8_t c = f[y * W + x];
                if (c) {
                    px(x + dx, y + dy, c);
                }
            }
        }
        return;
    }
    if (scale < 0.05f) {
        return;
    }
    float inv = 1.0f / scale;
    for (int y = 0; y < H; y++) {
        int sy = (int)floorf((y - dy - H / 2 + 0.5f) * inv + H / 2);
        if ((unsigned)sy >= H) {
            continue;
        }
        for (int x = 0; x < W; x++) {
            int sx = (int)floorf((x - dx - W / 2 + 0.5f) * inv + W / 2);
            if ((unsigned)sx < W && f[sy * W + sx]) {
                px(x, y, f[sy * W + sx]);
            }
        }
    }
}

/* A dithered glow behind the wolf, in the state's colour. */
static void draw_aura(float cx, float cy, float radius, float strength)
{
    for (int y = (int)(cy - radius); y <= (int)(cy + radius); y++) {
        for (int x = (int)(cx - radius); x <= (int)(cx + radius); x++) {
            float dx = x + 0.5f - cx, dy = (y + 0.5f - cy) * 1.1f;
            float d = sqrtf(dx * dx + dy * dy) / radius;
            if (d >= 1.0f) {
                continue;
            }
            float i = (1.0f - d) * strength;
            float b = bayer(x, y);
            if (i > 0.55f + b * 0.35f) {
                px(x, y, C_AURA2);
            } else if (i > b * 0.9f) {
                px(x, y, C_AURA1);
            }
        }
    }
}

/* Dotted rings spreading out (listening, speaking), behind the wolf. */
static void draw_rings(float cx, float cy, float t, float level, float speed)
{
    for (int k = 0; k < 2; k++) {
        float ph = fracf(t * speed + k * 0.5f);
        float r = 22 + ph * 9;
        float fade = (1 - ph) * (0.35f + level);
        int n = (int)(r * 2.2f);
        for (int i = 0; i < n; i++) {
            float a = i * TAU / n;
            int x = iround(cx + cosf(a) * r), y = iround(cy + sinf(a) * r * 0.92f);
            uint8_t under = get_px(x, y);
            if ((under == C_BG || under == C_AURA1) && bayer(x, y) < fade) {
                px(x, y, fade > 0.6f ? C_ACC : C_AURA2);
            }
        }
    }
}

static void draw_shadow(float cx, int y, float half_w)
{
    for (int row = 0; row < 2; row++) {
        float hw = half_w * (row ? 0.72f : 1.0f);
        for (int x = iround(cx - hw); x <= iround(cx + hw); x++) {
            float edge = fabsf(x + 0.5f - cx) / hw;
            if (bayer(x, y + row) > edge * 0.8f) {
                px(x, y + row, C_SHADOW);
            }
        }
    }
}

/* A few fixed points around the wolf that twinkle in turn. */
static void draw_sparkles(float t, float speed, int count)
{
    static const int8_t AT[8][2] = { { 6, 14 }, { 56, 18 }, { 9, 40 }, { 58, 44 }, { 14, 6 }, { 50, 6 }, { 4, 28 }, { 60, 30 } };
    for (int i = 0; i < count && i < 8; i++) {
        float ph = fracf(t * speed * 0.5f + i * 0.37f);
        if (ph > 0.5f) {
            continue;
        }
        int x = AT[i][0], y = AT[i][1];
        uint8_t c = ph < 0.25f ? C_SPK : C_ACC;
        if (get_px(x, y) == C_BG || get_px(x, y) == C_AURA1) {
            px(x, y, C_WHITE);
            px(x - 1, y, c);
            px(x + 1, y, c);
            px(x, y - 1, c);
            px(x, y + 1, c);
        }
    }
}

static void draw_thought_dots(int x, int y, float t)
{
    int active = (int)(fracf(t * 1.6f) * 3.0f);
    for (int i = 0; i < 3; i++) {
        int bx = x + i * 4, by = y - i * 3 - (i == active ? 1 : 0);
        uint8_t c = i == active ? C_G0 : C_ACC;
        px(bx, by, c);
        px(bx + 1, by, c);
        px(bx, by + 1, c);
        px(bx + 1, by + 1, c);
    }
}

static void draw_hearts(float t, float amount)
{
    static const char *const HEART[] = { ".#.#.", "#o###", "#####", ".###.", "..#.." };
    for (int i = 0; i < 2; i++) {
        float ph = fracf(t * 0.9f + i * 0.5f);
        if (ph > amount) {
            continue;
        }
        int hx = iround((i ? 47.0f : 10.0f) + sinf(ph * TAU + i) * 2);
        int hy = iround(18 - ph * 12);
        stamp(HEART, 5, hx, hy, C_HEART, C_WHITE);
    }
}

static void draw_alert(int x, int y)
{
    static const char *const BANG[] = { "##", "##", "##", "##", "..", "##" };
    stamp(BANG, 6, x, y, C_ACC, C_ACC);
}

/* "z"s drifting up while it falls asleep. */
static void draw_sleep(float mode_t)
{
    static const char *const Z[] = { "###", "..#", ".#.", "#..", "###" };
    for (int i = 0; i < 2; i++) {
        float ph = fracf(mode_t * 0.45f + i * 0.5f);
        if (mode_t < 0.6f + i * 0.8f) {
            continue;
        }
        stamp(Z, 5, 42 + i * 5 + iround(sinf(ph * TAU) * 1.5f), iround(14 - ph * 10), C_ACC, C_ACC);
    }
}

/* The mouth, open by `open` (0..1), over the standing wolf's smile. */
static void draw_mouth(int dx, int dy, float open)
{
    int rows = open < 0.15f ? 0 : open < 0.5f ? 1 : open < 0.85f ? 2 : 3;
    for (int r = 0; r < rows; r++) {
        int half = r == rows - 1 && rows > 1 ? 1 : 2;
        for (int x = -half; x < half; x++) {
            px(MOUTH_X + 1 + x + dx, MOUTH_Y + r + dy, r == rows - 1 && rows > 2 ? C_TONGUE : C_MOUTH);
        }
    }
}

/* ---------------------------------------------------------------------------
 * Blinking
 * ------------------------------------------------------------------------- */

static float s_next_blink = 2.0f;
static float s_blink_until;

/* Whether the eyes are shut now: every few seconds, now and then twice. */
static bool blinking(float t)
{
    if (t >= s_next_blink) {
        s_blink_until = t + 0.14f;
        s_next_blink = t + (frand() < 0.2f ? 0.3f : 2.5f + frand() * 3.5f);
    }
    return t < s_blink_until;
}

/* ---------------------------------------------------------------------------
 * Frame
 * ------------------------------------------------------------------------- */

#define MAP_MAX 512
static uint8_t s_map[MAP_MAX];
static int s_size;

void muse_pixel_set_size(int px)
{
    s_size = px < MAP_MAX ? px : MAP_MAX;
    bool grid = s_size >= 3 * W;
    for (int i = 0; i < s_size; i++) {
        int cell = i * W / s_size;
        bool edge = grid && (i + 1) * W / s_size != cell;
        s_map[i] = (uint8_t)(cell | (edge ? 0x80 : 0));
    }
}

void muse_pixel_scale(uint16_t *dst, int stride_px, int x0, int x1, int y0, int y1)
{
    int n = x1 - x0 + 1;
    const uint8_t *xmap = &s_map[x0];
    const uint16_t *prev = NULL;
    uint8_t prev_m = 0;
    for (int y = y0; y <= y1; y++, dst += stride_px) {
        uint8_t m = s_map[y];
        if (prev && m == prev_m) {
            memcpy(dst, prev, n * sizeof(uint16_t));
            continue;
        }
        const uint8_t *row = &s_fb[(m & 0x7f) * W];
        if (m & 0x80) {
            for (int i = 0; i < n; i++) {
                dst[i] = s_pal_dim[row[xmap[i] & 0x7f]];
            }
        } else {
            for (int i = 0; i < n; i++) {
                uint8_t xm = xmap[i];
                uint8_t c = row[xm & 0x7f];
                dst[i] = xm & 0x80 ? s_pal_dim[c] : s_pal[c];
            }
        }
        prev = dst;
        prev_m = m;
    }
}

/* The logo while Muse boots: the head grows in with its eyes shut, opens them,
 * and every few seconds tilts its head. */
static void render_boot(float t, float mode_t)
{
    float pop = clampf(mode_t / 0.5f, 0, 1);
    float scale = pop < 1 ? pop * 1.08f : 1.0f + sinf(clampf((mode_t - 0.5f) / 0.25f, 0, 1) * 3.1416f) * 0.05f;
    draw_aura(32, 32, 30.0f + sinf(t * 1.5f), 0.8f * pop);
    draw_sparkles(t, 1.0f, (int)(pop * 6));
    int frame = WOLF_HEAD;
    if (mode_t < 0.9f || (mode_t > 1.0f && blinking(t))) {
        frame = WOLF_HEAD_BLINK;
    } else if (mode_t > 1.6f && fracf((mode_t - 1.6f) / 3.5f) < 0.18f) {
        frame = WOLF_HEAD_TILT;
    }
    draw_frame(frame, 0, iround(sinf(t * 1.8f) * 0.6f), scale);
}

void muse_pixel_render(const muse_pose_t *p)
{
    static float last_t = -1;
    float dt = last_t >= 0 ? clampf(p->t - last_t, 0, 0.2f) : 0.04f;
    last_t = p->t;

    muse_mode_t mode = p->mode < MUSE_MODE_COUNT ? p->mode : MUSE_MODE_IDLE;
    float t = p->t, mt = p->mode_t, level = p->level, happy = p->happy;
    float off = mode == MUSE_MODE_OFF ? clampf(mt / 1.3f, 0, 1) : 0.0f;
    float tint = mode == MUSE_MODE_ERROR ? 0.12f : 0.0f;
    update_palette(mode, dt, tint, 1.0f - off * 0.55f);

    memset(s_fb, C_BG, sizeof(s_fb));
    if (mode == MUSE_MODE_BOOT) {
        render_boot(t, mt);
        return;
    }

    /* ---- how it moves ---- */
    float bob = 0, sway = 0, scale = 1.0f, hop = 0;
    switch (mode) {
    case MUSE_MODE_LISTENING:
        bob = sinf(t * 3.0f) * 0.7f;
        break;
    case MUSE_MODE_THINKING:
        bob = sinf(t * 2.4f) * 0.6f;
        sway = sinf(t * 1.3f) * 1.4f;
        break;
    case MUSE_MODE_SPEAKING:
        bob = sinf(t * 5.0f) * 0.5f - level * 1.2f;
        break;
    case MUSE_MODE_ERROR:
        sway = mt < 0.6f ? sinf(t * 40.0f) * 1.5f : 0.0f;
        break;
    case MUSE_MODE_OFF:
        bob = off * 1.0f;
        break;
    default:
        bob = sinf(t * 1.8f) * 0.8f;
        break;
    }
    /* Coming out of boot (or back from off), the wolf hops in. */
    bool arriving = mode == MUSE_MODE_IDLE && mt < 0.5f;
    if (arriving) {
        scale = 0.7f + 0.3f * clampf(mt / 0.35f, 0, 1);
        hop = sinf(clampf(mt / 0.5f, 0, 1) * 3.1416f) * 4.0f;
    }
    if (happy > 0) {
        hop = fabsf(sinf(t * 9.0f)) * 3.0f * happy;
    }
    int dx = iround(sway), dy = iround(bob - hop);

    int frame = WOLF_BODY;
    if (happy > 0.15f || arriving || (mode == MUSE_MODE_IDLE && hop > 1.5f)) {
        frame = WOLF_BODY_HOP;
    } else if (mode == MUSE_MODE_OFF ? mt > 0.4f : (mode != MUSE_MODE_LISTENING && blinking(t))) {
        frame = WOLF_BODY_BLINK;
    }

    /* ---- behind ---- */
    float aura = (0.7f + level * 0.5f) * (1.0f - off);
    if (mode == MUSE_MODE_LISTENING) {
        aura += 0.15f + sinf(t * 4.0f) * 0.1f;
    }
    draw_aura(32 + dx, 34, 28.0f + level * 4.0f + sinf(t * 1.5f), aura);
    if (mode == MUSE_MODE_LISTENING || mode == MUSE_MODE_SPEAKING) {
        draw_rings(32, 32, t, level, mode == MUSE_MODE_LISTENING ? 0.9f : 0.6f);
    }
    draw_shadow(32 + dx, WOLF_FLOOR, 13.0f - hop);
    float spk_speed = mode == MUSE_MODE_THINKING ? 2.8f : mode == MUSE_MODE_LISTENING ? 1.4f : 0.7f;
    draw_sparkles(t, spk_speed, (int)(6 * (1.0f - off)));

    /* ---- the wolf ---- */
    draw_frame(frame, dx, dy, scale);
    if (mode == MUSE_MODE_SPEAKING && frame == WOLF_BODY) {
        draw_mouth(dx, dy, level * 1.3f + 0.15f * (0.5f + 0.5f * sinf(t * 22.0f)));
    }

    /* ---- in front ---- */
    if (mode == MUSE_MODE_THINKING) {
        draw_thought_dots(48, HEAD_TOP + 2, t);
    }
    if (mode == MUSE_MODE_ERROR) {
        draw_alert(52, HEAD_TOP - 2);
    }
    if (mode == MUSE_MODE_OFF) {
        draw_sleep(mt);
    }
    if (happy > 0) {
        draw_hearts(t, happy);
    }
}
