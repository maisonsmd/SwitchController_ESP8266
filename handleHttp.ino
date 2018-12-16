#include <Arduino.h>

boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

/** Wifi config page handler */
void handleWifi() {
  if (!is_authentified()) {
    handleLogin();
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent("<html><head><title>Config</title> <style>body"
                     "{font-family: 'Open Sans', sans-serif;"
                     "font-size: 200%;"
                     "text-align: center;"
                     "margin: 0;"
                     "width: 100%;"
                     "height: 100%;"
                     "background-color: #646464;}"
                     "table"
                     "{font-family: 'Open Sans', sans-serif;"
                     "border-collapse: collapse;"
                     "width: 100%;}"
                     "td, th {border: 1px solid #dddddd;"
                     "text-align: center;"
                     "padding: 15px;}"
                     "tr:nth-child(even) {background: rgba(255,255,255,0.7);}"
                     "tr:nth-child(odd) {background: rgba(200,200,200,0.7);}"
                     ".header{float: center;"
                     "color: #5379fa;"
                     "padding-top: 1%;"
                     "padding-bottom: 1%;"
                     "color: white;"
                     "background-color: #428bca;}"
                     ".body{float: top;"
                     "z-index: 0;}"
                     "button{"
                     "width: 400px;"
                     "border: none;"
                     "background-color: #428bca;"
                     "margin: 50px 50px;"
                     "padding: 10px 10px;"
                     "border-radius: 10px;"
                     "color: white;"
                     "text-decoration: none;"
                     "font-family: 'Open Sans', sans-serif;"
                     "font-size: 100%;"
                     "cursor: pointer; }"
                     "p{margin-bottom: 0;"
                     "font-size: 150%;"
                     "text-align: left;"
                     "color: white;} </style>"
                     "</head><body><div class=\"header\">"
                     "<h1>WiFi Connections</h1></div><div class=\"body\"><body>"
                     "<p>Current status</p>");
  //Table 1
  server.sendContent("<table>"
                     "<tr style=\"background-color: #4CAF50;color: white;font-size: 130%;\">"
                     "<th>Type</th>"
                     "<th>SSID</th>"
                     "<th>IP address</th>"
                     "</tr>"
                     "<tr><td>Hotspot</td><td>" + String(softAP_ssid) + "</td><td>" + toStringIp(WiFi.softAPIP()) + "</td></tr>"
                     "<tr><td>WiFi</td><td>" + ssid + "</td><td>" + toStringIp(WiFi.localIP()) + "</td></tr>"
                     "<tr><td>UID</td><td><a href=\"javascript:void(0);\" onclick=\"changeUid();\">" + String(uid) + "</td> <td></td></tr></table>");

  //Table 2
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");

  server.sendContent("<p>Available networks (" + String(n) + " found)</p>"
                     "<table>"
                     "<tr style=\"background-color: #4CAF50;color: white; font-size: 130%;\">"
                     "<th>No.</th>"
                     "<th>SSID</th>"
                     "<th>Encryption</th>"
                     "<th>Signal strengh</th>"
                     "</tr>");

  if (n > 0) {
    for (int i = 0; i < n; i++) {
      String connType = "NONE";
      switch (WiFi.encryptionType(i)) {
        case ENC_TYPE_NONE:
          connType = "open network";
          break;
        case ENC_TYPE_WEP:
          connType = "WEP";
          break;
        case ENC_TYPE_TKIP:
          connType = "WPA / PSK";
          break;
        case ENC_TYPE_CCMP:
          connType = "WPA2 / PSK";
          break;
        case ENC_TYPE_AUTO:
          connType = "WPA / WPA2 / PSK";
          break;
      }
      String signalStrength = String(constrain(2 * (WiFi.RSSI(i) + 100), 0, 100)) + "%";
      server.sendContent("<tr><td>" + String(i + 1) + "</td><td><a href=\"javascript:void(0);\" onclick=\"connect(this.innerText);\">" + WiFi.SSID(i) + "</td><td>" + connType + "</td><td>" + signalStrength + "</td>\
</tr>");
    }
  }
  server.sendContent("</table><br>"
                     "<form style=\"display: none;\" id=\"targetForm\" method='POST' action='wifisave'>"
                     "<input id=\"targetID\" name=\"netName\">"
                     "<input id=\"targetPW\" name=\"netPW\">"
                     "</form>"
                     "<form style=\"display: none;\" id=\"uidForm\" method='POST' action='uidsave'>"
                     "<input id=\"newUid\" name=\"uid\">"
                     "</form>"
                     "<button style=\"font-size:150%;\" onclick=\"location.href='/login?DISCONNECT=YES'\">&#x00ab Sign out</button></div>"
                     "</body><script>function connect(text){var input = prompt(\"Enter password for \\\"\"+text+\"\\\"\");"
                     "if(input === null) return;"
                     "document.getElementById(\"targetID\").value = text;"
                     "document.getElementById(\"targetPW\").value = input;"
                     "alert(\"Please wait! Reload this page in seconds\");"
                     "document.getElementById(\"targetForm\").submit();}"

                     "function changeUid(){var input = prompt(\"enter new UID:\");"
                     "if(input == null) return;"
                     "document.getElementById(\"newUid\").value = input;"
                     "document.getElementById(\"uidForm\").submit();}"
                     "</script></html>");
  server.client().stop(); // Stop is needed because we sent no content length
}

void handleUidSave() {
  if (!is_authentified()) {
    handleLogin();
    return;
  }
  Serial.println("uid update");
  server.arg("uid").toCharArray(uid, sizeof(uid) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
}
/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {
  if (!is_authentified()) {
    handleLogin();
    return;
  }
  Serial.println("wifi save");
  server.arg("netName").toCharArray(ssid, sizeof(ssid) - 1);
  server.arg("netPW").toCharArray(password, sizeof(password) - 1);
  server.sendHeader("Location", "wifi", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  saveCredentials();
  connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}
