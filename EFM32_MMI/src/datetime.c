/*******************************************************************
**                                                                **
** Date/Time management functions                                 **
**                                                                **
*******************************************************************/

#include "datetime.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
/* Private helpers */

static const unsigned short nbDays[] = {31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 };
#define daysinyear(m,y) ((((m) > 0) && ((m) < 13)) ? (nbDays[(m)-1] + (((((y)%4) == 0) && ((m) > 1)) ? 1 : 0)) : 0)
static const unsigned char daysInMonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
static short _timeOffset = 0;
static unsigned char *_DSTSpecs = NULL; //EUROPEAN_DST;

static void alignShortCopy(unsigned char *buffer, unsigned short i) {
  buffer[0] = (unsigned char)(i & 0xFF);
  buffer[1] = (unsigned char)((i >> 8) & 0xFF);
}

static unsigned short alignGetShort(unsigned char* buffer) {
  return (buffer[1] * 256) + buffer[0];
}

unsigned short DateTimeDaysInYear(unsigned char month, unsigned short year) {
	return daysinyear(month,year);
}

unsigned char DateTimeDaysInMonth(unsigned char month, unsigned short year) {
	if (!month || (month > 12)) return 0;
	return daysInMonth[month-1] + ((((year %4)==0) && (month==2)) ? 1 : 0);
}

void DateTimeFromSeconds(const unsigned long seconds, DATETIME_STRUCT* dt) {
  if (dt) {
	  memset(dt,0,sizeof(DATETIME_STRUCT));
    // Calculate the number of days
    unsigned long N = seconds / 86400UL;	// (24*60*60) = 86400 seconds per day
    if (N) {
    	// Set Day of week
    	// if N = 1 -> 01Jan2000 = Saturday - So offset day of week by 6-1
    	dt->dayOfWeek = ((N % 7) + 5) % 7;
		// Calculate Year
		unsigned short year = 0;	// Starting year is 2000
		while (N > daysinyear(12,year))
		{
			N -=daysinyear(12,year);
			year++;
		}
		// Compute current month
		dt->month = 0;
		while ((++dt->month < 12) && (N > daysinyear(dt->month,year))) ;
		N -= daysinyear(dt->month-1,year);
		dt->day = (unsigned char)N;
	    alignShortCopy((unsigned char*)&dt->year,year+2000);
    }
    else
    {
    	dt->dayOfWeek = DateTimeFriday;
    	dt->month = 12;
    	dt->day = 31;
        alignShortCopy((unsigned char*)&dt->year,1999);
    }
    // This strange code is necessary if the provided buffer is not memory aligned
    // Compute current time
    N = seconds % 86400UL;
    dt->hour= (unsigned char)(N / 3600UL);     	// (24*60*60) = 86400 seconds per day
    N %= 3600UL;
    dt->min= (unsigned char)(N / 60UL);      // 60 seconds per minute
    dt->sec= (unsigned char) (N % 60UL);
  }
}

unsigned long DateTimeGetSeconds(const DATETIME_STRUCT *dt)
{
  unsigned short year;
  if (dt) {
    // This strange code is necessary if the provided buffer is not aligned
    year = alignGetShort((unsigned char*)&dt->year);
    if (year < 2000) return 0;
    unsigned long N = (365*(year-2000)) + ((year-2000) / 4) + 1;	// Compute number of days since 2000 is a leap year
    if ((year%4)==0) N--;                         // Catch current leap year
    N += daysinyear(dt->month-1,year);	    // Compute elapsed days for the previous months
    N += dt->day;					    // Add the number of days
    N *= 86400L;
    N+= (unsigned long)dt->hour * 3600L;
    N+= (unsigned long)dt->min * 60L;
    N+= dt->sec;
    N-= (dt->zoneOffset * 3600);
//    N = ((((N*24)+dt->hour)*60)+dt->min)*60+dt->sec;  // Transfer days/hour/min into seconds
    return N;
  }
  else
    return 0;
}

unsigned char DateTimeStringFormat(const unsigned long time,unsigned char *Buffer)
{
  return DateTimeStringFormatExt(time,Buffer,'/',':');
}

