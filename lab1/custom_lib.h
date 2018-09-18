#pragma once

#include <stdio.h>
#include <string.h>

#ifdef _WIN32

#define sprintf_custom sprintf_s
#define ZeroMemory_custom ZeroMemory

#else /* _WIN32 */

#define sprintf_custom sprintf
#define ZeroMemory_custom(ptr, size) memset(ptr, 0, size)

#endif /* _WIN32 */

#ifndef __cplusplus

#define nullptr NULL

#endif /* __cplusplus */
