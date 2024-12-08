#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <SPIFFS.h>
#include "WiFiCreds.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_DATA)
  {
    if (len == (SCREEN_WIDTH * SCREEN_HEIGHT) / 8)
    { // Проверка размера кадра
      display.clearDisplay();
      display.drawBitmap(0, 0, data, SCREEN_WIDTH, SCREEN_HEIGHT, WHITE);
      display.display();
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
    Serial.println("Failed to connect to WiFi.");
  }

  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
    if (request->url().startsWith("/ffmpeg/"))
    {
      String fileName = request->url() + ".js";
      if (SPIFFS.exists(fileName))
      {
        request->send(SPIFFS, fileName, "text/javascript");
      }
      else
      {
        request->send(404, "text/plain", "File not found");
      }
    }
    else
    {
      request->send(404, "text/plain", "Not Found");
    } });

  server.begin();
}

void loop()
{
  ws.cleanupClients();
}
