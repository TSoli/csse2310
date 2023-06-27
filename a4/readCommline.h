/* FILE: readCommline.h
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * Provides useful functions for processing and validating the command line
 * arguments passed to a program.
 */

#ifndef READ_COMMLINE_H
#define READ_COMMLINE_H

#include "utilities.h"
#include <stdio.h>
#include <stdbool.h>

/* Check that the number of arguments passed is valid.
 *
 * Params:
 *      argc: The length of the argv array.
 *      min: The minimum number of acceptable args.
 *      max: The maximum number of acceptable args (or 0 if there isn't a max).
 * 
 * Return:
 *      0 if it is a valid number of arguments.
 *      -1 if it is less than the minimum.
 *      1 if it is more than the maximum.
 */
int check_num_args(int argc, int min, int max);

#endif
