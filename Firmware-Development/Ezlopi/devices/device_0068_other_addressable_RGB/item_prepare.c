#include "trace.h"
#include "esp_netif.h"

#include "ezlopi_wifi.h"
#include "ezlopi_devices_list.h"
#include "ezlopi_devices.h"
#include "ezlopi_device_value_updated.h"
#include "ezlopi_cloud_constants.h"
#include "ezlopi_i2c_master.h"
#include "ezlopi_spi_master.h"

#include "item_prepare.h"

// Define a structure to hold item information
typedef struct {
    l_ezlopi_item_t *item;
    void (*prepareFunction)(l_ezlopi_item_t *item, cJSON *cjson_device);
} DeviceItem;

// Declare a pointer to an array of DeviceItem structures
DeviceItem* deviceItems = NULL;
int itemCount = 0;

//Main Device Items to prepare
const enum ItemNames unique_parent_device_item_to_prepare[] =
{
    LED_SEGMENTS,
    // LED_EFFECTS,
    // LED_PALETTES,
};

//Child Device Items to prepare
const enum ItemNames common_child_device_item_to_prepare[] =
{
    /*Implemented Items*/
    // SWITCH,
    // DIMMER,
    // COLOR_EFFECT_SELECT,
    // COLOR_PALETTE_SELECT,
    // COLOR_SEGMENT_REVERSE,
    // COLOR_EFFECT_SPEED,
    // COLOR_EFFECT_INTENSITY,
    // COLOR_SEGMENT_PRIMARY_COLOR,
    // COLOR_SEGMENT_SECONDARY_COLOR,
    // COLOR_SEGMENT_TERNARY_COLOR,

    /*Un-implemented Items*/
    // COLOR_SEGMENT_GROUPING,
    // COLOR_SEGMENT_SPACING,
    // COLOR_SEGMENT_OFFSET,
    // COLOR_SEGMENT_MIRROR,
    // COLOR_EFFECT_CUSTOM_SLIDER_1,
    // COLOR_EFFECT_CUSTOM_SLIDER_2,
    // COLOR_EFFECT_CUSTOM_SLIDER_3,
    // COLOR_EFFECT_OPTION_1,
    // COLOR_EFFECT_OPTION_2,
    // COLOR_EFFECT_OPTION_3,
};

// Function to add a new device item
void add_item_to_device_struct(l_ezlopi_item_t *newItem, void* prepareFunction) {
    // Resize the array to accommodate the new item
    deviceItems = realloc(deviceItems, (size_t)((sizeof(DeviceItem) * itemCount) + sizeof(DeviceItem)));
    if (!deviceItems) {
        // Handle memory allocation failure
        return;
    }

    // Add the new item to the array
    deviceItems[itemCount].item = newItem;
    deviceItems[itemCount].prepareFunction = prepareFunction;
    ++itemCount;
}

// Function to prepare all device items
void prepare_all_items(void* prep_arg) {
    cJSON* cj_device = (cJSON*)prep_arg;
    for (size_t i = 0; i < itemCount; ++i) {
        deviceItems[i].prepareFunction(deviceItems[i].item, cj_device);
        ESP_LOGW("", "Preparing Items");
    }
}


// Array of function pointers
void* item_prepare_function_pointers[] = {
    __prepare_ADDRESSABLE_LED_segment_item,
    __prepare_ADDRESSABLE_LED_effects_item,
    __prepare_ADDRESSABLE_LED_palettes_item,
    __prepare_ADDRESSABLE_LED_switch_item,
    __prepare_ADDRESSABLE_LED_dimmer_item,
    __prepare_ADDRESSABLE_LED_color_effect_select_item,
    __prepare_ADDRESSABLE_LED_color_palette_select_item,
    __prepare_ADDRESSABLE_LED_color_segment_grouping_item,
    __prepare_ADDRESSABLE_LED_color_segment_spacing_item,
    __prepare_ADDRESSABLE_LED_color_segment_offset_item,
    __prepare_ADDRESSABLE_LED_color_segment_revers_item,
    __prepare_ADDRESSABLE_LED_color_segment_mirror_item,
    __prepare_ADDRESSABLE_LED_color_effect_speed_item,
    __prepare_ADDRESSABLE_LED_color_effect_intencity_item,
    __prepare_ADDRESSABLE_LED_color_effect_custom_slider_1_item,
    __prepare_ADDRESSABLE_LED_color_effect_custom_slider_2_item,
    __prepare_ADDRESSABLE_LED_color_effect_custom_slider_3_item,
    __prepare_ADDRESSABLE_LED_color_effect_option_1_item,
    __prepare_ADDRESSABLE_LED_color_effect_option_2_item,
    __prepare_ADDRESSABLE_LED_color_effect_option_3_item,
    __prepare_ADDRESSABLE_LED_color_segment_primary_color_item,
    __prepare_ADDRESSABLE_LED_color_segment_secondary_color_item,
    __prepare_ADDRESSABLE_LED_color_segment_ternary_color_item,
    // Add more function pointers as needed
};


void* return_prep_fcn_ptr(int index)
{

    if (index >= 0 && index < sizeof(item_prepare_function_pointers) / sizeof(item_prepare_function_pointers[0]))
    {
        return item_prepare_function_pointers[index];
    }
    return NULL;
    
}

