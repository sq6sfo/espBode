#include <ESP8266WiFi.h>
#include "esp_network.h"
#include "esp_config.h"

#include "ESPTelnet.h"

#if AWG == FY6800
  #include "esp_fy6800.h"
#elif AWG == FY6900
  #include "esp_fy6900.h"
#else
  #error "Please select an AWG in esp_config.h"
#endif

WiFiServer rpc_server(RPC_PORT);
WiFiServer lxi_server(LXI_PORT);
ESPTelnet telnet;

void setup() {

    Serial.begin(115200);

    pinMode(LED_BUILTIN, OUTPUT);

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
    WiFi.softAP(WIFI_SSID, WIFI_PSK);
#else
    #error PLEASE SELECT WIFI_MODE_AP OR WIFI_MODE_CLIENT!
#endif

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG(".");
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }

    DEBUG("WiFi connected");
    DEBUG("IP address: ");
    DEBUG(WiFi.localIP().toString());

    telnet.begin();

    rpc_server.begin();
    lxi_server.begin();
}

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
        telnet.loop();        
        if(0 != handlePacket(lxi_client))
        {
            lxi_client.stop();
            DEBUG("RESTARTING");
            return;
        }else{
          // Lets give the user some feedback
          digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        }
    }        
}