unsigned char DateTimeStringFormatExt(const unsigned long time,unsigned char *Buffer, const unsigned char DateDelimiter, const unsigned char HourDelimiter)
{
  if (Buffer == (unsigned char*)0) return 0; // Avoid null pointer assignments
  DATETIME_STRUCT timestamp;
  DateTimeFromSeconds(time, &timestamp);
  return (unsigned char)sprintf((char*)Buffer,"%04d%c%02d%c%02d %02d%c%02d%c%02d",
		  timestamp.year,DateDelimiter,
		  timestamp.month,DateDelimiter,
		  timestamp.day,
		  timestamp.hour,HourDelimiter,
		  timestamp.min,HourDelimiter,
		  timestamp.sec
		  );
}
unsigned char DateTimeSecondsToISO8601Format(const unsigned long time, unsigned char* buffer) {
  if (buffer == (unsigned char*)0) return 0; // Avoid null pointer assignements
  DATETIME_STRUCT timestamp;
  DateTimeFromSeconds(time, &timestamp);
  return DateTimeToISO8601Format(&timestamp, buffer);
}

unsigned char DateTimeToISO8601Format(const DATETIME_STRUCT *timestamp,unsigned char* Buffer)
{
  if (Buffer == (unsigned char*)0) return 0; // Avoid null pointer assignments
  return (unsigned char)sprintf((char*)Buffer,"%04d-%02d-%02dT%02d:%02d:%02d%+02d:00",
		  timestamp->year,
		  timestamp->month,
		  timestamp->day,
		  timestamp->hour,
		  timestamp->min,
		  timestamp->sec,
		  timestamp->zoneOffset
		  );
}

unsigned long DateTimeSecondsFromISO8601String(const unsigned char* string)
{
  DATETIME_STRUCT timestamp;
  if (string != (unsigned char*)0) {
	  if (DateTimeFromISO8601String(&timestamp, string))
		  return DateTimeGetSeconds(&timestamp);
  }
  return 0L;
}
// 0000000000111111111122222
// 0123456789012345678901234
// 2015-02-15T11:00:00+00:00
unsigned char DateTimeFromISO8601String(DATETIME_STRUCT *dt, const unsigned char *string)
{
  if (string == (unsigned char*)0) return 0;
  if (dt == (DATETIME_STRUCT*)0) return 0;
  memset(dt,0,sizeof(DATETIME_STRUCT));
  if ((string[4] != string[7]) && (string[4] != '-') && (string[13] != string[16]) && (string[13] != ':')) return 0;
  unsigned short year = (unsigned short)atoi((char const*)string);
  if (year < 2000) return 0;
  // This strange code is necessary if the provided buffer is not memory aligned
  alignShortCopy((unsigned char*)&dt->year,year);
  dt->month = (unsigned char)atoi((char const*)string+5);
  if ((dt->month < 1) || (dt->month > 12)) return 0;
  dt->day = (unsigned char)atoi((char const*)string+8);
  if ((dt->day < 1) || (dt->day > DateTimeDaysInMonth(dt->month,dt->year))) return 0;
  dt->hour = (unsigned char)atoi((char const*)string+11);
  if (dt->hour > 23) return 0;
  dt->min = (unsigned char)atoi((char const*)string+14);
  if (dt->min > 59) return 0;
  dt->sec = (unsigned char)atoi((char const*)string+17);
  if (dt->sec > 59) return 0;
  if ((string[19] == '+') || (string[19] == '-')) {
	unsigned long delta = (unsigned long)atoi((char const*)string+20) * 3600L;
	if (string[22] == ':') delta += (unsigned long)atoi((char const*)string+23) * 60L;
	if (delta) {
		unsigned long t = DateTimeGetSeconds(dt);
		if (string[19] == '+') {
			t += delta;
		} else {
			t -= delta;
		}
		DateTimeFromSeconds(t,dt);
		dt->zoneOffset = (string[19] == '+') ? (delta / 3600L) : (-delta/3600L);
	}
  }
  return 1;
}

