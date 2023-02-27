#include <ESP8266WiFi.h>

#define PIN_TEMP_VCC D1
#define PIN_PH_VCC D2
#define PIN_TDS_VCC D3

#define MCOUNT 10

float g_measurements_buffer[MCOUNT]

const float ph_slope_line_value = -5.70; 
const float ph_calibration_value = 21.34;

const char* wifi_SSID = "SSID of your WiFi";
const char* wifi_PASS = "Password to your WiFi";
const char* server = "192.168.1.1"; //IP of backend server

const int g_section_pause = 10000;
const int g_measurement_pause = 100000;

WiFiClient client;

 
void setup() {
  //serial logger
  Serial.begin(115200);

  //set pins as Vcc output for multiplexing and set them to 0
  pinMode(PIN_TEMP_VCC,OUTPUT);
  pinMode(PIN_PH_VCC,OUTPUT);
  pinMode(PIN_TDS_VCC,OUTPUT);
  digitalWrite(PIN_TEMP_VCC, LOW);
  digitalWrite(PIN_PH_VCC, LOW);
  digitalWrite(PIN_TDS_VCC, LOW);

  
  WiFi.begin(wifi_SSID, wifi_PASS);
  
  //waiting for successful connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi");
  Serial.println(wifi_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {

  float val_temp = 0;
  float val_ph = 0;
  float val_tds = 0;

  //Temperature measurement
  digitalWrite(PIN_TDS_VCC, LOW);
  digitalWrite(PIN_TEMP_VCC, HIGH);
  delay(5000);
  for (byte i = 0; i < MCOUNT; i++){
    //Temperature Measurement

    delay(g_measurement_pause);
  }
  val_temp = getMedian(g_measurements_buffer);


  //pH measurement
  digitalWrite(PIN_TEMP_VCC, LOW);
  digitalWrite(PIN_PH_VCC, HIGH);
  delay(5000);
  for (byte i = 0; i < MCOUNT; i++){
    //pH Measurement with recalculation for pH 0 - 14
    g_measurements_buffer[i] = * 5.0 / 1024;
    delay(g_measurement_pause);
  }
  val_ph = ph_slope_line_value * getMedian(g_measurements_buffer) + ph_calibration_value;

  //TDS measurement
  digitalWrite(PIN_PH_VCC, LOW);
  digitalWrite(PIN_TDS_VCC, HIGH);
  delay(5000);
  for (byte i = 0; i < MCOUNT; i++){
    //TDS Measurement

    delay(g_measurement_pause);
  }
  val_tds = getMedian(g_measurements_buffer);



  //send data to aquaserver
  if (client.connect(server,3000)) {
    
    sendPostRequest(val_temp, val_ph, val_tds);
  }
  client.stop();

  //TODO send data to thingspeak


  
  Serial.println("Delay before next scan");
  delay(30000);
}


void sendPostRequest(float val_temp, float val_ph, float val_tds){
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
  //client.print("X-THINGSPEAKAPIKEY: "+apiKlic+"\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(msg.length());
  client.print("\n\n");
  client.print(msg);

  Serial.print("POST sent: ");
  //Serial.print(cas);
}

float getMedian(int arr[]){
  float tmp_arr[MCOUNT];
  for (byte i = 0; i < MCOUNT; i++)
    tmp_arr[i] = arr[i];
  int i, j;
  float result;
  for (j = 0; j < MCOUNT - 1; j++){
    for (i = 0; i < MCOUNT - j - 1; i++){
      if (tmp_arr[i] > tmp_arr[i + 1]){
        result = tmp_arr[i];
        tmp_arr[i] = tmp_arr[i + 1];
        tmp_arr[i + 1] = result;
      }
    }
  }
  if ((MCOUNT & 1) > 0)
    result = tmp_arr[(MCOUNT - 1) / 2];
  else
    result = (tmp_arr[MCOUNT / 2] + tmp_arr[MCOUNT / 2 - 1]) / 2;
  return result;

}
