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
  state->userData = (void *) "WiFi";
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y
  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Storage";
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Alignment";
  // Text alignment demo
  display->setFont(Monospaced_plain_10);

  // The coordinates define the left starting point of the text
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 11 + y, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128 + x, 33 + y, "Right aligned (128,33)");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Long Text";
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(Monospaced_plain_10);
  display->drawStringMaxWidth(0 + x, 10 + y, 128, "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore.");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  state->userData = (void *) "Empty";
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

// how many frames are there?
int frameCount = 5;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay, titleOverlay };
int overlaysCount = 2;


void initialiseUi () {
  // The ESP is capable of rendering 60fps in 80Mhz mode
  // but that won't give you much time for anything else
  // run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(60);

  // Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
}

String csrfToken = "";

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

double doubleFromXMLTag (String xml, String tag) {
  // TODO not found case
  String value = "";
  int start = xml.indexOf("<" + tag + ">") + tag.length() + 2;
  int end = xml.indexOf("</" + tag + ">", start);
  value = xml.substring(start, end);
  return value.toDouble();
}

void getDeviceStats() {
  Serial.println("[HTTP] GET device stats");

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
      double capacity = doubleFromXMLTag(payload, "Capacity");
      double available = doubleFromXMLTag(payload, "Available");
      Serial.println(capacity);
      Serial.println(capacity / 1024 / 1024 / 1024); // TB
      Serial.println(available);
      Serial.println(available / 1024 / 1024 / 1024); // TB
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booted.");

  wifi.addAP(WIFI_SSID_1, WIFI_PSK_1);
  wifi.addAP(WIFI_SSID_2, WIFI_PSK_2);
  wifi.addAP(WIFI_SSID_3, WIFI_PSK_3);

  Serial.println("Connecting Wifi...");
  if (wifi.run() == WL_CONNECTED) {
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Authenticate with ReadyNAS & get device stats
    if (getCsrfToken()) {
      getDeviceStats();
    }

  } else {
    Serial.println("Not connected, rebooting...");
    delay(1000);
    ESP.restart();
  }

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
