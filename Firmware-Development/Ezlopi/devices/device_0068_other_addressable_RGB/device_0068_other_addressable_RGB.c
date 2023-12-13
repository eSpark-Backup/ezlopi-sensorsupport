
#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_timer.h"
#include "items.h"
#include "math.h"

#include "ezlopi_cloud.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_devices.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_i2c_master.h"
#include "ezlopi_spi_master.h"

#include "ezlopi_valueformatter.h"

#include "led_strip.h"
#include "driver/gpio.h"
#include "color_codes.h"

#include "device_0068_other_addressable_RGB.h"

#include "esp_heap_caps.h"
#include "esp_system.h"

#include "config.h"
#include "led.h"
#include "set_cjson_value.h"
static bool ws2812_led_strip_initialized = false;
struct LED_CONFIG* led_config;

char* PaletteNames[] = {
    #define PALETTE(x, y) y,
    #include "palette_list.h"
};

static l_ezlopi_device_t *device = NULL;

static l_ezlopi_item_t *dimmer_up_item = NULL;
static l_ezlopi_item_t *dimmer_down_item = NULL;
static l_ezlopi_item_t *dimmer_stop_item = NULL;
static l_ezlopi_item_t *rgb_color_item = NULL;

static l_ezlopi_item_t *segment_item = NULL;
static l_ezlopi_item_t *effects_item = NULL;
static l_ezlopi_item_t *palettes_item = NULL;
static l_ezlopi_item_t *switch_item = NULL;
static l_ezlopi_item_t *dimmer_item = NULL;
static l_ezlopi_item_t *color_effect_select_item = NULL;
static l_ezlopi_item_t *color_palette_select_item = NULL;
static l_ezlopi_item_t *color_segment_grouping_item = NULL;
static l_ezlopi_item_t *color_segment_spacing_item = NULL;
static l_ezlopi_item_t *color_segment_offset_item = NULL;
static l_ezlopi_item_t *color_segment_revers_item = NULL;
static l_ezlopi_item_t *color_segment_mirror_item = NULL;
static l_ezlopi_item_t *color_effect_speed_item = NULL;
static l_ezlopi_item_t *color_effect_intencity_item = NULL;
static l_ezlopi_item_t *color_effect_custom_slider_1_item = NULL;
static l_ezlopi_item_t *color_effect_custom_slider_2_item = NULL;
static l_ezlopi_item_t *color_effect_custom_slider_3_item = NULL;
static l_ezlopi_item_t *color_effect_option_1_item = NULL;
static l_ezlopi_item_t *color_effect_option_2_item = NULL;
static l_ezlopi_item_t *color_effect_option_3_item = NULL;
static l_ezlopi_item_t *color_segment_primary_color_item = NULL;
static l_ezlopi_item_t *color_segment_secondary_color_item = NULL;
static l_ezlopi_item_t *color_segment_ternary_color_item = NULL;



static int __prepare(void *arg);
static int __init(l_ezlopi_item_t *item);
static int __set_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __get_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __add_cjson_value(l_ezlopi_item_t *item, void *arg);
static int __remove_cjson_value(l_ezlopi_item_t *item, void *arg);


int device_0068_other_addressable_RGB(e_ezlopi_actions_t action, l_ezlopi_item_t *item, void *arg, void *user_arg)
{
    int ret = 0;

    switch (action)
    {
    case EZLOPI_ACTION_PREPARE:
    {
        ret = __prepare(arg);
        break;
    }
    case EZLOPI_ACTION_INITIALIZE:
    {
        ret = __init(item);
        break;
    }
    case EZLOPI_ACTION_SET_VALUE:
    {
        ret = __set_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_HUB_GET_ITEM:
    case EZLOPI_ACTION_GET_EZLOPI_VALUE:
    {
        ret = __get_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_ADD_VALUE:
    {
        ret = __add_cjson_value(item, arg);
        break;
    }
    case EZLOPI_ACTION_REMOVE_VALUE:
    {
        ret = __remove_cjson_value(item, arg);
        break;
    }
    default:
    {
        break;
    }
    }

    return ret;
}

static int __get_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    if (NULL != arg)
    {
        process_item_get(item->cloud_properties.item_name, arg);
    }
    return ret;
}

static int __set_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    static uint8_t previous_brightness = 0;

    if ((NULL != arg))
    {
        processItem(item->cloud_properties.item_name, arg);
    }
    return ret;
}

