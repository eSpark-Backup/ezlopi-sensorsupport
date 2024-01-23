/* SPDX-License-Identifier: AGPL-3.0-or-later */
/* LED Controller for a matrix of smart LEDs */
/* Copyright (C) 2018-2021 Symonics GmbH, Christian Hoene */

/*
 * config.c
 *
 *  Created on: 29.07.2018
 *      Author: hoene
 */

#include "config.h"

#include <string.h>

// #include "playlist.h"
// #include "bonjour.h"
#include "esp_err.h"
#include "esp_log.h"
// #include "esp_ota_ops.h"
#include "esp_system.h"
// #include "mbedtls/base64.h"
// #include "mbedtls/md.h"
#include "nvs_flash.h"
#include "ownled.h"
#include "palette.h"
#include "cJSON.h"

static const char *TAG = "#config";

static char wifi_sta_ssid[32];
static char wifi_sta_password[64];
static char wifi_ap_ssid[32];
static char wifi_ap_password[64];
static uint16_t udp_port;
static char udp_server[32];
static struct LED_CONFIG led_config;
static char partition_name[17] = NVS_DEFAULT_PART_NAME;

struct LED_CONFIG_CHANNEL_V1_1 {
	enum LED_MODE mode;
	enum LED_ORIENTATION orientation;
	int16_t sx;
	int16_t sy;
	int16_t ox;
	int16_t oy;
	int16_t black[3];
};

struct LED_CONFIG_V1_1 {
	int16_t refresh_rate;
	uint8_t channels;
	uint8_t prefix_leds;
	struct LED_CONFIG_CHANNEL_V1_1 channel[8];
	uint16_t artnet_width;
	uint16_t artnet_universe_offset;
};

struct LED_CONFIG_CHANNEL_V1_0 {
	enum LED_MODE mode;
	enum LED_ORIENTATION orientation;
	int sx;
	int sy;
	int ox;
	int oy;
};

struct LED_CONFIG_V1_0 {
	uint8_t refresh_rate;
	uint8_t channels;
	uint8_t prefix_leds;
	struct LED_CONFIG_CHANNEL_V1_0 channel[8];
	uint16_t artnet_width;
	uint16_t artnet_universe_offset;
};

#if 0
static const char secret[] = "LNBFXVIvbiYLDHvt+R0CC1x/6Zqd6Wm0+YgwvoQ/ftoV";

static void getPassword(const char *in, char output[32]) {
	mbedtls_md_context_t ctx;
	uint8_t shaResult[MBEDTLS_MD_MAX_SIZE];

	mbedtls_md_init(&ctx);
	ESP_ERROR_CHECK(
			mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_MD5),
					0));
	ESP_ERROR_CHECK(mbedtls_md_starts(&ctx));
	ESP_ERROR_CHECK(
			mbedtls_md_update(&ctx, (const unsigned char* ) secret,
					strlen(secret)));
	ESP_ERROR_CHECK(
			mbedtls_md_update(&ctx, (const unsigned char* ) in, strlen(in)));
	ESP_ERROR_CHECK(mbedtls_md_finish(&ctx, shaResult));
	mbedtls_md_free(&ctx);

	size_t size;
	ESP_ERROR_CHECK(
			mbedtls_base64_encode((unsigned char* ) output, 32, &size,
					shaResult, 128 / 8));
	output[size - 2] = 0;
}
#endif

// static uint8_t defaultModes[8] = { LED_MODE_WHITE, LED_MODE_GRAY, LED_MODE_RED,
// 		LED_MODE_YELLOW, LED_MODE_GREEN, LED_MODE_CYAN, LED_MODE_BLUE,
// 		LED_MODE_MAGENTA };

// set values to default
static void setDefaults() {
	led_config.channels = 1;
	led_config.prefix_leds = 0;
	led_config.refresh_rate = 20.;
	led_config.total_leds = 300;
	ESP_LOGI(TAG, "CHANNELS ACTIVE : %d", config_get_channels());
	for (int i = 0; i < config_get_channels(); i++) {
	//allocate memory for a segment
	led_config.channel[i] = (struct LED_CONFIG_CHANNEL*)malloc(sizeof(struct LED_CONFIG_CHANNEL));

		// led_config.channel[i]->sx = led_config.channel[i].sy = 6;
		// led_config.channel[i]->ox = led_config.channel[i].oy = 0;

		// led_config.channel[i]->orientation = LED_ORI0_ZIGZAG;

		led_config.channel[i]->selected = true;
		// led_config.channel[0]->selected = true;

		// led_config.channel[i]->mode = LED_MODE_SOLID + i; //:FIXME: Revert back to solid by default
		led_config.channel[i]->mode = FX_MODE_STATIC;
		// led_config.channel[i]->mode = FX_MODE_DNA2D;

		led_config.channel[i]->black[0] = led_config.channel[i]->black[1] =
				led_config.channel[i]->black[2] = -1;

		led_config.channel[i]->start = i*10;
		led_config.channel[i]->stop = led_config.channel[i]->start+10;

		led_config.channel[i]->segment_id = i;
		char *default_segment_name = "main";
		led_config.channel[i]->segment_name = strdup(default_segment_name);

		led_config.channel[i]->number_of_leds = led_config.channel[i]->stop - led_config.channel[i]->start;

		led_config.channel[i]->speed = 1000;
		led_config.channel[i]->intensity = 128;

		led_config.channel[i]->options = 0;

		led_config.channel[i]->color[0] = 0xFFFF00;
		led_config.channel[i]->color[1] = 0x00FF00;
		led_config.channel[i]->color[2] = 0x0000FF;

		led_config.channel[i]->color_buffer[0] = led_config.channel[i]->color[0];
		led_config.channel[i]->color_buffer[1] = led_config.channel[i]->color[1];
		led_config.channel[i]->color_buffer[2] = led_config.channel[i]->color[2];

		led_config.channel[i]->brightness = 0.5;

		led_config.channel[i]->palette = DEFAULT_PALETTE;

		led_config.channel[i]->sx = 10;
		led_config.channel[i]->sy = 10;
		led_config.channel[i]->orientation = LED_ORI0_MEANDER;


	}

	config_coloring_defaults();
	ownled_setColorOrder(OWNLED_GRB);
	// config_set_brightness(50);

	led_config.artnet_universe_offset = 0;
	led_config.artnet_width = 512 / 3;

	ownled_set_default();
	

	// playlist_defaultConfig();
}

