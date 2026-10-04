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
 * What the settings pages (muse_settings_ui.c) read and change, kept in memory
 * so the pages can be previewed: a few networks to scan, one saved, a battery
 * run under way. Nothing is stored and no radio is touched.
 */
#include <stdio.h>
#include <string.h>

#include "muse_audio.h"
#include "muse_battery.h"
#include "muse_ble.h"
#include "muse_chat.h"
#include "muse_input.h"
#include "muse_link.h"
#include "muse_settings.h"
#include "muse_voice.h"
#include "muse_wifi.h"

static const muse_wifi_ap_t SIM_APS[] = {
    { "Muse Simulator", -45, true },
    { "شبكة البيت", -58, true },
    { "Cafe Guest 5G", -71, false },
};
static const muse_wifi_saved_t SIM_SAVED[] = {
    { "Muse Simulator", false },
    { "شبكة البيت", false },
};

static bool s_wifi_on = true;
static bool s_ble_on = true;
static int s_volume = 70;
static int s_mic_gain = 24;
static int s_sleep_s = 60;
static char s_host[MUSE_HOST_MAX + 1];
static char s_vm[MUSE_VM_MAX + 1];

bool muse_settings_wifi_on(void) { return s_wifi_on; }
void muse_settings_set_wifi_on(bool on) { s_wifi_on = on; }
void muse_settings_set_wifi(const char *ssid, const char *pass) { (void)ssid; (void)pass; }
bool muse_settings_ble_on(void) { return s_ble_on; }
void muse_settings_set_ble_on(bool on) { s_ble_on = on; }
int muse_settings_volume(void) { return s_volume; }
void muse_settings_set_volume(int pct) { s_volume = pct; }
int muse_settings_mic_gain(void) { return s_mic_gain; }
void muse_settings_set_mic_gain(int db) { s_mic_gain = db; }
int muse_settings_sleep_s(void) { return s_sleep_s; }
void muse_settings_set_sleep_s(int secs) { s_sleep_s = secs; }

void muse_settings_hatch_host(char out[MUSE_HOST_MAX + 1]) { snprintf(out, MUSE_HOST_MAX + 1, "%s", s_host); }
void muse_settings_set_hatch_host(const char *host) { snprintf(s_host, sizeof(s_host), "%s", host); }
void muse_settings_hatch_vm(char out[MUSE_VM_MAX + 1]) { snprintf(out, MUSE_VM_MAX + 1, "%s", s_vm); }
void muse_settings_set_hatch_vm(const char *vm) { snprintf(s_vm, sizeof(s_vm), "%s", vm); }
size_t muse_settings_hatch_token_len(void) { return 0; }

esp_err_t muse_settings_set_hatch_token(const char *token, bool append)
{
    (void)token;
    (void)append;
    return ESP_OK;
}

void muse_audio_set_volume(int volume) { (void)volume; }
void muse_audio_set_mic_gain(int db) { (void)db; }
float muse_voice_monitor_db(void) { return -24.0f; }
void muse_voice_request_chirp(void) {}
void muse_voice_set_monitor(bool on) { (void)on; }

esp_err_t muse_wifi_scan(void) { return ESP_OK; }
bool muse_wifi_scanning(void) { return false; }
void muse_wifi_apply(void) {}
void muse_wifi_forget(const char *ssid) { (void)ssid; }

int muse_wifi_scan_results(muse_wifi_ap_t *out, int max, uint32_t *gen)
{
    int n = 0;
    for (; n < max && n < (int)(sizeof(SIM_APS) / sizeof(SIM_APS[0])); n++) {
        out[n] = SIM_APS[n];
    }
    *gen = 1;
    return n;
}

int muse_wifi_saved(muse_wifi_saved_t *out, int max)
{
    int n = 0;
    for (; n < max && n < (int)(sizeof(SIM_SAVED) / sizeof(SIM_SAVED[0])); n++) {
        out[n] = SIM_SAVED[n];
    }
    return n;
}

void muse_ble_forget_all(void) {}
void muse_hatch_test(void) {}
void muse_input_request_power_off(void) {}
bool muse_link_hatch_linked(void) { return true; }
void muse_link_reset_setup(void) {}

/* As muse_link.c and muse_chat.c name them. */
const char *muse_link_state_name(muse_link_state_t state)
{
    switch (state) {
    case MUSE_LINK_BOOT: return "Starting";
    case MUSE_LINK_UNPAIRED: return "Ready to pair";
    case MUSE_LINK_PAIRING: return "App connected";
    case MUSE_LINK_CONFIRM: return "Confirm pairing";
    case MUSE_LINK_CONNECTING: return "Connecting";
    case MUSE_LINK_ONLINE: return "Online";
    case MUSE_LINK_OFFLINE: return "Offline";
    case MUSE_LINK_ERROR: return "Error";
    }
    return "";
}

const char *muse_hatch_state_name(muse_hatch_state_t state)
{
    switch (state) {
    case MUSE_HATCH_NOT_SET: return "Not set up";
    case MUSE_HATCH_OFFLINE: return "Offline";
    case MUSE_HATCH_UNTESTED: return "Saved";
    case MUSE_HATCH_TESTING: return "Connecting";
    case MUSE_HATCH_REACHABLE: return "Connected";
    case MUSE_HATCH_UNREACHABLE: return "Can't connect";
    }
    return "";
}

/* Two and a half hours on battery, 18% used. */
void muse_battery_read(muse_battery_t *out)
{
    *out = (muse_battery_t){
        .started = true,
        .running = true,
        .secs = 9000,
        .pct_start = 90,
        .pct_now = 72,
        .mv_start = 4100,
        .mv_now = 3970,
        .screen_off_pm = 640,
        .resting_pm = 600,
        .slept_pm = 512,
        .sleeps = 4200,
        .busy_pm = 87,
        .awake = "wifi 4%, bt 1%",
    };
}

bool muse_battery_drain(const muse_battery_t *b, int *rate10, int *full_h)
{
    *rate10 = (int)((b->pct_start - b->pct_now) * 36000LL / b->secs);
    *full_h = *rate10 ? 1000 / *rate10 : 0;
    return true;
}

void muse_battery_reset(void) {}
