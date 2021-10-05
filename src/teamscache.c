#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/errno.h>
#include <stdbool.h>

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

static void usage(const char *);

#define TEAMSCFSTR CFSTR("Microsoft Teams Identities Cache")

int
main(int argc, char *argv[])
{
	CFMutableDictionaryRef query = NULL;
	CFMutableDataRef databuf = NULL;
	CFTypeRef result = NULL;
	SecKeychainRef keychain = NULL;
	OSStatus ret;
	int c;
	FILE *data;
	bool input = false;

	while ((c = getopt(argc, argv, "io")) != EOF) {
		switch (c) {
		case 'i':
			input = true;
			break;
		case 'o':
			input = false;	
			break;
		case '?':
		default:
			usage(argv[0]);
		}
	}

	if (argc > optind + 1)
		usage(argv[0]);

	if (argc == optind) {
		data = input ? stdin : stdout;
	} else {
		data = fopen(argv[optind], input ? "r" : "w");
		if (! data) {
			fprintf(stderr, "Unable to open %s: %s\n",
				argv[optind], strerror(errno));
			exit(1);
		}
	}

	query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
					  &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);
	if (! query) {
		fprintf(stderr, "Unable to create query dictionary\n");
		exit(1);
	}

	CFDictionaryAddValue(query, kSecClass, kSecClassGenericPassword);
	CFDictionaryAddValue(query, kSecAttrAccount, TEAMSCFSTR);
	CFDictionaryAddValue(query, kSecAttrService, TEAMSCFSTR);

	if (input)
		CFDictionaryAddValue(query, kSecReturnRef, kCFBooleanTrue);
	else
		CFDictionaryAddValue(query, kSecReturnData, kCFBooleanTrue);

	ret = SecItemCopyMatching(query, &result);

	CFRelease(query);

	if (! input) {
		if (result == NULL) {
			fprintf(stderr, "No identity cache found, return "
				"code = %d\n", ret);
			exit(1);
		}

		if (CFGetTypeID(result) != CFDataGetTypeID()) {
			fprintf(stderr, "Unexpected type in result!\n");
			exit(1);
		}

		fwrite(CFDataGetBytePtr(result), CFDataGetLength(result), 1,
		       data);

		exit(0);
	}

	query = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
					  &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);

	if (! query) {
		fprintf(stderr, "Unable to create add dictionary!\n");
		exit(1);
	}

	databuf = CFDataCreateMutable(kCFAllocatorDefault, 0);

	if (! databuf) {
		fprintf(stderr, "Cannot create data buffer\n");
		exit(1);
	}

	while (! feof(data) && ! ferror(data)) {
		unsigned char dbuf[4096];
		size_t cc;

		cc = fread(dbuf, 1, sizeof(dbuf), data);

		if (cc > 0)
			CFDataAppendBytes(databuf, dbuf, cc);
	}

	fclose(data);

	CFDictionaryAddValue(query, kSecValueData, databuf);

	CFRelease(databuf);

	if (result == NULL) {
		SecKeychainRef keychain;

		ret = SecKeychainCopyDefault(&keychain);

		if (ret) {
			fprintf(stderr, "Unable to get default "
				"keychain: %d\n", ret);
			exit(1);
		}

		CFDictionaryAddValue(query, kSecUseKeychain, keychain);
		CFDictionaryAddValue(query, kSecClass,
				     kSecClassGenericPassword);
		CFDictionaryAddValue(query, kSecAttrAccount, TEAMSCFSTR);
		CFDictionaryAddValue(query, kSecAttrService, TEAMSCFSTR);

		ret = SecItemAdd(query, NULL);

		if (ret) {
			fprintf(stderr, "SecItemAdd failed: %d\n", ret);
			exit(1);
		}
	} else {
		CFMutableDictionaryRef upquery;

		upquery = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
					  &kCFTypeDictionaryKeyCallBacks,
					  &kCFTypeDictionaryValueCallBacks);

		if (! upquery) {
			fprintf(stderr, "Unable to create update query!\n");
			exit(1);
		}

		CFDictionaryAddValue(upquery, kSecValueRef, result);

		ret = SecItemUpdate(upquery, query);

		if (ret) {
			fprintf(stderr, "SecItemUpdate failed: %d\n", ret);
			exit(1);
		}
	}

	exit(0);
}

static void
usage(const char *argv0)
{
	fprintf(stderr, "Usage: %s [-i] [-o] [data file]\n", argv0);
	fprintf(stderr, "\t-i for input mode\n");
	fprintf(stderr, "\t-o for output mode (default)\n");
	fprintf(stderr, "\tdata file - file for input/output, no argument "
		"means stdin/stdout\n");

	exit(1);
}
