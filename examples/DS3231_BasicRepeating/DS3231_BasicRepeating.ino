
#include <OPEnS_RTC.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

const int ALARM_DURATION = 10; 		// Number of seconds before alarm goes off

// Instance of DS3231 RTC
RTC_DS3231 RTC_DS; 


#define ALARM_PIN 6

void clear_alarms();


volatile bool alarmFlag = false;
void alarmISR() { 
	detachInterrupt(digitalPinToInterrupt(ALARM_PIN)); 
	clear_alarms();
	alarmFlag = true;
}


void setup() 
{ 
	pinMode(ALARM_PIN, INPUT_PULLUP);  // Pull up resistors required for Active-Low interrupts
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	RTC_DS.begin();
	RTC_DS.adjust(DateTime(__DATE__, __TIME__)); 

	Serial.begin(115200);
	while (!Serial); // Won't start anything until serial is open, comment this line out if powering from battery
	delay(1000);
	Serial.println("Initialized Serial");

	// Setup interrupt and alarm
	clear_alarms();
	attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, LOW );

	DateTime alarmTime = RTC_DS.now()+TimeSpan(ALARM_DURATION);
	RTC_DS.setAlarm(ALM1_MATCH_HOURS, alarmTime.second(), alarmTime.minute(), alarmTime.hour(), 0);   //set your wake-up time here
	RTC_DS.alarmInterrupt(1, true);
	digitalWrite(LED_BUILTIN, LOW);

	Serial.println("\n ** Setup Complete ** ");
}

void loop() 
{
	if (alarmFlag) {
		digitalWrite(LED_BUILTIN, HIGH);
		Serial.println("Alarm triggered, resetting alarm");
		delay(1000);
		
		// Setup interrupt and alarm
		clear_alarms();
		attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, LOW );

		DateTime alarmTime = RTC_DS.now()+TimeSpan(ALARM_DURATION);
		RTC_DS.setAlarm(ALM1_MATCH_HOURS, alarmTime.second(), alarmTime.minute(), alarmTime.hour(), 0);   //set your wake-up time here
		RTC_DS.alarmInterrupt(1, true);
		digitalWrite(LED_BUILTIN, LOW);

		digitalWrite(LED_BUILTIN, LOW);
		alarmFlag = false;
	}
}

void clear_alarms() 
{
	RTC_DS.armAlarm(1, false);
	RTC_DS.clearAlarm(1);
	RTC_DS.alarmInterrupt(1, false);
	RTC_DS.armAlarm(2, false);
	RTC_DS.clearAlarm(2);
	RTC_DS.alarmInterrupt(2, false);

}
