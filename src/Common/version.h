#ifndef EMULATOR_NAME

#define EMULATOR_NAME					"Cemu"

#define EMULATOR_VERSION_SUFFIX			""

#define _XSTRINGFY(s) _STRINGFY(s)
#define _STRINGFY(s) #s

#if EMULATOR_VERSION_MAJOR != 0
#if EMULATOR_VERSION_PATCH != 0
#define BUILD_VERSION_WITH_NAME_STRING (EMULATOR_NAME " " _XSTRINGFY(EMULATOR_VERSION_MAJOR) "." _XSTRINGFY(EMULATOR_VERSION_MINOR) "." _XSTRINGFY(EMULATOR_VERSION_PATCH) EMULATOR_VERSION_SUFFIX)
#define BUILD_VERSION_STRING (_XSTRINGFY(EMULATOR_VERSION_MAJOR) "." _XSTRINGFY(EMULATOR_VERSION_MINOR) "." _XSTRINGFY(EMULATOR_VERSION_PATCH) EMULATOR_VERSION_SUFFIX)
#else
#define BUILD_VERSION_WITH_NAME_STRING (EMULATOR_NAME " " _XSTRINGFY(EMULATOR_VERSION_MAJOR) "." _XSTRINGFY(EMULATOR_VERSION_MINOR) EMULATOR_VERSION_SUFFIX)
#define BUILD_VERSION_STRING (_XSTRINGFY(EMULATOR_VERSION_MAJOR) "." _XSTRINGFY(EMULATOR_VERSION_MINOR) EMULATOR_VERSION_SUFFIX)
#endif
#else
// no version provided. Only show commit hash
#define BUILD_VERSION_STRING (_XSTRINGFY(EMULATOR_HASH) EMULATOR_VERSION_SUFFIX)
#define BUILD_VERSION_WITH_NAME_STRING (EMULATOR_NAME " " _XSTRINGFY(EMULATOR_HASH) EMULATOR_VERSION_SUFFIX)
#endif

#endif