// set values to default
void config_setMode(uint8_t mode) {

	// struct LED_COLORING my_led_coloring  = { 
	// 	0, /*contrast*/
	// 	1, /*brightness*/
	// 	0, /*red_contrast*/
	// 	1, /*reg_brightness*/
	// 	1, /*green_contrast*/
	// 	0, /*green_brightness*/
	// 	1, /*blue_contrast*/
	// 	0, /*blue_brightness*/
	// 	1, /*saturation*/
	// 	0  /*hue*/
	// 	};
	
	// led_coloring.brightness = 1;
	// led_coloring.contrast = 1;

	// led_coloring.red_brightness = 1;
	// led_coloring.red_contrast = 1;

	// led_coloring.blue_brightness = 1;
	// led_coloring.blue_contrast = 1;

	// led_coloring.green_brightness = 1 ;
	// led_coloring.green_contrast = 1;

	// led_coloring.saturation = 1 - 0.5;
	// led_coloring.hue = 1 - 0;
	ESP_LOGI(TAG, "SET MODE");

	for (int i = 0; i < config_get_channels(); i++) {
		if(led_config.channel[i] != NULL && led_config.channel[i]->selected){
			ESP_LOGI(TAG, "MODE SET for SEG  :%d with ID :%d", i, led_config.channel[i]->segment_id);
			led_config.channel[i]->mode = mode;
			led_config.channel[i]->sx = led_config.channel[i]->sy = 10;
			led_config.channel[i]->ox = led_config.channel[i]->oy = 0;
			led_config.channel[i]->orientation = LED_ORI0_MEANDER;
			led_config.channel[i]->black[0] = led_config.channel[i]->black[1] =
				led_config.channel[i]->black[2] = -1;
		}		
	}
	ownled_setColorOrder(OWNLED_GRB);
	led_set_config(&led_config);


}

void config_setColor(uint32_t* client_color) {
	for (int i = 0; i < config_get_channels(); i++) {
		if(led_config.channel[i] != NULL && led_config.channel[i]->selected){
			for(int j = 0; j<3; j++){
				ESP_LOGI(TAG, "COLOR RECIEVED : %x", client_color[j]);
				// led_config.config_color[j] = *(client_color+j);
				led_config.channel[i]->color[j] = *(client_color+j);
				led_config.channel[i]->color_buffer[j] = led_config.channel[i]->color[j];
			}
		}
	}
	led_set_config(&led_config);
}

void config_set_single_color(uint32_t color_received, uint8_t pri_sec_ter)
{
	for (int i = 0; i < config_get_channels(); i++) {
		if(led_config.channel[i] != NULL && led_config.channel[i]->selected && pri_sec_ter < 3 && pri_sec_ter > -1){
					led_config.channel[i]->color[pri_sec_ter] = color_received;
					led_config.channel[i]->color_buffer[pri_sec_ter] = led_config.channel[i]->color[pri_sec_ter];
		}
	}
	led_set_config(&led_config);
}

uint32_t *config_read_color(uint8_t id)
{
	return led_config.channel[id]->color;
}

void config_set_segment_brightness(uint8_t brightness, uint8_t segment_index)
{
	float brightness_float = ((float)(brightness)/(255.f));
	
	for (int i = 0; i < config_get_channels(); i++) {
		if(led_config.channel[i] != NULL && led_config.channel[i]->segment_id == segment_index){
				ESP_LOGI(TAG, "BRIGHTNESS SET TO : %f", brightness_float);
				led_config.channel[i]->prev_brightness = led_config.channel[i]->brightness;
				led_config.channel[i]->brightness = brightness_float;
				ESP_LOGI(TAG, "PREVIOUS BRIGHTNESS SET TO : %f", led_config.channel[i]->prev_brightness);
		}
	}
	led_set_config(&led_config);
}

void config_switch(bool flag) //TODO:
{
	uint8_t old_brightness = 0;
	int i;
	for (i = 0; i < config_get_channels(); i++) {
		if(led_config.channel[i] != NULL && led_config.channel[i]->selected){
				old_brightness = led_config.channel[i]->brightness;		
				led_config.channel[i]->brightness = flag ? led_config.channel[i]->brightness : 0;
				break;
		}
	}
	led_set_config(&led_config);
	led_config.channel[i]->brightness = old_brightness;
}

void config_set_brightness(uint8_t bri){
	
	float fBri = 0;
	fBri = (bri/255.f);
	
	led_coloring.brightness = fBri;
	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])){
			led_config.channel[i]->brightness = fBri;
		}
	}
	led_set_config(&led_config);
}

void config_set_selected_flag(int id, bool select_flag){
	// if(led_config.channel[id] != NULL){
	// 	led_config.channel[id]->selected = select_flag;
	// }

	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->segment_id == id)){
			led_config.channel[i]->selected = select_flag;
			break;
		}
	}
	led_set_config(&led_config);

}

