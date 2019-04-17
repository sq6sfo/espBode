#include <ESP8266WiFi.h>
#include "esp_network.h"
#include "esp_config.h"
#include "esp_fy6800.h"

const char * ssid = STASSID; // your network SSID (name)
const char * pass = STAPSK;  // your network password

WiFiServer rpc_server(RPC_PORT);
WiFiServer lxi_server(LXI_PORT);

void setup() {
    IPAddress ip(192,168,1,6);
    IPAddress gateway(192,168,1,1);
    IPAddress subnet(255,255,255,0);

    Serial.begin(115200);

    // We start by connecting to a WiFi network
    DEBUG("Connecting to ");
    DEBUG(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.config(ip, gateway, subnet);

    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        DEBUG(".");
    }

    DEBUG("WiFi connected");
    DEBUG("IP address: ");
    DEBUG(WiFi.localIP());

    rpc_server.begin();
    lxi_server.begin();

    // initDevice();


    // IPAddress debug_ip(192,168,1,112);
    // debug_client.connect(debug_ip, 123);

    // while(!debug_client.availableForWrite());
    // debug_client.write((uint8_t*)"KACZKA", 6);
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

