#define MDNS_NAME                   "drybox"

#define PIN_HEATER_SENSOR           A0
#define PIN_DHT11_INNER             D7 //Data
#define PIN_DHT11_OUTER             D3 //Data
#define PIN_HEATER_PWM              D1 //Heater element PWM
#define PIN_FAN_PWM                 D5 //Fan speed
#define PIN_ON_OFF_BTN              D4 //On-Off button
#define PIN_LED_ORANGE              D6 //Has errors (sensors, LittleFS)
#define PIN_LED_RED                 D0 //Dry box is active
#define PIN_LED_GREEN               D2 //Wi-Fi

#define LED_ON(x)                   do{digitalWrite(x, HIGH);}while(0)
#define LED_OFF(x)                  do{digitalWrite(x, LOW);}while(0)

#define LED_STATUS_HEATER           PIN_LED_RED
#define PID_SAMPLES                 5
#define PID_TEMP_PROXIMITY          5
#define PID_KP                      65
#define PID_KI                      81
#define PID_KD                      50

#define PWM_HEATER_INVERT_VALUES    0
#define PWM_MAX_VALUE               255
#define HEATER_DUTY_OFF             0

#define LIMIT_TEMP_IN_MAX           70
#define LIMIT_TEMP_HEATER_MAX       125

#define WIFI_CHECK_CONNECTION_MS    10000

#define DEF_DEBUG_PID               0
#define DEF_DEBUG_HEATER_SAMPLES    0
#define DEF_DEBUG_SENSOR_SAMPLES    0
#define DEF_DEBUG_PWM_VALUES        0
#define DEF_DEBUG_LITTLEFS          0
#define DEF_DEBUG_WEB_API           0
