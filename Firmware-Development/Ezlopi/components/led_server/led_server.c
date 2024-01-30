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
#include "esp_tls.h"


#include "credentials.h"

#include "led_server.h"
#include "uart_reader.h"
#include "effect_list_json.h"

#include "./compressed_webpage_files/compressed_index_html.h"
#include "./compressed_webpage_files/compressed_index_css.h"
#include "./compressed_webpage_files/compressed_index_js.h"
#include "./compressed_webpage_files/compressed_iro_js.h"
#include "./compressed_webpage_files/compressed_rangetouch_js.h"
// #include "./compressed_webapge_files/compressed_EzloSans-Thin_array.h"
// #include "./compressed_webapge_files/compressed_EzloSans-Light_array.h"

// #define USE_MALLOC
#ifdef USE_MALLOC
char *response_buffer;
#else
char response_buffer[2048 * 1];
#endif

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static void connect_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
static void stop_webserver(httpd_handle_t server);

void adder_free_func(void *ptr);

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
    NOT_FOUND,
};

/* Prototypes */
void populate_response(enum webpage_name page, httpd_req_t *req);

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
        // start_webserver();
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
    populate_response(INDEX_HTML, req);

    return ESP_OK;
}

static esp_err_t get_index_css_handler(httpd_req_t *req)
{

    populate_response(INDEX_CSS, req);

    return ESP_OK;
}

static esp_err_t get_index_js_handler(httpd_req_t *req)
{
    populate_response(INDEX_JS, req);

    return ESP_OK;
}

static esp_err_t get_iro_js_handler(httpd_req_t *req)
{
    populate_response(IRO_JS, req);

    return ESP_OK;
}

static esp_err_t get_rangetouch_js_handler(httpd_req_t *req)
{
    populate_response(RANGETOUCH_JS, req);

    return ESP_OK;
}

static esp_err_t get_json_si_js_handler(httpd_req_t *req)
{
    populate_response(JSON_SI, req);

    return ESP_OK;
}

static esp_err_t get_json_effects_js_handler(httpd_req_t *req)
{
    populate_response(JSON_EFFECTS, req);

    return ESP_OK;
}

static esp_err_t get_json_palettes_js_handler(httpd_req_t *req)
{
    populate_response(JSON_PALETTES, req);

    return ESP_OK;
}

static esp_err_t not_found_url_handler(httpd_req_t *req)
{
    populate_response(NOT_FOUND, req);

    return ESP_OK;
}

esp_err_t post_handler(httpd_req_t *req)
{
    // if (!req->sess_ctx) {
    //     ESP_LOGI(TAG, "/adder allocating new session");
    //     req->sess_ctx = malloc(sizeof(int));
    //     req->free_ctx = adder_free_func;
    //     *(int *)req->sess_ctx = 0;
    // }
    // int *ctx_data = (int *)req->sess_ctx;

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
    const char resp[] = "POST Request Received!";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

typedef struct {
    const char *uri;
    httpd_method_t method;
    httpd_handle_t handler;
} Route;

// Define the routes
static const Route routes[] = {
    /*GET ROUTES*/
    {"/", HTTP_GET, get_handler},
    {"/index.css", HTTP_GET, get_index_css_handler},
    {"/index.js", HTTP_GET, get_index_js_handler},
    {"/iro.js", HTTP_GET, get_iro_js_handler},
    {"/rangetouch.js", HTTP_GET, get_rangetouch_js_handler},
    {"/json/si", HTTP_GET, get_json_si_js_handler},
    {"/json/effects", HTTP_GET, get_json_effects_js_handler},
    {"/json/palettes", HTTP_GET, get_json_palettes_js_handler},
    // {"/favicon.ico", HTTP_GET, not_found_url_handler},

    /* POST ROUTES */
    {"/", HTTP_POST, post_handler},
    {"/json/si", HTTP_POST, post_handler},
};

#define NUM_LED_ROUTES (sizeof(routes) / sizeof(Route))

void server_init()
{
    static httpd_handle_t server = NULL;

    // ESP_ERROR_CHECK(nvs_flash_init());
    // ESP_ERROR_CHECK(esp_netif_init());
    // ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Register event handlers to start server when Wi-Fi or Ethernet is connected,
     * and stop server when disconnection happens.
     */
    #define CONFIG_EXAMPLE_CONNECT_WIFI
    #ifdef CONFIG_EXAMPLE_CONNECT_WIFI
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, &server));
    #endif // CONFIG_EXAMPLE_CONNECT_WIFI

}

