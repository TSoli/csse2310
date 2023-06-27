/* FILE: readCommline.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * Provides useful functions for processing and validating the command line
 * arguments passed to a program.
 */

#include "readCommline.h"

int check_num_args(int argc, int min, int max) {
    int valid = check_num_in_range(argc, min, max);

    if (valid < 0) {
        return -1;
    }
    if (valid > 0 && max != 0) {
        return 1;
    }

    return 0;
}
