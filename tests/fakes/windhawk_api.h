#pragma once
#include <windows.h>
struct WH_FIND_SYMBOL {
    void* address;
    PCWSTR symbol;
    PCWSTR symbolDecorated;
};
void Wh_Log(PCWSTR, ...);
int Wh_GetIntSetting(PCWSTR);
PCWSTR Wh_GetStringSetting(PCWSTR);
void Wh_FreeStringSetting(PCWSTR);
HANDLE Wh_FindFirstSymbol(HMODULE, const void*, WH_FIND_SYMBOL*);
BOOL Wh_FindNextSymbol(HANDLE, WH_FIND_SYMBOL*);
void Wh_FindCloseSymbol(HANDLE);
BOOL Wh_SetFunctionHook(void*, void*, void**);