static int __init(l_ezlopi_item_t *item)
{
    int ret = 0;
    // led_strip_t *addressable_led_strip = (led_strip_t *)item->user_arg;
    // if ((!ws2812_led_strip_initialized) && (NULL != addressable_led_strip))
    // {
    //     addressable_led_strip->type = LED_STRIP_WS2812;
    //     addressable_led_strip->length = 100;
    //     addressable_led_strip->gpio = item->interface.pwm.gpio_num;
    //     addressable_led_strip->buf = NULL;
    //     addressable_led_strip->brightness = 10;
    //     addressable_led_strip->channel = RMT_CHANNEL_0;

    //     led_strip_install();
    //     ESP_ERROR_CHECK(led_strip_init(addressable_led_strip));
    //     rgb_t color = {
    //         .red = 255,
    //         .green = 255,
    //         .blue = 255,
    //     };
    //     led_strip_fill(addressable_led_strip, 0, addressable_led_strip->length, color);
    //     led_strip_set_brightness(addressable_led_strip, addressable_led_strip->brightness);
    //     ESP_ERROR_CHECK(led_strip_flush(addressable_led_strip));
    //     ws2812_led_strip_initialized = true;
    // }
    
    if(!ws2812_led_strip_initialized)
    {
        config_init();
        led_on();
        config_update_channels();
        ws2812_led_strip_initialized = true;
        led_config = config_return_config_pointer();
    }

    return ret;
}

static void __prepare_device_properties(l_ezlopi_device_t *device, cJSON *cj_device)
{
    char *device_name = NULL;
    CJSON_GET_VALUE_STRING(cj_device, "dev_name", device_name);
    ASSIGN_DEVICE_NAME_V2(device, device_name);

    device->cloud_properties.category = category_dimmable_light;
    device->cloud_properties.subcategory = subcategory_dimmable_colored;
    device->cloud_properties.device_type = dev_type_device;

    // device->cloud_properties.category = category_dimmable_light;
    // device->cloud_properties.subcategory = subcategory_dimmable_colored;
    // device->cloud_properties.device_type = dev_type_dimmer_outlet;

    device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();
}

static void __prepare_ADDRESSABLE_LED_segment_item(l_ezlopi_item_t *item, cJSON *cj_device)
{
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_segments,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;

    item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM; //TODO:
    item->interface.pwm.channel = 0;
    item->interface.pwm.value = 0;
    CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.pwm.gpio_num);
    CJSON_GET_VALUE_INT(cj_device, "duty_cycle", item->interface.pwm.duty_cycle);
    CJSON_GET_VALUE_INT(cj_device, "freq_hz", item->interface.pwm.freq_hz);
    item->interface.pwm.pwm_resln = 12;
    // item->interface.gpio.gpio_out.enable = true;
    // item->interface.gpio.gpio_out.gpio_num = 0;
    // item->interface.gpio.gpio_out.interrupt = false;
    // item->interface.gpio.gpio_out.invert = false;
    // item->interface.gpio.gpio_out.mode = GPIO_MODE_INPUT;
    // item->interface.gpio.gpio_out.pull = false;
    // item->interface.gpio.gpio_out.value = false;
    // item->interface.gpio.gpio_in.enable = false;
}

static void __prepare_ADDRESSABLE_LED_effects_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_effects,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_palettes_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_palettes,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_switch_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_switch,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
    CJSON_GET_VALUE_INT(cj_device, "gpio_out", item->interface.gpio.gpio_out.gpio_num);
}

static void __prepare_ADDRESSABLE_LED_dimmer_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    // item->cloud_properties.has_getter = true;
    // item->cloud_properties.has_setter = true;
    // item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    // item->cloud_properties.item_name = ezlopi_item_name_dimmer,
    // item->cloud_properties.show = true;
    // item->cloud_properties.scale = NULL;
    // item->cloud_properties.value_type = value_type_int;
    // item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM; 

    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_dimmer,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM;
    item->interface.pwm.channel = 0;
    item->interface.pwm.value = 0;
    CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.pwm.gpio_num);
    CJSON_GET_VALUE_INT(cj_device, "duty_cycle", item->interface.pwm.duty_cycle);
    CJSON_GET_VALUE_INT(cj_device, "freq_hz", item->interface.pwm.freq_hz);
    item->interface.pwm.pwm_resln = 12;
}

