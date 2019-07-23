
// The PCF8523 alarm only supports down to minute resolution, no seconds 


#include <OPEnS_RTC.h>

#define ALARM_PIN 6


// Instance of DS3231 RTC
PCF8523 PCF; 

volatile bool alarmFlag = false;
volatile int count = 0;
void alarmISR() {
	// PCF.ackTimer1();
	count++;
	alarmFlag = true;
}


void print_DateTime(DateTime time);



// ======= SETUP ======
void setup()
{
	pinMode(ALARM_PIN, INPUT_PULLUP);  // Pull up resistors required for Active-Low interrupts
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	Serial.begin(115200);
	while (!Serial); // Won't start anything until serial is open, comment this line out if powering from battery

	PCF.begin();
	PCF.adjust(DateTime(__DATE__, __TIME__));

	PCF.setTimer1(eTB_SECOND, 5);

	attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, FALLING );

	digitalWrite(LED_BUILTIN, LOW);
	Serial.println("Setup Complete");
}


// ======= LOOP ======
void loop()
{
	if (alarmFlag) {
		digitalWrite(LED_BUILTIN, HIGH);
		Serial.println("Timer triggered, resetting alarm");
		
		delay(2000);

		digitalWrite(LED_BUILTIN, LOW);
		alarmFlag = false;
	}
	Serial.print("Count: "); Serial.println(count);
	print_DateTime(PCF.now());

	delay(1000);
}


// Print an RTC time
void print_DateTime(DateTime time)
{
	Serial.print(time.year());	Serial.print('/');
	Serial.print(time.month());	Serial.print('/');
	Serial.print(time.day());	Serial.print(' ');
	Serial.print(time.hour());	Serial.print(':');
	Serial.print(time.minute());Serial.print(':');
	Serial.println(time.second());
}
