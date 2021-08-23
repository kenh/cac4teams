/*
 * cac4teams - A shared library shim to make CACs functional with
 *	       Microsoft Teams for Mac
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdarg.h>

#include <dispatch/dispatch.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

static void logdict(FILE *, const char *, CFTypeRef);
static void logmessage(FILE *, const char *, ...);
static void logflush(FILE *);

static const CFStringRef *retkeylist[] = {
	&kSecReturnData,
	&kSecReturnAttributes,
	&kSecReturnRef,
	&kSecReturnPersistentRef,
	NULL,
};

static OSStatus
cac4teams_SecItemCopyMatching(CFDictionaryRef query, CFTypeRef *result)
{
	static dispatch_once_t init;
	static FILE *debug = NULL;
	CFMutableDictionaryRef modquery = NULL;
	const void *value;
	Boolean exist;
	OSStatus ret;
	int i;

	dispatch_once(&init, ^{
		char *name = getenv("CAC4TEAMS_DEBUG");
		if (name)
			debug = fopen(name, "a");
	});

	logdict(debug, "Query dictionary", query);

	/*
	 * So what's happening here?
	 *
	 * It seems that when Teams is calling SecItemCopyMatching, the
	 * query dictionary doesn't have any return flags in it to
	 * tell SecItemCopyMatching() exactly what it is supposed to
	 * This would be things like kSecReturnRef, kSecReturnAtributes,
	 * et cetera.  As a result, SecItemCopyMatching() returns nothing.
	 *
	 * Our minimal fix is to modify the query dictionary when the
	 * following conditions are true:
	 *
	 * - The query is for identities (kSecClassIdentity)
	 * - There are no return flags set (see the list before this
	 *   function)
	 *
	 * If both of those things are true, add kSecReturnRef to the
	 * query dictionary and return the results.
	 */

	exist = CFDictionaryGetValueIfPresent(query, kSecClass, &value);

	if (! exist || ! CFEqual(value, kSecClassIdentity))
		goto skipmodify;

	for (i = 0; retkeylist[i] != NULL; i++) {
		exist = CFDictionaryGetValueIfPresent(query,
						      *retkeylist[i], NULL);
		if (exist)
			goto skipmodify;
	}

	logmessage(debug, "Adding kSecReturnRef to query dictionary\n");

	modquery = CFDictionaryCreateMutableCopy(kCFAllocatorDefault, 0, query);

	CFDictionaryAddValue(modquery, kSecReturnRef, kCFBooleanTrue);

	/*
	 * If I don't restrict it to "signing" identities, email encryption
	 * identities can show up in the certificate selection dialog,
	 * which would never work.
	 */

	CFDictionaryAddValue(modquery, kSecAttrCanSign, kCFBooleanTrue);

skipmodify:

	ret = SecItemCopyMatching(modquery ? modquery : query, result);

	if (modquery)
		CFRelease(modquery);

	logdict(debug, "Result dictionary", *result);

	logflush(debug);

	return ret;
}

/*
 * If configured, log a message to a file
 */

static void
logmessage(FILE *f, const char *fmt, ...)
{
	va_list ap;

	if (! f)
		return;

	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
}

/*
 * Log a CFType to a file using CFCopyDescription
 */

static void
logdict(FILE *f, const char *tag, CFTypeRef data)
{
	struct timeval tv;
	struct tm tm;
	CFStringRef desc = NULL;
	const char *str = NULL;
	char *strbuf = NULL;

	if (! f)
		return;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);

	logmessage(f, "[%d/%02d/%02d %02d:%02d:%02d.%d] %s\n",
		   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour,
		   tm.tm_min, tm.tm_sec, tv.tv_usec, tag);

	if (! data) {
		logmessage(f, "<nil>\n");
		return;
	}

	desc = CFCopyDescription(data);

	str = CFStringGetCStringPtr(desc, kCFStringEncodingUTF8);

	if (! str) {
		CFIndex len = CFStringGetLength(desc);
		CFIndex size = CFStringGetMaximumSizeForEncoding(len,
						kCFStringEncodingUTF8) + 1;

		strbuf = malloc(size);

		if (! CFStringGetCString(desc, strbuf, size,
					 kCFStringEncodingUTF8)) {
			free(strbuf);
			strbuf = NULL;
			str = "Unknown description";
		}
	}

	logmessage(f, "%s\n", str ? str : strbuf);

	if (desc)
		CFRelease(desc);

	if (strbuf)
		free(strbuf);
}

static void
logflush(FILE *f)
{
	if (f)
		fflush(f);
}

__attribute__ ((used)) static struct {
	const void *replacement;
	const void *replaceee;
} _interpose_cac4teams __attribute__ ((section ("__DATA,__interpose"))) = {
	(const void *) &cac4teams_SecItemCopyMatching,
	(const void *) &SecItemCopyMatching
};
