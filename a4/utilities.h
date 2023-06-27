/* FILE: utilities.h
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * Provides some generally useful functions.
 */
#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <ctype.h>

/* Checks if the string passed can be converted to an integer and returns true
 * iff it can be. The string must be well-formed (end in a '\0').
 *
 * Params:
 *      str: The string to check if it is an int
 * Return:
 *      true iff the string represents an integer (only contains digits but 
 *      may be negative). Infinite and NaN are not considered valid integers.
 *
 * Reference:
 *      This is from the worldle program I wrote for a1.
 */
bool is_int(char* str); 

/* Checks if a number is in a given range.
 *
 * Params:
 *      num: The number of interest.
 *      min: The minimum value.
 *      max: The max value.
 * Return:
 *      -1 if the number is less than min.
 *      1 i  the number is greater than min.
 *      0 if the number is in the specified range.
 */
int check_num_in_range(int num, int min, int max);

#endif
