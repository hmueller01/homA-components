/**
 * @file
 * @brief Common ESP8266 user functions
 * @author Holger Mueller
 */

#include <c_types.h>
#include <ctype.h> // needed for isspace()
#include <stdlib.h> // needed for abs()

#include "common.h"


/**
 ******************************************************************
 * @brief  Float to ASCII with precision 1
 * @author Holger Mueller
 * @date   2015-10-08, 2018-04-01
 *
 * @param  *s - Buffer to convert to.
 * @param  f - Float to convert.
 * @return Returns pointer to s string.
 ******************************************************************
 */
char * ICACHE_FLASH_ATTR
ftoa(char *s, float f) {
	char sign[2];
	sign[0] = (f < 0) ? '-' : '\0';
	sign[1] = '\0';
	os_sprintf(s, "%s%d.%d",
			sign, abs((int) f), abs((int) ((f - (int) f) * 10)));
	return s;
}


/**
 ******************************************************************
 * @brief  Integer to ASCII
 * @author Holger Mueller
 * @date   2017-06-07
 *
 * @param  *s - Buffer to convert to.
 * @param  i - Integer to convert.
 * @return Returns pointer to s string.
 ******************************************************************
 */
char * ICACHE_FLASH_ATTR
itoa(char *s, uint16_t i) {
	os_sprintf(s, "%d", i);
	return s;
}


/**
 ******************************************************************
 * @brief  Simple ASCII to double implementation
 * @author Matthew Hoggan, Holger Mueller
 * @date   2011-11-28, 2018-03-28
 * based on
 * https://codereview.stackexchange.com/questions/6370/implementation-of-the-atof-function
 *
 * @param  *s - Buffer to convert.
 * @return Returns double value.
 ******************************************************************
 */
double ICACHE_FLASH_ATTR
atof(const char *s) {
	double val, power;
	int sign;
	bool dot;
	int eval, esign;
	int i;
	double result;

	// remove leading spaces
	while (*s && isspace(*s)) ++s;

	// handle sign
	sign = (*s == '-') ? -1 : 1;
	if (*s == '+' || *s == '-') ++s;

	// handle NAN and INF
	if ((*s == 'n' || *s == 'N') &&
			(s[1] == 'a' || s[1] == 'A') &&
			(s[2] == 'n' || s[2] == 'N'))
		return NAN * sign;
	if ((*s == 'i' || *s == 'I') &&
			(s[1] == 'n' || s[1] == 'N') &&
			(s[2] == 'f' || s[2] == 'F'))
		return INFINITY * sign;

	// cycle through mantissa
	for (val = 0.0, power = 1.0, dot = FALSE; isdigit(*s) || *s == '.'; ++s) {
		if (*s == '.') {
			dot = TRUE; // there is a dot
			continue;
		}
		val = 10.0 * val + (*s - '0');
		if (dot) power *= 10.0; // if dot, add decimal place
	}
	result = sign * val / power;

	// next work on exponent (only integer exponent is supported)
	if (*s == 'e' || *s == 'E') {
		++s;
		esign = (*s == '-') ? -1 : 1;
		if (*s == '+' || *s == '-') ++s;
		for (eval = 0; isdigit(*s); ++s) {
			eval = 10 * eval + (*s - '0');
		}
		for (i = 0; i < eval; i++) {
			(esign >= 0) ? (result *= 10) : (result /= 10);
		}
	}

	return result;
}


// define constants for expf() and logf()
#define EXP_A 184
#define EXP_C 16249

/**
 ******************************************************************
 * @brief  Simple exp(x) (e^x) implementation.
 * @author SirGuy, Holger Mueller
 * @date   2012-03-22, 2018-04-01
 * based on
 * https://stackoverflow.com/questions/9652549/self-made-pow-c
 *
 * @param  x - exponent.
 * @return Returns e^x.
 ******************************************************************
 */
float ICACHE_FLASH_ATTR
expf(float x) {
	union {
		float d;
		struct {
#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) || defined(__LITTLE_ENDIAN__)
			short j, i;
#else
			short i, j;
#endif
		} n;
	} eco;
	eco.n.i = EXP_A * x + EXP_C;
	eco.n.j = 0;
	return eco.d;
}


/**
 ******************************************************************
 * @brief  Simple log(x) implementation.
 * @author SirGuy, Holger Mueller
 * @date   2012-03-22, 2018-04-01
 * based on
 * https://stackoverflow.com/questions/9652549/self-made-pow-c
 *
 * @param  x - ln parameter.
 * @return Returns ln(x).
 ******************************************************************
 */
float ICACHE_FLASH_ATTR
logf(float x) {
	int *nTemp = (int *) &x;
	x = (*nTemp) >> 16;
	return (x - EXP_C) / EXP_A;
}


/**
 ******************************************************************
 * @brief  Simple pow(x,y) (x^y) implementation.
 * @author SirGuy, Holger Mueller
 * @date   2012-03-22, 2018-04-01
 * based on
 * https://stackoverflow.com/questions/9652549/self-made-pow-c
 *
 * @param  x - base.
 * @param  y - exponent.
 * @return Returns x^y.
 ******************************************************************
 */
float ICACHE_FLASH_ATTR
powf(float x, float y) {
	return expf(logf(x) * y);
}