void config_set_segment_start_stop(int seg_id, int seg_start, int seg_stop){

	// if(seg_id > (led_config.channels-1) ){led_config.channel[seg_id] = (struct LED_CONFIG_CHANNEL*)calloc(1,sizeof(struct LED_CONFIG_CHANNEL));}
	bool found = false;

	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->segment_id == seg_id)){
			led_config.channel[i]->start = seg_start;
			led_config.channel[i]->stop = seg_stop;
			led_config.channel[i]->number_of_leds = seg_stop-seg_start;
			led_set_config(&led_config);
			found = true;
			break;
		}
	}

	// if(led_config.channel[seg_id] != NULL && led_config.channel[seg_id]->segment_id == seg_id){
	// 	if((led_config.channel[seg_id]->selected == true)){
	// 		led_config.channel[seg_id]->start = seg_start;
	// 		led_config.channel[seg_id]->stop = seg_stop;
	// 		led_config.channel[seg_id]->number_of_leds = seg_stop-seg_start;
	// 		//update neighbouring segments start and stop
	// 		led_set_config(&led_config);
	// 	}
	// }
	// else{
	// 	ESP_LOGI(TAG, "ADDING NEW SEGMENT");
	// 	// int channels = config_get_channels();
	// 	config_set_channels(seg_id);
	// 	led_set_config(&led_config);
	// }

	if(!found){
		ESP_LOGI(TAG, "ADDING NEW SEGMENT");
		config_set_channels(seg_id);
		// led_set_config(&led_config);
		config_set_segment_start_stop( seg_id,  seg_start,  seg_stop);
	}

	
}

void config_set_effect_intensity(uint8_t intensity){
	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->selected)){
			led_config.channel[i]->intensity = intensity;
		}		
	}
	led_set_config(&led_config);
}

void config_set_effect_speed(uint8_t speed){

	speed = (speed > 255) | (speed < 0) ? 128 : speed;


	uint16_t new_speed;
	new_speed = ((float)speed/255.)*2000.;
	ESP_LOGI(TAG, "NEW SPEED  = %d", new_speed);

	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->selected)){
			// led_config.channel[i]->speed = (uint16_t)1000;
			led_config.channel[i]->speed = (uint16_t)new_speed;
		}		
	}

	led_set_config(&led_config);

}

//Sets reverse option of a particular segment selected using id parameter 
void config_set_reverse_option(int id, bool rev_flag){
	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->segment_id == id)){
			led_config.channel[i]->options  = (rev_flag << 7)|(led_config.channel[i]->options & 0x7F);  //set/reset 16th bit for reverse option
			ESP_LOGI(TAG, "OPTION %x", led_config.channel[i]->options);
		}
	}
	led_set_config(&led_config);
}

// Set any option of a segment
void config_set_option(int id, bool flag, uint8_t bit_mask){
	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->segment_id == id)){
			// led_config.channel[i]->options  = (bit_mask)|(led_config.channel[i]->options & ~(bit_mask));  //set/reset 16th bit for reverse option
			if(flag){
				led_config.channel[i]->options |= (bit_mask); 
			}
			else{
				led_config.channel[i]->options &= ~(bit_mask); 
			}
		}
	}
	led_set_config(&led_config);
}


/*Deletes Segment using its ID*/
void config_delete_segment(int id){ 
	int deletion_index  ;
	ESP_LOGI(TAG, "in Segmnet Delete func");

	//iterates through every segment, frees, decrement total led_config.channels
	for(int i = 0; i < led_config.channels; i++){
		ESP_LOGI(TAG, "delete iteration %d", i);
		if((NULL != led_config.channel[i]) && (led_config.channel[i]->segment_id == id)){
			// free(led_config.channel[i]);
			// led_config.channel[i] = NULL;
			// led_config.channels--;
			deletion_index = i;
			break;
		}
		else{
			deletion_index = -1;
		}
	}
	ESP_LOGI(TAG, "Deletion Index :%d", deletion_index); 

	// iterates again and reconfigure the indexes
	for(int i = 0; i < led_config.channels; i++){  // 0-1-2-3
		if(deletion_index == -1) break;

		if((i >= deletion_index)){
			led_config.channel[i] = (led_config.channel[i+1] != NULL) ? led_config.channel[i+1]: NULL;
		}
	}

	if(deletion_index != -1){
		free(led_config.channel[led_config.channels-1]); //free the last segment address
		led_config.channel[led_config.channels-1] = NULL; //NULL last segment

		//update the queue
		led_set_config(&led_config);
	}

	// led_config.channels--; //FIXME: It was commented before ezlo integration

}

void config_apply_palette(uint8_t palette){
	for(int i = 0; i < led_config.channels; i++){
		if((NULL != led_config.channel[i])&&(led_config.channel[i]->selected)){
			led_config.channel[i]->palette = palette;
		}
	}
	led_set_config(&led_config);
}




/*
 * test the NVS storage
 */

static void testConfig() {

	nvs_handle my_handle;
	esp_err_t res;
	uint16_t value1, value2;

	//> tries to open the "config" storage
	if ((res=nvs_open_from_partition(partition_name,"config", NVS_READONLY, &my_handle))!=ESP_OK) {
		ESP_LOGI(TAG, "cannot open config %d", res);
		//> ok, no config found. Now write it first
		ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READWRITE, &my_handle));
		ESP_ERROR_CHECK(nvs_set_u16(my_handle, "test123", 0x1234));
		nvs_close(my_handle);
		ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READONLY, &my_handle));
	}

	// read a value
	ESP_ERROR_CHECK(nvs_get_u16(my_handle, "test123", &value1));
	ESP_LOGI(TAG, "read %d", value1);
	nvs_close(my_handle);

	// now write a change value
	ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READWRITE, &my_handle));
	value1++;
	ESP_ERROR_CHECK(nvs_set_u16(my_handle, "test123", value1));
	ESP_ERROR_CHECK(nvs_commit(my_handle));
	nvs_close(my_handle);

	// read it again and compare it with the changed value
	ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READONLY, &my_handle));
	ESP_ERROR_CHECK(nvs_get_u16(my_handle, "test123", &value2));
	nvs_close(my_handle);
	if(value1 != value2) {
		ESP_LOGE(TAG, "config storage broken! %d <> %d", value1,value2);
	}
}


