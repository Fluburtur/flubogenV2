// Simplified version of remote, to be used with new Protobrain

// true: Use internal pullup resistors of the Arduino
// false: Need to connect external pulldown resistors
#define BUTTON_PULLUP true

#if BUTTON_PULLUP
bool pullups = true;
const uint8_t buttons[] = {A1, A0, 12, 11, 5, 6, 7, 8};
#else
bool pullups = false;
const uint8_t buttons[] = {A0, 13, 12, 11, 5, 6, 7, 8};
#endif

uint8_t data[2];

void setup() {
	Serial.begin(31250);
	for (uint8_t i = 0; i < sizeof(buttons); i++) {
		pinMode(buttons[i], pullups ? INPUT_PULLUP : INPUT);
	}
}

void loop() {
	data[0] = 0xAC;
	data[1] = 0;
	data[2] = 0;

	for (uint8_t i = 0; i < sizeof(buttons); i++) {
		bool buttonPressed = BUTTON_PULLUP ? !digitalRead(buttons[i])	: digitalRead(buttons[i]);
		if (buttonPressed)
			data[1 + (i >> 3)] |= (1 << (i & 7));
	}

	Serial.write(data, sizeof(data));

	delay(10);	// 100Hz refresh
}
