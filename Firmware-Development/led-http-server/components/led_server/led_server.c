#include <stdio.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"

#include <esp_event.h>
#include <esp_system.h>
#include <esp_https_server.h>
#include "esp_spiffs.h"

#include "credentials.h"

#include "led_server.h"
#include "uart_reader.h"

#define INDEX_HTML_PATH "/spiffs/index.htm"
#define INDEX_CSS_PATH "/spiffs/index.css"
#define INDEX_JS_PATH "/spiffs/index.js"
#define IRO_JS_PATH "/spiffs/iro.js"
#define RANGETOUCH_JS_PATH "/spiffs/rangetouch.js"

char response_buffer[4096 * 28];

static const char *TAG = "HTTPS";

enum webpage_name {
    INDEX_HTML,
    INDEX_CSS,
    INDEX_JS,
    IRO_JS,
    RANGETOUCH_JS,
    JSON_SI,
    JSON_EFFECTS,
    JSON_PALETTES,
};

/* Prototypes */
static httpd_handle_t start_webserver(void);
void populate_response(enum webpage_name);
void configure_spiffs(void);

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        printf("WiFi connecting ... \n");
        break;
    case WIFI_EVENT_STA_CONNECTED:
        printf("WiFi connected ... \n");
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
        printf("WiFi lost connection ... \n");
        break;
    case IP_EVENT_STA_GOT_IP:
        printf("WiFi got IP ... \n\n");
        configure_spiffs();
        start_webserver();
        printf("http server was initiated ...........\n\n");
        break;
    default:
        break;
    }
}

void wifi_connection()
{
    // 1 - Wi-Fi/LwIP Init Phase
    esp_netif_init();                    // TCP/IP initiation 					s1.1
    esp_event_loop_create_default();     // event loop 			                s1.2
    esp_netif_create_default_wifi_sta(); // WiFi station 	                    s1.3
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation); // 					                    s1.4
    // 2 - Wi-Fi Configuration Phase
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, &instance_got_ip);
    wifi_config_t wifi_configuration = {
        .sta = {
            .ssid = SSID,
            .password = PASS}};
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    // 3 - Wi-Fi Start Phase
    esp_wifi_start();
    // 4- Wi-Fi Connect Phase
    esp_wifi_connect();
}

static esp_err_t get_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    populate_response(INDEX_HTML);
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_index_css_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    httpd_resp_set_type(req, "text/css");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    populate_response(INDEX_CSS);
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_index_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /js Response");
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(INDEX_JS);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_iro_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /iro.js Response");
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(IRO_JS);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_rangetouch_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /rangetouch.js Response");
    httpd_resp_set_type(req, "text/javascript");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(RANGETOUCH_JS);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_json_si_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /json/si Response");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(JSON_SI);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_json_effects_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /json/effects Response");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(JSON_EFFECTS);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t get_json_palettes_js_handler(httpd_req_t *req)
{
    // const char resp[4096 *6] = "GET Response .................";
    ESP_LOGE(TAG, "Sending /json/palettes Response");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    ESP_LOGE(TAG, "Populating Repsonse");
    populate_response(JSON_PALETTES);
    ESP_LOGE(TAG, "Sending Repsonse");
    httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req)
{
    char *content = (char *)malloc(sizeof(char) * req->content_len);
    size_t recv_size = MIN(req->content_len, 2048);
    int ret = httpd_req_recv(req, content, recv_size);

    // If no data is send the error will be:
    // W (88470) httpd_uri: httpd_uri: uri handler execution failed
    printf("\nPOST content: %s\n", content);
    parse_json(content);
    free(content);
    content = NULL;

    if (ret <= 0)
    { /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }
    /* Send a simple response */
    const char resp[] = "POST Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static const httpd_uri_t uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = get_handler
};

static const httpd_uri_t uri_get_index_css = {
    .uri = "/index.css",
    .method = HTTP_GET,
    .handler = get_index_css_handler
};

static const httpd_uri_t uri_get_index_js = {
    .uri = "/index.js",
    .method = HTTP_GET,
    .handler = get_index_js_handler
};

static const httpd_uri_t uri_get_iro_js = {
    .uri = "/iro.js",
    .method = HTTP_GET,
    .handler = get_iro_js_handler
};

static const httpd_uri_t uri_get_rangetouch_js = {
    .uri = "/rangetouch.js",
    .method = HTTP_GET,
    .handler = get_rangetouch_js_handler
};

static const httpd_uri_t uri_get_json_si_js = {
    .uri = "/json/si",
    .method = HTTP_GET,
    .handler = get_json_si_js_handler
};

static const httpd_uri_t uri_get_json_effects_js = {
    .uri = "/json/effects",
    .method = HTTP_GET,
    .handler = get_json_effects_js_handler
};

