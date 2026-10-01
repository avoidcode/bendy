#pragma once

// Temporary macOS diagnostics. Writes to /tmp/bendy_mac.log so the plugin can
// report what it sees without a debugger. No-op on other platforms.
#ifdef __APPLE__
#include <cstdio>
#include <cstdarg>

inline void BENDY_LOG (const char* fmt, ...)
{
	FILE* f = fopen ("/tmp/bendy_mac.log", "a");
	if (!f)
		return;
	va_list ap;
	va_start (ap, fmt);
	vfprintf (f, fmt, ap);
	va_end (ap);
	fputc ('\n', f);
	fclose (f);
}
#else
#define BENDY_LOG(...) ((void)0)
#endif
