#pragma once

#include <stdio.h>

#ifdef _WIN32

#define sprintf_custom sprintf_s

#else /* _WIN32 */

#define sprintf_custom sprintf

#endif /* _WIN32 */

#ifndef __cplusplus

#define nullptr NULL

#endif /* __cplusplus */
