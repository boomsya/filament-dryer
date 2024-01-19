#include <ArduinoJson.h>
#include "DHTStable.h"
#include <FS.h>
#include <LittleFS.h> 
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
//#include <ESP8266mDNS.h>
#include <ESPAsyncWebServer.h>
//thermistor setting
#include <Thermistor2.h> //install ThermistorLibrary by Miguel Califa
#include "GyverFilters.h" //install GyverFilters by AlexGyver
//Local import
#include "config.h"

//Wi-Fi
const char* ssid = "YOUR WIFI NETWORK";//Your WiFi network
const char* password =  "WIFI PASSWORD";//WiFi network password
AsyncWebServer server(80);
int WiFi_status = WL_IDLE_STATUS;
unsigned long wifi_tick_previous = 0;

//Sensors
Thermistor2 therm1(PIN_HEATER_SENSOR, false, 10000, 10000, 25, 3950, 1); //connect NTC 100K B3950 thermistor to A0 pin
GMedian<10, int> filtered_temperature; //temperature read filter (ADC is not very good)
DHTStable temp_sensor_in;//inner temperature sensor
DHTStable temp_sensor_out;//outer temperature sensor
byte sensor_inner_founded = 0;//if sensor not founded - can`t start drying
byte sensor_outer_founded = 0;//if sensor not founded - can`t start drying

//Sensors data (5 sensors, 15 last measurements, last 3:30h)
int statistics[5][15];
unsigned long last_statistics_update_time = 999999;

uint8_t ts_pos = 0;
float target_humidity_in = 15.0;
float target_temperature_in = 45.0;
float max_temperature_heater = 75.0;
uint8_t box_status = 0; //status ON-OFF
unsigned long begin_at_time = 0;

//Sensors data
float temperature_in = 0;
float temperature_out = 0;
float temperature_heater = 0;
float humidity_in = 0;
float humidity_out = 0;

//Fan params
uint16_t fan_status = 1; //fan status ON-OFF

//PID
unsigned long pid_first_millis = 0;
unsigned long pid_last_millis = 0;
float temperature_samples_in[PID_SAMPLES] = {0};
float temperature_samples_heater[PID_SAMPLES] = {0};

void IRAM_ATTR IntOnOffButtonCallback()
{
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  if (interrupt_time - last_interrupt_time > 500){
    //ON-OFF
    if (box_status == 1) {
      off();
    } else {
      on();
    }
    last_interrupt_time = interrupt_time;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Setting up GPIO");
  pinMode(PIN_ON_OFF_BTN, INPUT);
  
  //Status led default OFF
  pinMode(PIN_LED_ORANGE, OUTPUT);
  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);

  LED_OFF(PIN_LED_ORANGE);
  LED_OFF(PIN_LED_RED);
  LED_OFF(PIN_LED_GREEN);

  // Heater PWM -- Make sure heater is initially OFF
  pinMode(PIN_HEATER_PWM, OUTPUT);
  analogWrite(PIN_HEATER_PWM, LOW);

  //FAN -- Make sure FAN is initially OFF
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, LOW);

  //Check temperature and humidity sensors
  uint8_t i = 5;
  int chk1 = temp_sensor_out.read11(PIN_DHT11_OUTER);
  while (i-- && (chk1 != DHTLIB_OK)) {
    Serial.println("Can't find outer DHT11 sensor!");
    delay(250);
    LED_ON(PIN_LED_RED);
    delay(250);
    LED_OFF(PIN_LED_RED);
    delay(250);
    chk1 = temp_sensor_out.read11(PIN_DHT11_OUTER);
  }

  if (chk1 == DHTLIB_OK) {
    sensor_outer_founded = 1;
  }

  i = 5;
  int chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
  while (i-- && (chk2 != DHTLIB_OK)) {
    Serial.println("Can't find inner DHT11 sensor!");
    delay(250);
    LED_ON(PIN_LED_RED);
    delay(250);
    LED_OFF(PIN_LED_RED);
    delay(250);
    chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
  }

  if (chk2 == DHTLIB_OK) {
    sensor_inner_founded = 1;
  }

  // Connect to WiFi
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);

  //iFi.hostname(MDNS_NAME);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // while wifi not connected yet, print '.'
  // then after it connected, get out of the loop (10 attempts)
  i = 10;
  while (i-- && (WiFi.status() != WL_CONNECTED)) {
    delay(600);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    LED_ON(PIN_LED_GREEN);
  }
  Serial.println("");
  WiFi.onStationModeDisconnected(onStationDisconnected);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  /*if (!MDNS.begin(MDNS_NAME, WiFi.localIP())) {
    Serial.println("Error starting mDNS");
  } else {
    Serial.println((String) "mDNS http://" + MDNS_NAME + ".local");
  }*/

  Serial.print("WiFi IP: ");
  Serial.println(WiFi.localIP());

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    while (1) {
      LED_ON(PIN_LED_RED);
      delay(500);
      LED_OFF(PIN_LED_RED);
      delay(500);
    }
  }

  setupWebServer();
  server.begin();
  //MDNS.addService("http", "tcp", 80);

  #if DEF_DEBUG_LITTLEFS
    debug_littlefs_files();
  #endif

  temperature_heater = readHeaterTemperature(true);

  for (uint8_t i = 0; i < 5; i++) {
    for (uint8_t j = 0; j < 15; j++) {
      statistics[i][j] = 0;
    }
  }
  
  attachInterrupt(digitalPinToInterrupt(PIN_ON_OFF_BTN), IntOnOffButtonCallback, FALLING); //start-stop button

  Serial.println("Ready to go.");
}

