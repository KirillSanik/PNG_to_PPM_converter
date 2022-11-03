#ifndef main_h
#define main_h

#include "return_codes.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//#define ONMYPC

//#define ZLIB
//#define LIBDEFLATE
//#define ISAL

#ifdef ZLIB
	#include <zlib.h>
#elif defined(LIBDEFLATE)
	#include <libdeflate.h>
#elif defined(ISAL)
	#ifdef ONMYPC
		#include <isa-l/igzip_lib.h>
	#else
		#include <include/igzip_lib.h>
	#endif
#else
	#error no def macros
#endif

typedef unsigned int uint;
typedef unsigned long uLong;
typedef unsigned char uchar;

#endif
