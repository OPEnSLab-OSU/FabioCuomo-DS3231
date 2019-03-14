//Code by JeeLabs http://news.jeelabs.org/code/
// Released to the public domain! Enjoy!

#include <Wire.h>
#include "RTClibExtended.h"
#ifdef __AVR__
 #include <avr/pgmspace.h>
 
#elif defined(ESP8266)
 #include <pgmspace.h>
#elif defined(ARDUINO_ARCH_SAMD)
// nothing special needed
#elif defined(ARDUINO_SAM_DUE)
 #define PROGMEM
 #define pgm_read_byte(addr) (*(const unsigned char *)(addr))
 #define Wire Wire
#endif

#define WIRE Wire
#define _BV(n) (1 << n)

#if (ARDUINO >= 100)
 #include <Arduino.h> // capital A so it is error prone on case-sensitive filesystems
 // Macro to deal with the difference in I2C write functions from old and new Arduino versions.
 #define _I2C_WRITE write
 #define _I2C_READ  read
#else
 #include <WProgram.h>
 #define _I2C_WRITE send
 #define _I2C_READ  receive
#endif

static uint8_t read_i2c_register(uint8_t addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire.endTransmission();

  Wire.requestFrom(addr, (byte)1);
  return Wire._I2C_READ();
}

static void write_i2c_register(uint8_t addr, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(addr);
  Wire._I2C_WRITE((byte)reg);
  Wire._I2C_WRITE((byte)val);
  Wire.endTransmission();
}

////////////////////////////////////////////////////////////////////////////////
// utility code, some of this could be exposed in the DateTime API if needed

const uint8_t daysInMonth [] PROGMEM = { 31,28,31,30,31,30,31,31,30,31,30,31 };

// number of days since 2000/01/01, valid for 2001..2099
static uint16_t date2days(uint16_t y, uint8_t m, uint8_t d) {
    if (y >= 2000)
        y -= 2000;
    uint16_t days = d;
    for (uint8_t i = 1; i < m; ++i)
        days += pgm_read_byte(daysInMonth + i - 1);
    if (m > 2 && y % 4 == 0)
        ++days;
    return days + 365 * y + (y + 3) / 4 - 1;
}

static long time2long(uint16_t days, uint8_t h, uint8_t m, uint8_t s) {
    return ((days * 24L + h) * 60 + m) * 60 + s;
}

////////////////////////////////////////////////////////////////////////////////
// DateTime implementation - ignores time zones and DST changes
// NOTE: also ignores leap seconds, see http://en.wikipedia.org/wiki/Leap_second

DateTime::DateTime (uint32_t t) {
  t -= SECONDS_FROM_1970_TO_2000;    // bring to 2000 timestamp from 1970

    ss = t % 60;
    t /= 60;
    mm = t % 60;
    t /= 60;
    hh = t % 24;
    uint16_t days = t / 24;
    uint8_t leap;
    for (yOff = 0; ; ++yOff) {
        leap = yOff % 4 == 0;
        if (days < 365 + leap)
            break;
        days -= 365 + leap;
    }
    for (m = 1; ; ++m) {
        uint8_t daysPerMonth = pgm_read_byte(daysInMonth + m - 1);
        if (leap && m == 2)
            ++daysPerMonth;
        if (days < daysPerMonth)
            break;
        days -= daysPerMonth;
    }
    d = days + 1;
}

DateTime::DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t min, uint8_t sec) {
    if (year >= 2000)
        year -= 2000;
    yOff = year;
    m = month;
    d = day;
    hh = hour;
    mm = min;
    ss = sec;
}

DateTime::DateTime (const DateTime& copy):
  yOff(copy.yOff),
  m(copy.m),
  d(copy.d),
  hh(copy.hh),
  mm(copy.mm),
  ss(copy.ss)
{}

static uint8_t conv2d(const char* p) {
    uint8_t v = 0;
    if ('0' <= *p && *p <= '9')
        v = *p - '0';
    return 10 * v + *++p - '0';
}

