all:
	cd build/cmake && \
	cmake . && \
	$(MAKE) $(*)

release:
	cd build/cmake && \
	cmake . -DCMAKE_BUILD_TYPE=Release && \
	$(MAKE) $(*)

debug:
	cd build/cmake && \
	cmake . -DCMAKE_BUILD_TYPE=Debug && \
	$(MAKE) $(*)

test:
	cd build/cmake && \
	cmake . && \
	$(MAKE) test $(*)

tests:
	cd build/cmake && \
	cmake . && \
	$(MAKE) tests $(*)

clean:
	cd build/cmake && \
	( test -e Makefile && $(MAKE) clean $(*) ) || true

DEPVC8REV=HEAD
DEPVC9REV=6
DEPOSXREV=15
DEPSOURCE=152
DEPARCHINDEP=6
#========== Dependencies ===========

distributions:
	case "`uname`" in \
		*arwin*) \
			svn co -r$(DEPOSXREV) http://sirikataosx.googlecode.com/svn/trunk dependencies \
			;; \
		*MINGW*|*CYGWIN*|*win32*) \
			if [ x = x"$(VCVER)" ]; then \
				echo To force a specific version of visual studio, set VCVER to 8 or 9 ; \
				[ -e c:/Program\ Files/Microsoft\ Visual\ Studio\ 9*/VC/bin ] && \
					svn co -r$(DEPVC9REV) http://sirikatawin32.googlecode.com/svn/trunk dependencies || \
					svn co -r$(DEPVC8REV) http://sirikata.googlecode.com/svn/trunk/win32vc8 dependencies ; \
			else \
				if [ x"9" = x"$(VCVER)" ]; then \
					svn co -r$(DEPVC9REV) http://sirikatawin32.googlecode.com/svn/trunk dependencies ; \
				else \
					svn co -r$(DEPVC8REV) http://sirikata.googlecode.com/svn/trunk/win32vc$(VCVER) dependencies ; \
				fi \
			fi \
			;; \
		*) \
			svn co -r$(DEPSOURCE) http://sirikata.googlecode.com/svn/trunk/source dependencies \
			;; \
	esac ; \
	svn co -r$(DEPARCHINDEP) http://sirikatamachindep.googlecode.com/svn/trunk/ dependencies/machindependencies

update-dependencies: distributions
	git submodule init || true
	git submodule update || true
	case "`uname`" in \
		*arwin*) \
			cd dependencies && svn update -r$(DEPOSXREV) \
			;; \
		*MINGW*|*CYGWIN*|*win32*) \
			if [ x = x"$(VCVER)" ]; then \
				echo To force a specific version of visual studio, set VCVER to 8 or 9 ; \
				cd dependencies && [ -e c:/Program\ Files/Microsoft\ Visual\ Studio\ 9*/VC/bin ] && \
					svn update -r$(DEPVC9REV)  || \
					svn update -r$(DEPVC8REV); \
			else \
				if [ x"9" = x"$(VCVER)" ]; then \
					cd dependencies && svn update -r$(DEPVC9REV) ; \
				else \
					cd dependencies && svn update -r$(DEPVC8REV); \
				fi \
			fi \
			;; \
		*) \
			cd dependencies && svn update -r$(DEPSOURCE) \
			;; \
	esac ; \
	svn co -r$(DEPARCHINDEP) http://sirikatamachindep.googlecode.com/svn/trunk/ dependencies/machindependencies

minimaldepends: update-dependencies
	$(MAKE) -C dependencies minimaldepends $(*)

minimalfulldepends: update-dependencies
	$(MAKE) -C dependencies minimalrootdepends minimaldepends $(*)

depends: update-dependencies
	$(MAKE) -C dependencies depends $(*)

fulldepends: update-dependencies
	$(MAKE) -C dependencies fulldepends $(*)
