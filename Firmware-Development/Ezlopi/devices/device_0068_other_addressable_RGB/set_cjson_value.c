#include <string.h>
#include <stdio.h>
#include "sdkconfig.h"

#include "cJSON.h"
#include "trace.h"
#include "ezlopi_timer.h"
#include "items.h"
#include "math.h"

#include "ezlopi_devices.h"  //for iterating and checking selected flag
#include "ezlopi_valueformatter.h"
#include "ezlopi_item_name_str.h"


#include "led.h"
#include "config.h"
#include "set_cjson_value.h"

static struct LED_CONFIG *led_config;
uint8_t get_segment_number(void *arg);

static void processLedSegments(void* arg) {
    printf("SET: Processing LED Segments\n");
    
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    cJSON* element = cJSON_GetObjectItem(cjson_params, "element");
    cJSON* value = cJSON_GetObjectItem(element, "value");
    int start = 0, end = 0; 
    char *key = NULL, *name = NULL;

    CJSON_GET_VALUE_INT(value, "start", start);
    CJSON_GET_VALUE_INT(value, "end", end);
    CJSON_GET_VALUE_STRING(cjson_params, "key", key);

    if(atoi(key) < config_get_channels())
    {
        config_set_segment_start_stop(atoi(key) , start, end);
        CJSON_GET_VALUE_STRING(value, "name", name);
        led_config->channel[atoi(key)]->segment_name = strdup(name);
        led_config->channel[atoi(key)]->segment_id = atoi(key);
    }
}

static void processLedEffects(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("SET: Processing LED Effects\n");
}

static void processLedPalettes(void* arg) {
    //There will be no implementation for this as this is just for getting palette names setting api is listed below
    printf("SET: Processing LED Palettes\n");
}

static void processSwitch(void* arg) {
    printf("SET: Processing Switch\n");

    int value_recieved = 0;
    uint8_t new_brightness = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);


    new_brightness = value_recieved ? (uint8_t)((led_config->channel[segment_index]->prev_brightness)*(100)) : 0;
    config_set_segment_brightness((uint8_t)new_brightness, segment_index);
}

