#include "ft_script.h"

/**
 * Copies src into dst, including the null terminator.
 * Returns dst.
 */
char    *ft_strcpy(char *dst, const char *src)
{
    char    *ret;

    ret = dst;
    while (*src)
        *dst++ = *src++;
    *dst = '\0';
    return (ret);
}

/**
 * Writes the decimal representation of n into buf and null-terminates it.
 * Supports unsigned int only (no negative numbers, no sign character).
 * Returns buf.
 */
char    *ft_uitoa(char *buf, unsigned int n)
{
    char        tmp[10];
    int         i;
    int         len;

    if (n == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return (buf);
    }
    i = 0;
    while (n > 0)
    {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
    }
    len = i;
    i = 0;
    while (i < len)
    {
        buf[i] = tmp[len - 1 - i];
        i++;
    }
    buf[len] = '\0';
    return (buf);
}
