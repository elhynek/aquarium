//WiFi module library
#include <ESP8266WiFi.h>

//Temperature sensor DS18B20 libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ANALOG_READ A0 //pin to read analog data from
#define PIN_TDS_VCC D3     //pin to power TDS sensor
#define PIN_TEMP_DAT D4    //pin to read temperature data (digital)
#define PIN_PH_VCC D5      //pin to power pH sensor
#define RELAY_PIN    D7    //pin to control relay for lights

#define VREF 3.3           //analog reference voltage (V)

//Count of measurements to get median from
#define MCOUNT 10

//Predefined waiting times
#define PAUSE_LOOP 300000 //5 minutes
#define PAUSE_MEASUREMENT 10000 //10 seconds
#define PAUSE_COOLDOWN 10000 //10 seconds

//Create instance of oneWireDS from OneWire library
OneWire oneWireDS(PIN_TEMP_DAT);
//Create instance of sensorDS from DallasTemperature
DallasTemperature sensorDS(&oneWireDS);

float g_measurements_buffer[MCOUNT];
bool g_current_lights_on = false;

const float ph_slope_line_value = -5.70; 
const float ph_calibration_value = 21.34 + 2.0;

const char* wifi_SSID = "Your WiFi SSID";
const char* wifi_PASS = "Your WiFi password";
const char* server = "192.168.0.25"; //IP of backend server
const char* serverTS = "api.thingspeak.com";
const String tsAPIkey = "Your ThinkSpeak API KEY";

WiFiClient client;
 
void setup() {
  //Serial logger
  Serial.begin(115200);

  //Turn on communication with temperature sensor
  sensorDS.begin();
  
  //Set pins as Vcc output for multiplexing and set them to 0
  pinMode(PIN_PH_VCC,OUTPUT);
  pinMode(PIN_TDS_VCC,OUTPUT);
  pinMode(RELAY_PIN,OUTPUT);
  digitalWrite(PIN_PH_VCC, LOW);
  digitalWrite(PIN_TDS_VCC, LOW);
  digitalWrite(RELAY_PIN, LOW);
  
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
    sensorDS.requestTemperatures();
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
    g_measurements_buffer[i] = analogRead(PIN_ANALOG_READ) * VREF / 1024;
    delay(PAUSE_MEASUREMENT);
  }
  //val_ph = 3.5 * getMedian(g_measurements_buffer);
  val_ph = ph_slope_line_value * getMedian(g_measurements_buffer) + ph_calibration_value;
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
  float avg_voltage = getMedian(g_measurements_buffer) * (float)VREF / 1024.0;
  float compensationCoefficient = 1.0 + 0.02 * (val_temp - 25.0);
  float compensationVoltage = avg_voltage / compensationCoefficient;
  val_tds = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage - 255.86 * compensationVoltage * compensationVoltage + 857.39 * compensationVoltage) * 0.5;
  
  Serial.print("TDS Value: ");
  Serial.println(val_tds);


  //send data to aquaserver
  if (client.connect(server,3000)) {
    sendPostRequest(val_temp, val_ph, val_tds);
  }
  client.stop();
  
  //send data to thingspeak
  if (client.connect(serverTS,80)) {
    sendThingSpeakRequest(val_temp, val_ph, val_tds);
  }
  client.stop();
  
  //lights management
  String lightMsg;
  bool lightsOn;
  if (client.connect(server,3000)) {
    lightMsg = sendGetRequest();
  }
  client.stop();

  lightsOn = isLightOn(getTimeOn(lightMsg), getCurrentTime(lightMsg), getTimeOff(lightMsg));
  
  if(lightsOn && !g_current_lights_on){
    //turn on the lights
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Turning on the lights.");
    g_current_lights_on = true;
  }else if(!lightsOn && g_current_lights_on){
    //turn off the lights
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Turning off the lights.");
    g_current_lights_on = false;
  }
  
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
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(msg.length());
  client.print("\n\n");
  client.print(msg);

  Serial.print("Aquarium POST sent: ");
  Serial.print(msg);
}

void sendThingSpeakRequest(float val_temp, float val_ph, float val_tds){
  String msg = "field1=";
  msg += String(val_temp);
  msg +="&field2=";
  msg += String(val_ph);
  msg +="&field3=";
  msg += String(val_tds);
  msg += "\r\n\r\n";
    
  client.print("POST /update HTTP/1.1\n");
  client.print("Host: api.thingspeak.com\n");
  client.print("Connection: close\n");
  client.print("X-THINGSPEAKAPIKEY: " + tsAPIkey + "\n");
  //client.print("X-THINGSPEAKAPIKEY: ");
  //client.print(tsAPIkey);
  //client.print("\n");
  client.print("Content-Type: application/x-www-form-urlencoded\n");
  client.print("Content-Length: ");
  client.print(msg.length());
  client.print("\n\n");
  client.print(msg);

  Serial.print("ThingSpeak POST sent: ");
  Serial.println(msg);
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

String sendGetRequest(){
  String result = "";
  client.print("GET /lightTimes HTTP/1.1\n");
  client.print("Host: 192.168.0.25:3000\n");
  client.println("Connection: close");
  client.println();
  
  while(client.connected()) {
      if(client.available()){
        char c = client.read();
        result.concat(c);
      }
  }
  return result;
}

//check if lights should be on
bool isLightOn(String timeOn, String currentTime, String timeOff){
  int minutesOn = timeToMinutes(timeOn);
  int minutesCur = timeToMinutes(currentTime);
  int minutesOff = timeToMinutes(timeOff);

  //turnOff after midnight
  if(minutesOff < minutesOn){
    if(minutesOn <= minutesCur || minutesCur < minutesOff){
      return true;
    }else{
      return false;
    }
  }else{
    if(minutesOn <= minutesCur && minutesCur < minutesOff){
      return true;
    }else{
      return false;
    }
  }
  return false;
}

//parse HH:mm to minutes from 00:00
int timeToMinutes(String ttime){
  int minutes = 0;
  minutes += 60 * ttime.substring(0, 2).toInt();
  minutes += ttime.substring(3, 5).toInt();
  return minutes;
}

String getCurrentTime(String lightMsg){
  int dateIndex;
  int gmtIndex;
  String dateTime;
  
  dateIndex = lightMsg.indexOf("Date:");
  gmtIndex = lightMsg.indexOf("GMT", dateIndex);
  dateTime = lightMsg.substring(gmtIndex-9, gmtIndex-4);
  return dateTime;
}

String getTimeOn(String lightMsg){
  int timeOnIndex;
  String timeOn;
  timeOnIndex = lightMsg.indexOf("timeOn");
  timeOn = lightMsg.substring(timeOnIndex+9, timeOnIndex+14);
  return timeOn;
}
String getTimeOff(String lightMsg){
  int timeOffIndex;
  String timeOff;
  
  timeOffIndex = lightMsg.indexOf("timeOff");
  timeOff = lightMsg.substring(timeOffIndex+10, timeOffIndex+15);
  return timeOff;
}
