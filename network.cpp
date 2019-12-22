#include "network.h"
#include "credentials.h"
#include "definitions.h"
#include "utils.h"
#include <HTTPClient.h>

HTTPClient http;

String csrfToken = "";

// Device
String model;
String firmwareVersion;
String uptime;

// Storage
String capacity;
String available;
String used;
String percentFree;


bool getCsrfToken() {
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

void getStorageStats() {
  Serial.println("[HTTP] GET storage stats");

  http.begin(READYNAS_DBBROKER_URI);
  http.setTimeout(10000);
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
  http.setTimeout(10000);
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
