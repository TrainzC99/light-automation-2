#include <Arduino.h>

// Preprocessor directives
#define SEC * 1000

// Function declarations
void motion_detect(void *parameters);
TaskHandle_t motion_detect_handle = NULL;

void lamp_switch(void *parameters);
TaskHandle_t lamp_switch_handle = NULL;
	
// Global variables
	// LAMP
const uint32_t lamp_pin_no = 26;
bool lamp_state = false;

  // MOTION SENSOR
const uint32_t motion_pin_no = 14;
uint32_t state = LOW;
bool once = false;

void setup() {
	// SERIAL
	Serial.begin(115200);

	// PIN
	pinMode(lamp_pin_no, OUTPUT);
	pinMode(motion_pin_no, INPUT);

	// FREE RTOS TASK
	xTaskCreate(motion_detect, "Detect motion",
		1000, NULL, 1, &motion_detect_handle);
	xTaskCreate(lamp_switch, "Switch lamp", 
		1000, NULL, 1, &lamp_switch_handle);
}

void loop() {

}

void motion_detect(void *parameters){
	for(;;){
		vTaskSuspend(lamp_switch_handle);
		state = digitalRead(motion_pin_no);
		Serial.println("Sensor read: " + state);
		vTaskResume(lamp_switch_handle);
		vTaskDelay( 10 / portTICK_PERIOD_MS);
	}
}

void lamp_switch(void *parameters){
	for(;;){
		vTaskSuspend(motion_detect_handle);
		digitalWrite(lamp_pin_no, state);
		Serial.println("Lamp switched to: " + state);
		vTaskResume(motion_detect_handle);
		vTaskDelay( 10 / portTICK_PERIOD_MS);
	}
}

// void motion(void)
// {
// 	state = digitalRead(motion_pin_no);  // read input value

//   	if (state == HIGH)  // check if the input is HIGH
// 	{            
// 		if (once == false)  // only turn on LED if it's currently off
// 		{
// 			digitalWrite(led_pin_no, HIGH);
// 			once = true;  // update ledState
// 		}
// 	} 
//   	else 
// 	{
// 		if (once == true)  // only turn off LED if it's currently on
// 		{
// 			digitalWrite(led_pin_no, LOW);
// 			once = false;  // update ledState
// 		}
// 	}
// }
