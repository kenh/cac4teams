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
of the dyanmic linker (unless specifically permitted by entitlements);
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

## Yikes.  That all seems like ... a lot.  It's not that I don't trust you or anything ...

No, no, it's okay. I understand.

The best thing I can tell you is take a look at the source code.  It is
actually quite small!  The shell script source code is
[here](https://github.com/kenh/cac4teams/blob/main/src/cac4teams.in) and
the shared library shim is
[here.](https://github.com/kenh/cac4teams/blob/main/src/cac4teams.c)
It really is a small amount of code.

If you feel uncomfortable about running the ad-hoc signed copy of Teams
from `/tmp`, I did discover that your identity token is cached for an
unknown amount of time by the application.  So you CAN run `cac4teams`,
log in with your smartcard, quit the `cac4teams` copy of Teams and
then launch the real copy of Teams and it will use your saved identity
token.  I only tested that with Navy Flank Speed; how that works on other
instances of Teams is unknown.

I realize it's not possible to know if the pre-built binary package
actually corresponds to that source code unless you disassemble the
library, and most people won't be capable of doing that.  The installers
are provided as a service to the community to make distribution easier,
but I encourage anyone who unsure about the installer to build the package
theirself.  You'll need XCode or the Command Line Developer Tools
installed to do that.  Note that if you wish to build your own installer
package all of the tooling is in the `Makefile` (see the `product` and
`notarize` targets) but you'll need both a Developer ID Application and a
Developer ID Installer identity from Apple in your Keychain to create a
signed and notarized package.  See `configure --help` for more information
on how to specify those identities.

## I tried running this thing, but I get a whole lot of warnings on the command line

Yeah, it turns out that if you run Teams from the command line, you get a
whole pile of warnings, even without `cac4teams`.  You just normally don't
see all of those warnings because they're hidden by the MacOS X Finder.

## Instead of writing this thing, did you try submitting this bug to Microsoft?

Well, I **did** try but I ran into a problem right away, which is that
I couldn't figure how to do that.

If you select "Help" in Microsoft Teams, it just pulls up a generic help
page in the application that has things like "What to do if you have
trouble logging in".  There was no place to submit bug reports that
I could find.  I looked around the Microsoft web site, but there didn't
seem to be a way to submit bugs for Teams (there was a "submit comments
about Teams, but that didn't seem like a place for bug reports).

I asked my management chain if there was a way to contact Microsoft through
official DoD channels, and everyone pretty much said ¯\\_(ツ)_/¯.

I understand that Microsoft takes this approach to cut down on support
costs, but it does make submitting bugs difficult.

## Couldn't you have started at the Help Desk and pushed it that way?

Well, have you ever TRIED doing that?  In my experience when dealing with
problems like this and large organizations it is very difficult to
get problems like this past bureaucratic intertia and into the hands
of the appropriate developers.

Let's take a completely, made-up example, which is in no way related to
any real-world events.  Let's say you are trying to get a DoD PKI
certificate issued with an `id-pkinit-san` SubjectAlterateName, which
the NPE portal claims it supports.  But when you get the signed
certificate back it doesn't work at all.  So after a few days of
staring at the output of `openssl asn1parse` and reading multiple
RFCs, you find that the ASN.1 explicit tags for the `KRB5PrincipalName`
field are all 0 where they should be 1, in violation of
[RFC 4556](https://datatracker.ietf.org/doc/html/rfc4556.html).  So you
figure this is a very clear bug, and you decide to work the system
the proper way.  So you write up some very long and hopefully clear
emails and you start working it up the management chain, starting at your
local security office, and eventually you get punted to someone
at DISA who seems to understand your bug, you start a dialog ... and
you end up getting completely ghosted, with the bug never getting
fixed at all.

It is incredibly demoralizing and depressing to go through all of the
effort to document and submit a bug through multiple levels of
bureaucracy only to get radio silence.  So I decided to focus my
energy on creating a solution rather than just complain about the problem.

I would again like the emphasize the above scenario is COMPLETELY
hypothetical and absolutely DID NOT happen to me.

## Um, okay. So there's no hope in getting this bug fixed?

Well, if I am being COMPLETELY honest what I am hoping is that eventually
this package makes its way to the developers at Microsoft and the bug is
quietly fixed
and rolled out in a subsequent releae of Teams.  I know that sounds
farfetched, but
[stranger things have happened](https://github.com/kenh/keychain-pkcs11/commit/b3d2d3245153a3280eda890500b11b2b7613de7b).

## I don't quite feel comfortable running this software package, but I'd still like to help somehow.  Is there anything I can do?

Yes! If you could try to run this down via the management chain at YOUR
organization, that would be incredibly helpful. My impression is that
the more people that complain about a problem, the better.  Don't make
the case that you're asking for a feature request; this is obviously
a bugfix.

## This seems like a heck of a way to run a railroad

[Yes. Yes it is.](https://www.youtube.com/watch?v=TNBR2Js1dH0)

## Author

* **Ken Hornstein** - [kenh@cmf.nrl.navy.mil](mailto:kenh@cmf.nrl.navy.mil)
