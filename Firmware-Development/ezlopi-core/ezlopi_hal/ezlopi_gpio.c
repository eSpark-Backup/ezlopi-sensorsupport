#include "ezlopi_gpio.h"
// #include "ezlopi_devices.h"

#if CONFIG_IDF_TARGET_ESP32
e_ezlopi_gpio_num_t ezlopi_valid_gpio_array[GPIO_NUM_MAX] = {
    GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14,
    GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_21, GPIO_NUM_22, GPIO_NUM_23,
    GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27, GPIO_NUM_32, GPIO_NUM_33, GPIO_NUM_34, GPIO_NUM_35};
#elif CONFIG_IDF_TARGET_ESP32S3
e_ezlopi_gpio_num_t ezlopi_valid_gpio_array[GPIO_NUM_MAX] = {
    GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_11,
    GPIO_NUM_12, GPIO_NUM_13, GPIO_NUM_14, GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_17, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_20, GPIO_NUM_21, GPIO_NUM_38,
    GPIO_NUM_39, GPIO_NUM_40, GPIO_NUM_41, GPIO_NUM_42, GPIO_NUM_43, GPIO_NUM_44, GPIO_NUM_45, GPIO_NUM_46, GPIO_NUM_47, GPIO_NUM_48};
#elif CONFIG_IDF_TARGET_ESP32C3
e_ezlopi_gpio_num_t ezlopi_valid_gpio_array[GPIO_NUM_MAX] = {
    GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_3, GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_6,
    GPIO_NUM_7, GPIO_NUM_8, GPIO_NUM_9, GPIO_NUM_10, GPIO_NUM_18, GPIO_NUM_19, GPIO_NUM_20,
    GPIO_NUM_21};
#endif

esp_err_t ezlopi_is_gpio_valid(e_ezlopi_gpio_num_t gpio_num)
{
    esp_err_t ret = ESP_FAIL;
    int i = 0;
    while (i < GPIO_NUM_MAX)
    {
        if (ezlopi_valid_gpio_array[i] == gpio_num)
        {
            ret = ESP_OK;
            break;
        }
        i++;
    }
    return ret;
}

// int ezlopi_gpio_init(s_ezlopi_device_properties_t *properties)
// {
//     int ret = 0;

//     if (GPIO_IS_VALID_OUTPUT_GPIO(properties->interface.gpio.gpio_out.gpio_num))
//     {
//         const gpio_config_t io_conf = {
//             .pin_bit_mask = (1ULL << properties->interface.gpio.gpio_out.gpio_num),
//             .mode = GPIO_MODE_OUTPUT,
//             .pull_up_en = ((properties->interface.gpio.gpio_out.pull == GPIO_PULLUP_ONLY) ||
//                            (properties->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
//                               ? GPIO_PULLUP_ENABLE
//                               : GPIO_PULLUP_DISABLE,
//             .pull_down_en = ((properties->interface.gpio.gpio_out.pull == GPIO_PULLDOWN_ONLY) ||
//                              (properties->interface.gpio.gpio_out.pull == GPIO_PULLUP_PULLDOWN))
//                                 ? GPIO_PULLDOWN_ENABLE
//                                 : GPIO_PULLDOWN_DISABLE,
//             .intr_type = GPIO_INTR_DISABLE,
//         };

//         gpio_config(&io_conf);
//         // digital_io_write_gpio_value(properties);
//     }

//     if (GPIO_IS_VALID_GPIO(properties->interface.gpio.gpio_in.gpio_num))
//     {
//         const gpio_config_t io_conf = {
//             .pin_bit_mask = (1ULL << properties->interface.gpio.gpio_in.gpio_num),
//             .mode = GPIO_MODE_OUTPUT,
//             .pull_up_en = ((properties->interface.gpio.gpio_in.pull == GPIO_PULLUP_ONLY) ||
//                            (properties->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
//                               ? GPIO_PULLUP_ENABLE
//                               : GPIO_PULLUP_DISABLE,
//             .pull_down_en = ((properties->interface.gpio.gpio_in.pull == GPIO_PULLDOWN_ONLY) ||
//                              (properties->interface.gpio.gpio_in.pull == GPIO_PULLUP_PULLDOWN))
//                                 ? GPIO_PULLDOWN_ENABLE
//                                 : GPIO_PULLDOWN_DISABLE,
//             .intr_type = (GPIO_PULLUP_ONLY == properties->interface.gpio.gpio_in.pull)
//                              ? GPIO_INTR_POSEDGE
//                              : GPIO_INTR_NEGEDGE,
//         };

//         gpio_config(&io_conf);
//         // digital_io_isr_service_init(properties);
//     }

//     return ret;
// }