// 	** Main loop **
void loop()
{
  //MDNS.update();

  //Sample temperature and humidity from all available sensors
  temperature_heater = readHeaterTemperature(false);
  sample_sens_in_and_out();

  //Debug
  #if DEF_DEBUG_HEATER_SAMPLES
    Serial.println((String)temperature_in + "\t" + temperature_heater + "\t" + target_temperature_in);
  #endif

  //Check if we have WiFi connection, if not try to reconnect
  check_wifi_connection();
  
  if (box_status == 1) { //Dry box is active
    LED_ON(LED_STATUS_HEATER);
    if (sample_temperatures(temperature_in, temperature_heater)) { //Sample temperature values
      heater_recalc_pwm(); // Recalculate PWM value for the heater
    }
    if ((humidity_in > 0.0) && (humidity_in <= target_humidity_in)) {
      off();
    }
  } else { // Drybox is off. Turn/Keep off the heater
    LED_OFF(LED_STATUS_HEATER);
    set_heater_duty(HEATER_DUTY_OFF);
  }

  if (millis() - last_statistics_update_time > 900000) { //every 15 min
    //shift data left
    for (uint8_t i = 0; i < 5; i++) {
      for (uint8_t j = 0; j < 14; j++) {
        statistics[i][j] = statistics[i][j+1];
      }
    }
    //Fill last column with a new data
    uint8_t i = 5;
    int chk2 = temp_sensor_out.read11(PIN_DHT11_OUTER);
    while (i-- && (chk2 != DHTLIB_OK)) {
      delay(25);
      chk2 = temp_sensor_out.read11(PIN_DHT11_OUTER);
    }
    if (chk2 == DHTLIB_OK) {
      sensor_outer_founded = 1;
      statistics[2][14] = temp_sensor_out.getTemperature();//Temperature outside
      statistics[4][14] = temp_sensor_out.getHumidity();//Humidity outside
    }else{ 
      sensor_outer_founded = 0;
      statistics[2][14] = 0;
      statistics[4][14] = 0;
    }
    i = 5;
    chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
    while (i-- && (chk2 != DHTLIB_OK)) {
      delay(25);
      chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
    }
    if (chk2 == DHTLIB_OK) {
      sensor_inner_founded = 1;
      statistics[1][14] = temp_sensor_in.getTemperature();//Temperature inside
      statistics[3][14] = temp_sensor_in.getHumidity();//Humidity inside
    } else {
      sensor_inner_founded = 0;
      statistics[1][14] = 0;
      statistics[3][14] = 0;
    }
    statistics[0][14] = readHeaterTemperature(false);//Heater temperature
    last_statistics_update_time = millis();
  }
}

