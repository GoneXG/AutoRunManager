#include "autorun_core.h"

#include <windows.h>
#include <cwctype>
#include <cstdio>

namespace ac {

namespace {

// Run key visible in Task Manager -> Startup tab
const wchar_t* kRunKey      = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
// Private metadata store for entries registered by this program
const wchar_t* kStoreKey    = L"Software\\ShiAutoRunMgr\\Entries";
const wchar_t* kValuePrefix = L"ShiAR_";

bool RegSetStr(HKEY root, const std::wstring& sub, const std::wstring& name, const std::wstring& val) {
    HKEY k = nullptr;
    LONG r = RegCreateKeyExW(root, sub.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr);
    if (r != ERROR_SUCCESS) return false;
    r = RegSetValueExW(k, name.c_str(), 0, REG_SZ,
                       reinterpret_cast<const BYTE*>(val.c_str()),
                       static_cast<DWORD>((val.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

bool RegSetDword(HKEY root, const std::wstring& sub, const std::wstring& name, DWORD val) {
    HKEY k = nullptr;
    LONG r = RegCreateKeyExW(root, sub.c_str(), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr);
    if (r != ERROR_SUCCESS) return false;
    r = RegSetValueExW(k, name.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&val), sizeof(val));
    RegCloseKey(k);
    return r == ERROR_SUCCESS;
}

void RegDelValue(HKEY root, const std::wstring& sub, const std::wstring& name) {
    HKEY k = nullptr;
    if (RegOpenKeyExW(root, sub.c_str(), 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        RegDeleteValueW(k, name.c_str());
        RegCloseKey(k);
    }
}

std::wstring MakeId(const std::wstring& path) {
    std::wstring lp = path;
    for (wchar_t& c : lp) c = static_cast<wchar_t>(towlower(static_cast<wint_t>(c)));
    unsigned h = 2166136261u;
    for (wchar_t c : lp) {
        h ^= static_cast<unsigned char>(c & 0xFF);
        h *= 16777619u;
        h ^= static_cast<unsigned char>((c >> 8) & 0xFF);
        h *= 16777619u;
    }
    wchar_t buf[16];
    swprintf_s(buf, 16, L"%08X", h);
    return buf;
}

bool IsExeLike(const std::wstring& p) {
    size_t dot = p.find_last_of(L'.');
    if (dot == std::wstring::npos) return false;
    std::wstring ext = p.substr(dot);
    for (wchar_t& c : ext) c = static_cast<wchar_t>(towlower(static_cast<wint_t>(c)));
    return ext == L".exe" || ext == L".com" || ext == L".bat" || ext == L".cmd";
}

} // namespace

std::wstring GetExePath() {
    wchar_t buf[4096];
    DWORD n = GetModuleFileNameW(nullptr, buf, 4096);
    if (n == 0 || n >= 4096) return L"";
    return std::wstring(buf, n);
}

std::wstring NormalizePath(const std::wstring& p) {
    if (p.empty()) return p;
    wchar_t buf[4096];
    DWORD n = GetFullPathNameW(p.c_str(), 4096, buf, nullptr);
    std::wstring full = (n > 0 && n < 4096) ? std::wstring(buf, n) : p;
    wchar_t lbuf[4096];
    DWORD ln = GetLongPathNameW(full.c_str(), lbuf, 4096);
    if (ln > 0 && ln < 4096) full = lbuf;
    return full;
}

bool FileExists(const std::wstring& p) {
    DWORD attr = GetFileAttributesW(p.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

bool IsElevated() {
    HANDLE h = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &h)) return false;
    TOKEN_ELEVATION te{};
    DWORD sz = 0;
    BOOL ok = GetTokenInformation(h, TokenElevation, &te, sizeof(te), &sz);
    CloseHandle(h);
    return ok && te.TokenIsElevated != 0;
}

bool RegisterEntry(const std::wstring& path, int perm, std::wstring* err) {
    std::wstring full = NormalizePath(path);
    DWORD attr = GetFileAttributesW(full.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES) {
        if (err) *err = L"\u6587\u4EF6\u4E0D\u5B58\u5728\u6216\u8DEF\u5F84\u65E0\u6548\uFF1A" + full;
        return false;
    }
    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        if (err) *err = L"\u4E0D\u80FD\u6CE8\u518C\u6587\u4EF6\u5939\uFF1A" + full;
        return false;
    }
    if (perm != kPermAdmin) perm = kPermNormal;

    std::wstring id = MakeId(full);
    std::wstring sub = std::wstring(kStoreKey) + L"\\" + id;

    bool ok = RegSetStr(HKEY_CURRENT_USER, sub, L"Path", full);
    ok = RegSetDword(HKEY_CURRENT_USER, sub, L"Perm", static_cast<DWORD>(perm)) && ok;

    std::wstring cmd;
    if (perm == kPermAdmin) {
        // Run this tool at logon; the admin manifest elevates it, then it launches the target.
        cmd = L"\"" + GetExePath() + L"\" --run \"" + full + L"\"";
    } else if (IsExeLike(full)) {
        cmd = L"\"" + full + L"\"";
    } else {
        // non-executable (e.g. document): open it through the shell association
        cmd = L"cmd.exe /c start \"\" \"" + full + L"\"";
    }

    ok = RegSetStr(HKEY_CURRENT_USER, kRunKey, kValuePrefix + id, cmd) && ok;
    if (!ok && err) *err = L"\u5199\u5165\u6CE8\u518C\u8868\u5931\u8D25\uFF0C\u8BF7\u786E\u8BA4\u7A0B\u5E8F\u4EE5\u7BA1\u7406\u5458\u6743\u9650\u8FD0\u884C\u3002";
    return ok;
}

bool UnregisterById(const std::wstring& id, std::wstring* err) {
    if (id.empty()) return false;
    RegDelValue(HKEY_CURRENT_USER, kRunKey, kValuePrefix + id);
    std::wstring sub = std::wstring(kStoreKey) + L"\\" + id;
    LONG r = RegDeleteTreeW(HKEY_CURRENT_USER, sub.c_str());
    if (r != ERROR_SUCCESS && r != ERROR_FILE_NOT_FOUND && r != ERROR_PATH_NOT_FOUND) {
        if (err) *err = L"\u6CE8\u9500\u5931\u8D25\uFF0C\u9519\u8BEF\u7801 " + std::to_wstring(r);
        return false;
    }
    return true;
}

bool UnregisterByPath(const std::wstring& path, std::wstring* err) {
    return UnregisterById(MakeId(NormalizePath(path)), err);
}

std::vector<Entry> ListEntries() {
    std::vector<Entry> out;
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kStoreKey, 0, KEY_READ, &k) != ERROR_SUCCESS) return out;
    DWORD i = 0;
    wchar_t name[256];
    for (;;) {
        DWORD nsize = 256;
        LONG r = RegEnumKeyExW(k, i++, name, &nsize, nullptr, nullptr, nullptr, nullptr);
        if (r != ERROR_SUCCESS) break;
        std::wstring sub = std::wstring(kStoreKey) + L"\\" + name;
        HKEY sk = nullptr;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, sub.c_str(), 0, KEY_READ, &sk) != ERROR_SUCCESS) continue;

