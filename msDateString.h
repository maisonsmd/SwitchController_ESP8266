// msDateString.h

//a clone of Arduino DateString, no copyright from me
#ifndef _MSDATESTRING_h
#define _MSDATESTRING_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#else
#include "WProgram.h"
#endif

/* 
 * Definitions for date strings for use with the Time library
 *
 * Updated for Arduino 1.5.7 18 July 2014
 *
 * No memory is consumed in the sketch if your code does not call any of the string methods
 * You can change the text of the strings, make sure the short strings are each exactly 3 characters
 * the long strings can be any length up to the constant dt_MAX_STRING_LEN defined in TimeLib.h
 *
 */

#if defined(__AVR__)
#include <avr/pgmspace.h>
#else
 // for compatiblity with Arduino Due and Teensy 3.0 and maybe others?
#define PROGMEM
#define PGM_P  const char *
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#define pgm_read_word(addr) (*(const unsigned char **)(addr))
#define strcpy_P(dest, src) strcpy((dest), (src))
#endif
#include <string.h> // for strcpy_P or strcpy

// the short strings for each day or month must be exactly dt_SHORT_STR_LEN
#define dt_SHORT_STR_LEN  3 // the length of short strings
#define dt_MAX_STRING_LEN 9

static char dtBuffer[dt_MAX_STRING_LEN + 1];  // must be big enough for longest string and the terminating null

const char monthStr0[] PROGMEM = "";
const char monthStr1[] PROGMEM = "January";
const char monthStr2[] PROGMEM = "February";
const char monthStr3[] PROGMEM = "March";
const char monthStr4[] PROGMEM = "April";
const char monthStr5[] PROGMEM = "May";
const char monthStr6[] PROGMEM = "June";
const char monthStr7[] PROGMEM = "July";
const char monthStr8[] PROGMEM = "August";
const char monthStr9[] PROGMEM = "September";
const char monthStr10[] PROGMEM = "October";
const char monthStr11[] PROGMEM = "November";
const char monthStr12[] PROGMEM = "December";

const PROGMEM char * const PROGMEM monthNames_P[] =
{
	monthStr0,monthStr1,monthStr2,monthStr3,monthStr4,monthStr5,monthStr6,
	monthStr7,monthStr8,monthStr9,monthStr10,monthStr11,monthStr12
};

const char monthShortNames_P[] PROGMEM = "ErrJanFebMarAprMayJunJulAugSepOctNovDec";

const char dayStr0[] PROGMEM = "Err";
const char dayStr1[] PROGMEM = "Sunday";
const char dayStr2[] PROGMEM = "Monday";
const char dayStr3[] PROGMEM = "Tuesday";
const char dayStr4[] PROGMEM = "Wednesday";
const char dayStr5[] PROGMEM = "Thursday";
const char dayStr6[] PROGMEM = "Friday";
const char dayStr7[] PROGMEM = "Saturday";

const PROGMEM char * const PROGMEM dayNames_P[] =
{
   dayStr0,dayStr1,dayStr2,dayStr3,dayStr4,dayStr5,dayStr6,dayStr7
};

const char dayShortNames_P[] PROGMEM = "ErrSunMonTueWedThuFriSat";

/* functions to return date strings */

static char* monthStr(uint8_t month) {
	memset(dtBuffer, 0, sizeof(dtBuffer));
	strcpy_P(dtBuffer, (PGM_P)pgm_read_word(&(monthNames_P[month])));
	return dtBuffer;
}

static char* monthShortStr(uint8_t month) {
	memset(dtBuffer, 0, sizeof(dtBuffer));
	for (int i = 0; i < dt_SHORT_STR_LEN; i++)
		dtBuffer[i] = pgm_read_byte(&(monthShortNames_P[i + (month*dt_SHORT_STR_LEN)]));
	return dtBuffer;
}

static char* dayStr(uint8_t day) {
	memset(dtBuffer, 0, sizeof(dtBuffer));
	strcpy_P(dtBuffer, (PGM_P)pgm_read_word(&(dayNames_P[day])));
	return dtBuffer;
}

static char* dayShortStr(uint8_t day) {
	memset(dtBuffer, 0, sizeof(dtBuffer));
	uint8_t index = day * dt_SHORT_STR_LEN;
	for (int i = 0; i < dt_SHORT_STR_LEN; i++) {
		dtBuffer[i] =  pgm_read_byte(&(dayShortNames_P[index + i]));
	}
	return dtBuffer;
}

#endif
