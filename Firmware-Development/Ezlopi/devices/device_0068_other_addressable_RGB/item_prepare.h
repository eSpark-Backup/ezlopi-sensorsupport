#ifndef ITEM_PREPARE_H
#define ITEM_PREPARE_H

extern int itemCount;

enum ItemNames {
    LED_SEGMENTS =  0,
    LED_EFFECTS =   1,
    LED_PALETTES =  2,
    SWITCH =        3,
    DIMMER =        4,
    COLOR_EFFECT_SELECT =   5,
    COLOR_PALETTE_SELECT =  6,
    COLOR_SEGMENT_GROUPING =    7,
    COLOR_SEGMENT_SPACING = 8,
    COLOR_SEGMENT_OFFSET =  9,
    COLOR_SEGMENT_REVERSE = 10,
    COLOR_SEGMENT_MIRROR =  11,
    COLOR_EFFECT_SPEED =    12,
    COLOR_EFFECT_INTENSITY =    13,
    COLOR_EFFECT_CUSTOM_SLIDER_1 =  14,
    COLOR_EFFECT_CUSTOM_SLIDER_2 =  15,
    COLOR_EFFECT_CUSTOM_SLIDER_3 =  16,
    COLOR_EFFECT_OPTION_1 = 17,
    COLOR_EFFECT_OPTION_2 = 18,
    COLOR_EFFECT_OPTION_3 = 19,
    COLOR_SEGMENT_PRIMARY_COLOR =   20,
    COLOR_SEGMENT_SECONDARY_COLOR = 21,
    COLOR_SEGMENT_TERNARY_COLOR = 22,
    DEVICE_SETTING_LSIT = 23,
    ITEM_UNKNOWN_MAX = 24
};

extern const enum ItemNames unique_parent_device_item_to_prepare[];
extern const enum ItemNames common_child_device_item_to_prepare[];


int size_of_unique_parent();
int size_of_common_child();

void add_item_to_device_struct(l_ezlopi_item_t *newItem, void* prepareFunction);
void prepare_all_items(void* prep_arg);
void* return_prep_fcn_ptr(int index);

void __prepare_device_properties(l_ezlopi_device_t *device, cJSON *cj_device);

void __prepare_ADDRESSABLE_LED_segment_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_effects_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_palettes_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_switch_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_dimmer_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_select_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_palette_select_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_grouping_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_spacing_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_offset_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_revers_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_mirror_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_speed_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_intencity_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_1_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_2_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_custom_slider_3_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_option_1_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_option_2_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_effect_option_3_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_primary_color_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_secondary_color_item (l_ezlopi_item_t *item, cJSON *cj_device);
void __prepare_ADDRESSABLE_LED_color_segment_ternary_color_item (l_ezlopi_item_t *item, cJSON *cj_device);

#endif ITEM_PREPARE_H