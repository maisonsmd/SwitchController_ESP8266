bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}
void handleRoot() {
  Serial.println("Enter handleRoot");
  String header;
  if (is_authentified()) {
    handleWifi();
    //header = "HTTP/1.1 301 OK\r\nLocation: /wifi\r\nCache-Control: no-cache\r\n\r\n";
    //server.sendContent(header);
    return;
  }
  header = "HTTP/1.1 301 OK\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
  server.sendContent(header);
  return;
}
void handleLogin() {
  String message;
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")) {
    Serial.println("Disconnection");
    String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=0\r\nLocation: /login\r\nCache-Control: no-cache\r\n\r\n";
    server.sendContent(header);
    return;
  }
  if (is_authentified()) { //Move to /wifi if is signed in
    handleWifi();
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    if (server.arg("USERNAME") == loginAccount &&  server.arg("PASSWORD") == loginPassword) {
      String header = "HTTP/1.1 301 OK\r\nSet-Cookie: ESPSESSIONID=1\r\nLocation: /\r\nCache-Control: no-cache\r\n\r\n";
      server.sendContent(header);
      Serial.println("Log in Successful");
      return;
    }
    message = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html><head><title>Login page</title><style> body{"
                   "font-family: 'Open Sans', sans-serif;"
                   "font-size: 200%;"
                   "text-align: center;"
                   "margin: 0;"
                   "width: 100%;"
                   "height: 100%;"
                   "background-color: #dddddd;}.header{"
                   "float: center;"
                   "color: white;"
                   "padding-top: 1%;"
                   "padding-bottom: 1%;"
                   "background-color: #428bca;}.body{"
                   "float: top;"
                   "padding-top: 10%;"
                   "z-index: 0;}input{"
                   "width: 400px;"
                   "color: black;"
                   "font-size: 150%;"
                   "background-color: #ffffff;"
                   "border-bottom: none;"
                   "border-radius: 10px;"
                   "text-align: center;"
                   "padding: 10px 10px;"
                   "margin-bottom: 15px;}button{margin: 50px 50px; margin bottom: 50px; width: 400px; border: none;border-radius: 10px;background-color: #4CAF50;color: white;"
                   "font-family: 'Open Sans', sans-serif;font-size: 150%;padding: 10px 10px;cursor: pointer;"
                   "}</style></head><body><div class=\"header\"><h1>Authentification</h1>"
                   "</div><div class=\"body\"><form action='/login' method='POST'>"
                   "<input type='text' name='USERNAME' placeholder='username'></input><br>"
                   "<input type='password' name='PASSWORD' placeholder='password'></input><br>"
                   "<button type='submit' name='SUBMIT'>Log in &#x00bb</button><br><p style=\"color: #aa2a2a; background-color: #eeeeee;\">"
                   + message + "</p><br></form></div></body></html>";
  server.send(200, "text/html", content);
}
