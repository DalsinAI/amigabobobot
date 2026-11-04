/* xsdl: BoboBot's frame clock on the Amiga (see xsdl.c).
 * The AmiCradle Team. GPL 2, like BoboBot. */
#ifndef XSDL_TIME_H
#define XSDL_TIME_H

struct timeval;
int xsdl_gettimeofday(struct timeval *tv, void *tz);
int xsdl_usleep(unsigned long us);

#ifndef XSDL_NO_TIME_MACROS
#define gettimeofday(tv, tz) xsdl_gettimeofday((tv), (tz))
#define usleep(us) xsdl_usleep(us)
#endif

#endif
