# OPEnS_RTC-DS3231-PCF8523
A fork of https://github.com/FabioCuomo/FabioCuomo-DS3231 with added support for PCF8523 interrupts by incorporating [radikalbytes](https://github.com/radikalbytes) [PCF8523 library](https://github.com/radikalbytes/PCF8523).



A modified arduino library for DS3231 RTC

This is a fork of the Adafruit library https://github.com/adafruit/RTClib, which I extended for DS3231 only by integrating some additional commands found in Jack Christensen's library https://github.com/JChristensen/DS3232RTC.
I added commands for reading temperature and for using DS3231 alarms, that I needed for implementing wake-up of arduino on interrupt after entering the sleep state.

Update 06-Jan-2017

Added functions for reading and writing bytes on DS3231 memory control registers:
- byte read(byte addr);
- void write(byte addr, byte value);

Update 09-Jan-2017

1) DS3231 temperature registers are updated after every 64 seconds.
   Added function forceConversion() for forcing temperature reading.

2) Corrected bug in function for temperature reading.
   I found in internet a lot of wrong implementations.
   I made several tests in controlled environments and I can now ensure that current implementation returns correct values
   for both positive and negative temperatures.

Update 14-Mar-2019

1) PCF8523 support included from radikalbytes library
