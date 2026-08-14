#include <Arduino.h>

// Preprocessor directives
#define SEC * 1000

// Function declarations
void motion_detect(void *parameters);
TaskHandle_t motion_detect_handle = NULL;

void lamp_switch(void *parameters);
TaskHandle_t lamp_switch_handle = NULL;

void light_detect(void *parameters);
TaskHandle_t light_detect_handle = NULL;
	
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

void setup() {
	// SERIAL
	Serial.begin(115200);

	// PIN
	pinMode(lamp_pin_no, OUTPUT);
	pinMode(motion_pin_no, INPUT);
	pinMode(light_pin_no, INPUT);

	// FREE RTOS TASK
	xTaskCreate(motion_detect, "Detect motion",
		1000, NULL, 1, &motion_detect_handle);
	xTaskCreate(lamp_switch, "Switch lamp", 
		1000, NULL, 1, &lamp_switch_handle);
	xTaskCreate(light_detect, "Detect light",
		1000, NULL, 1, &light_detect_handle);
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