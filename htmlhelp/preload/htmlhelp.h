// support HTML Help libs from recent MS SDKs.

#ifdef _MSC_VER
#error "htmlhelp/preload/htmlhelp.h is only for non-MSVC compilers"
#endif

#define _MSC_VER 1400 // simulate a high enough version to make sure htmlhelp doesn't define junk

#define __in
#define __in_opt

#include_next <htmlhelp.h>

#undef __in
#undef __in_opt

#undef _MSC_VER