// reads configuration (if any)
// static void readConfig() {
// 	nvs_handle my_handle;
// 	int res;
// 	if ((res=nvs_open_from_partition(partition_name,"config", NVS_READONLY, &my_handle))) {
// 		ESP_LOGI(TAG, "cannot open config %d", res);
// 		return;
// 	}

// 	size_t size = sizeof(wifi_sta_ssid);
// 	nvs_get_blob(my_handle, "wifi_sta_ssid", wifi_sta_ssid, &size);
// 	size = sizeof(wifi_ap_ssid);
// 	nvs_get_blob(my_handle, "wifi_ap_ssid", wifi_ap_ssid, &size);
// 	size = sizeof(wifi_sta_password);
// 	nvs_get_blob(my_handle, "wifi_sta_pw", wifi_sta_password, &size);
// 	size = sizeof(wifi_ap_password);
// 	nvs_get_blob(my_handle, "wifi_ap_pw", wifi_ap_password, &size);

// 	size = sizeof(udp_server);
// 	nvs_get_blob(my_handle, "udp_server", udp_server, &size);

// 	nvs_get_u16(my_handle, "udp_port", &udp_port);

// 	size = sizeof(struct LED_CONFIG_V1_1);// is larger than sizeof(struct LED_CONFIG_V1_0));
// 	struct LED_CONFIG_V1_1 tmp;
// 	if (nvs_get_blob(my_handle, "leds", &tmp, &size) == ESP_OK) {
// 		if (size == sizeof(struct LED_CONFIG_V1_1)) {
// 			ESP_LOGI(TAG, "led bin config size %d v1.1", size);
// 			for (int i = 0; i < 8; i++) {
// 				led_config.channel[i]->mode = tmp.channel[i]->mode;
// 				led_config.channel[i]->orientation = tmp.channel[i]->orientation;
// 				led_config.channel[i]->sx = tmp.channel[i]->sx;
// 				led_config.channel[i]->sy = tmp.channel[i]->sy;
// 				led_config.channel[i]->ox = tmp.channel[i]->ox;
// 				led_config.channel[i]->oy = tmp.channel[i]->oy;
// 				led_config.channel[i]->black[0] = tmp.channel[i]->black[0];
// 				led_config.channel[i]->black[1] = tmp.channel[i]->black[1];
// 				led_config.channel[i]->black[2] = tmp.channel[i]->black[2];
// 			}
// 		} else if (size == sizeof(struct LED_CONFIG_V1_0)) {
// 			ESP_LOGI(TAG, "led bin config size %d v1.0", size);
// 			struct LED_CONFIG_V1_0 *ptmp = (struct LED_CONFIG_V1_0 *) &tmp;
// 			for (int i = 0; i < 8; i++) {
// 				led_config.channel[i].mode = ptmp->channel[i].mode;
// 				led_config.channel[i].orientation =
// 						ptmp->channel[i].orientation;
// 				led_config.channel[i].sx = ptmp->channel[i].sx;
// 				led_config.channel[i].sy = ptmp->channel[i].sy;
// 				led_config.channel[i].ox = ptmp->channel[i].ox;
// 				led_config.channel[i].oy = ptmp->channel[i].oy;
// 			}
// 		}
// 	}

// 	for (int i = 0; i < 8; i++) {
// 		char varname[32];

// 		uint8_t mode = led_config.channel[i].mode;
// 		sprintf(varname, "leds%dmode", i);
// 		nvs_get_u8(my_handle, varname, &mode);
// 		led_config.channel[i].mode = mode;

// 		uint8_t orientation = led_config.channel[i].orientation;
// 		sprintf(varname, "leds%dori", i);
// 		nvs_get_u8(my_handle, varname, &orientation);
// 		led_config.channel[i].orientation = orientation;

// 		sprintf(varname, "leds%dsx", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].sx);
// 		sprintf(varname, "leds%dsy", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].sy);
// 		sprintf(varname, "leds%dox", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].ox);
// 		sprintf(varname, "leds%doy", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].oy);
// 		sprintf(varname, "leds%db0", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].black[0]);
// 		sprintf(varname, "leds%db1", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].black[1]);
// 		sprintf(varname, "leds%db2", i);
// 		nvs_get_i16(my_handle, varname, &led_config.channel[i].black[2]);
// 	}

// 	size = sizeof(led_coloring);
// 	nvs_get_blob(my_handle, "coloring", &led_coloring, &size);

// 	nvs_get_u8(my_handle, "channels", &led_config.channels);
// 	nvs_get_u8(my_handle, "prefix_leds", &led_config.prefix_leds);
// 	nvs_get_i16(my_handle, "refresh_rate", &led_config.refresh_rate);

// 	nvs_get_u16(my_handle, "artnet_width", &led_config.artnet_width);
// 	nvs_get_u16(my_handle, "artnet_u_offset",
// 			&led_config.artnet_universe_offset);

