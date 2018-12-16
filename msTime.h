// msTime.h

#ifndef _MSTIME_h
#define _MSTIME_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

//#include <string.h>
#include "msDateString.h"

typedef int32_t Epoch_t;

//convenience macros to convert to and from tm years
#define  eYearToCalendar(Y) ((Y) + 1970)  // full four digit year
#define  CalendarYrToTm(Y)   ((Y) - 1970)
#define  eYearToY2k(Y)      ((Y) - 30)    // offset is from 2000
#define  y2kYearToTm(Y)      ((Y) + 30)

/*==============================================================================*/
/* Useful Constants */
#define SECS_PER_MIN  ((Epoch_t)(60UL))
#define SECS_PER_HOUR ((Epoch_t)(3600UL))
#define SECS_PER_DAY  ((Epoch_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((Epoch_t)(7UL))
#define SECS_PER_WEEK ((Epoch_t)(SECS_PER_DAY * DAYS_PER_WEEK))
#define SECS_PER_YEAR ((Epoch_t)(SECS_PER_WEEK * 52UL))
#define SECS_YR_2000  ((Epoch_t)(946684800UL)) // the time at the start of y2k

/* Useful Macros for getting elapsed time */
#define numberOfSeconds(_time_) (_time_ % SECS_PER_MIN)
#define numberOfMinutes(_time_) ((_time_ / SECS_PER_MIN) % SECS_PER_MIN)
#define numberOfHours(_time_) (( _time_% SECS_PER_DAY) / SECS_PER_HOUR)
#define dayOfWeek(_time_)  ((( _time_ / SECS_PER_DAY + 4)  % DAYS_PER_WEEK)+1) // 1 = Sunday
#define elapsedDays(_time_) ( _time_ / SECS_PER_DAY)  // this is number of days since Jan 1 1970
#define elapsedSecsToday(_time_)  (_time_ % SECS_PER_DAY)   // the number of seconds since last midnight
// The following macros are used in calculating alarms and assume the clock is set to a date later than Jan 1 1971
// Always set the correct time before settting alarms
#define previousMidnight(_time_) (( _time_ / SECS_PER_DAY) * SECS_PER_DAY)  // time at the start of the given day
#define nextMidnight(_time_) ( previousMidnight(_time_)  + SECS_PER_DAY )   // time at the end of the given day
#define elapsedSecsThisWeek(_time_)  (elapsedSecsToday(_time_) +  ((dayOfWeek(_time_)-1) * SECS_PER_DAY) )   // note that week starts on day 1
#define previousSunday(_time_)  (_time_ - elapsedSecsThisWeek(_time_))      // time at the start of the week for the given time
#define nextSunday(_time_) ( previousSunday(_time_)+SECS_PER_WEEK)          // time at the end of the week for the given time

/* Useful Macros for converting elapsed time to a Epoch_t */
#define minutesToEpoch ((M)) ( (M) * SECS_PER_MIN)
#define hoursToEpoch  ((H)) ( (H) * SECS_PER_HOUR)
#define daysToEpoch   ((D)) ( (D) * SECS_PER_DAY) // fixed on Jul 22 2011

#define weeksToEpoch   ((W)) ( (W) * SECS_PER_WEEK)

#define LEAP_YEAR(Y)     ( ((1970+Y)>0) && !((1970+Y)%4) && ( ((1970+Y)%100) || !((1970+Y)%400) ) )
static  const uint8_t monthDays[] = { 31,28,31,30,31,30,31,31,30,31,30,31 }; // API starts months from 1, this array starts from 0

struct Time_t {
  uint8_t Hour = 0;
  uint8_t Minute = 0;
  uint8_t Second = 0;
  uint8_t Day = 1;
  uint8_t Month = 1;
  int8_t Year = 0;   // offset from 1970;
  uint8_t Wday = 5;   // day of week, sunday is day 1
  //uint32_t Millisecond;
  bool IsNegative = false;

  Time_t(uint8_t hour, uint8_t min, uint8_t sec = 0,
       uint8_t day = 1, uint8_t month = 1, int8_t year = 0,
       uint8_t wday = 5, bool negative = false) {
    Hour = hour;
    Minute = min;
    Second = sec;
    Day = day;
    Month = month;
    Year = year;
    Wday = wday;
    IsNegative = negative;
  }
  Time_t(const Time_t & time) {
    Hour = time.Hour;
    Minute = time.Minute;
    Second = time.Second;
    Day = time.Day;
    Month = time.Month;
    Year = time.Year;
    Wday = time.Wday;
    IsNegative = time.IsNegative;
  }
  Time_t() { }
};

#define dt_TEXT_BUFFER_SIZE 25
static char dtTextBuffer[dt_TEXT_BUFFER_SIZE];

class msTime {
protected:
  Time_t time = { 0,0,0,1,1,0,1 };
  Epoch_t epoch = 0;
  /*void(*syncProvider)() = nullptr;
  Epoch_t syncInterval = 0;*/