// A convenient constructor for using "the compiler's time":
//   DateTime now (__DATE__, __TIME__);
// NOTE: using F() would further reduce the RAM footprint, see below.
DateTime::DateTime (const char* date, const char* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    yOff = conv2d(date + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec 
    switch (date[0]) {
        case 'J': m = date[1] == 'a' ? 1 : m = date[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = date[2] == 'r' ? 4 : 8; break;
        case 'M': m = date[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(date + 4);
    hh = conv2d(time);
    mm = conv2d(time + 3);
    ss = conv2d(time + 6);
}

// A convenient constructor for using "the compiler's time":
// This version will save RAM by using PROGMEM to store it by using the F macro.
//   DateTime now (F(__DATE__), F(__TIME__));
DateTime::DateTime (const __FlashStringHelper* date, const __FlashStringHelper* time) {
    // sample input: date = "Dec 26 2009", time = "12:34:56"
    char buff[11];
    memcpy_P(buff, date, 11);
    yOff = conv2d(buff + 9);
    // Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec
    switch (buff[0]) {
        case 'J': m = buff[1] == 'a' ? 1 : m = buff[2] == 'n' ? 6 : 7; break;
        case 'F': m = 2; break;
        case 'A': m = buff[2] == 'r' ? 4 : 8; break;
        case 'M': m = buff[2] == 'r' ? 3 : 5; break;
        case 'S': m = 9; break;
        case 'O': m = 10; break;
        case 'N': m = 11; break;
        case 'D': m = 12; break;
    }
    d = conv2d(buff + 4);
    memcpy_P(buff, time, 8);
    hh = conv2d(buff);
    mm = conv2d(buff + 3);
    ss = conv2d(buff + 6);
}

uint8_t DateTime::dayOfTheWeek() const {    
    uint16_t day = date2days(yOff, m, d);
    return (day + 6) % 7; // Jan 1, 2000 is a Saturday, i.e. returns 6
}

uint32_t DateTime::unixtime(void) const {
  uint32_t t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  t += SECONDS_FROM_1970_TO_2000;  // seconds from 1970 to 2000

  return t;
}

long DateTime::secondstime(void) const {
  long t;
  uint16_t days = date2days(yOff, m, d);
  t = time2long(days, hh, mm, ss);
  return t;
}

DateTime DateTime::operator+(const TimeSpan& span) {
  return DateTime(unixtime()+span.totalseconds());
}

DateTime DateTime::operator-(const TimeSpan& span) {
  return DateTime(unixtime()-span.totalseconds());
}

TimeSpan DateTime::operator-(const DateTime& right) {
  return TimeSpan(unixtime()-right.unixtime());
}

////////////////////////////////////////////////////////////////////////////////
// TimeSpan implementation

TimeSpan::TimeSpan (int32_t seconds):
  _seconds(seconds)
{}

TimeSpan::TimeSpan (int16_t days, int8_t hours, int8_t minutes, int8_t seconds):
  _seconds((int32_t)days*86400L + (int32_t)hours*3600 + (int32_t)minutes*60 + seconds)
{}

TimeSpan::TimeSpan (const TimeSpan& copy):
  _seconds(copy._seconds)
{}

TimeSpan TimeSpan::operator+(const TimeSpan& right) {
  return TimeSpan(_seconds+right._seconds);
}

TimeSpan TimeSpan::operator-(const TimeSpan& right) {
  return TimeSpan(_seconds-right._seconds);
}

static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
static uint8_t bin2bcd (uint8_t val) { return val + 6 * (val / 10); }

////////////////////////////////////////////////////////////////////////////////
// PCF8523 implementation

uint8_t PCF8523::begin(void) {
  return 1;
}

// Example: bool a = PCF8523.isrunning();
// Returns 1 if RTC is running and 0 it's not 
uint8_t PCF8523::isrunning(void) {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(0);
  WIRE.endTransmission();

  WIRE.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t ss = WIRE._I2C_READ();
  ss = ss & 32;
  return !(ss>>5);
}

// Example: PCF8523.adjust (DateTime(2014, 8, 14, 1, 49, 0))
// Sets RTC time to 2014/14/8 1:49 a.m.
void PCF8523::adjust(const DateTime& dt) {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(0x03);
  WIRE._I2C_WRITE(bin2bcd(dt.second()));
  WIRE._I2C_WRITE(bin2bcd(dt.minute()));
  WIRE._I2C_WRITE(bin2bcd(dt.hour()));
  WIRE._I2C_WRITE(bin2bcd(dt.day()));
  WIRE._I2C_WRITE(bin2bcd(0));
  WIRE._I2C_WRITE(bin2bcd(dt.month()));
  WIRE._I2C_WRITE(bin2bcd(dt.year() - 2000));
  WIRE._I2C_WRITE(0);
  WIRE.endTransmission();
}

// Example: DateTime now = PCF8523.now();
// Returns Date and time in RTC in:
// year = now.year()
// month = now.month()
// day = now.day()
// hour = now.hour()
// minute = now.minute()
// second = now.second()
DateTime PCF8523::now() {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(3);   
  WIRE.endTransmission();

  WIRE.requestFrom(PCF8523_ADDRESS, 7);
  uint8_t ss = bcd2bin(WIRE._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(WIRE._I2C_READ());
  uint8_t hh = bcd2bin(WIRE._I2C_READ());
  uint8_t d = bcd2bin(WIRE._I2C_READ());
  WIRE._I2C_READ();
  uint8_t m = bcd2bin(WIRE._I2C_READ());
  uint16_t y = bcd2bin(WIRE._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

// Example: PCF8523.read_reg(buf,size,address);
// Returns:   buf[0] = &address
//            buf[1] = &address + 1
//      ..... buf[size-1] = &address + size
void PCF8523::read_reg(uint8_t* buf, uint8_t size, uint8_t address) {
  int addrByte = address;
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(addrByte);
  WIRE.endTransmission();
  
  WIRE.requestFrom((uint8_t) PCF8523_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = WIRE._I2C_READ();
  }
}

// Example: PCF8523.write_reg(address,buf,size);
// Write:     buf[0] => &address
//            buf[1] => &address + 1
//      ..... buf[size-1] => &address + size
void PCF8523::write_reg(uint8_t address, uint8_t* buf, uint8_t size) {
  int addrByte = address;
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    WIRE._I2C_WRITE(buf[pos]);
  }
  WIRE.endTransmission();
}

// Example: val = PCF8523.read_reg(0x08);
// Reads the value in register addressed at 0x08
// and returns data
uint8_t PCF8523::read_reg(uint8_t address) {
  uint8_t data;
  read_reg(&data, 1, address);
  return data;
}

// Example: PCF8523.write_reg(0x08, 0x25);
// Writes value 0x25 in register addressed at 0x08
void PCF8523::write_reg(uint8_t address, uint8_t data) {
  write_reg(address, &data, 1);
}

// Example: PCF8523.set_alarm(10,5,45)
// Set alarm at day = 5, 5:45 a.m.
void PCF8523::set_alarm(uint8_t day_alarm, uint8_t hour_alarm,uint8_t minute_alarm ) {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(0x0A);
  WIRE._I2C_WRITE(bin2bcd(minute_alarm));
  WIRE._I2C_WRITE(bin2bcd(hour_alarm));
  WIRE._I2C_WRITE(bin2bcd(day_alarm));
  WIRE._I2C_WRITE(0);
  WIRE.endTransmission();
}

void PCF8523::set_alarm(uint8_t hour_alarm,uint8_t minute_alarm ) {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(0x0A);
  WIRE._I2C_WRITE(bin2bcd(minute_alarm));
  WIRE._I2C_WRITE(bin2bcd(hour_alarm));
  WIRE._I2C_WRITE(bin2bcd(0));
  WIRE._I2C_WRITE(0);
  WIRE.endTransmission();
}

// Example: PCF8523.get_alarm(a);
// Returns a[0] = alarm minutes, a[1] = alarm hour, a[2] = alarm day
void PCF8523::get_alarm(uint8_t* buf) {
  WIRE.beginTransmission(PCF8523_ADDRESS);
  WIRE._I2C_WRITE(0x0A);
  WIRE.endTransmission();
  WIRE.requestFrom((uint8_t) PCF8523_ADDRESS, (uint8_t)3);
  for (uint8_t pos = 0; pos < 3; ++pos) {
    buf[pos] = bcd2bin((WIRE._I2C_READ() & 0x7F));
  }
  
}

void PCF8523::start_counter_1(uint8_t value){
    // Set timer freq at 1Hz
    write_reg(PCF8523_TMR_A_FREQ_CTRL , 2);
    // Load Timer value
    write_reg(PCF8523_TMR_A_REG,value); 
    // Set counter mode TAC[1:0] = 01 
    // Disable Clockout
    uint8_t tmp;
    tmp = read_reg(PCF8523_TMR_CLKOUT_CTRL);
    tmp |= (1<<7)|(1<<5)|(1<<4)|(1<<3)|(1<<1);
    tmp &= ~(1<<2);
    write_reg(PCF8523_TMR_CLKOUT_CTRL , tmp);
    // Set countdown flag CTAF
    // Enable interrupt CTAIE
    tmp = read_reg(PCF8523_CONTROL_2);
    tmp|=_BV(PCF8523_CONTROL_2_CTAF_BIT)|_BV(PCF8523_CONTROL_2_CTAIE_BIT);
    write_reg(PCF8523_CONTROL_2,tmp);
}

// Example: PCF8523.reset();
// Reset the PCF8523
void PCF8523::reset(){
    write_reg(PCF8523_CONTROL_1, 0x58);
}

uint8_t PCF8523::clear_rtc_interrupt_flags() {
  uint8_t rc2 = read_reg(PCF8523_CONTROL_2) & (PCF8523_CONTROL_2_SF_BIT | PCF8523_CONTROL_2_AF_BIT);
  write_reg(PCF8523_CONTROL_2, 0);  // Just zero the whole thing
  return rc2 != 0;
}

// Stop the default 32.768KHz CLKOUT signal on INT1.
void PCF8523::stop_32768_clkout() {
    uint8_t tmp = (read_reg (PCF8523_TMR_CLKOUT_CTRL))|RTC_CLKOUT_DISABLED;

    write_reg(PCF8523_TMR_CLKOUT_CTRL , tmp);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS1307 implementation



boolean RTC_DS1307::begin(void) {
  Wire.begin();
  return true;
}

uint8_t RTC_DS1307::isrunning(void) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return !(ss>>7);
}

void RTC_DS1307::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();
}

DateTime RTC_DS1307::now() {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE((byte)0);	
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

Ds1307SqwPinMode RTC_DS1307::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS1307_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds1307SqwPinMode>(mode);
}

void RTC_DS1307::writeSqwPinMode(Ds1307SqwPinMode mode) {
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(DS1307_CONTROL);
  Wire._I2C_WRITE(mode);
  Wire.endTransmission();
}

void RTC_DS1307::readnvram(uint8_t* buf, uint8_t size, uint8_t address) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t) DS1307_ADDRESS, size);
  for (uint8_t pos = 0; pos < size; ++pos) {
    buf[pos] = Wire._I2C_READ();
  }
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t* buf, uint8_t size) {
  int addrByte = DS1307_NVRAM + address;
  Wire.beginTransmission(DS1307_ADDRESS);
  Wire._I2C_WRITE(addrByte);
  for (uint8_t pos = 0; pos < size; ++pos) {
    Wire._I2C_WRITE(buf[pos]);
  }
  Wire.endTransmission();
}

uint8_t RTC_DS1307::readnvram(uint8_t address) {
  uint8_t data;
  readnvram(&data, 1, address);
  return data;
}

void RTC_DS1307::writenvram(uint8_t address, uint8_t data) {
  writenvram(address, &data, 1);
}

////////////////////////////////////////////////////////////////////////////////
// RTC_Millis implementation

long RTC_Millis::offset = 0;

void RTC_Millis::adjust(const DateTime& dt) {
    offset = dt.unixtime() - millis() / 1000;
}

DateTime RTC_Millis::now() {
  return (uint32_t)(offset + millis() / 1000);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// RTC_PCF8563 implementation

boolean RTC_PCF8523::begin(void) {
  Wire.begin();
  return true;
}

boolean RTC_PCF8523::initialized(void) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 1);
  uint8_t ss = Wire._I2C_READ();
  return ((ss & 0xE0) != 0xE0);
}

void RTC_PCF8523::adjust(const DateTime& dt) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3); // start at location 3
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(0)); // skip weekdays
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();

  // set to battery switchover mode
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)PCF8523_CONTROL_3);
  Wire._I2C_WRITE((byte)0x00);
  Wire.endTransmission();
}