unsigned long DateTimeSecondsToFatTime(unsigned long seconds)
{
  // Calculate the number of days
  unsigned short N = seconds / 86400;	// (24*60*60) = 86400 seconds per day
  // Calculate Year
  unsigned short year = 0;	// Starting year is 2000
  while (N > (((year % 4) == 0) ? 366 : 365))
  {
      N -=(((year % 4) == 0) ? 366 : 365);
      year++;
  }
  // Compute current month
  unsigned char month = 0;
  while ((++month < 12) && (N > daysinyear(month,year))) ;
  N -= daysinyear(month-1,year);
  short day = (N) ? N : 1;
  year += 2000;
  // Compute current time
  short hour=(seconds % 86400)/3600;     	// (24*60*60) = 86400 seconds per day
  short min= (seconds % 86400)%3600 / 60;      // 60 seconds per minute
  short sec= (seconds % 86400)%3600 % 60;
  return  ((unsigned long)(year - 1980) << 25)
                | ((unsigned long)month << 21)
                | ((unsigned long)day << 16)
                | ((unsigned long)hour << 11)
                | ((unsigned long)min << 5)
                | ((unsigned long)sec >> 1);
}
unsigned long DateTimeFatTimeToSeconds(unsigned long fatTime) {
    short year = (short)((fatTime >> 25) + 1980);			// DOS year
    unsigned long N = (365*(year-2000)) + ((year-2000) / 4) + 1;	// Compute number of days since 2000 is a leap year
    if ((year%4)==0) N--;                         			// Catch current leap year
    N += daysinyear(((fatTime >> 21) & 0x0F)-1,year);	    // Compute elapsed days for the previous months
    N += (fatTime >> 16) & 0x1F;					    	// Add the number of days
    N *= 86400L;
    N+= ((fatTime >> 11) & 0x1F) * 3600L;
    N+= ((fatTime >> 5) & 0x3F) * 60L;
    N+= (fatTime & 0x1F) << 1;
    return N;
}

unsigned char DateTimeDayOfWeek(int year, unsigned char month, unsigned char day)
{
    static const unsigned char t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
    year -= month < 3;
    return (year + (year/4) - (year/100) + (year/400) + t[month-1] + day) % 7;
}

int DateTimeSecondsIsDST(unsigned long seconds) {
	  DATETIME_STRUCT timestamp;
	  DateTimeFromSeconds(seconds, &timestamp);
	  return DateTimeIsDST(&timestamp);
}

static unsigned char * getDSTData(unsigned char* DSTSpecs,unsigned char* month,
		unsigned char* week, unsigned char* dow, unsigned char *time) {
	if (!DSTSpecs) return 0;
	if (*(DSTSpecs++) != 'M') return 0;
	*month = *week = *dow = *time = 0;
	while((*DSTSpecs >= '0') && (*DSTSpecs <='9')) {
		*month *= 10;
		*month += *(DSTSpecs++) - '0';
	}
	if (*(DSTSpecs++) != '.') return 0;
	while((*DSTSpecs >= '0') && (*DSTSpecs <='9')) {
		*week *= 10;
		*week += *(DSTSpecs++) - '0';
	}
	if (*(DSTSpecs++) != '.') return 0;
	while((*DSTSpecs >= '0') && (*DSTSpecs <='9')) {
		*dow *= 10;
		*dow += *(DSTSpecs++) - '0';
	}
	if (*(DSTSpecs) == '/') {
		DSTSpecs++;
		while((*DSTSpecs >= '0') && (*DSTSpecs <='9')) {
			*time *= 10;
			*time += *(DSTSpecs++) - '0';
		}
	}
	while(*DSTSpecs && (*DSTSpecs != ',')) DSTSpecs++;
	return (*DSTSpecs) ? ++DSTSpecs : DSTSpecs;
}

int DateTimeIsDST(DATETIME_STRUCT *dt) {
	return DateTimeIsDSTExt(dt,_DSTSpecs);
}

int DateTimeSecondsIsDSTExt(const unsigned long seconds, unsigned char* DSTSpecs) {
	  DATETIME_STRUCT timestamp;
	  DateTimeFromSeconds(seconds, &timestamp);
	  return DateTimeIsDSTExt(&timestamp,DSTSpecs);
}

int DateTimeIsDSTExt(DATETIME_STRUCT *dt, unsigned char* DSTSpecs) {
	unsigned char m1, w1, d1, t1;
	unsigned char m2, w2, d2, t2;
	if (!DSTSpecs) return 0;
	unsigned char* p = DSTSpecs;
	// Skip zone name and standard names
	p = (unsigned char*)strchr((char*)p,',');
	if (*p) p++;
	p = getDSTData(p,&m1,&w1,&d1,&t1);
	if (p) p = getDSTData(p,&m2,&w2,&d2,&t2);
	if (!p) return 0;
	if ((dt->month < m1) || (dt->month > (m2+1))) return 0;
	if ((dt->month > m1) && (dt->month < m2)) return 1;
	// So now, we are in either m1 or m2 month
	// Get first day of week of the current month (either m1 or m2)
	unsigned char day = 1;
	unsigned char dow = DateTimeDayOfWeek(dt->year,dt->month,day);
	// Seach for first d1 or d2
	while (dow != ((dt->month == m1) ? d1 : d2)) { day++; dow = (dow + 1) % 7; }
	// search for w1 or w2 date in month
	day += (7 * (((dt->month == m1) ? w1 : w2)-1));
	while (day > DateTimeDaysInMonth(dt->month,dt->year)) day -= 7;	// in case of last day of month, check for overflow
	if (dt->day < day) return (dt->month == m1) ? 0 : 1;
	if (dt->day > day) return  (dt->month == m1) ? 1 : 0;
	// Ouch ! We are exactly the day of time change
	if (dt->hour < ((dt->month == m1) ? t1 : t2)) return  (dt->month == m1) ? 0 : 1;
	if (dt->hour >= ((dt->month == m1) ? t1 : t2)) return  (dt->month == m1) ? 1 : 0;
	return 0;
}