        wchar_t path[4096];
        DWORD psz = sizeof(path);
        DWORD ptype = 0;
        LONG rp = RegQueryValueExW(sk, L"Path", nullptr, &ptype, reinterpret_cast<BYTE*>(path), &psz);

        DWORD perm = static_cast<DWORD>(kPermNormal);
        DWORD qsz = sizeof(perm);
        DWORD qtype = 0;
        if (RegQueryValueExW(sk, L"Perm", nullptr, &qtype, reinterpret_cast<BYTE*>(&perm), &qsz) != ERROR_SUCCESS)
            perm = static_cast<DWORD>(kPermNormal);

        RegCloseKey(sk);

        if (rp == ERROR_SUCCESS && ptype == REG_SZ) {
            Entry e;
            e.id = name;
            e.path = path;
            e.perm = (perm == static_cast<DWORD>(kPermAdmin)) ? kPermAdmin : kPermNormal;
            out.push_back(std::move(e));
        }
    }
    RegCloseKey(k);
    return out;
}

std::wstring ReadRunValue(const std::wstring& id) {
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &k) != ERROR_SUCCESS) return L"";
    wchar_t buf[8192];
    DWORD sz = sizeof(buf);
    DWORD type = 0;
    LONG r = RegQueryValueExW(k, (kValuePrefix + id).c_str(), nullptr, &type, reinterpret_cast<BYTE*>(buf), &sz);
    RegCloseKey(k);
    if (r != ERROR_SUCCESS || type != REG_SZ) return L"";
    return std::wstring(buf);
}

std::wstring PermName(int perm) {
    return perm == kPermAdmin ? std::wstring(L"\u7BA1\u7406\u5458\u6743\u9650")
                              : std::wstring(L"\u666E\u901A\u6743\u9650");
}

bool LaunchTarget(const std::wstring& path, std::wstring* err) {
    HINSTANCE r = ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(r) <= 32) {
        if (err) *err = L"\u542F\u52A8\u76EE\u6807\u6587\u4EF6\u5931\u8D25\u3002";
        return false;
    }
    return true;
}

} // namespace ac
