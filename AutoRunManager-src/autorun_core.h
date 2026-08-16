#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif

#include <string>
#include <vector>

namespace ac {

constexpr int kPermNormal = 0;   // normal (current user) permission
constexpr int kPermAdmin  = 1;   // run as administrator

struct Entry {
    std::wstring id;      // internal entry id (hash of normalized path)
    std::wstring path;    // absolute file path
    int perm = kPermNormal;
};

std::wstring GetExePath();
std::wstring NormalizePath(const std::wstring& p);
bool FileExists(const std::wstring& p);
bool IsElevated();

bool RegisterEntry(const std::wstring& path, int perm, std::wstring* err = nullptr);
bool UnregisterById(const std::wstring& id, std::wstring* err = nullptr);
bool UnregisterByPath(const std::wstring& path, std::wstring* err = nullptr);
std::vector<Entry> ListEntries();
std::wstring ReadRunValue(const std::wstring& id);

std::wstring PermName(int perm);
bool LaunchTarget(const std::wstring& path, std::wstring* err = nullptr);

} // namespace ac