static void __prepare_ADDRESSABLE_LED_color_effect_select_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_select,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_palette_select_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_palette_select,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_grouping_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_grouping,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_spacing_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_spacing,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_offset_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_offset,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_revers_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_revers,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_mirror_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_mirror,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_speed_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_speed,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_intencity_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_intensity,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_1_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_1,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_2_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_2,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_3_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_3,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_option_1_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_1,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_option_2_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_2,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_effect_option_3_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_3,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_primary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_primary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_secondary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_secondary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static void __prepare_ADDRESSABLE_LED_color_segment_ternary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_ternary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; // TODO:
}

static int __prepare(void *arg)
{
    int ret = 0;
    // led_config.channel[0] = (struct LED_CONFIG_CHANNEL*)malloc(sizeof(struct LED_CONFIG_CHANNEL));

    s_ezlopi_prep_arg_t *prep_arg = (s_ezlopi_prep_arg_t *)arg;
    if (prep_arg && prep_arg->cjson_device)
    {
        device = ezlopi_device_add_device();
        if (device)
        {
            __prepare_device_properties(device, prep_arg->cjson_device);
            // rgb_color_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // dimmer_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // dimmer_up_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // dimmer_down_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // dimmer_stop_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // switch_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);

            segment_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            effects_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            palettes_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            switch_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            dimmer_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_effect_select_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_palette_select_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_segment_grouping_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_segment_spacing_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_segment_offset_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_segment_revers_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_segment_mirror_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_effect_speed_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_effect_intencity_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_custom_slider_1_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_custom_slider_2_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_custom_slider_3_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_option_1_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_option_2_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            // color_effect_option_3_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_segment_primary_color_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_segment_secondary_color_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);
            color_segment_ternary_color_item = ezlopi_device_add_item_to_device(device, device_0068_other_addressable_RGB);

            if (segment_item && effects_item && palettes_item && switch_item && dimmer_item && color_effect_select_item)
            {
                // __prepare_ADDRESSABLE_RGB_color_item(rgb_color_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_RGB_dimmer_item(dimmer_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_RGB_dimmer_up_item(dimmer_up_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_RGB_dimmer_down_item(dimmer_down_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_RGB_dimmer_stop_item(dimmer_stop_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_onoff_switch_item(switch_item, prep_arg->cjson_device);

                __prepare_ADDRESSABLE_LED_segment_item(segment_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_effects_item(effects_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_palettes_item(palettes_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_switch_item(switch_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_dimmer_item(dimmer_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_effect_select_item(color_effect_select_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_palette_select_item(color_palette_select_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_segment_grouping_item(color_segment_grouping_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_segment_spacing_item(color_segment_spacing_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_segment_offset_item(color_segment_offset_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_segment_revers_item(color_segment_revers_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_segment_mirror_item(color_segment_mirror_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_effect_speed_item(color_effect_speed_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_effect_intencity_item(color_effect_intencity_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_custom_slider_1_item(color_effect_custom_slider_1_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_custom_slider_2_item(color_effect_custom_slider_2_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_custom_slider_3_item(color_effect_custom_slider_3_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_option_1_item(color_effect_option_1_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_option_2_item(color_effect_option_2_item, prep_arg->cjson_device);
                // __prepare_ADDRESSABLE_LED_color_effect_option_3_item(color_effect_option_3_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_segment_primary_color_item(color_segment_primary_color_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_segment_secondary_color_item(color_segment_secondary_color_item, prep_arg->cjson_device);
                __prepare_ADDRESSABLE_LED_color_segment_ternary_color_item(color_segment_ternary_color_item, prep_arg->cjson_device);


                
                // led_strip_t *addressable_led_strip = (led_strip_t *)malloc(sizeof(led_strip_t));
                // if (addressable_led_strip)
                // {
                //     memset(addressable_led_strip, 0, sizeof(led_strip_t));
                //     // switch_item->user_arg = addressable_led_strip;
                //     // dimmer_item->user_arg = addressable_led_strip;
                //     // dimmer_up_item->user_arg = addressable_led_strip;
                //     // dimmer_down_item->user_arg = addressable_led_strip;
                //     // dimmer_stop_item->user_arg = addressable_led_strip;
                //     // rgb_color_item->user_arg = addressable_led_strip;

                //     segment_item->user_arg = addressable_led_strip;
                //     effects_item->user_arg = addressable_led_strip;
                //     palettes_item->user_arg = addressable_led_strip;
                //     switch_item->user_arg = addressable_led_strip;
                //     dimmer_item->user_arg = addressable_led_strip;
                //     color_effect_select_item->user_arg = addressable_led_strip;
                //     color_palette_select_item->user_arg = addressable_led_strip;
                //     // color_segment_grouping_item->user_arg = addressable_led_strip;
                //     // color_segment_spacing_item->user_arg = addressable_led_strip;
                //     // color_segment_offset_item->user_arg = addressable_led_strip;
                //     color_segment_revers_item->user_arg = addressable_led_strip;
                //     // color_segment_mirror_item->user_arg = addressable_led_strip;
                //     color_effect_speed_item->user_arg = addressable_led_strip;
                //     color_effect_intencity_item->user_arg = addressable_led_strip;
                //     // color_effect_custom_slider_1_item->user_arg = addressable_led_strip;
                //     // color_effect_custom_slider_2_item->user_arg = addressable_led_strip;
                //     // color_effect_custom_slider_3_item->user_arg = addressable_led_strip;
                //     // color_effect_option_1_item->user_arg = addressable_led_strip;
                //     // color_effect_option_2_item->user_arg = addressable_led_strip;
                //     // color_effect_option_3_item->user_arg = addressable_led_strip;
                //     color_segment_primary_color_item->user_arg = addressable_led_strip;
                //     color_segment_secondary_color_item->user_arg = addressable_led_strip;
                //     color_segment_ternary_color_item->user_arg = addressable_led_strip;


                // }
            }
            else
            {
                ezlopi_device_free_device(device);
            }
        }
        else
        {
            ezlopi_device_free_device(device);
        }
    }

    return ret;
}

static int __add_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;

    char *segment_name = NULL;
    int segment_start = 0, segment_stop = 0;
    cJSON *cjson_params = (cJSON *)arg;
    if(NULL !=  cjson_params)
    {
        if (ezlopi_item_name_led_segments == item->cloud_properties.item_name)
        {
            cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
            cJSON* value = cJSON_GetObjectItem(element, "value");
            CJSON_GET_VALUE_STRING(value, "name", segment_name);
            CJSON_GET_VALUE_INT(value, "start", segment_start);
            CJSON_GET_VALUE_INT(value, "end", segment_stop);

            config_set_segment_start_stop(config_get_channels(), segment_start, segment_stop);
            led_config->channel[config_get_channels()-1]->segment_name = strdup(segment_name);
        }

        //iterate to last device and malloc next device
        l_ezlopi_device_t *curr_device = ezlopi_device_get_head();
        while (curr_device->next)
        {
            curr_device = curr_device->next;
        }

        curr_device->next = ezlopi_device_add_device();
        l_ezlopi_device_t *new_device = curr_device->next;

        if(new_device)
        {
            ASSIGN_DEVICE_NAME_V2(new_device, "test_device");

            /*Prepare Device*/
            cJSON* cj_device = cJSON_CreateObject();
            cJSON_AddStringToObject(cj_device, "dev_name", segment_name);
            __prepare_device_properties(new_device, cj_device);

            new_device->items =  ezlopi_device_add_item_to_device(new_device, device_0068_other_addressable_RGB);
            l_ezlopi_item_t *test_item_device = new_device->items;

            test_item_device->cloud_properties.item_name = ezlopi_item_name_color_palette_select;
            test_item_device->cloud_properties.value_type = value_type_int;
            test_item_device->cloud_properties.has_getter = true;
            test_item_device->cloud_properties.has_setter = true;
            test_item_device->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
            test_item_device->cloud_properties.item_name = ezlopi_item_name_color_segment_primary_color,
            test_item_device->cloud_properties.show = true;
            test_item_device->cloud_properties.scale = NULL;
            test_item_device->cloud_properties.value_type = value_type_rgb;
            test_item_device->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT;
        }
    }

    return ret;
}

static int __remove_cjson_value(l_ezlopi_item_t *item, void *arg)
{
    int ret = 0;
    char* key = 0;
    cJSON *cjson_params = (cJSON *)arg;
    if(NULL !=  cjson_params)
    {
        if (ezlopi_item_name_led_segments == item->cloud_properties.item_name)
        {
            CJSON_GET_VALUE_STRING(cjson_params, "key", key);
            config_delete_segment(atoi(key));
        }
    }

    return ret;
}