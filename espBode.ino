#include <ESP8266WiFi.h>
#include "esp_network.h"
#include "esp_config.h"
#include "esp_fy6800.h"

WiFiServer rpc_server(RPC_PORT);
WiFiServer lxi_server(LXI_PORT);

void setup() {

    Serial.begin(115200);

    // We start by connecting to a WiFi network
    DEBUG("Connecting to ");
    DEBUG(WIFI_SSID);

#if defined(STATIC_IP)
    IPAddress ip(ESP_IP);
    IPAddress mask(ESP_MASK);
    IPAddress gateway(ESP_GW);
    WiFi.config(ip, gateway, mask);
#endif

#if defined(WIFI_MODE_CLIENT)
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PSK);
#elif defined(WIFI_MODE_AP)
    Wifi.softAp(WIFI_SSID, WIFI_PSK);
#else
    #error PLEASE SELECT WIFI_MODE_AP OR WIFI_MODE_CLIENT!
#endif

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG(".");
    }

    DEBUG("WiFi connected");
    DEBUG("IP address: ");
    DEBUG(WiFi.localIP());

    rpc_server.begin();
    lxi_server.begin();
}
uint8_t output = 0;


void loop() {
    WiFiClient rpc_client;
    WiFiClient lxi_client;

    lxi_client.setTimeout(1000);

    do
    {
        rpc_client = rpc_server.available();
    }
    while(!rpc_client);
    DEBUG("RPC CONNECTION.");

    handlePacket(rpc_client);
    rpc_client.stop();

    do
    {
        lxi_client = lxi_server.available();
    }
    while(!lxi_client);
    DEBUG("LXI CONNECTION.");

    while(1)
    {
        if(0 != handlePacket(lxi_client))
        {
            lxi_client.stop();
            DEBUG("RESTARTING");
            return;
        }
    }
}

