#include <Arduino.h>
#include <TM1637Display.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <RTClib.h>
#include "wificredentials.h"

// Preprocessor directives
#define SEC *1000
#define MIN *(60 SEC)
#define H *(60 MIN)
#define DAY *(24 H)

// Function declarations
void motion_detect(void *parameters);
TaskHandle_t motion_detect_handle = NULL;

void lamp_switch(void *parameters);
TaskHandle_t lamp_switch_handle = NULL;

void light_detect(void *parameters);
TaskHandle_t light_detect_handle = NULL;

void connect_keep_WiFi(void *parameters);
TaskHandle_t connect_keep_WiFi_handle = NULL;

void time_display(void *parameters);
TaskHandle_t time_display_handle = NULL;

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
const uint8_t allON[] = {0xff, 0xff, 0xff, 0xff};
const uint8_t allOFF[] = {0x00, 0x00, 0x00, 0x00};
uint32_t numbers[2] = {0};

// NTP CLIENT
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// RTC MODULE
RTC_PCF8563 rtc;
const uint32_t rtc_i2c_sda_pin_no = 21;
const uint32_t rtc_i2c_scl_pin_no = 22;
uint8_t last_second = 0;
DateTime now;

void setup()
{
	// SERIAL
	Serial.begin(115200);

	// I2C
	if (!Wire.begin())
	{
		Serial.println("Could not start I2C!");
	}

	// PIN
	pinMode(lamp_pin_no, OUTPUT);
	pinMode(motion_pin_no, INPUT);
	pinMode(light_pin_no, INPUT);

	// DISPLAY
	display.setBrightness(4);
	display.setSegments(allON);

	// NTP & RTC TIME
	timeClient.setTimeOffset(10800);
	if (!rtc.begin())
	{
		Serial.println("PCF8563 module not found!");
	}

	// FREE RTOS TASK
	xTaskCreate(motion_detect, "Detect motion",
				1000, NULL, 1, &motion_detect_handle);
	xTaskCreate(lamp_switch, "Switch lamp",
				1000, NULL, 1, &lamp_switch_handle);
	xTaskCreate(light_detect, "Detect light",
				1000, NULL, 1, &light_detect_handle);
	xTaskCreate(time_display, "Display time",
				3000, NULL, 1, &time_display_handle);
	xTaskCreatePinnedToCore(connect_keep_WiFi, "Connect and keep Wi-Fi",
							2000, NULL, 2, &connect_keep_WiFi_handle,
							ARDUINO_RUNNING_CORE);
}

void loop()
{
}

void motion_detect(void *parameters)
{
	for (;;)
	{
		vTaskSuspend(lamp_switch_handle);
		motion_state = digitalRead(motion_pin_no);
		vTaskResume(lamp_switch_handle);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void lamp_switch(void *parameters)
{
	for (;;)
	{
		digitalWrite(lamp_pin_no, motion_state);
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

void light_detect(void *parameters)
{
	for (;;)
	{
		light_state = digitalRead(light_pin_no);
		if (light_state == LOW)
		{ // LOW = light
			vTaskSuspend(motion_detect_handle);
			vTaskSuspend(lamp_switch_handle);
			motion_state = LOW;
			digitalWrite(lamp_pin_no, motion_state);
		}
		else if (light_state == HIGH)
		{ // HIGH = darkness
			vTaskResume(motion_detect_handle);
			vTaskResume(lamp_switch_handle);
		}
		vTaskDelay(5 / portTICK_PERIOD_MS);
	}
}

void connect_keep_WiFi(void *parameters)
{
	for (;;)
	{
		switch (WiFi.status())
		{
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

		if (WiFi.status() != WL_CONNECTED)
		{
			// TODO: this might fail upon network reconnect whilst a run (TEST)
			WiFi.mode(WIFI_STA);
			WiFi.begin(WIFI_NETWORK, WIFI_PASSWORD);
			vTaskDelay((5 SEC) / portTICK_PERIOD_MS);
		}
		else if (WiFi.status() == WL_CONNECTED)
		{
			vTaskDelay((30 MIN) / portTICK_PERIOD_MS);
		}
	}
}

void time_display(void *parameters)
{

	TickType_t xLastWakeTime;
	const TickType_t xFrequency = (1 MIN) / portTICK_PERIOD_MS;

	xLastWakeTime = xTaskGetTickCount();

	// Wait for internet connection
	while (WiFi.status() != WL_CONNECTED)
	{
		vTaskDelayUntil(&xLastWakeTime, (5 SEC) / portTICK_PERIOD_MS);
		Serial.println("No internet! [NTP sync]");
	}

	// Sync with NTP server
	for (uint8_t i = 0; i < 10; i++)
	{
		last_second = timeClient.getSeconds();
		vTaskDelayUntil(&xLastWakeTime, (1 SEC) / portTICK_PERIOD_MS);
		timeClient.forceUpdate();
		numbers[0] = timeClient.getHours();
		numbers[1] = timeClient.getMinutes();
		display.showNumberDecEx(numbers[0] * 100 + numbers[1],
								0b01000000, true, 4, 0);
		if (timeClient.getSeconds() - last_second != 1)
		{
			i--;
		}
	}

	// Sync with next minute
	while (timeClient.getSeconds() != 0)
	{
		vTaskDelayUntil(&xLastWakeTime, (1 SEC) / portTICK_PERIOD_MS);
		timeClient.forceUpdate();
	}

	// Update clock per minute
	for (;;)
	{
		numbers[0] = timeClient.getHours();
		numbers[1] = timeClient.getMinutes();
		display.showNumberDecEx(numbers[0] * 100 + numbers[1],
								0b01000000, true, 4, 0);
		vTaskDelayUntil(&xLastWakeTime, xFrequency);
		timeClient.forceUpdate();
	}
}