void debug_littlefs_files(void)
{
  File root = LittleFS.open("/", "r");
  File file = root.openNextFile();

  while (file) {
    Serial.print("FILE: ");
    Serial.println(file.name());
    file = root.openNextFile();
  }
}

void sample_sens_in_and_out(void)
{
  float tmp_temp = 0.0;
  float tmp_humid = 0.0;
  uint8_t i = 5;
  
  if (sensor_outer_founded == 1) {
    // Read temperature and humidity
    int chk2 = temp_sensor_out.read11(PIN_DHT11_OUTER);
    while (i-- && (chk2 != DHTLIB_OK)) {
      delay(25);
      chk2 = temp_sensor_out.read11(PIN_DHT11_OUTER);
    }
    if (chk2 == DHTLIB_OK) {
      tmp_temp = temp_sensor_out.getTemperature();
      tmp_humid = temp_sensor_out.getHumidity();
    } else {
      tmp_temp = 0.0;
      tmp_humid = 0.0;
    }

    // Make sure final values are valid
    if ((tmp_temp != 0.0) && (tmp_humid != 0.0)) {
      temperature_out = tmp_temp;
      humidity_out = tmp_humid;
      #if DEF_DEBUG_SENSOR_SAMPLES
        Serial.println((String) "TempOut:" + temperature_out + " - HumidOut:" + humidity_out);
      #endif
    } else { // Otherwise we have a problem
        Serial.println("Out sensor error!");
    }
  }

  if (sensor_inner_founded == 1) {
    // Read temperature and humidity
    i = 5;
    int chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
    while (i-- && (chk2 != DHTLIB_OK)) {
      delay(25);
      chk2 = temp_sensor_in.read11(PIN_DHT11_INNER);
    }
    if (chk2 == DHTLIB_OK) {
      tmp_temp = temp_sensor_in.getTemperature();
      tmp_humid = temp_sensor_in.getHumidity();
    } else {
      tmp_temp = 0.0;
      tmp_humid = 0.0;
    }

    // Make sure final values are valid
    if ((tmp_temp != 0.0) && (tmp_humid != 0.0)) {
      temperature_in = tmp_temp;
      humidity_in = tmp_humid;
      #if DEF_DEBUG_SENSOR_SAMPLES
        Serial.println((String) "TempIn:" + temperature_in + " - HumidIn:" + humidity_in);
      #endif
    } else { // Otherwise we have a problem
      Serial.println("In sensor error!");
    }
  }
}

float readHeaterTemperature(bool firstRead)
{
  float tempC;
  if (firstRead){
    tempC = filtered_temperature.filtered(therm1.readTemperature()); //read temperature near heater
    tempC = filtered_temperature.filtered(therm1.readTemperature()); //read temperature near heater
  } else {
    tempC = filtered_temperature.filtered(therm1.readTemperature()); //read temperature near heater
  }
  #if DEF_DEBUG_SENSOR_SAMPLES
    Serial.print("Temp C: ");
    Serial.println(tempC);
  #endif
  return tempC;
}

void enable_fan(uint16_t enabled)
{
  fan_status = enabled;
  if (enabled == 1){
    digitalWrite(PIN_FAN, HIGH);
  } else {
    digitalWrite(PIN_FAN, LOW);
  }
}

