
#include <OPEnS_RTC.h>

const int ALARM_DURATION = 10; 		// Number of seconds before alarm goes off

// Instance of DS3231 RTC
RTC_DS3231 RTC_DS; 


#define ALARM_PIN 12


volatile bool alarmFlag;
void alarmISR() { 
	detachInterrupt(digitalPinToInterrupt(ALARM_PIN)); 
	RTC_DS.clearAlarm();
	alarmFlag = true;
	Serial.println("Alarm triggered");
}


void setup() 
{ 
	pinMode(ALARM_PIN, INPUT_PULLUP);  // Pull up resistors required for Active-Low interrupts
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	RTC_DS.begin();
	DateTime t = DateTime(__DATE__, __TIME__); 
  t = t + TimeSpan(5);   //account for compilation time
  RTC_DS.adjust(t);

	Serial.begin(115200);
	while (!Serial); // Won't start anything until serial is open, comment this line out if powering from battery
	delay(1000);
	Serial.println("Initialized Serial");

	alarmFlag = true;

	Serial.println("\n ** Setup Complete ** ");
}

void loop() 
{
	if (alarmFlag) {
		digitalWrite(LED_BUILTIN, HIGH);
		delay(1000);
		
		// Setup interrupt and alarm
		// Mysterious issue: Must call this function twice
		attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, LOW );
		attachInterrupt( digitalPinToInterrupt(ALARM_PIN), alarmISR, LOW );

		DateTime alarmTime = RTC_DS.now()+TimeSpan(ALARM_DURATION);
		Serial.print("Alarm set to ");
		Serial.println(alarmTime.text());
		RTC_DS.setAlarm(alarmTime);
		digitalWrite(LED_BUILTIN, LOW);

		digitalWrite(LED_BUILTIN, LOW);
		alarmFlag = false;
	}
}