/* Setting Handling */
int device_0068_other_addressable_RGB_settings(e_ezlopi_settings_action_t action, l_ezlopi_device_settings_v3_t *setting, void *arg, void *user_arg)
{
    int ret = 0;
    switch (action)
    {
    case EZLOPI_SETTINGS_ACTION_GET_SETTING:
    {
        cJSON* params = (cJSON*)arg;

        cJSON *label = cJSON_CreateObject();
        cJSON_AddItemToObject(params, "label", label);

        // Add properties to "label" object
        cJSON_AddStringToObject(label, "text", "IP Address");
        cJSON_AddStringToObject(label, "lang_tag", "addressable_led_ip_address_label");

        cJSON *desc = cJSON_CreateObject();
        cJSON_AddItemToObject(params, "description", desc);

        // Add properties to "label" object
        cJSON_AddStringToObject(desc, "text", "The IP address of this instance.");
        cJSON_AddStringToObject(desc, "lang_tag", "addressable_led_ip_address_label");

        cJSON_AddBoolToObject(params, "hasSetter", false);
        cJSON_AddStringToObject(params, "valueType", "string");
        
        char tmp_string[64];
        esp_netif_ip_info_t *ip_info = ezlopi_wifi_get_ip_infos();
        snprintf(tmp_string, sizeof(tmp_string), IPSTR, IP2STR(&ip_info->ip));

        cJSON_AddStringToObject(params, "value", tmp_string);
    
        
        break;
    }
    default:
    {
        break;
    }
    }
    return ret;
}

void __prepare_device_properties(l_ezlopi_device_t *device, cJSON *cj_device)
{
    char *device_name = NULL;
    CJSON_GET_VALUE_STRING(cj_device, "dev_name", device_name);
    ASSIGN_DEVICE_NAME_V2(device, device_name);

    // device->cloud_properties.category = category_led_strip;
    // device->cloud_properties.subcategory = subcategory_addressable_main;
    // device->cloud_properties.device_type = dev_type_device;

    device->cloud_properties.category = category_dimmable_light;
    device->cloud_properties.subcategory = subcategory_dimmable_colored;
    device->cloud_properties.device_type = dev_type_dimmer_outlet;

    device->cloud_properties.device_id = ezlopi_cloud_generate_device_id();

    l_ezlopi_device_settings_v3_t *device_setting = (l_ezlopi_device_settings_v3_t*)malloc(sizeof(l_ezlopi_device_settings_v3_t));
    device_setting->cloud_properties.setting_id = ezlopi_cloud_generate_item_id();
    device_setting->cloud_properties.device_id = device->cloud_properties.device_id;
    device_setting->func = device_0068_other_addressable_RGB_settings;
    device_setting->next = NULL;

    device->settings = device_setting;
}

void __prepare_ADDRESSABLE_LED_segment_item(l_ezlopi_item_t *item, cJSON *cj_device)
{
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_segments,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;

    // item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM;
    // item->interface.pwm.channel = 0;
    // item->interface.pwm.value = 0;
    // CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.pwm.gpio_num);
    // CJSON_GET_VALUE_INT(cj_device, "duty_cycle", item->interface.pwm.duty_cycle);
    // CJSON_GET_VALUE_INT(cj_device, "freq_hz", item->interface.pwm.freq_hz);
    // item->interface.pwm.pwm_resln = 12;
}

void __prepare_ADDRESSABLE_LED_effects_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_effects,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_palettes_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = false;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_led_palettes,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_dictionary;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_switch_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_switch,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
    CJSON_GET_VALUE_INT(cj_device, "gpio_out", item->interface.gpio.gpio_out.gpio_num);
}

void __prepare_ADDRESSABLE_LED_dimmer_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM;
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.show = true;
    item->cloud_properties.item_name = ezlopi_item_name_dimmer,
    item->cloud_properties.value_type = value_type_int;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.scale = NULL;
    
    // item->interface.pwm.channel = 0;
    // item->interface.pwm.value = 0;
    // CJSON_GET_VALUE_INT(cj_device, "gpio", item->interface.pwm.gpio_num);
    // CJSON_GET_VALUE_INT(cj_device, "duty_cycle", item->interface.pwm.duty_cycle);
    // CJSON_GET_VALUE_INT(cj_device, "freq_hz", item->interface.pwm.freq_hz);
    // item->interface.pwm.pwm_resln = 12;
}

void __prepare_ADDRESSABLE_LED_color_effect_select_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_select,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_palette_select_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_palette_select,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_grouping_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_grouping,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_spacing_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_spacing,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_offset_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_offset,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_revers_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_revers,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_mirror_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_mirror,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_speed_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_speed,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_intencity_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_intensity,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_1_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_1,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_2_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_2,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_3_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_custom_slider_3,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_int;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_option_1_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_1,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_option_2_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_2,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_effect_option_3_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_effect_option_3,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_bool;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_primary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_primary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_PWM; 
}

void __prepare_ADDRESSABLE_LED_color_segment_secondary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_secondary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

void __prepare_ADDRESSABLE_LED_color_segment_ternary_color_item(l_ezlopi_item_t *item, cJSON *cj_device) {
    item->cloud_properties.has_getter = true;
    item->cloud_properties.has_setter = true;
    item->cloud_properties.item_id = ezlopi_cloud_generate_item_id();
    item->cloud_properties.item_name = ezlopi_item_name_color_segment_ternary_color,
    item->cloud_properties.show = true;
    item->cloud_properties.scale = NULL;
    item->cloud_properties.value_type = value_type_rgb;
    item->interface_type = EZLOPI_DEVICE_INTERFACE_DIGITAL_OUTPUT; 
}

/* enum size */

int size_of_unique_parent()
{
    return sizeof(unique_parent_device_item_to_prepare)/sizeof(unique_parent_device_item_to_prepare[0]);
}

int size_of_common_child()
{
    return sizeof(common_child_device_item_to_prepare)/sizeof(common_child_device_item_to_prepare[0]);
}