// 	uint32_t frequency;
// 	uint8_t one, zero, order;
// 	nvs_get_u32(my_handle, "led_frequency", &frequency);
// 	nvs_get_u8(my_handle, "led_one", &one);
// 	nvs_get_u8(my_handle, "led_zero", &zero);
// 	nvs_get_u8(my_handle, "led_order", &order);
// 	// ownled_set_pulses(frequency, one, zero);
// 	ownled_setColorOrder(OWNLED_GRB);

// 	playlist_readConfig(my_handle);

// 	nvs_close(my_handle);
// }

// // write configuration
// void config_write_all() {
// 	nvs_handle my_handle;
// 	ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READWRITE, &my_handle));

// 	ESP_ERROR_CHECK(
// 			nvs_set_blob(my_handle, "wifi_sta_ssid", wifi_sta_ssid,
// 					sizeof(wifi_sta_ssid)));
// 	ESP_ERROR_CHECK(
// 			nvs_set_blob(my_handle, "wifi_ap_ssid", wifi_ap_ssid,
// 					sizeof(wifi_ap_ssid)));
// 	ESP_ERROR_CHECK(
// 			nvs_set_blob(my_handle, "wifi_sta_pw", wifi_sta_password,
// 					sizeof(wifi_sta_password)));
// 	ESP_ERROR_CHECK(
// 			nvs_set_blob(my_handle, "wifi_ap_pw", wifi_ap_password,
// 					sizeof(wifi_ap_password)));
// 	ESP_ERROR_CHECK(
// 			nvs_set_blob(my_handle, "udp_server", udp_server,
// 					sizeof(udp_server)));
// 	ESP_ERROR_CHECK(nvs_set_u16(my_handle, "udp_port", udp_port));

// 	nvs_erase_key(my_handle, "leds");	// remove config from version 1.0

// 	for (int i = 0; i < 8; i++) {
// 		char varname[32];

// 		uint8_t mode = led_config.channel[i].mode;
// 		sprintf(varname, "leds%dmode", i);
// 		nvs_set_u8(my_handle, varname, mode);

// 		uint8_t orientation = led_config.channel[i].orientation;
// 		sprintf(varname, "leds%dori", i);
// 		nvs_set_u8(my_handle, varname, orientation);

// 		sprintf(varname, "leds%dsx", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].sx);
// 		sprintf(varname, "leds%dsy", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].sy);
// 		sprintf(varname, "leds%dox", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].ox);
// 		sprintf(varname, "leds%doy", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].oy);
// 		sprintf(varname, "leds%db0", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].black[0]);
// 		sprintf(varname, "leds%db1", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].black[1]);
// 		sprintf(varname, "leds%db2", i);
// 		nvs_set_i16(my_handle, varname, led_config.channel[i].black[2]);
// 	}

// 	ESP_ERROR_CHECK(nvs_set_u8(my_handle, "channels", led_config.channels));
// 	ESP_ERROR_CHECK(
// 			nvs_set_u8(my_handle, "prefix_leds", led_config.prefix_leds));
// 	ESP_ERROR_CHECK(
// 			nvs_set_u8(my_handle, "refresh_rate", led_config.refresh_rate));

// 	ESP_ERROR_CHECK(
// 			nvs_set_u16(my_handle, "artnet_width", led_config.artnet_width));
// 	ESP_ERROR_CHECK(
// 			nvs_set_u16(my_handle, "artnet_u_offset",
// 					led_config.artnet_universe_offset));

// 	ESP_ERROR_CHECK(
// 			nvs_set_u32(my_handle, "led_frequency",
// 					ownled_get_pulse_frequency()));
// 	ESP_ERROR_CHECK(nvs_set_u8(my_handle, "led_one", ownled_get_pulse_one()));
// 	ESP_ERROR_CHECK(nvs_set_u8(my_handle, "led_zero", ownled_get_pulse_zero()));
// 	ESP_ERROR_CHECK(nvs_set_u8(my_handle, "led_order", ownled_getColorOrder()));

// 	playlist_writeConfig(my_handle);

// 	nvs_close(my_handle);
// }

void config_update_channels() {
	led_set_config(&led_config);
}

void config_init() {
	// Initialize NVS
	esp_err_t ret = nvs_flash_init_partition(partition_name);
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
		ESP_LOGE(TAG, "ESP_ERR_NVS_NO_FREE_PAGES -> erase NVS");
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init_partition(partition_name);
	}
	ESP_ERROR_CHECK(ret);

    // testConfig();
	setDefaults();
	// readConfig();
}

const char *config_get_compile_date() {
	static char date[67] = "";
	// if (date[0] == 0) {
	// 	const esp_app_desc_t *description = esp_ota_get_app_description();
	// 	snprintf(date, sizeof(date), "%s %s %s", description->project_name,
	// 			description->date, description->time);
	// }
	return date;
}

const char *config_get_version() {
	// const esp_app_desc_t *description = esp_ota_get_app_description();
	// return description->version;
	return 'a';
}

//***********************************************************************************************
//                                B A C K T O F A C T O R Y *
//***********************************************************************************************
// Return to factory version. * This will set the otadata to boot from the
// factory image, ignoring previous OTA updates.     *
//***********************************************************************************************
static void back2factory() {
	// esp_partition_iterator_t pi;    // Iterator for find
	// const esp_partition_t *factory; // Factory partition

	// pi = esp_partition_find(ESP_PARTITION_TYPE_APP, // Get partition iterator for
	// 		ESP_PARTITION_SUBTYPE_APP_FACTORY, // factory partition
	// 		"factory");

	// // Check result
	// if (pi == NULL) {
	// 	ESP_LOGE(TAG, "Failed to find factory partition");
	// 	ESP_ERROR_CHECK(ESP_ERR_INVALID_ARG);
	// }

	// factory = esp_partition_get(pi);    // Get partition struct
	// esp_partition_iterator_release(pi); // Release the iterator
	// ESP_ERROR_CHECK(esp_ota_set_boot_partition(factory)); // Set partition for boot
}