DateTime RTC_PCF8523::now() {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE((byte)3);	
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  uint8_t d = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();  // skip 'weekdays'
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

Pcf8523SqwPinMode RTC_PCF8523::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)PCF8523_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode >>= 3;
  mode &= 0x7;
  return static_cast<Pcf8523SqwPinMode>(mode);
}

void RTC_PCF8523::writeSqwPinMode(Pcf8523SqwPinMode mode) {
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire._I2C_WRITE(PCF8523_CLKOUTCONTROL);
  Wire._I2C_WRITE(mode << 3);
  Wire.endTransmission();
}

////////////////////////////////////////////////////////////////////////////////
// RTC_DS3231 implementation

boolean RTC_DS3231::begin(void) {
  Wire.begin();
  return true;
}

bool RTC_DS3231::lostPower(void) {
  return (read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG) >> 7);
}

void RTC_DS3231::adjust(const DateTime& dt) {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0); // start at location 0
  Wire._I2C_WRITE(bin2bcd(dt.second()));
  Wire._I2C_WRITE(bin2bcd(dt.minute()));
  Wire._I2C_WRITE(bin2bcd(dt.hour()));
  Wire._I2C_WRITE(bin2bcd(0));
  Wire._I2C_WRITE(bin2bcd(dt.day()));
  Wire._I2C_WRITE(bin2bcd(dt.month()));
  Wire._I2C_WRITE(bin2bcd(dt.year() - 2000));
  Wire.endTransmission();

  uint8_t statreg = read_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG);
  statreg &= ~0x80; // flip OSF bit
  write_i2c_register(DS3231_ADDRESS, DS3231_STATUSREG, statreg);
}

