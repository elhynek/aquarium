#include <ESP8266WiFi.h>

const char* wifi_SSID = "a";
const char* wifi_PASS = "a";
const char* server = "192.168.0.25";


WiFiClient client;

 
void setup() {
  //serial logger
  Serial.begin(115200);
  
  WiFi.begin(wifi_SSID, wifi_PASS);
  
  //waiting for successful connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi");
  Serial.println(wifi_SSID);
  Serial.print("IP adresa: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  
  if (client.connect(server,3000)) {
    
	//temp, ph, tds values to be sent
    sendPostRequest(1, 1, 1);
    
  }
  
  client.stop();
  
  Serial.println("Pause before next data send");
  delay(30000);
}


void sendPostRequest(double val_temp, double val_ph, double val_tds){
  String msg = "temp=";
  msg += String(val_temp);
  msg +="&ph=";
  msg += String(val_ph);
  msg +="&tds=";
  msg += String(val_tds);
  msg += "\r\n\r\n";
    
  client.print("POST /aquarium HTTP/1.1\n");
  client.print("Host: 192.168.0.25:3000\n");
  //client.print("Connection: close\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(msg.length());
  client.print("\n\n");
  client.print(msg);

  Serial.print("POST sent: ");
  
}
