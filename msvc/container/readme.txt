This directory contains include files that short-circuit MSVC's standard container implementation
in favor of the one provided by Boost.Container. This allows, in particular, emplace / emplace_back
with variable arguments which MSVC is lacking (as of 2010).