DateTime RTC_DS3231::now() {
  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE((byte)0);	
  Wire.endTransmission();

  Wire.requestFrom(DS3231_ADDRESS, 7);
  uint8_t ss = bcd2bin(Wire._I2C_READ() & 0x7F);
  uint8_t mm = bcd2bin(Wire._I2C_READ());
  uint8_t hh = bcd2bin(Wire._I2C_READ());
  Wire._I2C_READ();
  uint8_t d = bcd2bin(Wire._I2C_READ());
  uint8_t m = bcd2bin(Wire._I2C_READ());
  uint16_t y = bcd2bin(Wire._I2C_READ()) + 2000;
  
  return DateTime (y, m, d, hh, mm, ss);
}

Ds3231SqwPinMode RTC_DS3231::readSqwPinMode() {
  int mode;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire._I2C_WRITE(DS3231_CONTROL);
  Wire.endTransmission();
  
  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
  mode = Wire._I2C_READ();

  mode &= 0x93;
  return static_cast<Ds3231SqwPinMode>(mode);
}

void RTC_DS3231::writeSqwPinMode(Ds3231SqwPinMode mode) {
  uint8_t ctrl;
  ctrl = read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL);

  ctrl &= ~0x04; // turn off INTCON
  ctrl &= ~0x18; // set freq bits to 0

  if (mode == DS3231_OFF) {
    ctrl |= 0x04; // turn on INTCN
  } else {
    ctrl |= mode;
  } 
  write_i2c_register(DS3231_ADDRESS, DS3231_CONTROL, ctrl);

  //Serial.println( read_i2c_register(DS3231_ADDRESS, DS3231_CONTROL), HEX);
}

