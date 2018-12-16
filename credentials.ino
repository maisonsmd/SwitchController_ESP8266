
/** Load WLAN credentials from EEPROM */
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, uid);
  EEPROM.get(0 + sizeof(uid), ssid);
  EEPROM.get(0 + sizeof(uid) + sizeof(ssid), password);
  char ok[2 + 1];
  EEPROM.get(0 + sizeof(ssid) + sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
    uid[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.print("ssid: ");
  Serial.println(ssid);
  Serial.print("password: ");
  Serial.println(strlen(password) > 0 ? "********" : "<no password>");
  Serial.print("uid: ");
  Serial.println(uid);
}
/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, uid);
  EEPROM.put(0 + sizeof(uid), ssid);
  EEPROM.put(0 + sizeof(uid) + sizeof(ssid), password);
  char ok[2 + 1] = "OK";
  EEPROM.put(0 + sizeof(uid) + sizeof(ssid) + sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}
