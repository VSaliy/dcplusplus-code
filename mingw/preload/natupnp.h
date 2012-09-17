// support includes from recent MS SDKs.

#define _MSC_VER 1400 // simulate a high enough version to make sure the include doesn't define junk

#define __in
#define __in_opt
#define _In_
#define _In_opt_
#define __RPC__in
#define __RPC__in_opt
#define __RPC__in_xcount(x)
#define __RPC__inout_xcount(x)
#define __RPC__out
#define __RPC__deref_out_opt
#define __REQUIRED_RPCNDR_H_VERSION__ 400

#include_next <natupnp.h>

#undef __in
#undef __in_opt
#undef _In_
#undef _In_opt_
#undef __RPC__in
#undef __RPC__in_opt
#undef __RPC__in_xcount
#undef __RPC__inout_xcount
#undef __RPC__out
#undef __RPC__deref_out_opt
#undef __REQUIRED_RPCNDR_H_VERSION__

#undef _MSC_VER
