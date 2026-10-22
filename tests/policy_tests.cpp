#define NATIVE_SMOOTH_POLICY_TEST
#include "../native-smooth-taskbar.wh.cpp"

#include <cstdlib>
#include <iostream>

namespace {
int checks = 0;
void Check(bool condition, const char* description) {
    ++checks;
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        std::exit(1);
    }
}
}

int main() {
    using namespace NativeSmooth;
    for (bool enabled : {false, true}) {
        for (bool respect : {false, true}) {
            for (bool modifier : {false, true}) {
                Check(ShouldCustomize(enabled, respect, modifier) ==
                      (enabled && (!respect || !modifier)),
                      "master switch and modifier bypass");
            }
        }
    }
    for (int group = -1; group <= 5; ++group) {
        for (int action = -1; action <= 8; ++action) {
            for (bool cycle : {false, true}) {
                for (bool customize : {false, true}) {
                    const bool change = group == 3 && action == 0 && cycle && customize;
                    Check(ClickAction(action, group, cycle, customize) ==
                          (change ? 4 : action),
                          "only ordinary combined-group clicks are remapped");
                }
            }
        }
    }
    for (auto request : {Request::Other, Request::Minimize, Request::Restore}) {
        for (bool minimize : {false, true}) {
            for (bool restore : {false, true}) {
                for (bool eligible : {false, true}) {
                    for (bool iconic : {false, true}) {
                        const bool expected = eligible &&
                            ((request == Request::Minimize && minimize && !iconic) ||
                             (request == Request::Restore && restore && iconic));
                        Check(ShouldQueue(request, minimize, restore, eligible, iconic) == expected,
                              "async request eligibility and window state");
                    }
                }
            }
        }
    }
    const auto excluded = [](std::wstring_view list, std::wstring_view name) {
        return Excluded(list, name, EqualName);
    };
    Check(!excluded(L"", L"app.exe"), "empty list");
    Check(excluded(L"", L""), "unknown process fails closed");
    Check(excluded(L"game.exe;EDITOR.EXE", L"editor.exe"), "case-insensitive exclusion");
    Check(excluded(L" ; \tgame.exe\r\n; ; ", L"GAME.exe"), "whitespace and empty entries");
    Check(!excluded(L"game.exe", L"mygame.exe"), "no suffix match");
    Check(!excluded(L"game.exe", L"game.exe.bak"), "no prefix match");
    Check(!excluded(L"*.exe", L"app.exe"), "wildcards are not silently expanded");
    Check(!excluded(L"C:\\app.exe", L"app.exe"), "paths do not match basenames");
    Check(excluded(L"a.exe;b.exe;c.exe", L"c.exe"), "last entry");
    Check(!excluded(L";;; \t", L"app.exe"), "all entries empty");
    Check(!ShouldQueue(Request::Restore, true, true, true, false),
          "restoring an already maximized window must remain native");
    Check(ClickAction(4, 3, true, true) == 4, "native Ctrl+click never flipped back");
    std::cout << checks << " policy checks passed\n";
}