void config_reset_to_defaults() {
	ESP_ERROR_CHECK(nvs_flash_erase());
	setDefaults();
	back2factory();
}

const char *config_get_wifi_sta_ssid() {
	// at most 32
	return wifi_sta_ssid;
}

const char *config_get_wifi_sta_password() {
	// at most 64
	return wifi_sta_password;
}

const char *config_get_wifi_ap_ssid() {
	// at most 32
	return wifi_ap_ssid;
}

const char *config_get_wifi_ap_password() {
	// at most 64
	return wifi_ap_password;
}

int config_get_udp_port() {
	return udp_port;
}

const char *config_get_udp_server() {
	return udp_server;
}

static void scpy(char *dst, const char *src, int len) {
	strncpy(dst, src, len-1);
	dst[len - 1] = 0;
}

void config_set_wifi_sta_ssid(const char *ssid) {
	scpy(wifi_sta_ssid, ssid, sizeof(wifi_sta_ssid));
}
void config_set_wifi_sta_password(const char *password) {
	scpy(wifi_sta_password, password, sizeof(wifi_sta_password));
}

void config_set_wifi_ap_ssid(const char *ssid) {
	scpy(wifi_ap_ssid, ssid, sizeof(wifi_ap_ssid));
}

void config_set_wifi_ap_password(const char *password) {
	scpy(wifi_ap_password, password, sizeof(wifi_ap_password));
}

// void config_set_udp_port(uint16_t port) {
// 	if (port == udp_port)
// 		return;
// 	udp_port = port;
// 	bonjour_off();
// 	bonjour_on();
// }

void config_set_udp_server(const char *server) {
	scpy(udp_server, server, sizeof(udp_server));
}

static int compare_short(const void *p1, const void *p2) {
	int a = *(const short *) p1;
	int b = *(const short *) p2;
	return a - b;
}

// void config_set_led_line(int line, enum LED_MODE mode, int size_x, int size_y,
// 		int offset_x, int offset_y, enum LED_ORIENTATION orientation,
// 		const char *options) {
// 	if (line < 0 || line >= config_get_channels())
// 		return;

// 	led_config.channel[line].mode = mode;
// 	led_config.channel[line].sx = size_x;
// 	led_config.channel[line].sy = size_y;
// 	led_config.channel[line].ox = offset_x;
// 	led_config.channel[line].oy = offset_y;
// 	led_config.channel[line].orientation = orientation;
// 	led_config.channel[line].black[0] = -1;
// 	led_config.channel[line].black[1] = -1;
// 	led_config.channel[line].black[2] = -1;

// 	if (0 == strncmp(options, "black=", 6)) {
// 		int16_t *black = led_config.channel[line].black;
// 		int res = sscanf(options + 6, "%hd,%hd,%hd", &black[0], &black[1],
// 				&black[2]);
// 		ESP_LOGI(TAG, "option res %d", res);

// 		for (int i = 0; i < res; i++) {
// 			if (black[i] < 0 || black[i] >= size_x * size_y) {
// 				black[i] = black[res - 1];
// 				res--;
// 				i--;
// 			}
// 		}
// 		qsort(black, res, sizeof(int16_t), compare_short);
// 		for (int i = res; i < 3; i++)
// 			black[i] = -1;

// 		ESP_LOGI(TAG, "options %d %d %d %d", res, black[0], black[1], black[2]);
// 	}
// }

// void config_get_led_line(int line, enum LED_MODE *mode, int *size_x,
// 		int *size_y, int *offset_x, int *offset_y,
// 		enum LED_ORIENTATION *orientation, char options[65]) {
// 	if (line < 0 || line >= config_get_channels())
// 		return;

// 	*size_x = led_config.channel[line].sx;
// 	*size_y = led_config.channel[line].sy;
// 	*offset_x = led_config.channel[line].ox;
// 	*offset_y = led_config.channel[line].oy;
// 	*orientation = led_config.channel[line].orientation;
// 	*mode = led_config.channel[line].mode;

// 	if (led_config.channel[line].black[0] == -1)
// 		options[0] = 0;
// 	else if (led_config.channel[line].black[1] == -1)
// 		sprintf(options, "black=%d", led_config.channel[line].black[0]);
// 	else if (led_config.channel[line].black[2] == -1)
// 		sprintf(options, "black=%d,%d", led_config.channel[line].black[0],
// 				led_config.channel[line].black[1]);
// 	else
// 		sprintf(options, "black=%d,%d,%d", led_config.channel[line].black[0],
// 				led_config.channel[line].black[1],
// 				led_config.channel[line].black[2]);
// }

/**
 * handle the coloring information
 */

void config_coloring_reload() {
	nvs_handle my_handle;
	if (nvs_open_from_partition(partition_name,"config", NVS_READONLY, &my_handle))
		return;

	size_t size = sizeof(led_coloring);
	nvs_get_blob(my_handle, "coloring", &led_coloring, &size);

	nvs_close(my_handle);
}

static struct LED_COLORING defaults_led_coloring = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };
// static struct LED_COLORING defaults_led_coloring = { 0, 1, 0, 1, 0, 1, 0, 1, 0, 1 };

void config_coloring_defaults() {
	led_coloring = defaults_led_coloring;
}

