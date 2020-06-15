
#include "LowPower.h" 		// Sparkfun low power library found here https://github.com/rocketscream/Low-Power
#include <OPEnS_RTC.h>

#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>

const int DELAY_COUNT = 5; 		// Number of seconds to wait before running loop()
#define WAKE_PIN 12				// Attach DS3231 RTC Interrupt pin to this pin on Feather

#define DEBUG 1  //test to allow print to serial monitor


// Macros for printing to Serial if Loom Debug is enabled
#define DEBUG_Print(X)          (DEBUG==0) ? :  Serial.print(X)
#define DEBUG_Println(X)        (DEBUG==0) ? :  Serial.println(X)
#define DEBUG_Print2(X,Y)       DEBUG_Print(X); DEBUG_Print(Y)
#define DEBUG_Println2(X,Y)     DEBUG_Print(X); DEBUG_Println(Y)

// Instance of DS3231 RTC
RTC_DS3231 RTC_DS;


// ======= FUNCTION PROTOTYPES ======
void sleep();
void countdown();
void InitializeRTC();
void print_DateTime(DateTime time);


// ======= RTC ISR ======
void wakeUp_RTC()
{
	detachInterrupt(digitalPinToInterrupt(WAKE_PIN));
	RTC_DS.clearAlarm();
}


// ======= SETUP ======
void setup()
{
	pinMode(WAKE_PIN, INPUT_PULLUP);  // Pull up resistors required for Active-Low interrupts
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

#if DEBUG
	Serial.begin(115200);
	while (!Serial); // Won't start anything until serial is open, comment this line out if powering from battery
#endif
	countdown(); 

	InitializeRTC();
	DEBUG_Println("Setup Complete");
}


// ======= LOOP ======
void loop()
{
	// Pretent to do work
	DEBUG_Print("\nProcessing ");
	for (int i = 0; i < 3; i++) {
		delay(1000);
		DEBUG_Print(". ");
	}
	DEBUG_Println(" Done Processing");

	// Set alarms (5 seconds here)
	setRTCAlarm_Relative(0, 0, DELAY_COUNT);
	
	// Go to sleep
	sleep();
}


// ======= FUNCTIONS ======


// Simple function to call to hide
// Pre/post aux functions
void sleep()
{
	pre_sleep();
	LowPower.standby();			// Go to sleep here
	post_sleep();
}


// Prepare for sleep
// Disconnect Serial, attach interrupt, turn off LED
void pre_sleep()
{
	DEBUG_Println("\nEntering STANDBY");
	delay(50);
	Serial.end();
	USBDevice.detach();

	// Don't know why this has to happen twice but it does
	// I think it clears the interrupt flag
	attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp_RTC, LOW);
	attachInterrupt(digitalPinToInterrupt(WAKE_PIN), wakeUp_RTC, LOW);

	digitalWrite(LED_BUILTIN, LOW);
}


// Post sleep actions that should happen
// Clear alarms, reconnect Serial, turn on LED
void post_sleep()
{
	USBDevice.attach();
	Serial.begin(115200);
	digitalWrite(LED_BUILTIN, HIGH);
#if DEBUG == 1
	while(!Serial);	// Wait for the user to open the serial port
#endif
	DEBUG_Println("WAKE");

	print_DateTime(RTC_DS.now());
}


// Setup RTC time
void InitializeRTC()
{
	// RTC Timer settings here
	if (! RTC_DS.begin()) {
		DEBUG_Println("Couldn't find RTC");
		while (1);
	}
	// This may end up causing a problem in practice - what if RTC loses power in field? Shouldn't happen with coin cell batt backup
	if (RTC_DS.lostPower()) {
		DEBUG_Println("RTC lost power, lets set the time!");
		// Set the RTC to the date & time this sketch was compiled
		RTC_DS.adjust(DateTime(F(__DATE__), F(__TIME__)));
	}
	// Clear any pending alarms
	RTC_DS.clearAlarm();
}


// Set an RTC alarm some duration from now
void setRTCAlarm_Relative(int hours, int minutes, int seconds)
{
	DEBUG_Print("\nCurrent Time: ");
	print_DateTime( RTC_DS.now() );

	DateTime future( RTC_DS.now() + TimeSpan(0, hours, minutes, seconds) );
	DEBUG_Print("\nReset Alarm for: ");
	print_DateTime(future);

	RTC_DS.setAlarm(future);
}


// Debugging and upload helper,
// Allow uploads by waiting for serial monitor to open,
// prevent device from going to standby too soon
void countdown()
{
	DEBUG_Println("Countdown:");
	for (int count = DELAY_COUNT; count > 0; count--) {
		DEBUG_Println2(count, " s");
		delay(1000);
	}
}


// Print an RTC time
void print_DateTime(DateTime time)
{
	DEBUG_Println(time.text());
}
