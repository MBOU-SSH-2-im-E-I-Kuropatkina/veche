#include <stdio.h>
#include <stdint.h>

// Runtime функции для скомпилированных программ на Вече

void veche_print_int64(int64_t value) {
    printf("%lld\n", (long long)value);
}

int64_t veche_input_int64(void) {
    int64_t value;
    scanf("%lld", (long long*)&value);
    return value;
}

void veche_print_double(double value) {
    printf("%f\n", value);
}

double veche_input_double(void) {
    double value;
    scanf("%lf", &value);
    return value;
}