/*
 * Define this if you have a /dev/urandom which can supply good random numbers.
 */
#undef HAVE_DEV_URANDOM

/*
 * Define this if you want IPv6 support.
 */
#undef HAVE_IPv6
#undef HAVE_IN6_H

/*
 * Define this if your C library doesn't have vsnprintf.
 */
#undef NEED_VSNPRINTF

/*
 * Defines that were Solaris specific...
 */

#undef HAVE_STROPTS_H
#undef HAVE_FILIO_H

#undef HAVE_STDINT_H
#undef HAVE_INTTYPES_H

/*
 * If you don't have these types in <inttypes.h>, #define these to be
 * the types you do have.
 */
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t
#undef uint8_t
#undef uint16_t
#undef uint32_t

/*
 * Debugging:
 * DEBUG: general debugging
 * DEBUG_MEM: debug memory allocation
 */
#undef DEBUG
#undef DEBUG_MEM

/*
 * #defines for operating system.
 * THESE WANT TO GO AWAY!
 * Any checks for a specific OS should be replaced by a check for
 * a feature that OS supports or doesn't support.
 */
#undef SunOS
#undef Solaris
#undef Linux
#undef HPUX

@BOTTOM@

#ifndef WORDS_BIGENDIAN
#define WORDS_SMALLENDIAN
#endif

#include <sys/types.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

#ifdef HAVE_IPv6

#ifdef HAVE_IN6_H
/* Expect IPV6_{JOIN,LEAVE}_GROUP in in6.h, otherwise expect                 */
/* IPV_{ADD,DROP}_MEMBERSHIP in in.h                                         */
#include <netinet6/in6.h>
#else
#include <netinet/in.h>
#endif /* HAVE_IN6_H */

#ifndef IPV6_ADD_MEMBERSHIP
#define IPV6_ADD_MEMBERSHIP IPV6_JOIN_GROUP
#endif  /* IPV6_ADD_MEMBERSHIP */

#ifndef IPV6_DROP_MEMBERSHIP
#define IPV6_DROP_MEMBERSHIP IPV6_LEAVE_GROUP
#endif  /* IPV6_DROP_MEMBERSHIP */

#endif /* HAVE_IPv6 */
