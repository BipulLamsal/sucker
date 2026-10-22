#include <stdint.h>
#include <stdio.h>
#include <math.h>

uint32_t htonf(float f)  {
    uint32_t p;
    uint32_t sign = 0;

    float number = 1834.18;
    if (number < 0) {
        number = number * -1;
        sign = 1;
    } 

    // 0x72A                     0111 0010 1010
    // ---------------------0000 1111 1111 1111
    p = ((uint32_t)number & 0x7FFF)<<16 | (sign << 31); 
    uint32_t float_part = (number - (int)number) * pow(2, 16);
    p |= float_part;

    return p
}

float ntohf(uint32_t p)
{
    float f = ((p>>16)&0x7fff); // whole part
    f += (p&0xffff) / 65536.0f; // fraction

    if (((p>>31)&0x1) == 0x1) { f = -f; } // sign bit set

    return f;
}



int main(){

    
    printf("check this number: %d\n", p);
    return 0;
}
