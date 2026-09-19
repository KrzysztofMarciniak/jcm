#ifndef JCM_INPUT_H
#define JCM_INPUT_H

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* Read exactly one signed decimal integer, rejecting trailing non-whitespace. */
static int
jcm_scanf_number(const char *format, long *value)
{
    char buffer[128];
    char *end;
    long parsed;

    (void)format;
    if (value == NULL || fgets(buffer, sizeof(buffer), stdin) == NULL)
        return 0;

    errno = 0;
    parsed = strtol(buffer, &end, 10);
    if (end == buffer || errno == ERANGE)
        return 0;
    while (*end != '\0') {
        if (!isspace((unsigned char)*end))
            return 0;
        end++;
    }

    *value = parsed;
    return 1;
}

#define scanf(format, value) jcm_scanf_number((format), (value))

#endif