static void processDimmer(void* arg) {
    printf("SET: Processing Dimmer\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);
    config_set_segment_brightness(value_recieved, segment_index);

}

static void processColorEffectSelect(void* arg) {
    printf("SET: Processing Color Effect Select\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_setMode(value_recieved);
}

static void processColorPaletteSelect(void* arg) {
    printf("SET: Processing Color Palette Select\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_apply_palette(value_recieved);
}

static void processColorSegmentGrouping(void* arg) {
    printf("SET: Processing Color Segment Grouping\n");
}

static void processColorSegmentSpacing(void* arg) {
    printf("SET: Processing Color Segment Spacing\n");
}

static void processColorSegmentOffset(void* arg) {
    printf("SET: Processing Color Segment Offset\n");
}

static void processColorSegmentReverse(void* arg) {
    printf("SET: Processing Color Segment Reverse\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_reverse_option(0, value_recieved);

}

static void processColorSegmentMirror(void* arg) {
    printf("SET: Processing Color Segment Mirror\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    //TODO: Mirror funciton
}

static void processColorEffectSpeed(void* arg) {
    printf("SET: Processing Color Effect Speed\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_effect_speed(value_recieved);
}

static void processColorEffectIntensity(void* arg) {
    printf("SET: Processing Color Effect Intensity\n");

    int value_recieved = 0;
    cJSON *cjson_params = (cJSON *)arg;

    char *jsonString = cJSON_Print(cjson_params);
    printf("JSON String: %s\n", jsonString);
    free(jsonString);

    CJSON_GET_VALUE_INT(cjson_params, "value", value_recieved);

    config_set_effect_intensity(value_recieved);
}

static void processColorEffectCustomSlider1(void* arg) {
    printf("SET: Processing Color Effect Custom Slider 1\n");
}

static void processColorEffectCustomSlider2(void* arg) {
    printf("SET: Processing Color Effect Custom Slider 2\n");
}

static void processColorEffectCustomSlider3(void* arg) {
    printf("SET: Processing Color Effect Custom Slider 3\n");
}

static void processColorEffectOption1(void* arg) {
    printf("SET: Processing Color Effect Option 1\n");
}

static void processColorEffectOption2(void* arg) {
    printf("SET: Processing Color Effect Option 2\n");
}

static void processColorEffectOption3(void* arg) {
    printf("SET: Processing Color Effect Option 3\n");
}



uint32_t extract_rgb_value_from_json(void *arg)
{
    static uint32_t extracted_rgb;
    cJSON* cjson_params = (cJSON*)arg;
    cJSON* value_object = cJSON_GetObjectItem(cjson_params, "value");

    uint8_t red = cJSON_GetObjectItem(value_object, "red")->valueint;
    uint8_t green = cJSON_GetObjectItem(value_object, "green")->valueint;
    uint8_t blue = cJSON_GetObjectItem(value_object, "blue")->valueint;
    extracted_rgb = ((uint32_t)red << 16) | ((uint32_t)green << 8) | ((uint32_t)blue << 0);

    return extracted_rgb;
}

uint32_t rgbcolor[3] = {0};
static void processColorSegmentPrimaryColor(void* arg) {
    printf("SET: Processing Color Segment Primary Color\n");
    rgbcolor[0] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processColorSegmentSecondaryColor(void* arg) {
    printf("SET: Processing Color Segment Secondary Color\n");
    rgbcolor[1] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processColorSegmentTertiaryColor(void* arg) {
    printf("SET: Processing Color Segment Tertiary Color\n");
    rgbcolor[2] = extract_rgb_value_from_json(arg);
    config_setColor(rgbcolor);
}

static void processUnknown(void* arg) {
    // printf("Unknown item name\n");
    ESP_LOGE("set_cjson_value", "Unknown Item name!");
}


void processItem(const char* itemName, void *arg) {
    led_config = config_return_config_pointer();
 
  const ItemHandler itemHandlers[] = {
        {ezlopi_item_name_led_segments, processLedSegments},
        {ezlopi_item_name_switch, processSwitch},
        {ezlopi_item_name_dimmer, processDimmer},
        {ezlopi_item_name_led_effects, processLedEffects},
        {ezlopi_item_name_led_palettes, processLedPalettes},
        {ezlopi_item_name_color_effect_select, processColorEffectSelect},
        {ezlopi_item_name_color_palette_select, processColorPaletteSelect},
        {ezlopi_item_name_color_segment_grouping, processColorSegmentGrouping},
        {ezlopi_item_name_color_segment_spacing, processColorSegmentSpacing},
        {ezlopi_item_name_color_segment_offset, processColorSegmentOffset},
        {ezlopi_item_name_color_segment_revers, processColorSegmentReverse},
        {ezlopi_item_name_color_segment_mirror, processColorSegmentMirror},
        {ezlopi_item_name_color_effect_speed, processColorEffectSpeed},
        {ezlopi_item_name_color_effect_intensity, processColorEffectIntensity},
        {ezlopi_item_name_color_effect_custom_slider_1, processColorEffectCustomSlider1},
        {ezlopi_item_name_color_effect_custom_slider_2, processColorEffectCustomSlider2},
        {ezlopi_item_name_color_effect_custom_slider_3, processColorEffectCustomSlider3},
        {ezlopi_item_name_color_effect_option_1, processColorEffectOption1},
        {ezlopi_item_name_color_effect_option_2, processColorEffectOption2},
        {ezlopi_item_name_color_effect_option_3, processColorEffectOption3},
        {ezlopi_item_name_color_segment_primary_color, processColorSegmentPrimaryColor},
        {ezlopi_item_name_color_segment_secondary_color, processColorSegmentSecondaryColor},
        {ezlopi_item_name_color_segment_ternary_color, processColorSegmentTertiaryColor},
        {"unknown", processUnknown}  // Default case
    };

    size_t i;
    char *key_recieved_int = 0;

    get_segment_number(arg);
    
    for (i = 0; i < sizeof(itemHandlers) / sizeof(itemHandlers[0]); ++i) {
        if (strcmp(itemName, itemHandlers[i].itemName) == 0) {

            config_set_selected_flag(segment_index, true);
            vTaskDelay(200/portTICK_PERIOD_MS); //delays so that queue has time
            itemHandlers[i].itemHandler(arg);
            vTaskDelay(200/portTICK_PERIOD_MS);
            config_set_selected_flag(segment_index, false);

            return;
        }
    }

    // If itemName is not found, execute default handler
    itemHandlers[i].itemHandler(arg);
    
}

// Function to convert integer to dynamically allocated string
static char* intToString(int num) {
    // Determine the number of digits in the integer
    int digits = snprintf(NULL, 0, "%d", num);

    // Allocate memory for the string, including space for the null terminator
    char* str = (char*)malloc(digits + 1);

    // Check for memory allocation failure
    if (str == NULL) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    // Use snprintf to format the integer as a string
    snprintf(str, digits + 1, "%x", num);

    // Return the dynamically allocated string
    return str;
}



uint8_t get_segment_number(void *arg)
{
    l_ezlopi_device_t *device = ezlopi_device_get_head();
    uint8_t found_item = 0;
    segment_index = 0;

    char *item_id_rx = 0, *formatted_item_id = 0;

    cJSON* params = (cJSON*)arg;
    CJSON_GET_VALUE_STRING(params, "_id", item_id_rx);

    ESP_LOGW("", "ITEM ID %s", item_id_rx);

    while(device)
    {
        l_ezlopi_item_t *item = device->items;
        while(item)
        {
            // ESP_LOGW("", "segment Index %d", segment_index);
            formatted_item_id = intToString(item->cloud_properties.item_id);

            // ESP_LOGW("item_id_recieved : ", "%s",item_id_rx);
            // ESP_LOGW("item_id : ", "%s" ,formatted_item_id);
            if(!strcmp(formatted_item_id, item_id_rx))
            {
                found_item = 1;
                break;
            }
            item = item->next;
        }
        if(found_item)
        {
            break;
        }
        device = device->next;
        segment_index++;
    }
    free(formatted_item_id);
    return segment_index;
}