void config_coloring_write() {
	nvs_handle my_handle;
	ESP_ERROR_CHECK(nvs_open_from_partition(partition_name,"config", NVS_READWRITE, &my_handle));

	ESP_ERROR_CHECK(
			nvs_set_blob(my_handle, "coloring", &led_coloring,
					sizeof(led_coloring)));

	nvs_close(my_handle);
}

uint8_t config_get_channels() {
	return led_config.channels;
}

void config_set_channels(uint8_t seg_id) {

	// led_config.channels = seg_id < led_config.channels ? led_config.channels : seg_id+1;
	led_config.channel[led_config.channels] = (struct LED_CONFIG_CHANNEL*)calloc(1,sizeof(struct LED_CONFIG_CHANNEL));
	led_config.channel[led_config.channels]->segment_id = seg_id;
	led_config.channel[led_config.channels]->selected = true;
	led_config.channel[led_config.channels]->mode = FX_MODE_STATIC;
	led_config.channel[led_config.channels]->speed = 1000;

	led_config.channel[led_config.channels]->black[0] = led_config.channel[led_config.channels]->black[1] =
			led_config.channel[led_config.channels]->black[2] = -1;

	led_config.channel[led_config.channels]->speed = 1000;
	led_config.channel[led_config.channels]->intensity = 128;

	led_config.channel[led_config.channels]->options = 0;

	led_config.channel[led_config.channels]->color[0] = 0xFFFF00;
	led_config.channel[led_config.channels]->color[1] = 0x00FF00;
	led_config.channel[led_config.channels]->color[2] = 0x0000FF;

	led_config.channel[led_config.channels]->color_buffer[0] = led_config.channel[led_config.channels]->color[0];
	led_config.channel[led_config.channels]->color_buffer[1] = led_config.channel[led_config.channels]->color[1];
	led_config.channel[led_config.channels]->color_buffer[2] = led_config.channel[led_config.channels]->color[2];

	led_config.channel[led_config.channels]->brightness = 0.5;

	led_config.channel[led_config.channels]->palette = DEFAULT_PALETTE; 
	led_config.channels++;
}

void config_reduce_segment(){
	led_config.channels--;
}

uint8_t config_get_prefix_leds() {
	return led_config.prefix_leds;
}

void config_set_prefix_leds(uint8_t value) {
	if (value > 1)
		return;
	led_config.prefix_leds = value;
}

int16_t config_get_refresh_rate() {
	return led_config.refresh_rate;
}

void config_set_refresh_rate(int16_t value) {
	if ((value >= 2 && value <= 250) || value == -1)
		led_config.refresh_rate = value;
}

void config_set_artnet_width(uint16_t val) {
	led_config.artnet_width = val;
}

void config_set_artnet_universe_offset(uint16_t val) {
	led_config.artnet_universe_offset = val;
}

uint16_t config_get_artnet_width() {
	return led_config.artnet_width;
}

uint16_t config_get_artnet_universe_offset() {
	return led_config.artnet_universe_offset;
}

void config_set_partition_name(const char *pn)
{
	strcpy(partition_name, pn);
	ESP_LOGI(TAG, "NVS partition name is %s",partition_name);
}

const struct LED_CONFIG* config_return_config_pointer()
{
	return &led_config;
}

