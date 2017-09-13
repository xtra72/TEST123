/*******************************************************************
**                                                                **
** Date/Time management functions                                 **
**                                                                **
*******************************************************************/

#ifndef __DATETIME_H__
#define __DATETIME_H__
/** \addtogroup MMI MyMeterInfo add-on functions
 *  @{
 */

#define DateTimeSunday		0		//!< Weekday value for Sunday
#define DateTimeMonday		1		//!< Weekday value for Monday
#define DateTimeTuesday		2		//!< Weekday value for Tuesday
#define DateTimeWednesday	3		//!< Weekday value for Wednesday
#define DateTimeThursday	4		//!< Weekday value for Thursday
#define DateTimeFriday		5		//!< Weekday value for Friday
#define DateTimeSaturday	6		//!< Weekday value for Saturday

/*!
 * @brief Structure that contains date/time information.
 */
typedef struct __packed__
{
unsigned short year;	//!< Year with century (ex.: 2017)
unsigned char month;	//!< Month from 1 to 12
unsigned char day;		//!< Day from 1 to 31
unsigned char hour;		//!< Hour from 0 to 23
unsigned char min;		//!< Minute from 0 to 59
unsigned char sec;		//!< Second from 0 to 59
unsigned char dayOfWeek;//!< Day to the week (0: Sunday, 1: Monday...6:Saturday
short zoneOffset;		//!< Time Zone offset in hours
} DATETIME_STRUCT;

/*!
 * Converts a number of seconds from 01/01/2000 into a DATETIME_STRUCT in GMT
 * @param[in] seconds	Number of seconds since 01/01/2000 to convert
 * @param[out] dt		DATETIME_STRUCT to populate
 */
void DateTimeFromSeconds(const unsigned long seconds, DATETIME_STRUCT* dt);

/*!
 * Returns the number of elapsed days until a month for a specific year.
 * The function will take into account leap years.
 * @param[in] month		Month (from 1 to 12)
 * @param[in] year		Year (>= 2000)
 * @return 				Number of days elapsed. 0 if error.
 */
unsigned short DateTimeDaysInYear(unsigned char month, unsigned short year);

/*!
 * Returns the number of days in a month for a specific year.
 * The function will take into account leap years.
 * @param[in] month		Month (from 1 to 12)
 * @param[in] year		Year (>= 2000)
 * @return 				Number of days in the month. 0 if error.
 */
unsigned char DateTimeDaysInMonth(unsigned char month, unsigned short year);

/*!
 * Returns the GMT number of seconds elapsed since 01/01/2000 from a DATETIME_STRUCT
 * @param[in] dt	DATETIME_STRUCT to convert
 * @return			Number of elapsed seconds
 */
unsigned long DateTimeGetSeconds(const DATETIME_STRUCT *dt);

/*!
 * Converts a GMT number of seconds into a fixed formatted string using / and : delimiters
 * (ex. 2000/12/31 00:00:00)
 * @param[in] time		Number of seconds to convert
 * @param[out] Buffer	Character buffer to receive the conversion (minimum size of 20 chars)
 * @return				Number of characters stored in the buffer
 */
unsigned char DateTimeStringFormat(const unsigned long time,unsigned char *Buffer);

/*!
 * Converts a GMT number of seconds into a fixed formatted string using user defined delimiters
 * @param[in] time			Number of seconds to convert
 * @param[out] Buffer		Character buffer to receive the conversion (minimum size of 20 chars)
 * @param[in] DateDelimiter User defined delimiter for date elements
 * @param[in] HourDelimiter User defined delimiter for hour elements
 * @return					Number of characters stored in the buffer
 */
unsigned char DateTimeStringFormatExt(const unsigned long time,unsigned char *Buffer, const unsigned char DateDelimiter, const unsigned char HourDelimiter);

/*!
 * Converts a GMT number of seconds into a fixed formatted string using ISO8601 definition
 * @note The functions assume ISO8601 format and current time in GMT
 * therefore, the 26 character string (with null terminator) as output/input string is:
 * "YYYY-mm-ddTHH:MM:ss+00:00"
 * @param[in] time			Number of seconds to convert
 * @param[out] buffer		Character buffer to receive the conversion (minimum size of 26 chars)
 * @return					Number of characters stored in the buffer
 */
unsigned char DateTimeSecondsToISO8601Format(const unsigned long time, unsigned char* buffer);

/*!
 * Converts a DATETIME_STRUCT into a fixed formatted string using ISO8601 definition
 * @note The functions assume ISO8601 format. DATETIME_STRUCT time zone offset is included
 * therefore, the 26 character string (with null terminator) as output/input string is:
 * "YYYY-mm-ddTHH:MM:ss+zz:00"
 * @param[in] time			DATATIME_STRUCT to convert
 * @param[out] buffer		Character buffer to receive the conversion (minimum size of 26 chars)
 * @return					Number of characters stored in the buffer
 */
unsigned char DateTimeToISO8601Format(const DATETIME_STRUCT *time, unsigned char* buffer);

/*!
 * Converts an ISO8601 string into a GMT elapsed number of seconds since 01/01/2000
 * @param[in] string	String to convert
 * @return 				Number of GMT elapsed seconds since 01/01/2000
 */
