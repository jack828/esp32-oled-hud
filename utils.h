#ifndef Utils_h
#define Utils_h

#include <WString.h>

String stringFromXMLTag (String xml, String tag);
double doubleFromXMLTag (String xml, String tag);
long longFromXMLTag (String xml, String tag);
String formatBytes(double bytes);
String formatUptime(long uptime);

#endif
