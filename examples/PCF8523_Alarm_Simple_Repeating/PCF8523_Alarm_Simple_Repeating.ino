
// The PCF8523 alarm only supports down to minute resolution, no seconds 


#include <OPEnS_RTC.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

#define ALARM_PIN 12


// Instance of DS3231 RTC
PCF8523 PCF; 

volatile bool alarmFlag = false;
volatile int count = 0;
void alarmISR() {
	PCF.enable_alarm(false);
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
	// PCF.adjust(DateTime(2019, 1, 1, 8, 1, 50));

	PCF.stop_32768_clkout();

	DateTime nextMin = PCF.now()+TimeSpan(60);
	Serial.print("Setting alarm for minute of: ");
	print_DateTime(nextMin);
	PCF.set_alarm(nextMin.hour(), nextMin.minute());
	PCF.enable_alarm(true);
	attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, FALLING );

	digitalWrite(LED_BUILTIN, LOW);
	Serial.println("Setup Complete");
}


// ======= LOOP ======
void loop()
{
	if (alarmFlag) {
		digitalWrite(LED_BUILTIN, HIGH);
		Serial.println("Alarm triggered, resetting alarm");
		PCF.clear_rtc_interrupt_flags();
		
		delay(2000);

		DateTime nextMin = PCF.now()+TimeSpan(60);
		Serial.print("Setting alarm for minute of: ");
		print_DateTime(nextMin);
		PCF.set_alarm(nextMin.hour(), nextMin.minute());
		PCF.enable_alarm(true);

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
