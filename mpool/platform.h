// @platform.h

#ifndef __CYX_PLATFORM
#define __CYX_PLATFORM

#if defined(__x86_64__) || defined(_M_X64)
    #define PLATFORM_X64 64
#elif defined(__i386__) || defined(_M_X86)
    #define PLATFORM_X86 32
#elif defined(__aarch64__)
    #define PLATFORM_ARM64 64
#elif defined(__arm__)
    #define PLATFORM_ARM32 32
#elif defined(__AVR__)
    #define PLATFORM_AVR 8
#else
    #error "Unsupported platform"
#endif

#endif