void DateTimeSetLocale(unsigned char *DSTSpecs) {
	_DSTSpecs = DSTSpecs;
	_timeOffset = 0;
	if (DSTSpecs) {
		// DSTSpecs = "std +/- offset dst_std,rules
		// If std is not defined, <> can be used. ex. <+05>-05
		// Note that offset is relative to UTC. ex. +05 means that UTC is 5 hours ahead
		// Skip zone name is any
		while(*DSTSpecs && (*DSTSpecs != ';')) DSTSpecs++;
		if (*DSTSpecs == ';') DSTSpecs++;	// Skip name
		else DSTSpecs = _DSTSpecs;			// Reset pointer if no timezone name found
		// Skip unquoted zone names in the form <+05>-05 for instance
		if (*DSTSpecs == '<') while(*DSTSpecs && (*DSTSpecs != '>')) DSTSpecs++;
		// Lookup for +/- sign or digit. If we reached , => malformed
		while(*DSTSpecs && (*DSTSpecs != ',')
			&& ((*DSTSpecs != '-') && (*DSTSpecs != '+')
			&& ((*DSTSpecs < '0') || (*DSTSpecs > '9'))))
				DSTSpecs++;
		// We found +/- or digit
		if(*DSTSpecs && (*DSTSpecs != ',')) {
			int i = -1;	// If offset > 0, offset is subtracted from UTC time
			if (*DSTSpecs == '-') { i = 1; DSTSpecs++; } // If offset < 0, offset is added to UTC
			if (*DSTSpecs == '+') DSTSpecs++;
			if ((*DSTSpecs >= '0') && (*DSTSpecs <= '9')) { _timeOffset = (*DSTSpecs - '0');  DSTSpecs++; }
			if ((*DSTSpecs >= '0') && (*DSTSpecs <= '9')) { _timeOffset = (_timeOffset * 10) + (*DSTSpecs - '0'); }
			_timeOffset *= i;
		}
	}
}

short DateTimeGetLocaleOffset(void) {
	return _timeOffset;
}

void DateTimeLocale(DATETIME_STRUCT* dt) {
	if (dt) {
		DateTimeLocaleFromSeconds(DateTimeGetSeconds(dt), dt);
	}
}
void DateTimeLocaleFromSeconds(const unsigned long seconds, DATETIME_STRUCT* dt) {
	if (dt) {
		// Compute dt with timeOffset
		DateTimeFromSeconds(seconds + (_timeOffset * 3600L),dt);
		dt->zoneOffset = _timeOffset;
		if (DateTimeIsDSTExt(dt,_DSTSpecs)) {
			// Recompute dt with timeOffset + 1 for DST period
			DateTimeFromSeconds(seconds + ((_timeOffset + 1) * 3600L),dt);
			dt->zoneOffset = _timeOffset + 1;
		}
	}
}

unsigned char DateTimeLocaleToISO8601Format(DATETIME_STRUCT *timestamp,unsigned char* Buffer)
{
  if (Buffer == (unsigned char*)0) return 0; // Avoid null pointer assignments
  // This strange code is necessary if the provided buffer is not aligned
  return DateTimeLocaleSecondsToISO8601Format(DateTimeGetSeconds(timestamp), Buffer);
}

unsigned char DateTimeLocaleSecondsToISO8601Format(const unsigned long time, unsigned char* buffer) {
  if (buffer == (unsigned char*)0) return 0; // Avoid null pointer assignments
  DATETIME_STRUCT timestamp;
  DateTimeLocaleFromSeconds(time, &timestamp);
  return DateTimeToISO8601Format(&timestamp,buffer);
}
