#ifndef LED_SERVER_H
#define LED_SERVER_H
#include <esp_https_server.h>

httpd_handle_t start_webserver(void);
void wifi_connection();

#endif //LED_SERVER_H