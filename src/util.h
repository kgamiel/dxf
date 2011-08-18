/** @file util.h 
 *  @brief Utility functions.
 *
 * Various utility functions.
 */
#ifndef _UTIL_H_
#define _UTIL_H_

/**
 * Boolean values.
 * True or false.
 */
typedef short util_bool_t;

/**
Trims all leading and trailing whitespace from a string.
Trims in-place by sliding (leading) non-whitespace characters to the left.
Trailing whitepsace characters are removed by placing a NULL.  Note, any 
whitespace characters within the string are ignored, only leading and trailing
are removed.
@param  s   NULL-terminated ASCII string.  On return, value of s is changed to
a NULL-terminated string with no leading or trailing whitespace.
*/
void util_trim(char *s);

#endif

