#pragma once

#define LOG(...) { _LogDbgView(__VA_ARGS__); }

void _LogDbgView(const char* fmt, ...);
