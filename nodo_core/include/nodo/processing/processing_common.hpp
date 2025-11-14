#pragma once

// Windows-specific defines that must come before any standard library includes
// This is required for PMP library which uses M_PI and other math constants
#ifdef _WIN32
  #ifndef _USE_MATH_DEFINES
    #define _USE_MATH_DEFINES
  #endif
#endif

// Common includes for processing module
#include <cmath>