/*----------------------------------------------------------------------*/

float RTC_DS3231::getTemp() {
  int8_t temp_msb, temp_lsb;

  Wire.beginTransmission(DS3231_ADDRESS);
  Wire.write(DS3231_TEMP);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)2);
  temp_msb = Wire._I2C_READ();
  temp_lsb = (Wire._I2C_READ() >> 6) & 0x03;
  Wire.endTransmission();

  if(temp_msb & 0b10000000) {     //check if negative number
    temp_msb  ^= 0b11111111;
    temp_msb  += 0x1;
    return (-1.0 * ((float)temp_msb) + ((float)temp_lsb * 0.25));
  }
  else {
    return ((float)temp_msb + ((float)temp_lsb * 0.25));
  }
}

/*----------------------------------------------------------------------*
 * Enable or disable an alarm "interrupt" which asserts the INT pin     *
 * on the RTC.                                                          *
 *----------------------------------------------------------------------*/
void RTC_DS3231::alarmInterrupt(byte alarmNumber, bool interruptEnabled)
{
    uint8_t controlReg, mask;
    
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    controlReg = Wire.endTransmission();
    if (!controlReg) {
      Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
      controlReg = Wire._I2C_READ();
      Wire.endTransmission();
    }

    mask = _BV(A1IE) << (alarmNumber - 1);
    if (interruptEnabled)
        controlReg |= mask;
    else
        controlReg &= ~mask;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.write(controlReg);
    Wire.endTransmission();
}

