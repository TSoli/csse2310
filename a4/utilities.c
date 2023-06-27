/* FILE: utilities.c
 *
 * AUTHOR: Tariq Soliman
 * STUDENT NO.: 45287316
 *
 * DESCRIPTION:
 * Provides some generally useful functions.
 */

#include "utilities.h"

bool is_int(char* str) {
    int i = 0;

    if (str[i] == '-') { // It is a negative number so start at the next digit
        i++;
    }

    if (str[i] == '\0') { // String is empty
        return false;
    }

    while (str[i] != '\0') {
        if (!isdigit(str[i])) {
            return false;
        }
        i++;
    }

    return true;
}

int check_num_in_range(int num, int min, int max) {
    if (num < min) {
        return -1;
    }
    if (num > max) {
        return 1;
    }
    return 0;
}
