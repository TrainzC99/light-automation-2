#include <Arduino.h>

// Function declarations
bool button(void);
void motion(void);
void relay_start(void);
	
// Global variables

	// BUTTON
bool button_state = false;
bool last_button_state = false;
uint64_t last_button_time = 0;
const uint32_t debounce_delay_timeout_ms = 30;
const uint32_t button_pin_no = 25;

	// LED
const uint32_t led_pin_no = 26;
bool led_state = false;

	// RELAY
const uint32_t relay_pin_no = 27;
bool relay_state = false;

  // MOTION SENSOR
const uint32_t motion_pin_no = 14;
uint32_t motion_state = LOW;
uint32_t state = LOW;
bool once = false;

void setup() {
	pinMode(led_pin_no, OUTPUT);
	pinMode(button_pin_no, INPUT);
	pinMode(relay_pin_no, OUTPUT);
	pinMode(motion_pin_no, INPUT);

	relay_start();

	led_state = true;
	relay_state = false;
}


void loop() {
	Serial.begin(115200);
	motion();
}

void relay_start(void)
{
	digitalWrite(relay_pin_no, HIGH);
}

void motion(void)
{
	state = digitalRead(motion_pin_no);  // read input value

  	if (state == HIGH)  // check if the input is HIGH
	{            
		if (once == false)  // only turn on LED if it's currently off
		{
			digitalWrite(relay_pin_no, LOW);
			digitalWrite(led_pin_no, HIGH);
			once = true;  // update ledState
		}

		if (motion_state == LOW)  // only print message when state changes
			motion_state = HIGH;  // update pirState
	} 
  	else 
	{
		if (once == true)  // only turn off LED if it's currently on
		{
			digitalWrite(relay_pin_no, HIGH);
			digitalWrite(led_pin_no, LOW);
			once = false;  // update ledState
		}

		if (motion_state == HIGH)  // only print message when state changes
			motion_state = LOW;  // update pirState
	}
}

bool button(void)
{
	int state = digitalRead(button_pin_no);

	if (state != last_button_state)
		last_button_time = millis();

	if (millis() - last_button_time > debounce_delay_timeout_ms)
	{
		if (state != button_state)
		{
			button_state = state;
			if(button_state == LOW)
				return true;
		}
	}

	last_button_state = state;
	return false;
}