  static Time_t breakTime(Epoch_t _epoch) {
    uint8_t year;
    uint8_t month, monthLength;
    Time_t _time;
    unsigned long days;

    _time.IsNegative = (_epoch < 0);

    //_epoch = abs(_epoch);
    if (_epoch < 0)
      _epoch = -_epoch;
    _time.Second = _epoch % 60;
    _epoch /= 60; // now it is minutes
    _time.Minute = _epoch % 60;
    _epoch /= 60; // now it is hours
    _time.Hour = _epoch % 24;
    _epoch /= 24; // now it is days
    _time.Wday = ((_epoch + 4) % 7) + 1;  // Sunday is day 1

    year = 0;
    days = 0;

    while ((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= _epoch) {
      year++;
    }
    _time.Year = year; // year is offset from 1970

    days -= LEAP_YEAR(year) ? 366 : 365;
    _epoch -= days; // now it is days in this year, starting at 0

    days = 0;
    month = 0;
    monthLength = 0;
    for (month = 0; month < 12; month++) {
      if (month == 1) { // february
        if (LEAP_YEAR(year)) {
          monthLength = 29;
        } else {
          monthLength = 28;
        }
      } else {
        monthLength = monthDays[month];
      }

      if (_epoch >= monthLength) {
        _epoch -= monthLength;
      } else {
        break;
      }
    }
    _time.Month = month + 1;  // jan is month 1
    _time.Day = _epoch + 1;     // day of month
    return _time;
  }
  static Epoch_t makeEpoch(const Time_t _time) {
    uint32_t _epoch;
    // seconds from 1970 till 1 jan 00:00:00 of the given year
    _epoch = _time.Year*(SECS_PER_DAY * 365);
    for (uint16_t i = 0; i < _time.Year; i++) {
      if (LEAP_YEAR(i)) {
        _epoch += SECS_PER_DAY;   // add extra days for leap years
      }
    }

    // add days for this year, months start from 1
    for (uint16_t i = 1; i < _time.Month; i++) {
      if ((i == 2) && LEAP_YEAR(_time.Year)) {
        _epoch += SECS_PER_DAY * 29;
      } else {
        _epoch += SECS_PER_DAY * monthDays[i - 1];  //monthDay array starts from 0
      }
    }
    _epoch += (_time.Day - 1) * SECS_PER_DAY;
    _epoch += _time.Hour * SECS_PER_HOUR;
    _epoch += _time.Minute * SECS_PER_MIN;
    _epoch += _time.Second;
    if (_time.IsNegative)
      _epoch = -_epoch;

    return _epoch;
  }

  virtual void  doIncrement() {
    epoch++;
  }
public:
  msTime(const Epoch_t _epoch) {
    epoch = _epoch;
    time = breakTime(epoch);
  }
  msTime(const Time_t _time) {
    time = _time;
    epoch = makeEpoch(time);
  }
  msTime() {
    epoch = 0;
    time = breakTime(epoch);
  }
  Epoch_t getEpoch() { return epoch; }
  Time_t getTime() { return time; }

  virtual void setTime(const msTime _time) {
    epoch = _time.epoch;
    time = breakTime(epoch);
  }
  void adjustTime(const msTime adjustment) {
    epoch += adjustment.epoch;
    time = breakTime(epoch);
  }

  Epoch_t update() {
    static uint32_t prevMillis = 0;

    if (millis() < prevMillis + 1000)
      return epoch;

    while (millis() >= prevMillis + 1000) {
      doIncrement();
      prevMillis += 1000;
    }

    time = breakTime(epoch);
    return epoch;
  }
  // operator +
  msTime operator + (const msTime & rhs) {
    return msTime(this->epoch + rhs.epoch);
  }
  msTime operator + () {
    return msTime(this->epoch);
  }
  void operator += (const msTime & rhs) {
    adjustTime(rhs.epoch);
  }
  //operator -
  msTime operator - (const msTime & rhs) {
    return msTime(this->epoch - rhs.epoch);
  }
  msTime operator - () {
    return msTime(-this->epoch);
  }
  void operator -= (const msTime & rhs) {
    operator+=(-rhs.epoch);
  }
  //operator >
  bool operator > (const msTime & rhs) {
    return (this->epoch > rhs.epoch);
  }
  //operator >=
  bool operator >= (const msTime & rhs) {
    return (this->epoch >= rhs.epoch);
  }
  //operator <
  bool operator < (const msTime & rhs) {
    return (this->epoch < rhs.epoch);
  }
  //operator <=
  bool operator <= (const msTime & rhs) {
    return (this->epoch <= rhs.epoch);
  }
  //operator ==
  bool operator == (const msTime & rhs) {
    return (this->epoch == rhs.epoch);
  }
  //operator !=
  bool operator != (const msTime & rhs) {
    return (this->epoch != rhs.epoch);
  }

  msTime nextTimePeriod(Epoch_t sec) {
    if (sec < 1) sec = 1;
    msTime next(Time_t(this->getTime().Hour, 0, 0));
    while (next <= *this)
      next += sec;
    return next;
  }

  void setSyncProvider(void(*syncFunc)()) { }
  void setSyncInterval(Epoch_t interval) { }

  char * toString() {
    memset(dtTextBuffer, 0, sizeof(dtTextBuffer));
    sprintf(dtTextBuffer, "%c%02d:%02d:%02d %02d/%02d/%02d-%d", (time.IsNegative ? '-' : ' '), time.Hour, time.Minute, time.Second, time.Day, time.Month, eYearToY2k(time.Year), time.Wday);
    return dtTextBuffer;
  }
  char * toLongDateString() {
    memset(dtTextBuffer, '\0', sizeof(dtTextBuffer));
    strcat(dtTextBuffer, dayShortStr(time.Wday));
    sprintf(dtTextBuffer, "%s, %s %d %d", dtTextBuffer, monthShortStr(time.Month), time.Day, eYearToCalendar(((int16_t)time.Year)));
    return dtTextBuffer;
  }
  char * toShortDateString() {
    memset(dtTextBuffer, 0, sizeof(dtTextBuffer));
    sprintf(dtTextBuffer, "%02d/%02d/%02d", time.Day, time.Month, eYearToY2k(time.Year));
    return dtTextBuffer;
  }
  char * toTimeString() {
    memset(dtTextBuffer, 0, sizeof(dtTextBuffer));
    sprintf(dtTextBuffer, "%c%02d:%02d:%02d", (time.IsNegative ? '-' : ' '), time.Hour, time.Minute, time.Second);
    return dtTextBuffer;
  }
};

#endif

