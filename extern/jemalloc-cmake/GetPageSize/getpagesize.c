#include <windows.h>
#include <stdio.h>
int main(int argc, const char** argv) {
int result;
#ifdef _WIN32
SYSTEM_INFO si;
GetSystemInfo(&si);
result = si.dwPageSize;
#else
result = sysconf(_SC_PAGESIZE);
#endif
printf("%d", result);
return 0;
}
