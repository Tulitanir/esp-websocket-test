#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include "WiFiCreds.h"

// Create server and websocket objects
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void printRawData(const uint8_t *data, size_t len)
{
  for (size_t i = 0; i < len; i++)
  {
    Serial.printf("%d ", data[i]);
  }
  Serial.println();
}

// Function to handle incoming WebSocket messages
void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("WebSocket client #%u connected\n", client->id());
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_BINARY)
    {
      Serial.printf("Received binary data from client #%u, length: %u bytes\n", client->id(), len);
      Serial.print("Data (RAW): ");
      printRawData(data, len);
      client->binary(data, len);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  if (!SPIFFS.begin(true))
  {
    Serial.println("Failed to mount or format SPIFFS");
    return;
  }

  // Set ESP32 hostname
  if (!WiFi.setHostname(hostname))
  {
    Serial.println("Failed to set hostname");
  }

  WiFi.begin(ssid, password);
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 10)
  {
    delay(1000);
    retryCount++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Failed to connect to WiFi. Rebooting...");
    ESP.restart();
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.begin();
}

void loop()
{
  ws.cleanupClients();
}