char* json_update_config()
{
	//#define
	#define MYLED led_config.channel[seg_index]
	#define MYCOLOR(x) MYLED->color[x]
	// Create the outer cJSON object
    cJSON *root = cJSON_CreateObject();

    // Create and add "state" object
    cJSON *state = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "state", state);

    // Add properties to "state" object
    cJSON_AddBoolToObject(state, "on", true);
    cJSON_AddNumberToObject(state, "bri", (led_config.channel[0]->brightness)* 255);
    cJSON_AddNumberToObject(state, "transition", 7);

    // Create and add "nl" object
    cJSON *nl = cJSON_CreateObject();
    cJSON_AddItemToObject(state, "nl", nl);

    // Add properties to "nl" object
    cJSON_AddBoolToObject(nl, "on", false);
    cJSON_AddNumberToObject(nl, "dur", 60);
    cJSON_AddNumberToObject(nl, "mode", 1);
    cJSON_AddNumberToObject(nl, "tbri", 0);
    cJSON_AddNumberToObject(nl, "rem", -1);

    // Create and add "udpn" object
    cJSON *udpn = cJSON_CreateObject();
    cJSON_AddItemToObject(state, "udpn", udpn);

    // Add properties to "udpn" object
    cJSON_AddBoolToObject(udpn, "send", false);
    cJSON_AddBoolToObject(udpn, "recv", true);
    cJSON_AddNumberToObject(udpn, "sgrp", 1);
    cJSON_AddNumberToObject(udpn, "rgrp", 1);

    // Create and add "seg" array
    cJSON *seg_array = cJSON_CreateArray();
    cJSON_AddItemToObject(state, "seg", seg_array);

	for(size_t seg_index = 0; seg_index < led_config.channels; seg_index++)
	{
		if(MYLED)
		{
			// Create and add a "seg" object within the array
			cJSON *seg_object = cJSON_CreateObject();
			cJSON_AddItemToArray(seg_array, seg_object);

			// Add properties to the "seg" object
			cJSON_AddNumberToObject(seg_object, "id", MYLED->segment_id);
			cJSON_AddNumberToObject(seg_object, "start", MYLED->start);
			cJSON_AddNumberToObject(seg_object, "stop", MYLED->stop);

			// Create and add "col" array to "seg" object
			cJSON *col_array = cJSON_CreateArray();
			cJSON_AddItemToObject(seg_object, "col", col_array);

			// Add individual arrays to "col" array
			cJSON *col_element_1 = cJSON_CreateIntArray((int[]){(MYCOLOR(0) & 0xFF0000) >> 16, (MYCOLOR(0) & 0x00FF00) >> 8, (MYCOLOR(0) & 0x0000FF) >> 0, 0}, 4);
			cJSON *col_element_2 = cJSON_CreateIntArray((int[]){(MYCOLOR(1) & 0xFF0000) >> 16, (MYCOLOR(1) & 0x00FF00) >> 8, (MYCOLOR(1) & 0x0000FF) >> 0, 0}, 4);
			cJSON *col_element_3 = cJSON_CreateIntArray((int[]){(MYCOLOR(2) & 0xFF0000) >> 16, (MYCOLOR(2) & 0x00FF00) >> 8, (MYCOLOR(2) & 0x0000FF) >> 0, 0}, 4);

			cJSON_AddItemToArray(col_array, col_element_1);
			cJSON_AddItemToArray(col_array, col_element_2);
			cJSON_AddItemToArray(col_array, col_element_3);

			cJSON_AddNumberToObject(seg_object, "fx", MYLED->mode);
			cJSON_AddNumberToObject(seg_object, "sx", (uint8_t)((MYLED->speed / 2000.f)*255));
			cJSON_AddNumberToObject(seg_object, "ix", MYLED->intensity);
			cJSON_AddNumberToObject(seg_object, "pal", MYLED->palette);
			cJSON_AddNumberToObject(seg_object, "bri", (MYLED->brightness)* 255);
			cJSON_AddBoolToObject(seg_object, "sel", MYLED->selected);
			cJSON_AddBoolToObject(seg_object, "rev", MYLED->options && REVERSE);
			cJSON_AddBoolToObject(seg_object, "mi", false);
		}
	}
    
    // Create and add "info" object
    cJSON *info = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "info", info);

    // Add properties to "info" object
    cJSON_AddStringToObject(info, "ver", "0.14.0");
    cJSON_AddNumberToObject(info, "vid", 2310130);

    // Create and add "leds" object to "info" object
    cJSON *leds = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "leds", leds);

    // Add properties to "leds" object
    cJSON_AddNumberToObject(leds, "count", 100);
    cJSON_AddNumberToObject(leds, "pwr", 480);
    cJSON_AddNumberToObject(leds, "fps", 4);
    cJSON_AddNumberToObject(leds, "maxpwr", 850);
    // Create and add "seglc" array to "leds" object
    cJSON *seglc_array = cJSON_CreateArray();
    cJSON_AddItemToObject(leds, "seglc", seglc_array);

    // Add a single element to "seglc" array
    cJSON_AddNumberToObject(seglc_array, "", 1);
    // ... add other properties to "leds" object ...

    cJSON_AddFalseToObject(info, "str"); // Example for boolean value
    cJSON_AddStringToObject(info, "name", "WLED");
    cJSON_AddNumberToObject(info, "udpport", 21324);
    cJSON_AddFalseToObject(info, "live"); // Example for boolean value
    cJSON_AddNumberToObject(info, "liveseg", -1);
    cJSON_AddStringToObject(info, "lm", "");
    cJSON_AddStringToObject(info, "lip", "");
    cJSON_AddNumberToObject(info, "ws", 0);
    cJSON_AddNumberToObject(info, "fxcount", 187);
    cJSON_AddNumberToObject(info, "palcount", 71);
    cJSON_AddNumberToObject(info, "cpalcount", 0);

    // Create and add "maps" array to "info" object
    cJSON *maps_array = cJSON_CreateArray();
    cJSON_AddItemToObject(info, "maps", maps_array);

    // Create and add a "maps" object within the array
    cJSON *maps_object = cJSON_CreateObject();
    cJSON_AddItemToArray(maps_array, maps_object);

    cJSON_AddNumberToObject(maps_object, "id", 0);

    // Create and add "wifi" object to "info" object
    cJSON *wifi = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "wifi", wifi);

    // Add properties to "wifi" object
    cJSON_AddStringToObject(wifi, "bssid", "6C:44:2A:D4:03:C4");
    cJSON_AddNumberToObject(wifi, "rssi", -67);
    cJSON_AddNumberToObject(wifi, "signal", 66);
    cJSON_AddNumberToObject(wifi, "channel", 5);

    // Create and add "fs" object to "info" object
    cJSON *fs = cJSON_CreateObject();
    cJSON_AddItemToObject(info, "fs", fs);

    // Add properties to "fs" object
    cJSON_AddNumberToObject(fs, "u", 12);
    cJSON_AddNumberToObject(fs, "t", 983);
    cJSON_AddNumberToObject(fs, "pmt", 0);

    cJSON_AddNumberToObject(info, "ndc", 0);
    cJSON_AddStringToObject(info, "arch", "esp32");
    cJSON_AddStringToObject(info, "core", "v3.3.6-16-gcc5440f6a2");
    cJSON_AddNumberToObject(info, "lwip", 0);
    cJSON_AddNumberToObject(info, "freeheap", 198056);
    cJSON_AddNumberToObject(info, "uptime", 85);
    cJSON_AddStringToObject(info, "time", "1970-1-1, 00:01:25");
    cJSON_AddNumberToObject(info, "opt", 79);
    cJSON_AddStringToObject(info, "brand", "WLED");
    cJSON_AddStringToObject(info, "product", "FOSS");
    cJSON_AddStringToObject(info, "mac", "94e68604985c");
    cJSON_AddStringToObject(info, "ip", "192.168.18.151");

    // Print the generated JSON
    char *json_string = cJSON_Print(root);
    cJSON_Minify(json_string);

    // Free the cJSON object
    cJSON_Delete(root);
    return json_string;
}