#include <Arduino.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include "wificredentials.h"

// Preprocessor directives
#define SEC * 1000
#define MIN * (60 SEC)

// Function declarations
void motion_detect(void *parameters);
TaskHandle_t motion_detect_handle = NULL;

void lamp_switch(void *parameters);
TaskHandle_t lamp_switch_handle = NULL;

void light_detect(void *parameters);
TaskHandle_t light_detect_handle = NULL;

void display_update(void *parameters);
TaskHandle_t display_update_handle = NULL;

void connect_keep_WiFi(void *parameters);
TaskHandle_t connect_keep_WiFi_handle = NULL;

void sync_time(void *parameters);
TaskHandle_t sync_time_handle = NULL;
	
// Global variables
	// LAMP
const uint32_t lamp_pin_no = 26;
bool lamp_state = false;

	// MOTION SENSOR
const uint32_t motion_pin_no = 14;
uint32_t motion_state = LOW;

	// LIGHT SENSOR
const uint32_t light_pin_no = 27;
uint32_t light_state = LOW;

	// 4-DIGIT DISPLAY
const uint32_t display_clock_pin_no = 33;
const uint32_t display_dio_pin_no = 25;
TM1637Display display = TM1637Display(display_clock_pin_no, display_dio_pin_no);
const uint8_t allON[] = { 0xff, 0xff, 0xff, 0xff };
const uint8_t allOFF[] = { 0x00, 0x00, 0x00, 0x00 };
uint32_t timeno[2] = {0};

	// WI-FI
#define WIFI_TIMEOUT (20 SEC)

void setup() {
	// SERIAL
	Serial.begin(115200);

	// PIN
	pinMode(lamp_pin_no, OUTPUT);
	pinMode(motion_pin_no, INPUT);
	pinMode(light_pin_no, INPUT);

	// DISPLAY
	display.setBrightness(4);
	display.setSegments(allON);

	// FREE RTOS TASK
	xTaskCreate(motion_detect, "Detect motion",
		1000, NULL, 1, &motion_detect_handle);
	xTaskCreate(lamp_switch, "Switch lamp", 
		1000, NULL, 1, &lamp_switch_handle);
	xTaskCreate(light_detect, "Detect light",
		1000, NULL, 1, &light_detect_handle);
	xTaskCreate(display_update, "Update display",
		2000, NULL, 2, &display_update_handle);
	xTaskCreatePinnedToCore(connect_keep_WiFi, "Connect and keep Wi-Fi",
		2000, NULL, 2, &connect_keep_WiFi_handle, 
		ARDUINO_RUNNING_CORE);
	
}

void loop() {

}

void motion_detect(void *parameters){
	for(;;){
		vTaskSuspend(lamp_switch_handle);
		motion_state = digitalRead(motion_pin_no);
		vTaskResume(lamp_switch_handle);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void lamp_switch(void *parameters){
	for(;;){
		digitalWrite(lamp_pin_no, motion_state);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void light_detect(void *parameters){
	for(;;){
		light_state = digitalRead(light_pin_no);
		if(light_state == LOW){ // LOW = light
			vTaskSuspend(motion_detect_handle);
			vTaskSuspend(lamp_switch_handle);	
			motion_state = LOW;
			digitalWrite(lamp_pin_no, motion_state);
		}else if(light_state == HIGH){ // HIGH = darkness
			vTaskResume(motion_detect_handle);
			vTaskResume(lamp_switch_handle);
		}
		vTaskDelay( 5 / portTICK_PERIOD_MS );
	}
}

void display_update(void *parameters){

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = (1 SEC) / portTICK_PERIOD_MS;

	xLastWakeTime = xTaskGetTickCount();

	for(;;){
		display.showNumberDecEx(timeno[0] * 100 + timeno[1],
			0b01000000,true,4,0);
		timeno[1]++;
		timeno[0] = timeno[1];
		vTaskDelayUntil( &xLastWakeTime, xFrequency);
	}
}

void connect_keep_WiFi(void *parameters){
	for(;;){
		switch (WiFi.status()){
			case WL_CONNECTED:
				Serial.println("WiFi connected");
				break;
			case WL_CONNECTION_LOST:
				Serial.println("Wifi lost");
				break;
			case WL_DISCONNECTED:
				Serial.println("Wifi disconnected");
				break;
			case WL_CONNECT_FAILED:
				Serial.println("Wifi connection error");
				break;
			case WL_NO_SSID_AVAIL:
				Serial.println("WiFi not configured");
			default:
				Serial.println("Unknown WiFi status");
				break;
		}

		if(WiFi.status() != WL_CONNECTED){
			WiFi.mode(WIFI_STA);
			WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
			vTaskDelay((10 SEC) / portTICK_PERIOD_MS);
		}
		else if(WiFi.status() == WL_CONNECTED){
			vTaskDelay((30 MIN) / portTICK_PERIOD_MS);
		}
	}
}