unsigned long DateTimeSecondsFromISO8601String(const unsigned char* string);
/*!
 * Converts an ISO8601 string into a DATETIME_STRUCT including time zone offset
 * @param[out] dt		DATETIME_STRUCT that receives the conversion
 * @param[in] string	ISO8601 string to be converted
 * @return				1 if successful, 0 if string could not be converted
 */
unsigned char DateTimeFromISO8601String(DATETIME_STRUCT *dt, const unsigned char* string);
/*!
 * Convert GMT seconds elapsed since 01/01/2000 into unsigned long Date/Time in DOS double word format
 * @param[in] seconds	GMT number of seconds to be converted
 * @return 				DOS Date/Time double word conversion
 */
unsigned long DateTimeSecondsToFatTime(unsigned long seconds);

/*!
 * Convert unsigned long Date/Time in DOS double word format into GMT seconds elapsed since 01/01/2000
 * @param[in] seconds	DOS Date/Time double word to be converted
 * @return 				GMT number of seconds conversion
 */
unsigned long DateTimeFatTimeToSeconds(unsigned long fatTime);

/*!
 * Gets Day of the week for a specific date
 * @param[in] year		Year including century (ex. 2015)
 * @param[in] month		Month from 1 to 12
 * @param[in] day		Day from 1 to 31
 * @return				Day of the week: 0:Sunday, 1:Monday...6:Saturday
 */
unsigned char DateTimeDayOfWeek(int year, unsigned char month, unsigned char day);

/// \def West European time zone and Daylight Saving Time for France/Paris
// Check for Daylight Saving Time for Europe
// Europe/Paris = CET-1CEST,M3.5.0/2,M10.5.0/3
// DST Specs for Europe = between Last Sunday of March @ 02:00a
//                            and last Sunday of October @ 03:00a
#define EUROPEAN_DST  ((unsigned char*)"Europe/Paris;CET-1CEST,M3.5.0/2,M10.5.0/3")

/*!
 * Sets the current local time specifications using Linux tz database type of definition
 * @param[in] DSTSpecs	Specification using tz database format
 */
void DateTimeSetLocale(unsigned char *DSTSpecs);

/*!
 * Returns the offset +/- hours between GMT and local time
 * @return		hours offset (+/-)
 */
short DateTimeGetLocaleOffset(void);
/*!
 * Checks whether the provided GMT elapsed number of seconds since 01/01/2000 is in DST period of the
 * local time
 * @param[in] seconds	GMT elapsed number of seconds to test
 * @return				1 if within DST period, 0 otherwise
 */
int DateTimeSecondsIsDST(unsigned long seconds);

/*!
 * Checks whether the DATETIME_STRUCT is within DST period of the local time.
 * @param[in] dt		DATETIME_STRUCT to be tested. Time zone value is used.
 * @return				1 if within DST period, 0 otherwise
 */
int DateTimeIsDST(DATETIME_STRUCT *dt);

/*!
 * Checks whether the provided GMT elapsed number of seconds since 01/01/2000 is in DST period of the
 * provided DST specifications
 * @param[in] seconds	GMT elaspsed seconds since 01/01/2000
 * @param[in] DSTSpecs	DST specification to be used for verification
 * @return				1 if within DST period, 0 otherwise
 */
int DateTimeSecondsIsDSTExt(const unsigned long seconds, unsigned char* DSTSpecs);

/*!
 * Checks whether the DATETIME_STRUCT is within DST period of the provided DST specifications
 * @param[in] dt		DATETIME_STRUCT to be tested. Time zone value is used.
 * @param[in] DSTSpecs	DST specification to be used for verification
 * @return				1 if within DST period, 0 otherwise
 */
int DateTimeIsDSTExt(DATETIME_STRUCT *dt, unsigned char* DSTSpecs);

/*!
 *
 * @param[in][out] dt	Converts a DATETIME_STRUCT to local time
 */
void DateTimeLocale(DATETIME_STRUCT* dt);

/*!
 * Converts a GMT number of elapsed seconds since 01/01/2000 to a DATETIME_STRUCT using local time
 * @param[in] seconds	Elapsed number of seconds to convert
 * @param[out] dt		DATETIME_STRUCT that receives the converted value
 */
void DateTimeLocaleFromSeconds(const unsigned long seconds, DATETIME_STRUCT* dt);

/*!
 * Converts a DATETIME_STRUCT to an ISO8601 string using local time specifications.
 * @param[in] timestamp	DATETIME_STRUCT to be converted. Time zone if used.
 * @param[out] buffer	Buffer receiving the converted string (minimum 26 characters)
 * @return				Number of characters written to the buffer
 */
unsigned char DateTimeLocaleToISO8601Format(DATETIME_STRUCT *timestamp,unsigned char* buffer);

/*!
 * Converts a GMT Elapsed number of seconds since 01/01/2000 into an ISO8601 string using local time
 * specifications
 * @param[in] time		GMT Elapsed number of seconds to convert
 * @param[out] buffer	Buffer receiving the converted string (minimum 26 characters)
 * @return				Number of characters written to the buffer
 */
unsigned char DateTimeLocaleSecondsToISO8601Format(const unsigned long time, unsigned char* buffer);

/** }@ */

#endif