void set_heater_duty(uint8_t duty)
{
  uint32_t pwm_raw_heater = 0;
  if (duty > 100) { duty = 100; }
  pwm_raw_heater = ((PWM_MAX_VALUE * duty) / 100.0);

  #if DEF_DEBUG_PWM_VALUES
    Serial.println((String) "Setting heater duty " + duty + "%");
  #endif
  set_heater_pwm(pwm_raw_heater);
}

void set_heater_pwm(uint32_t pwm)
{
  if (pwm > PWM_MAX_VALUE) { pwm = PWM_MAX_VALUE; }

  #if PWM_HEATER_INVERT_VALUES
    pwm = PWM_MAX_VALUE - pwm;
  #endif
  #if DEF_DEBUG_PWM_VALUES
    Serial.println((String) "set_heater_duty: " + pwm + "/" + PWM_MAX_VALUE);
  #endif
  if (pwm > 0){
    LED_ON(PIN_LED_ORANGE);
  } else {
    LED_OFF(PIN_LED_ORANGE);
  }
  analogWrite(PIN_HEATER_PWM, pwm);
}

bool sample_temperatures(float in, float heater)
{
  temperature_samples_in[ts_pos] = in;
  temperature_samples_heater[ts_pos] = heater;

  ts_pos++;
  if (ts_pos == 0) {
    // Record milliseconds of our first sample
    pid_first_millis = millis();
  }
  if (ts_pos >= PID_SAMPLES) {
    pid_last_millis = millis();
    ts_pos = 0;
    return true;
  } else {
    return false;
  }
}

void heater_recalc_pwm(void)
{
  // We are using a simple PID-like control loop to calculate PWM value for a
  // heater Heater is heating up our box to a target temperature and keeping
  // it steady after/if that temperature is reached. At the same time we have
  // to ensure heater does not go beyond our "safe" temperature and start
  // damaging itself/fillament/cables/enclosure etc.
  //
  // As a simple solution, we will use the following flow
  // 1. Check if heater temperature is higher or equal to the maximum set
  // heater temperature
  // 		- if True, use heater max temperature as the target for our PID
  // controller
  // 		- if False, use enclosure maximum temperature as the target for
  // our PID controller

  float average = 0;
  float pid_del_p;
  float pid_del_i;
  float pid_del_d;
  int32_t pid_val_p;
  int32_t pid_val_i;
  int32_t pid_val_d;
  int32_t pwm_val;
  float pid_target_temperature = 0.0;
  float pid_temperature = 0.0;
  float pid_temperature_previous = 0.0;
  unsigned long pid_time_diff;

  // Make sure values are valid
  if (temperature_heater <= 0) {
    Serial.println("Invalid temperature values");
    Serial.println((String) "temp_heater:" + temperature_heater);
    Serial.println((String) "target_temperature_in:" + target_temperature_in);
    set_heater_duty(HEATER_DUTY_OFF);
    return;
  }

  // Inside temperature has NOT reached the target temperature
  // However, heater temperature is approaching it's maximum allowed
  // temperature. Then, regulate heater max temperature
  if ((temperature_heater >= (max_temperature_heater - PID_TEMP_PROXIMITY)) && (temperature_in < target_temperature_in)) {
    #if DEF_DEBUG_PID
      Serial.println("Using HEATER temperature as target.");
    #endif
    for (uint8_t i = 0; i < PID_SAMPLES; i++) {
      average += temperature_samples_heater[i];
    }
    average = average / PID_SAMPLES;

    pid_target_temperature = max_temperature_heater;
    pid_temperature = temperature_heater;
    pid_temperature_previous = temperature_samples_heater[PID_SAMPLES - 1];
  } else {
    // Heater is not close to it's maximum allowed temperature
    // Or, inside temperature has already reached the target value
    #if DEF_DEBUG_PID
      Serial.println("Using IN temperature as target.");
    #endif
    for (uint8_t i = 0; i < PID_SAMPLES; i++) {
      average += temperature_samples_in[i];
    }
    average = average / PID_SAMPLES;

    pid_target_temperature = target_temperature_in;
    pid_temperature = temperature_in;
    pid_temperature_previous = temperature_samples_in[PID_SAMPLES - 1];
  }

  // P
  pid_del_p = pid_target_temperature - pid_temperature;
  pid_val_p = (uint32_t)(pid_del_p)*PID_KP;

  // I
  pid_del_i = pid_target_temperature - average;
  pid_val_i = (uint32_t)(pid_del_i)*PID_KI;

  // D
  pid_del_d = pid_target_temperature - pid_temperature_previous;
  pid_time_diff = pid_last_millis - pid_first_millis;
  if (pid_time_diff > 0) {
    pid_val_d = (uint32_t)(pid_del_d) / pid_time_diff;
    pid_val_d = (uint32_t)(pid_val_d)*PID_KD;
  } else {
    pid_val_d = 0;
  }

  if (temperature_heater >= max_temperature_heater) {
    pwm_val = 0;
  } else {
    // P + I + D
    pwm_val = pid_val_p + pid_val_i + pid_val_d;
  }

  #if DEF_DEBUG_PID
    Serial.println((String) "Pd: " + pid_del_p);
    Serial.println((String) "Pv: " + pid_val_p);
    Serial.println((String) "Id: " + pid_del_i);
    Serial.println((String) "Iv: " + pid_val_i);
    Serial.println((String) "Dd: " + pid_del_d);
    Serial.println((String) "Dv: " + pid_val_d);
    Serial.println((String) "Calc PWM val: " + pwm_val);
  #endif

  if (pwm_val > PWM_MAX_VALUE) { 
    pwm_val = PWM_MAX_VALUE; 
  } else if (pwm_val < 0) {
    pwm_val = 0;
  }

  #if DEF_DEBUG_PID
    Serial.println((String) "New PWM val: " + pwm_val);
  #endif
  set_heater_pwm(pwm_val);
}