/*----------------------------------------------------------------------*
 * Set an alarm time. Sets the alarm registers only.  To cause the      *
 * INT pin to be asserted on alarm match, use alarmInterrupt().         *
 * This method can set either Alarm 1 or Alarm 2, depending on the      *
 * value of alarmType (use a value from the ALARM_TYPES_t enumeration). *
 * When setting Alarm 2, the seconds value must be supplied but is      *
 * ignored, recommend using zero. (Alarm 2 has no seconds register.)    *
 *----------------------------------------------------------------------*/
void RTC_DS3231::setAlarm(Ds3231_ALARM_TYPES_t alarmType, byte seconds, byte minutes, byte hours, byte daydate){

    uint8_t addr;
    byte alarmNumber;
    
    seconds = bin2bcd(seconds);
    minutes = bin2bcd(minutes);
    hours = bin2bcd(hours);
    daydate = bin2bcd(daydate);
    if (alarmType & 0x01) seconds |= _BV(A1M1);
    if (alarmType & 0x02) minutes |= _BV(A1M2);
    if (alarmType & 0x04) hours |= _BV(A1M3);
    if (alarmType & 0x10) hours |= _BV(DYDT);
    if (alarmType & 0x08) daydate |= _BV(A1M4);
    
    if ( !(alarmType & 0x80) ) {    //alarm 1
        alarmNumber = 1;
        addr = ALM1_SECONDS;
        Wire.beginTransmission(DS3231_ADDRESS);
        Wire.write(addr++);
        Wire.write(seconds);
        Wire.endTransmission();
    }
    else {
        alarmNumber = 2;
        addr = ALM2_MINUTES;      //alarm 2
    }

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(addr++);
    Wire.write(minutes);
    Wire.endTransmission();

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(addr++);
    Wire.write(hours);
    Wire.endTransmission();

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(addr++);
    Wire.write(daydate);
    Wire.endTransmission();

    armAlarm(alarmNumber, true);
    clearAlarm(alarmNumber);
}