static const httpd_uri_t uri_get_json_palettes_js = {
    .uri = "/json/palettes",
    .method = HTTP_GET,
    .handler = get_json_palettes_js_handler
};

static const httpd_uri_t uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = post_handler
};

static esp_err_t handle_ws_req(httpd_req_t *req)
{
    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }

    if (ws_pkt.len)
    {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }

    // ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);

    // if (ws_pkt.type == HTTPD_WS_TYPE_TEXT &&
    //     strcmp((char *)ws_pkt.payload, "toggle") == 0)
    // {
    //     free(buf);
    //     return trigger_async_send(req->handle, req);
    // }

    // vTaskDelay(1000/portTICK_PERIOD_MS);
    parse_json((char *)ws_pkt.payload);
    return ESP_OK;
}

/* WEBSOCKET */
httpd_uri_t uri_ws = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handle_ws_req,
        .user_ctx = NULL,
        .is_websocket = true
};


static httpd_handle_t start_webserver(void)
{
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;
    httpd_handle_t server = NULL;

    esp_err_t ret = httpd_start(&server, &config);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &uri_get);
    httpd_register_uri_handler(server, &uri_post);
    httpd_register_uri_handler(server, &uri_get_index_css);
    httpd_register_uri_handler(server, &uri_get_index_js);
    httpd_register_uri_handler(server, &uri_get_iro_js);
    httpd_register_uri_handler(server, &uri_get_rangetouch_js);
    httpd_register_uri_handler(server, &uri_get_json_si_js);
    httpd_register_uri_handler(server, &uri_get_json_effects_js);
    httpd_register_uri_handler(server, &uri_get_json_palettes_js);
    httpd_register_uri_handler(server, &uri_ws);
    return server;
}


