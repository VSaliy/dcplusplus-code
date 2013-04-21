// Note: this is for Windows...

#include "config.h.in"

#define ARCHIVE_CRYPTO_MD5_OPENSSL 1
#define ARCHIVE_CRYPTO_RMD160_OPENSSL 1
#define ARCHIVE_CRYPTO_SHA1_OPENSSL 1
#define ARCHIVE_CRYPTO_SHA256_OPENSSL 1
#define ARCHIVE_CRYPTO_SHA384_OPENSSL 1
#define ARCHIVE_CRYPTO_SHA512_OPENSSL 1
#define HAVE_BZLIB_H 1
#define HAVE_CTYPE_H 1
#define HAVE_DECL_INT64_MAX 1
#define HAVE_DECL_INT64_MIN 1
#define HAVE_DECL_SIZE_MAX 1
#define HAVE_DECL_SSIZE_MAX 1
#define HAVE_DECL_UINT32_MAX 1
#define HAVE_DECL_UINT64_MAX 1
#define HAVE_DIRENT_H 1
#define HAVE_ERRNO_H 1
#define HAVE_FCNTL_H 1
#define HAVE_GMTIME_R 1
#define HAVE_INTMAX_T 1
#define HAVE_INTTYPES_H 1
#define HAVE_LIMITS_H 1
#define HAVE_LOCALE_H 1
#define HAVE_LOCALTIME_R 1
#define HAVE_LONG_LONG_INT 1
#define HAVE_MEMMOVE 1
#define HAVE_MEMORY_H 1
#define HAVE_MEMSET 1
#define HAVE_OPENSSL_EVP_H 1
#define HAVE_SETENV 1
#define HAVE_SETLOCALE 1
#define HAVE_STDARG_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRCHR 1
#define HAVE_STRDUP 1
#define HAVE_STRERROR 1
#define HAVE_STRFTIME 1
#define HAVE_STRINGS_H 1
#define HAVE_STRING_H 1
#define HAVE_STRNCPY_S 1
#define HAVE_STRRCHR 1
#define HAVE_SYS_CDEFS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TIME_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_SYS_UTIME_H 1
#define HAVE_TIME_H 1
#define HAVE_TZSET 1
#define HAVE_UINTMAX_T 1
#define HAVE_UNISTD_H 1
#define HAVE_UNSETENV 1
#define HAVE_UNSIGNED_LONG_LONG 1
#define HAVE_UNSIGNED_LONG_LONG_INT 1
#define HAVE_UTIME 1
#define HAVE_UTIME_H 1
#define HAVE_VPRINTF 1
#define HAVE_WCHAR_H 1
#define HAVE_WCHAR_T 1
#define HAVE_WCRTOMB 1
#define HAVE_WCSCMP 1
#define HAVE_WCSCPY 1
#define HAVE_WCSLEN 1
#define HAVE_WCTOMB 1
#define HAVE_WCTYPE_H 1
#define HAVE_WINCRYPT_H 1
#define HAVE_WINDOWS_H 1
#define HAVE_WINIOCTL_H 1
#define HAVE_WMEMCMP 1
#define HAVE_WMEMCPY 1
#define HAVE_ZLIB_H 1
#define HAVE__CTIME64_S 1
#define HAVE__FSEEKI64 1
#define HAVE__GET_TIMEZONE 1
#define HAVE__LOCALTIME64_S 1
#define HAVE__MKGMTIME64 1
#define STDC_HEADERS 1
#define WINVER 0x502
#define _WIN32_WINNT 0x502
#define gid_t short
#define uid_t short

// for Visual Studio...
#ifdef _MSC_VER
#undef HAVE_INTTYPES_H
#undef HAVE_SYS_CDEFS_H
#undef HAVE_UNISTD_H
#define mode_t unsigned short
#define pid_t SSIZE_T
#define ssize_t SSIZE_T
#endif
