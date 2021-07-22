#ifndef RYULIB_DEBUG_TOOLS_HPP
#define RYULIB_DEBUG_TOOLS_HPP

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

class DebugOutput {
public:
	static void trace(const char* format, ...) {
		va_list vaList;

		#ifdef __linux__ 
			printf(format, vaList);
			printf("\n");
		#elif _WIN32
			char buffer[4096];
			va_start(vaList, format);
			_vsnprintf_s(buffer, 4096, format, vaList);
			va_end(vaList);
			OutputDebugStringA(buffer);
		#endif
	}
};

#endif  // RYULIB_DEBUG_TOOLS_HPP