httpd_handle_t start_webserver(void)
{
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");
    httpd_handle_t server = NULL;
    
    // httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    // config.httpd.max_uri_handlers = 10;
    // config.httpd.max_open_sockets = 6;
    // config.session_tickets = true;
    // config.transport_mode = HTTPD_SSL_TRANSPORT_INSECURE;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 10;

    /*certificates and key*/
    // extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    // extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    // config.cacert_pem = cacert_pem_start;
    // config.cacert_len = cacert_pem_end - cacert_pem_start;

    // extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    // extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    // config.prvtkey_pem = prvtkey_pem_start;
    // config.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    // esp_err_t ret = httpd_ssl_start(&server, &config);
    esp_err_t ret = httpd_start(&server, &config);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Register the routes in the HTTP server
    ESP_LOGI(TAG, "Registering URI handlers");    
    for (int i = 0; i < NUM_LED_ROUTES; i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }
    return server;
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_ssl_stop(server);
}

void adder_free_func(void *ptr) {
    free(ptr);
    ESP_LOGI(TAG, "Session context freed");
}

void populate_response(enum webpage_name page, httpd_req_t *req)
{
    char* src_res = NULL;
    
    if (!req->sess_ctx) {
        ESP_LOGI(TAG, "/adder allocating new session");
        req->sess_ctx = malloc(sizeof(int));
        req->free_ctx = adder_free_func;
        *(int *)req->sess_ctx = 0;
    }
    int *ctx_data = (int *)req->sess_ctx;

    unsigned int content_size_int = 0;
    const unsigned char *hex_dump = NULL;
    ESP_LOGI("POPULAITNG RESP","%d", page);
    // vTaskDelay(10000/portTICK_PERIOD_MS);
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    switch (page)
    {
    case INDEX_HTML:
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        content_size_int = gzip_index_html_gz_len;
        hex_dump = gzip_index_html_gz;
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 19);
        #endif
        break;
    case INDEX_CSS:
        httpd_resp_set_type(req, "text/css");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        content_size_int = gzip_index_css_gz_len;
        hex_dump = gzip_index_css_gz;
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 32);
        #endif
        break;
    case INDEX_JS:
        httpd_resp_set_type(req, "text/javascript");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        content_size_int = gzip_index_js_gz_len;
        hex_dump = gzip_index_js_gz;
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 103);
        #endif
        break;    
    case IRO_JS:
        httpd_resp_set_type(req, "text/javascript");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        content_size_int = gzip_iro_js_gz_len;
        hex_dump = gzip_iro_js_gz;
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 103);
        #endif
        break;
    case RANGETOUCH_JS:
        httpd_resp_set_type(req, "text/javascript");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        content_size_int = gzip_rangetouch_js_gz_len;
        hex_dump = gzip_rangetouch_js_gz;
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 103);
        #endif
        break;
    case JSON_SI:
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024 * 10);
        #endif

        memset((void *)response_buffer, 0, sizeof(response_buffer));
        char *json = json_update_config();
        sprintf(response_buffer, json);
        httpd_resp_send(req, response_buffer, strlen(json));
        free(json);
        return;
    case JSON_EFFECTS:
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024);
        #endif
        memset((void *)response_buffer, 0, sizeof(response_buffer));
        create_list_json_string(&src_res, EFFECT_JSON);
        if(src_res != NULL)
        {
            strcpy(response_buffer, src_res);
            // printf("response buffer %s", response_buffer);
        }
        else
        {
            printf("response buffer is NULL");
        }
        // sprintf(response_buffer, "[\"Solid\", \"Blink\", \"Breathe\", \"Wipe\", \"Wipe Random\", \"Random Colors\", \"Sweep\", \"Dynamic\", \"Colorloop\", \"Rainbow\", \"Scan\", \"Dual Scan\", \"Fade\", \"Chase\", \"Chase Rainbow\", \"Running\", \"Saw\", \"Twinkle\", \"Dissolve\", \"Dissolve Rnd\", \"Sparkle\", \"Dark Sparkle\", \"Sparkle+\", \"Strobe\", \"Strobe Rainbow\", \"Mega Strobe\", \"Blink Rainbow\", \"Android\", \"Chase\", \"Chase Random\", \"Chase Rainbow\", \"Chase Flash\", \"Chase Flash Rnd\", \"Rainbow Runner\", \"Colorful\", \"Traffic Light\", \"Sweep Random\", \"Running 2\", \"Red & Blue\",\"Stream\", \"Scanner\", \"Lighthouse\", \"Fireworks\", \"Rain\", \"Merry Christmas\", \"Fire Flicker\", \"Gradient\", \"Loading\", \"In Out\", \"In In\", \"Out Out\", \"Out In\", \"Circus\", \"Halloween\", \"Tri Chase\", \"Tri Wipe\", \"Tri Fade\", \"Lightning\", \"ICU\", \"Multi Comet\", \"Dual Scanner\", \"Stream 2\", \"Oscillate\", \"Pride 2015\", \"Juggle\", \"Palette\", \"Fire 2012\", \"Colorwaves\", \"BPM\", \"Fill Noise\", \"Noise 1\", \"Noise 2\", \"Noise 3\", \"Noise 4\", \"Colortwinkle\", \"Lake\", \"Meteor\", \"Smooth Meteor\", \"Railway\", \"Ripple\"]");
        httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
        free(src_res);
        return;
    case JSON_PALETTES:
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        #ifdef USE_MALLOC
        response_buffer = (char *)calloc(sizeof(char),  1024);
        #endif
        memset((void *)response_buffer, 0, sizeof(response_buffer));

        create_list_json_string(&src_res, PALETTE_JSON);
        if(src_res != NULL)
        {
            strcpy(response_buffer, src_res);
            // printf("response buffer %s", response_buffer);
        }
        else
        {
            printf("response buffer is NULL");
        }

        // sprintf(response_buffer, "[\"Default\", \"Random Cycle\", \"Primary Color\", \"Based on Primary\", \"Set Colors\", \"Based on Set\", \"Party\", \"Cloud\", \"Lava\", \"Ocean\", \"Forest\", \"Rainbow\", \"Rainbow Bands\", \"Sunset\", \"Rivendell\", \"Breeze\", \"Red & Blue\", \"Yellowout\", \"Analogous\", \"Splash\", \"Pastel\", \"Sunset 2\", \"Beech\", \"Vintage\", \"Departure\", \"Landscape\", \"Beach\", \"Sherbet\", \"Hult\", \"Hult 64\", \"Drywet\", \"Jul\", \"Grintage\", \"Rewhi\", \"Tertiary\", \"Fire\", \"Icefire\", \"Cyane\", \"Light Pink\", \"Autumn\", \"Magenta\", \"Magred\", \"Yelmag\", \"Yelblu\", \"Orange & Teal\", \"Tiamat\", \"April Night\"]");
        httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
        free(src_res);
        return;
    case NOT_FOUND:
        httpd_resp_set_type(req, "application/json");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        memset((void *)response_buffer, 0, sizeof(response_buffer));
        sprintf(response_buffer, "NOT FOUND");
        httpd_resp_send(req, response_buffer, HTTPD_RESP_USE_STRLEN);
        return;
    default:
        break;
    }

    // memset((void *)response_buffer, 0, sizeof(response_buffer));

    // Convert file size to string using snprintf
    // char content_length_str[16];
    // snprintf(content_length_str, sizeof(content_length_str), "%d", content_size_int);
    // httpd_resp_set_hdr(req, "Content-Length", content_length_str);

    // size_t chunk_size = 1024;
    // size_t offset = 0, dump_size = content_size_int;

    // // Send chunks using a while loop
    // while (offset < dump_size) {
    //     size_t remaining_size = dump_size - offset;
    //     size_t current_chunk_size = remaining_size < chunk_size ? remaining_size : chunk_size;

    //     // Get the current chunk
    //     const unsigned char* current_chunk = hex_dump + offset;

    //     // Send the chunk
    //     httpd_resp_send(req, (const char*)current_chunk, content_size_int);

    //     // Update offset for the next chunk
    //     offset += current_chunk_size;
    // }

    httpd_resp_send(req, (const char*)hex_dump, content_size_int);


    // End the response after sending all chunks
    // httpd_resp_send_chunk(req, NULL, 0);
}
