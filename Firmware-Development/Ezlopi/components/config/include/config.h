/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* LED Controller for a matrix of smart LEDs */
/* Copyright (C) 2018-2021 Symonics GmbH, Christian Hoene */

/*
 * config.h
 *
 *  Created on: 25.08.2018
 *      Author: hoene
 */

#ifndef MAIN_CONFIG_H_
#define MAIN_CONFIG_H_

#include "led.h"
#include <stdint.h>

#define REVERSE (uint8_t)0x80

typedef uint8_t byte;

void config_init();
void config_reset_to_defaults();
void config_write_all();

void config_coloring_reload();
void config_coloring_defaults();
void config_coloring_write();

void config_update_channels();
void config_setMode(uint8_t mode);
void config_setColor(uint32_t* client_color);
uint32_t *config_read_color(uint8_t id);
void config_set_single_color(uint32_t color_received, uint8_t pri_sec_ter);
void config_set_segment_brightness(uint8_t brightness, uint8_t segment_index);
void config_set_brightness(uint8_t bri);
void config_set_selected_flag(int id, bool select_flag);
void config_set_segment_start_stop(int seg_id, int seg_start, int seg_stop);
void config_set_effect_intensity(uint8_t intensity);
void config_set_effect_speed(uint8_t speed);
void config_set_reverse_option(int id, bool rev_flag);
void config_set_option(int id, bool flag, uint8_t bit_mask);
void config_delete_segment(int id);
void config_apply_palette(uint8_t palette);
void config_switch(bool flag);

char* json_update_config();

const struct LED_CONFIG* config_return_config_pointer();
// const char *config_get_wifi_sta_ssid();
// const char *config_get_wifi_sta_password();
// const char *config_get_wifi_ap_ssid();
// const char *config_get_wifi_ap_password();

// int config_get_udp_port();
// const char *config_get_udp_server();

void config_get_led_line(int line, enum LED_MODE *mode, int *size_x,
		int *size_y, int *offset_x, int *offset_y,
		enum LED_ORIENTATION *orientation, char options[65]);

uint8_t config_get_prefix_leds();
int16_t config_get_refresh_rate();

// void config_set_wifi_sta_ssid(const char *);
// void config_set_wifi_ap_ssid(const char *);
// void config_set_wifi_sta_password(const char *);
// void config_set_wifi_ap_password(const char *);
// void config_set_udp_port(uint16_t port);
// void config_set_udp_server(const char *);

void config_set_led_line(int line, enum LED_MODE mode, int size_x, int size_y,
		int offset_x, int offset_y, enum LED_ORIENTATION orientation,
		const char *string);

void config_set_prefix_leds(uint8_t);
void config_set_refresh_rate(int16_t);

void config_set_artnet_width(uint16_t);
void config_set_artnet_universe_offset(uint16_t);
uint16_t config_get_artnet_width();
uint16_t config_get_artnet_universe_offset();
uint8_t config_get_channels();
void config_set_channels(uint8_t seg_id);
void config_reduce_segment();
void config_set_group(int id, int set_number);
void config_set_segment_on_status(int id, bool on_status);
void config_set_segment_name(int id, char* name);

// const char *config_get_compile_date();
// const char *config_get_version();

void config_set_partition_name(const char *partitionName);


#endif /* MAIN_CONFIG_H_ */