void populate_response(enum webpage_name page)
{
    char *PATH = NULL;

    switch (page)
    {
    case INDEX_HTML:
        PATH = INDEX_HTML_PATH;
        break;
    case INDEX_CSS:
        PATH = INDEX_CSS_PATH;
        break;
    case INDEX_JS:
        PATH = INDEX_JS_PATH;
        break;    
    case IRO_JS:
        PATH = IRO_JS_PATH;
        break;
    case RANGETOUCH_JS:
        PATH = RANGETOUCH_JS_PATH;
        break;
    case JSON_SI:
        memset((void *)response_buffer, 0, sizeof(response_buffer));
        sprintf(response_buffer, "{\
    \"state\": {\
        \"on\": true,\
        \"bri\": 128,\
        \"transition\": 7,\
        \"ps\": -1,\
        \"pl\": -1,\
        \"nl\": {\
            \"on\": false,\
            \"dur\": 60,\
            \"mode\": 1,\
            \"tbri\": 0,\
            \"rem\": -1\
        },\
        \"udpn\": {\
            \"send\": false,\
            \"recv\": true,\
            \"sgrp\": 1,\
            \"rgrp\": 1\
        },\
        \"lor\": 0,\
        \"mainseg\": 0,\
        \"seg\": [\
            {\
                \"id\": 0,\
                \"start\": 0,\
                \"stop\": 30,\
                \"len\": 30,\
                \"grp\": 1,\
                \"spc\": 0,\
                \"of\": 0,\
                \"on\": true,\
                \"frz\": false,\
                \"bri\": 255,\
                \"cct\": 127,\
                \"set\": 0,\
                \"col\": [\
                    [\
                        255,\
                        160,\
                        0\
                    ],\
                    [\
                        0,\
                        0,\
                        0\
                    ],\
                    [\
                        0,\
                        0,\
                        0\
                    ]\
                ],\
                \"fx\": 0,\
                \"sx\": 128,\
                \"ix\": 128,\
                \"pal\": 0,\
                \"c1\": 128,\
                \"c2\": 128,\
                \"c3\": 16,\
                \"sel\": true,\
                \"rev\": false,\
                \"mi\": false,\
                \"o1\": false,\
                \"o2\": false,\
                \"o3\": false,\
                \"si\": 0,\
                \"m12\": 0\
            }\
        ]\
    },\
    \"info\": {\
        \"ver\": \"0.14.0\",\
        \"vid\": 2310130,\
        \"leds\": {\
            \"count\": 30,\
            \"pwr\": 480,\
            \"fps\": 4,\
            \"maxpwr\": 850,\
            \"maxseg\": 32,\
            \"seglc\": [\
                1\
            ],\
            \"lc\": 1,\
            \"rgbw\": false,\
            \"wv\": 0,\
            \"cct\": 0\
        },\
        \"str\": false,\
        \"name\": \"WLED\",\
        \"udpport\": 21324,\
        \"live\": false,\
        \"liveseg\": -1,\
        \"lm\": \"\",\
        \"lip\": \"\",\
        \"ws\": 0,\
        \"fxcount\": 187,\
        \"palcount\": 71,\
        \"cpalcount\": 0,\
        \"maps\": [\
            {\
                \"id\": 0\
            }\
        ],\
        \"wifi\": {\
            \"bssid\": \"6C:44:2A:D4:03:C4\",\
            \"rssi\": -67,\
            \"signal\": 66,\
            \"channel\": 5\
        },\
        \"fs\": {\
            \"u\": 12,\
            \"t\": 983,\
            \"pmt\": 0\
        },\
        \"ndc\": 0,\
        \"arch\": \"esp32\",\
        \"core\": \"v3.3.6-16-gcc5440f6a2\",\
        \"lwip\": 0,\
        \"freeheap\": 198056,\
        \"uptime\": 85,\
        \"time\": \"1970-1-1, 00:01:25\",\
        \"opt\": 79,\
        \"brand\": \"WLED\",\
        \"product\": \"FOSS\",\
        \"mac\": \"94e68604985c\",\
        \"ip\": \"192.168.18.117\"\
    }\
}");
        return;
    case JSON_EFFECTS:
        memset((void *)response_buffer, 0, sizeof(response_buffer));
        sprintf(response_buffer, "[\"Solid\", \"Blink\", \"Breathe\", \"Wipe\", \"Wipe Random\", \"Random Colors\", \"Sweep\", \"Dynamic\", \"Colorloop\", \"Rainbow\", \"Scan\", \"Dual Scan\", \"Fade\", \"Chase\", \"Chase Rainbow\", \"Running\", \"Saw\", \"Twinkle\", \"Dissolve\", \"Dissolve Rnd\", \"Sparkle\", \"Dark Sparkle\", \"Sparkle+\", \"Strobe\", \"Strobe Rainbow\", \"Mega Strobe\", \"Blink Rainbow\", \"Android\", \"Chase\", \"Chase Random\", \"Chase Rainbow\", \"Chase Flash\", \"Chase Flash Rnd\", \"Rainbow Runner\", \"Colorful\", \"Traffic Light\", \"Sweep Random\", \"Running 2\", \"Red & Blue\",\"Stream\", \"Scanner\", \"Lighthouse\", \"Fireworks\", \"Rain\", \"Merry Christmas\", \"Fire Flicker\", \"Gradient\", \"Loading\", \"In Out\", \"In In\", \"Out Out\", \"Out In\", \"Circus\", \"Halloween\", \"Tri Chase\", \"Tri Wipe\", \"Tri Fade\", \"Lightning\", \"ICU\", \"Multi Comet\", \"Dual Scanner\", \"Stream 2\", \"Oscillate\", \"Pride 2015\", \"Juggle\", \"Palette\", \"Fire 2012\", \"Colorwaves\", \"BPM\", \"Fill Noise\", \"Noise 1\", \"Noise 2\", \"Noise 3\", \"Noise 4\", \"Colortwinkle\", \"Lake\", \"Meteor\", \"Smooth Meteor\", \"Railway\", \"Ripple\"]");

    return;
    case JSON_PALETTES:
        memset((void *)response_buffer, 0, sizeof(response_buffer));
        sprintf(response_buffer, "[\"Default\", \"Random Cycle\", \"Primary Color\", \"Based on Primary\", \"Set Colors\", \"Based on Set\", \"Party\", \"Cloud\", \"Lava\", \"Ocean\", \"Forest\", \"Rainbow\", \"Rainbow Bands\", \"Sunset\", \"Rivendell\", \"Breeze\", \"Red & Blue\", \"Yellowout\", \"Analogous\", \"Splash\", \"Pastel\", \"Sunset 2\", \"Beech\", \"Vintage\", \"Departure\", \"Landscape\", \"Beach\", \"Sherbet\", \"Hult\", \"Hult 64\", \"Drywet\", \"Jul\", \"Grintage\", \"Rewhi\", \"Tertiary\", \"Fire\", \"Icefire\", \"Cyane\", \"Light Pink\", \"Autumn\", \"Magenta\", \"Magred\", \"Yelmag\", \"Yelblu\", \"Orange & Teal\", \"Tiamat\", \"April Night\"]");

        return;
    default:
        break;
    }

    memset((void *)response_buffer, 0, sizeof(response_buffer));
    struct stat st;
    if (stat(PATH, &st))
    {
        ESP_LOGE(TAG, "index.html not found");
        return;
    }

    FILE *fp = fopen(PATH, "r");
    if (fread(response_buffer, st.st_size, 1, fp) == 0)
    {
        ESP_LOGE(TAG, "fread failed");
    }
    fclose(fp);
}

void configure_spiffs(void)
{
    ESP_LOGI("POP_RESP", "SPIFF CONFIG");
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ESP_ERROR_CHECK(esp_vfs_spiffs_register(&conf));
}