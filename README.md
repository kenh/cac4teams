# CAC4TEAMS

This is a simple shim designed to enable smartcard login support
for the Microsoft Teams app on MacOS X.  When using this shim, you should
be able to launch Teams and use your smartcard to authenticate to
Teams.

## How do I use it?

Download the latest package from the
[release page](https://github.com/kenh/cac4teams/releases), open
Terminal.app, and run `/usr/local/bin/cac4teams` from a command-line
window.  If everything is working properly, Teams should come up
(the first time you might need to allow it access to some Microsoft
keychain items) and you should be able to use your smartcard to
authenticate to Teams at the appropriate point.

## Um, wait.  How does this work?  You added Smartcard support to the Teams app??

Well, SINCE you're asking ... not exactly.

I noticed when trying to use the Teams app that it got rather far in
the client certificate negotiation.  A window would pop up asking you
to select a client certificate to use, but the list of available certificates
was always blank.  This kind of surprised me, as in my experience if you
get that list of certificates the application should just work.

So, fast forward a bit later ... I poked around the Teams app a bit,
and I discovered that there was a legitimate bug in the Teams app, so
I wondered if I could create a workaround for the bug, would you be
able to use a Smartcard?  Turns out the answer is "yes"!

## So what is the bug in Teams?

Well, it gets kind of technical but since you asked ...

To query Keychain entries (including smartcards) most of the time you
call a function called `SecItemCopyMatching()`.  You give it a dictionary
containing all of the parameters of your query, and exactly what you
want it to return.  So if you want to find out a certificate with a
particular issuer, you might say, "I'm looking for certificates, they
have to have this issuer, and I'd like you to return the certificate
data".

When I dug into it, I found Teams was calling `SecItemCopyMatching()`
and the query dictionary said it was looking for identities (a certificate
plus private key, which is correct), certificates which matched a given
set of issuers (also correct), but the query dictionary did not include
any information on what to return.  So `SecItemCopyMatching()` would
always return nothing.  Also, the Teams app is signed and enrolled in
the hardened runtime environment (which is good) but it lacks a special
entitlement called `com.apple.security.smartcard` which grants
applications access to smartcards.

## Huh, that seems kind of surprising. What happened there?

Well, in digging more into this I found that they are using something
called [Electron](https://www.electronjs.org/) as a cross-platform
GUI framework for Teams.  And Electron contains a large chunk of the
Chromium source code in it, and Chromium on Mac already has support
for smartcards.  My guess is that some Microsoft programmer did a
cut & paste and missed one or two items when creating the query
dictionary for `SecItemCopyMatching()`.  Or maybe the bug is in
Electron somewhere; I felt like I had dug into it enough, tracking down
the problem more wasn't going to be helpful.  The bottom line is
that without a return flag set in the query dictionary, nothing would
ever be returned from `SecItemCopyMatching()`.

As for the missing `com.apple.security.smartcard` entitlement, I suspect
they just didn't know about that one.

## So, wait, if this bug in inside of Teams, how did you fix it?

What `cac4teams` does is it uses something called dyld interposing to
intercept the `SecItemCopyMatching()` function call.  If it detects that
it has been called with a query for identities AND there are no
results flag set in the dictionary, it will add the appropriate return
flag to the query dictionary and then call the real `SecItemCopyMatching()`
function.  In that case (assuming you have a working smartcard) the
certificate selection dialog will appear with the certificates on your
smartcard and at that point everything works.

## Wait, that sounds kind of scary.  You can just intercept functions in any application you want??

Well, no, not exactly.

Assuming your application is signed and notarized, it has to be enrolled
in what Apple calls the hardened runtime environment.  This means the
application has a bunch of security restrictions surrounding it, like
you don't get access to the microphone or the camera (or smartcards)
unless they are specifically granted by what are called "entitlements".
This is good!  That means applications that are buggy or compromised don't
get access to resources that they shouldn't have access to.

The way that dyld interposing works is you need to set a special
environment variable (`DYLD_INSERT_LIBRARIES`) pointing to a library
you wish the dynamic linker to load at startup time.  This library
has the ability to intercept any dynamically loaded function.

But applications that are enrolled in the hardened runtime environment
do not permit the use of environment variables to modify the behavior
of the dyanmic linker (unless specifically permitted by entitlements;
I personally haven't ever encountered an application that permits
this via entitlements, and the Teams app definitely does not.  So it's
not something you can do under ordinary circumstances.

## So if you can't do that, how does this possibly work?

So what is happening is `cac4teams` is a shell script which does the
following things:

* It copies the entire Teams application into `/tmp`.
* It extracts the existing entitlements used by Teams and adds two new
  ones: `com.apple.security.smartcard` for smartcard access and
  `com.apple.security.cs.allow-dyld-environment-variables` to allow
  the use of dyld environment variables to permit interposing.
* It signs the copy of Teams in `/tmp` using an ad-hoc signature
  with the extra entitlements added.
* It then runs the copy of Teams in `/tmp` with the special
  `DYLD_INSERT_LIBRARIES` environment variable pointing to a special
  shared library which does the appropriate adjustment to the
  `SecItemCopyMatching()` dictionary mentioned previously.