void setupWebServer(void)
{
  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.print("404:");
    Serial.println(request->url());
    request->send(404);
  });

  // send a file when /index is requested
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html");
  });

  // send a file when /index is requested
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html");
  });

  server.serveStatic("/images/", LittleFS, "/images/").setCacheControl("max-age=315360000");
  server.serveStatic("/css/", LittleFS, "/css/").setCacheControl("max-age=315360000");
  server.serveStatic("/js/", LittleFS, "/js/").setCacheControl("max-age=315360000");

  //Sensors statistics (last 3:30 hour)
  server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *request) {
    char buff[500] = {0};
    JsonDocument stats;
    copyArray(statistics, stats);
    /*for (uint8_t i = 0; i < 5; i++) {
      JsonArray array = stats.createNestedArray();
      for (uint8_t j = 0; j < 15; j++) {
        array.add(statistics[i][j]);
      }
    }*/
    serializeJson(stats, buff);
    request->send(200, "application/json", buff);
  });

  //Get dry box status
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request) {
    char buff[295] = {0};
    int len;
    unsigned long worktime = millis() - begin_at_time;
    uint16_t fanst;
    if ((fan_status == 1) && (box_status == 1)) {
      fanst = 1;
    } else {
      fanst = 0;
    }
    len = snprintf(buff, 295,
                  "{\"status\":%d,\"target_temp_in\":%f,\"max_temp_heater\":%f,\"temp_in\":%f,\"temp_heater\":%f,\"humid_in\":%f, "
                  "\"in_snsr_finded\":%d,\"out_snsr_finded\":%d,\"trgt_hmdty\":%f,\"temp_out\":%f,\"humid_out\":%f,\"fan_st\":%d,\"fanst\":%d,\"worktime\":%lu}",
                  box_status, target_temperature_in, max_temperature_heater, temperature_in, temperature_heater, humidity_in, 
                  sensor_inner_founded, sensor_outer_founded, target_humidity_in, temperature_out, humidity_out, fan_status, fanst, worktime);
    if (len) {
      request->send(200, "application/json", buff);
    } else {
      request->send(500, "application/json", "{\"status\": \"Internal server error\"}");
    }
  });

  //set target temperature, max heater temperature and target humidity inside
  server.on("/setparams", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Check if temperature, heater temperature and target humidity inside arguments are present
    if (request->hasParam("trgt_tmpr", true) && request->hasParam("max_htr_tmpr", true) && request->hasParam("trgt_hmdty", true) && request->hasParam("fan_st", true)) {
      int32_t temperature = 0;
      int32_t heater = 0;
      int32_t targethumidity = 0;
      int32_t fanstatus = 0;

      String str_fanstatus = request->getParam("fan_st", true)->value();
      fanstatus = str_fanstatus.toInt();

      String str_temperature = request->getParam("trgt_tmpr", true)->value();
      temperature = str_temperature.toInt();

      String str_heater = request->getParam("max_htr_tmpr", true)->value();
      heater = str_heater.toInt();

      String str_target_humidity = request->getParam("trgt_hmdty", true)->value();
      targethumidity = str_target_humidity.toInt();

      #if DEF_DEBUG_WEB_API
        Serial.println((String) "Target temp: " + temperature + "C Heater: " + heater + "C Target humidity: " + targethumidity);
      #endif

      // Check drybox temperature and heater temperature limit
      if ((temperature <= LIMIT_TEMP_IN_MAX) && (heater <= LIMIT_TEMP_HEATER_MAX)) {
        target_humidity_in = targethumidity;
        target_temperature_in = temperature;
        max_temperature_heater = heater;
        if (box_status == 1) {
          enable_fan(fanstatus); 
        }
        request->send(200, "application/json", "{\"status\": \"OK\"}");
        return;
      } else { // If either of them are out of range, return bad request status
        request->send(400, "application/json", "{\"status\": \"Bad request. Limit error!\"}");
        return;
      }
    } else { // Not all arguments are present in the request
      request->send(400, "application/json", "{\"status\": \"Bad request\"}");
      return;
    }
  });

  //Turn ON dry box
  server.on("/on", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("fan_st", true)) {
      int32_t fanstatus = 0;

      String str_fanstatus = request->getParam("fan_st", true)->value();
      fanstatus = str_fanstatus.toInt();

      box_status = 1; //Box ON
      enable_fan(fanstatus);
      LED_ON(LED_STATUS_HEATER);
      begin_at_time = millis();
      last_statistics_update_time = 999999;
      
      request->send(200, "application/json", "{\"status\": \"OK\"}");
      return;
    } else { // Not all arguments are present in the request
      request->send(400, "application/json", "{\"status\": \"Bad request\"}");
      return;
    }
  });

  //Turn OFF dry box
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request) {
    off();
    request->send(200, "application/json", "{\"status\": \"OK\"}");
  });
}

void off()
{
  box_status = 0;//Box OFF
  enable_fan(0);
  LED_OFF(LED_STATUS_HEATER);
  begin_at_time = 0;
}

void on()
{
  box_status = 1;//Box ON
  enable_fan(1);
  LED_ON(LED_STATUS_HEATER);
  begin_at_time = millis();
  last_statistics_update_time = 999999;
}

void onStationDisconnected(const WiFiEventStationModeDisconnected& event) {
  Serial.print("Disconnected from Wi-Fi: ");
  Serial.println(event.reason);
  // Attempt to reconnect to Wi-Fi network
  WiFi.begin(ssid, password);
}

void check_wifi_connection(void)
{
  unsigned long currentMillis = millis();

  // Check WiFi status every WIFI_CHECK_CONNECTION_MS miliseconds
  if ((currentMillis - wifi_tick_previous) >= WIFI_CHECK_CONNECTION_MS) {
    if (WiFi.status() == WL_CONNECTED) {
      LED_ON(PIN_LED_GREEN);
    } else {
      LED_OFF(PIN_LED_GREEN);
      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
    }

    wifi_tick_previous = currentMillis;
  }
}
