#include "StringUtilities.h"
#include "Platform/CommonWin.h" //TODO Fix <stringapi.h> and <Windows.h> conflict

std::wstring CharToString(const char* cstr) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, cstr, (int)strlen(cstr), nullptr, 0);
	std::wstring wcstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, cstr, (int)strlen(cstr), wcstr.data(), size_needed);
	return wcstr;
}
