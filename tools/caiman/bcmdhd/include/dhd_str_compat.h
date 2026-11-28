/*
 * Compatibility shim for strncpy(), removed from mainline Linux in favour
 * of strscpy(). The vendor DHD driver still calls strncpy() and, unlike
 * strlcpy()/strlcat() (which it provides itself in bcmstdlib_s.c), does not
 * supply its own. Force-included into every translation unit via the Kbuild.
 */

#ifndef _DHD_STR_COMPAT_H_
#define _DHD_STR_COMPAT_H_

#ifdef __KERNEL__
#include <linux/version.h>
#include <linux/types.h>
#include <linux/sched/clock.h>

static inline char *dhd_compat_strncpy(char *dest, const char *src, size_t count)
{
	size_t i;

	for (i = 0; i < count && src[i] != '\0'; i++)
		dest[i] = src[i];
	for (; i < count; i++)
		dest[i] = '\0';
	return dest;
}

#define strncpy(d, s, n) dhd_compat_strncpy((d), (s), (n))

/* strlcpy() is defined by the driver (bcmstdlib_s.c) but the kernel no longer
 * declares it; make its prototype visible to every TU that calls it. */
extern size_t strlcpy(char *dest, const char *src, size_t size);

/* in_irq() was removed; in_hardirq() is the current spelling. */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(6, 15, 0))
#include <linux/preempt.h>
#ifndef in_irq
#define in_irq() in_hardirq()
#endif
#endif
#endif /* __KERNEL__ */

#endif /* _DHD_STR_COMPAT_H_ */
