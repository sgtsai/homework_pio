/* 基本 bitwise 操作 */
#include <stdio.h>
#include <stdint.h>

int main(void) {
    uint32_t x = 0xA5;                              /* 1010 0101 */

    /* 六個運算子 */
    printf("x & 0x0F = 0x%02X\n", x & 0x0F);        /* 0x05  AND：只留低 4 位 */
    printf("x | 0x0F = 0x%02X\n", x | 0x0F);        /* 0xAF  OR ：低 4 位全設 1 */
    printf("x ^ 0xFF = 0x%02X\n", x ^ 0xFF);        /* 0x5A  XOR：每一位都反過來 */
    printf("~x       = 0x%02X\n", ~x & 0xFF);       /* 0x5A  NOT */
    printf("x << 4   = 0x%X\n",   x << 4);          /* 0xA50 左移 4 位 */
    printf("x >> 4   = 0x%X\n",   x >> 4);          /* 0xA   右移 4 位 */

    /* 常見用法 */
    printf("set bit 1    = 0x%02X\n", x | (1u << 1));    /* 0xA7 */
    printf("clear bit 0  = 0x%02X\n", x & ~(1u << 0));   /* 0xA4 */
    printf("test bit 7   = %u\n",     (x >> 7) & 1);     /* 1 */
    printf("bits 7..4    = 0x%X\n",   (x >> 4) & 0xF);   /* 0xA  先右移再 mask */
    printf("combine      = 0x%X\n",   (0x12u << 8) | 0x34); /* 0x1234 */

    return 0;
}
