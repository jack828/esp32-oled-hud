#include <WString.h>
#include <math.h>
#include "utils.h"

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
