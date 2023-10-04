
#include <Windows.h>
#include <stdarg.h>
#include <string>

void _LogDbgView(const char* fmt, ...) {
	va_list arg;
	va_start(arg, fmt);

	char data[2048];
	vsprintf_s(data, fmt, arg);
	va_end(arg);

	OutputDebugStringA(data);
}
