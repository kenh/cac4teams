##
## Our Makefile.am template for Makefile.in (and eventually, Makefile)
##
## Process this with automake to generate Makefile.in (or better yet,
## use autoreconf)
##

AM_CFLAGS = -Wall

bin_SCRIPTS = cac4teams
noinst_PROGRAMS = teamscache
CLEANFILES = $(bin_SCRIPTS)

EXTRA_DIST = \
	src/cac4teams.in \
	README.md \
	packaging/entitlements.plist \
	#

lib_LTLIBRARIES = libcac4teams.la

libcac4teams_la_SOURCES = \
		src/cac4teams.c \
		#

libcac4teams_la_LDFLAGS = \
		-shared \
		-avoid-version \
		-framework Security \
		#

teamscache_SOURCES = \
		src/teamscache.c \
		#
teamscache_LDFLAGS = -framework Security -framework CoreFoundation

do_subst = ( sed \
	-e 's,[@]libdir[@],$(libdir),g' \
	)

cac4teams: $(srcdir)/src/cac4teams.in Makefile
	rm -f $@ $@-t
	$(do_subst) < $(srcdir)/src/$@.in > $@-t
	chmod +x $@-t
	mv -f $@-t $@

##
## The targets to perform MacOS X packing.  In no particular order:
##
## destdir	- Build the "destination directory" for the package, which
##		  is done by executing "make install" with DESTDIR set to
##		  PACKAGEBUILDDIR.  This rule also codesigns any libraries
##		  or executables.
##
## packages	- Generate all of the component packages using pkgbuild.
##
## product	- Generate the product archive (the "real" installer) using
##		  the component packages and the XML distribution
##		  metadata file.  If anything needs to be generated to appear
##		  in the product archive it should be dependency of
##		  this target.  If a Developer ID Installer identity is
##		  available the product archive is signed.
##
## notarize	- Submit a pre-built product archive to the Apple notarization
##		  service for notarization and wait for notarization to
##		  complete.  If notarization is successful the package will
##		  have the notarization ticket stapled to the installer
##		  package.

.PHONY: destdir packages product notarize
PACKAGEBUILDDIR=/tmp/packagebuild
NOTARIZE_STATUS=/tmp/notarize-status
CLEANFILES+=$(PACKAGE_NAME)*.pkg

##
## Build a destination directory for our component packages
##

destdir:
	rm -rf $(PACKAGEBUILDDIR)
	$(MAKE) install DESTDIR=$(PACKAGEBUILDDIR)
	rm -f $(PACKAGEBUILDDIR)$(libdir)/libcac4teams.la
	@set -x; if [ "$(APP_SIGNING_ID)" != "unknown" ]; then \
          for binary in $$(find $(PACKAGEBUILDDIR)$(bindir) -type f -perm +111 -print); do \
	    $(CODESIGN) --timestamp --sign "$(APP_SIGNING_ID)" -o runtime --entitlements "$(srcdir)/packaging/entitlements.plist" --identifier "$(APPIDENTIFIER).$$(basename $$binary)" $$binary; \
	  done; \
	  for library in $$(find $(PACKAGEBUILDDIR)$(libdir) -type f -print); do \
	    $(CODESIGN) --timestamp --sign $(APP_SIGNING_ID) --identifier "$(APPIDENTIFIER).$$(basename $$library)" $$library; \
	  done; \
	fi

##
## Build our component packages necessary for the product archive.
##
## Note that component packages themselves do not need to be signed.
## Signing of the product archive takes care of that.
##

packages: destdir
	$(PKGBUILD) \
		--root "$(PACKAGEBUILDDIR)" \
		--identifier "$(APPIDENTIFIER).dylib" \
		--install-location "/" \
		--version $(PACKAGE_VERSION) \
		$(PACKAGE_NAME)-app.pkg

product: packages packaging/distribution.xml packaging/resources/welcome.rtf
	$(PRODUCTBUILD) \
		--identifier "$(APPIDENTIFIER)" \
		--version $(PACKAGE_VERSION) \
		--resources "$(srcdir)/packaging/resources" \
		--resources "./packaging/resources" \
		--distribution "./packaging/distribution.xml" \
		$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg
	@if [ "$(INSTALLER_SIGN_CN)" != "unknown" ]; then \
		rm -f tmp-$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg; \
		set -x; \
		$(PRODUCTSIGN) \
			--timestamp \
			--sign "$(INSTALLER_SIGN_CN)" \
			$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg \
			tmp-$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg; \
		mv -f tmp-$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg \
			$(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg; \
	fi

##
## Submit to the notarization service and wait for it to complete, then
## staple the package
##

notarize:
	@if [ "$(NOTARIZATION_ID)" = "unknown" -o "$(NOTARIZATION_PW)" = "unknown" ]; then \
		echo "NOTARIZATION_ID and NOTARIZATION_PW must be set for notarization support"; \
		exit 1; \
	fi
	rm -f $(NOTARIZE_STATUS)
	$(XCRUN) altool --notarize-app \
		-u "$(NOTARIZATION_ID)" \
		-p "$(NOTARIZATION_PW)" \
		--asc-provider "$(ASC_PROVIDER)" \
		--primary-bundle-id "$(APPIDENTIFIER)" \
		--output-format xml \
		-f $(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg \
		> $(NOTARIZE_STATUS)
	@UUID=`$(PLBUDDY) -c "Print :notarization-upload:RequestUUID" $(NOTARIZE_STATUS)`; \
	  echo "Waiting for notarization status for request ID $$UUID"; \
	  while true; do \
	    sleep 10; \
	    $(XCRUN) altool \
		--notarization-info $$UUID \
		-u "$(NOTARIZATION_ID)" \
		-p "$(NOTARIZATION_PW)" \
		--output-format xml > $(NOTARIZE_STATUS); \
	    if [ $$? -ne 0 ]; then \
	      echo "altool failed"; \
	      exit 1; \
	    fi; \
	    STATUS=`$(PLBUDDY) -c "Print :notarization-info:Status" $(NOTARIZE_STATUS)`; \
	    if [ "$$STATUS" != "in progress" ]; then \
	      break; \
	    fi; \
	  done; \
	  if [ "$$STATUS" != "success" ]; then \
	    echo "Notarization failed, check $(NOTARIZE_STATUS) for more info"; \
	    echo "Log file URL is: "; \
	    $(PLBUDDY) -c "Print :notarization-info:LogFileURL" $(NOTARIZE_STATUS); \
	    exit 1; \
	  fi; \
	  echo "Notarization successful"
	$(XCRUN) stapler staple $(PACKAGE_NAME)-$(PACKAGE_VERSION).pkg