/*----------------------------------------------------------------------*
 * Set an alarm time. Sets the alarm registers only.  To cause the      *
 * INT pin to be asserted on alarm match, use alarmInterrupt().         *
 * This method can set either Alarm 1 or Alarm 2, depending on the      *
 * value of alarmType (use a value from the ALARM_TYPES_t enumeration). *
 * However, when using this method to set Alarm 1, the seconds value    *
 * is set to zero. (Alarm 2 has no seconds register.)                   *
 *----------------------------------------------------------------------*/
void RTC_DS3231::setAlarm(Ds3231_ALARM_TYPES_t alarmType, byte minutes, byte hours, byte daydate) {
    setAlarm(alarmType, 0, minutes, hours, daydate);
}

/*----------------------------------------------------------------------*
 * This method arms or disarms Alarm 1 or Alarm 2, depending on the     *
 * value of alarmNumber (1 or 2) and arm (true or false).               *
 *----------------------------------------------------------------------*/
void RTC_DS3231::armAlarm(byte alarmNumber, bool armed) {
    uint8_t value, mask;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = Wire._I2C_READ();
    Wire.endTransmission();

    mask = _BV(alarmNumber - 1);
    if (armed) {
        value |= mask;
    }
    else {
        value &= ~mask;
    }

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.write(value);
    Wire.endTransmission();
}

/*----------------------------------------------------------------------*
 * This method clears the status register of Alarm 1 or Alarm 2,        *
 * depending on the value of alarmNumber (1 or 2).                      *
 *----------------------------------------------------------------------*/
void RTC_DS3231::clearAlarm(byte alarmNumber) {
    uint8_t value, mask;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_STATUSREG);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = Wire._I2C_READ();
    Wire.endTransmission();

    mask = _BV(alarmNumber - 1);
    value &= ~mask;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_STATUSREG);
    Wire.write(value);
    Wire.endTransmission();
}

/*----------------------------------------------------------------------*
 * This method can check either Alarm 1 or Alarm 2, depending on the    *
 * value of alarmNumber (1 or 2).                                       *
 *----------------------------------------------------------------------*/
bool RTC_DS3231::isArmed(byte alarmNumber) {
    uint8_t value;
    
    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = Wire._I2C_READ();
    Wire.endTransmission();

    if (alarmNumber == 1) {
      value &= 0b00000001;
    }
    else {
      value &= 0b00000010;
      value >>= 1;
    }
    return value;
}

/*----------------------------------------------------------------------*
 * This method writes a single byte in RTC memory                       *
 * Valid address range is 0x00 - 0x12, no checking.                     *
 *----------------------------------------------------------------------*/
void RTC_DS3231::write(byte addr, byte value) {

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(addr);
    Wire.write(value);
    Wire.endTransmission();
}

/*----------------------------------------------------------------------*
 * This method reads a single byte from RTC memory                      *
 * Valid address range is 0x00 - 0x12, no checking.                     *
 *----------------------------------------------------------------------*/
byte RTC_DS3231::read(byte addr) {
    uint8_t value;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(addr);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = Wire._I2C_READ();
    Wire.endTransmission();

    return value;
}

/*----------------------------------------------------------------------*
 * The temperature registers are updated after every 64-second          *
 * conversion. If you want force temperature conversion call this       *
 * function.                                                            *
 *----------------------------------------------------------------------*/
void RTC_DS3231::forceConversion(void) {
    uint8_t value;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.endTransmission();

    Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
    value = Wire._I2C_READ();
    Wire.endTransmission();

    value |= 0b00100000;

    Wire.beginTransmission(DS3231_ADDRESS);
    Wire.write(DS3231_CONTROL);
    Wire.write(value);
    Wire.endTransmission();

    do {
      Wire.beginTransmission(DS3231_ADDRESS);
      Wire.write(DS3231_CONTROL);
      Wire.endTransmission();

      Wire.requestFrom((uint8_t)DS3231_ADDRESS, (uint8_t)1);
      value = Wire._I2C_READ();
      Wire.endTransmission();
    } while ((value & 0b00100000) != 0);
} 
 
