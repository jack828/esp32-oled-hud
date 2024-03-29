#include <Wire.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"
#include "images.h"
#include "credentials.h"
#include "definitions.h"
#include "font.h"
#include "utils.h"
#include "network.h"

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "uk.pool.ntp.org", 0, 1000);
WiFiMulti wifi;

SSD1306Wire display(0x3c, 5, 4);
OLEDDisplayUi ui(&display);

void timeOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(Monospaced_plain_10);
  display->drawString(128, 0, timeClient.getFormattedTime());
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
    .process = "Initialising NTP",
    .callback = []() {
      timeClient.begin();
      while (!timeClient.update()) {
        timeClient.forceUpdate();
      }
      Serial.println(timeClient.getFormattedTime());
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
OverlayCallback overlays[] = { timeOverlay, titleOverlay };
int OVERLAY_COUNT = sizeof(overlays) / sizeof(OverlayCallback);


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
