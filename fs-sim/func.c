#include "func.h"
#include "FileSystem.h"

// reference: https://stackoverflow.com/questions/41384262/convert-string-to-binary-in-c
char *stringToBinary(char *s) {
    char *binary = calloc(128+1, 1);
    for(size_t i = 0; i < 16; ++i) {
        char ch = s[i];
        for(int j = 7; j >= 0; --j){
            if(ch & (1 << j)) {
                strcat(binary,"1");
            } else {
                strcat(binary,"0");
            }
        }
    }
    return binary;
}

bool isHighestBitSet(uint8_t num){
    return 1 == ((num >> 7) & 1U);
}

uint8_t setHighestBit(u_int8_t num) {
    num |= 1UL << 7;
    return num;
}

// reference: https://stackoverflow.com/questions/47981/how-do-you-set-clear-and-toggle-a-single-bit
uint8_t getSizeBit(uint8_t num) {
    num &= ~(1UL << 7);
    return num;
}

void setBitInRange(char *s, int start, int end) {
    // strlen(s) = 16
    // start and end are both inclusive
    int i = start / 8;
    int i_mod = start % 8;
    int j = end / 8;
    int j_mod = end % 8;
    if (i == j) {
        // same char
        char *ch = &s[i];
        for (int k=7-j_mod; k<=7-i_mod; k++) {
            *ch |= (1 << k);
        }
    } else {
        // diff char
        char *ch1 = &s[i];
        char *ch2 = &s[j];
        for (int k=0; k<=7-i_mod; k++) {
            *ch1 |= (1 << k);
        }
        for (int k=7-j_mod; k<=7; k++){
            *ch2 |= (1 << k);
        }
        if (j-i>1) {
            for (int k=i+1; k<j; k++) {
                s[k] = 0xFF;
            }
        }
    }
}





