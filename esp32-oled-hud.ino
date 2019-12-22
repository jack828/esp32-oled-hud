#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include "credentials.h"
#include "font.h"

WiFiMulti wifi;

HTTPClient http;

SSD1306Wire display(0x3c, 5, 4);

OLEDDisplayUi ui(&display);

String csrfToken = "";

// Device
String model = "";
String firmwareVersion = "";
String uptime = "";

// Storage
String capacity = "";
String available = "";
String used = "";
String percentFree = "";

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(Monospaced_plain_10);
  display->drawString(128, 0, String(millis()));
}

void titleOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0, 0, (char*)state->userData);
}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Device";
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0 + x, 10 + y, "Model:   " + model);
  display->drawString(0 + x, 20 + y, "Firmware: " + firmwareVersion);
  display->drawString(0 + x, 30 + y, "Uptime:   " + uptime);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Storage";
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawString(0 + x, 10 + y, "Capacity: " + capacity);
  display->drawString(0 + x, 20 + y, "Free:     " + available);
  display->drawString(0 + x, 30 + y, "Used:     " + used);
  display->drawString(0 + x, 40 + y, "% Free:   " + percentFree);
}

LoadingStage loadingStages[] = {
  {
    .process = "Connecting to WiFi",
    .callback = []() {
      wifi.addAP(WIFI_SSID_1, WIFI_PSK_1);
      wifi.addAP(WIFI_SSID_2, WIFI_PSK_2);
      wifi.addAP(WIFI_SSID_3, WIFI_PSK_3);

      Serial.println("Connecting WiFi...");
      if (wifi.run() == WL_CONNECTED) {
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
      } else {
        Serial.println("Not connected, rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  },
  {
    .process = "Obtaining CSRF Token",
    .callback = []() {
      // TODO handle error
      getCsrfToken();
    }
  },
  {
    .process = "Retrieving device info",
    .callback = []() {
      getDeviceInfo();
    }
  },
  {
    .process = "Retrieving storage info",
    .callback = []() {
      getStorageStats();
    }
  }
};

int LOADING_STAGES_COUNT = sizeof(loadingStages) / sizeof(LoadingStage);

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2 };

// how many frames are there?
int FRAME_COUNT = sizeof(frames) / sizeof(FrameCallback);

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay, titleOverlay };
int OVERLAY_COUNT = sizeof(overlays) / sizeof(OverlayCallback);

boolean getCsrfToken () {
  Serial.println("[HTTP] GET csrf token");

  http.begin(READYNAS_CSRF_URI);
  http.setAuthorization(READYNAS_USERNAME, READYNAS_PASSWORD);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      int start = payload.indexOf("\"csrfpId\", \"") + 12;
      int end = payload.indexOf("\");", start);
      csrfToken = payload.substring(start, end);
      Serial.print("[HTTP] Got csrf token: ");
      Serial.println(csrfToken);
      return true;
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
  return false;
}

String stringFromXMLTag (String xml, String tag) {
  // TODO not found case
  String value = "";
  int start = xml.indexOf("<" + tag + ">") + tag.length() + 2;
  int end = xml.indexOf("</" + tag + ">", start);
  value = xml.substring(start, end);
  return value;
}

double doubleFromXMLTag (String xml, String tag) {
  // TODO not found case
  String value = stringFromXMLTag(xml, tag);
  return value.toDouble();
}

long longFromXMLTag (String xml, String tag) {
  // TODO not found case
  String value = stringFromXMLTag(xml, tag);
  return value.toInt();
}

String formatBytes(double bytes) {
  String output = "";
  double gigabytes = bytes / 1024 / 1024;
  if (gigabytes > 1024) {
    output = String(gigabytes / 1024, 2) + " TB";
  } else {
    output = String(gigabytes, 2) + " GB";
  }
  return output;
}

String formatUptime(long uptime) {
  long dayInSeconds = 24 * 60 * 60;
  long hourInSeconds = 60 * 60;
  long minuteInSeconds = 60; // Obviously
  int days = floor(uptime / dayInSeconds);
  int hours = floor((uptime % dayInSeconds) / hourInSeconds);
  int minutes = floor((uptime % hourInSeconds) / minuteInSeconds);
  return String(days) + "d " + String(hours) + "h " + String(minutes) + "m";
}

void getStorageStats() {
  Serial.println("[HTTP] GET storage stats");

  http.begin(READYNAS_DBBROKER_URI);
  http.setAuthorization(READYNAS_USERNAME, READYNAS_PASSWORD);
  http.addHeader("csrfpid", csrfToken);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST("<?xml version=\"1.0\" encoding=\"UTF-8\"?><xs:nml xmlns:xs=\"http://www.netgear.com/protocol/transaction/NMLSchema-0.9\" xmlns=\"urn:netgear:nas:readynasd\" src=\"dpv_1576963606000\" dst=\"nas\"><xs:transaction id=\"njl_id_465\"><xs:get id=\"njl_id_464\" resource-id=\"Volumes\" resource-type=\"Volume_Collection\"/></xs:transaction></xs:nml>");

  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      double capacityBytes = doubleFromXMLTag(payload, "Capacity");
      double availableBytes = doubleFromXMLTag(payload, "Available");
      double usedBytes = capacityBytes - availableBytes;

      capacity = formatBytes(capacityBytes);
      available = formatBytes(availableBytes);
      used = formatBytes(usedBytes);

      percentFree = String((availableBytes / capacityBytes) * 100, 2) + " %";

      Serial.println(capacity);
      Serial.println(available);
      Serial.println(used);
      Serial.println(percentFree);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void getDeviceInfo() {
  Serial.println("[HTTP] GET device info");

  http.begin(READYNAS_DBBROKER_URI);
  http.setAuthorization(READYNAS_USERNAME, READYNAS_PASSWORD);
  http.addHeader("csrfpid", csrfToken);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  int httpCode = http.POST("<?xml version=\"1.0\" encoding=\"UTF-8\"?><xs:nml xmlns:xs=\"http://www.netgear.com/protocol/transaction/NMLSchema-0.9\" xmlns=\"urn:netgear:nas:readynasd\" src=\"dpv_1576970265000\" dst=\"nas\"><xs:transaction id=\"njl_id_29\"><xs:get id=\"njl_id_28\" resource-id=\"SystemInfo\" resource-type=\"SystemInfo\"></xs:get></xs:transaction></xs:nml>");

  if (httpCode > 0) {
    Serial.printf("[HTTP] GET... code: %d\n", httpCode);

    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
      model = stringFromXMLTag(payload, "Model");
      firmwareVersion = stringFromXMLTag(payload, "Firmware_Version");
      long uptimeSeconds = longFromXMLTag(payload, "System_Uptime");

      uptime = formatUptime(uptimeSeconds);

      Serial.println(model);
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void initialiseUi () {
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  ui.setIndicatorPosition(BOTTOM);

  ui.setIndicatorDirection(LEFT_RIGHT);

  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, FRAME_COUNT);

  ui.setOverlays(overlays, OVERLAY_COUNT);

  ui.init();

  ui.runLoadingProcess(loadingStages, LOADING_STAGES_COUNT);

  // If needed
  /* display.flipScreenVertically(); */
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booted.");
  initialiseUi();
}

void loop() {
  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    if (wifi.run() != WL_CONNECTED) {
      Serial.println("WiFi not connected!");
    }
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }
}
