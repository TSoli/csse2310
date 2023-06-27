#include <stdio.h>

void main() {
    for (int i = 0; i < 3; i++) {
        printf("i++ = %d\n", i);
    }

    for (int i = 0; i < 3; ++i) {
        printf("++i = %d\n", i);
    }
}
