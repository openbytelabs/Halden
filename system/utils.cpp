#include <stdint.h>
#include <stddef.h>

void* memcpy(void *dest, const void *src, size_t n) {
    uint8_t *p = (uint8_t *)dest;
    const uint8_t *q = (const uint8_t *)src;
    for (size_t i = 0; i < n; i++) p[i] = q[i];
    return dest;
}

void* memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    for (size_t i = 0; i < n; i++) p[i] = (uint8_t)c;
    return s;
}

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++; s2++;
    }
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++; s2++; n--;
    }
    if (n == 0) return 0;
    return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char* strcpy(char *dest, const char *src) {
    char *tmp = dest;
    while ((*dest++ = *src++));
    return tmp;
}

char* strcat(char *dest, const char *src) {
    char *tmp = dest;
    while (*dest) dest++;
    while ((*dest++ = *src++));
    return tmp;
}

void uint_to_str(uint64_t n, char* buffer) {
    char temp[20];
    int i = 0;
    if (n == 0) { buffer[0] = '0'; buffer[1] = 0; return; }
    while (n > 0) {
        temp[i++] = (n % 10) + '0';
        n /= 10;
    }
    int j = 0;
    while (i > 0) buffer[j++] = temp[--i];
    buffer[j] = 0;
}