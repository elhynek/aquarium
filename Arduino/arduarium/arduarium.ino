//WiFi module library
#include <ESP8266WiFi.h>

//Temperature sensor DS18B20 libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ANALOG_READ A0 //pin to read analog data from
#define PIN_TDS_VCC D3     //pin to power TDS sensor
#define PIN_TEMP_DAT D4    //pin to read temperature data (digital)
#define PIN_PH_VCC D5      //pin to power pH sensor

#define VREF 5.0           //analog reference voltage (V)

//Count of measurements to get median from
#define MCOUNT 10

//Predefined waiting times
#define PAUSE_LOOP 10000
#define PAUSE_MEASUREMENT 1000
#define PAUSE_COOLDOWN 5000

//Create instance of oneWireDS from OneWire library
OneWire oneWireDS(PIN_TEMP_DAT);
//Create instance of sensorDS from DallasTemperature
DallasTemperature sensorDS(&oneWireDS);

float g_measurements_buffer[MCOUNT];

const float ph_slope_line_value = -5.70; 
const float ph_calibration_value = 21.34;

const char* wifi_SSID = "PivniKralovstvi";
const char* wifi_PASS = "Prdelka555";
const char* server = "192.168.0.25"; //IP of backend server

WiFiClient client;
 
void setup() {
  //Serial logger
  Serial.begin(115200);

  //Turn on communication with temperature sensor
  sensorDS.begin();
  sensorDS.requestTemperatures();
  //Set pins as Vcc output for multiplexing and set them to 0
  pinMode(PIN_PH_VCC,OUTPUT);
  pinMode(PIN_TDS_VCC,OUTPUT);
  digitalWrite(PIN_PH_VCC, LOW);
  digitalWrite(PIN_TDS_VCC, LOW);
  
  //Set analog pin as input for measurements
  pinMode(PIN_ANALOG_READ, INPUT);
  
  WiFi.begin(wifi_SSID, wifi_PASS);
  
  //Waiting for successful connection
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

  float val_temp = 0.0;
  float val_ph = 0.0;
  float val_tds = 0.0;

  //Temperature measurement
  for (byte i = 0; i < MCOUNT; i++){
    //Temperature Measurement
    g_measurements_buffer[i] = sensorDS.getTempCByIndex(0);
    delay(PAUSE_MEASUREMENT);
  }
  val_temp = getMedian(g_measurements_buffer);
  Serial.print("Temperature: ");
  Serial.println(val_temp);

  //pH measurement
  digitalWrite(PIN_PH_VCC, HIGH);
  digitalWrite(PIN_TDS_VCC, LOW);
  delay(PAUSE_COOLDOWN);
  for (byte i = 0; i < MCOUNT; i++){
    //pH Measurement with recalculation for pH 0 - 14
    g_measurements_buffer[i] = analogRead(PIN_ANALOG_READ) * 5.0 / 1024;
    delay(PAUSE_MEASUREMENT);
  }
  val_ph = ph_slope_line_value * getMedian(g_measurements_buffer) + ph_calibration_value;
  val_ph *= -1;
  Serial.print("pH Value: ");
  Serial.println(val_ph);
  
  //TDS measurement
  digitalWrite(PIN_PH_VCC, LOW);
  digitalWrite(PIN_TDS_VCC, HIGH);
  delay(PAUSE_COOLDOWN);
  for (byte i = 0; i < MCOUNT; i++){
    g_measurements_buffer[i] = analogRead(PIN_ANALOG_READ);
    delay(PAUSE_MEASUREMENT);
  }
  //calculation of TDS based on manufacturer parameters and recommended compensations
  //float avg_voltage = getMedian(g_measurements_buffer) * (float)VREF / 1024;
  //float compensationCoefficient = 1.0 + 0.02 * (val_temp - 25.0);
  //float compensationVoltage = avg_voltage / compensationCoefficient;
  //val_tds = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  val_tds = getMedian(g_measurements_buffer);
  Serial.print("TDS Value: ");
  Serial.println(val_tds);


  //send data to aquaserver
  if (client.connect(server,3000)) {
    
    sendPostRequest(val_temp, val_ph, val_tds);
  }
  client.stop();

  //TODO send data to thingspeak


  
  Serial.println("Delay before next scan");
  delay(PAUSE_LOOP);
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
  Serial.print(msg);
}

float getMedian(float arr[]){
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
