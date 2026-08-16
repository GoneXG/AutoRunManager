#include "autorun_core.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#ifndef WINVER
#define WINVER 0x0601
#endif

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <string>
#include <vector>

#define IDC_LIST       1001
#define IDC_BTN_REG    1002
#define IDC_BTN_UNREG  1003
#define IDC_EDIT_PATH  1004
#define IDC_BTN_BROWSE 1005
#define IDC_COMBO_PERM 1006
#define IDC_BTN_OK     1007
#define IDC_BTN_CANCEL 1008

static HINSTANCE g_hInst = nullptr;
static HWND g_hMain = nullptr;
static HWND g_hList = nullptr;
static HWND g_hBtnReg = nullptr;
static HWND g_hBtnUnreg = nullptr;
static std::vector<ac::Entry> g_entries;
static int g_dlgResult = 0;

static const wchar_t* kMainClass = L"ShiAutoRunMgrMainWnd";
static const wchar_t* kRegClass  = L"ShiAutoRunMgrRegDlg";

static std::wstring Trim(const std::wstring& s) {
    size_t b = s.find_first_not_of(L" \t\r\n");
    if (b == std::wstring::npos) return L"";
    size_t e = s.find_last_not_of(L" \t\r\n");
    return s.substr(b, e - b + 1);
}

static void UpdateUnregButton() {
    int sel = ListView_GetNextItem(g_hList, -1, LVNI_SELECTED);
    EnableWindow(g_hBtnUnreg, sel >= 0);
}

static void RefreshList() {
    g_entries = ac::ListEntries();
    ListView_DeleteAllItems(g_hList);
    for (size_t i = 0; i < g_entries.size(); ++i) {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        item.pszText = const_cast<LPWSTR>(g_entries[i].path.c_str());
        ListView_InsertItem(g_hList, &item);
        std::wstring perm = ac::PermName(g_entries[i].perm);
        ListView_SetItemText(g_hList, static_cast<int>(i), 1, const_cast<LPWSTR>(perm.c_str()));
    }
    UpdateUnregButton();
}

static bool BrowseForFile(HWND owner, std::wstring& out) {
    HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool needUninit = (hrInit == S_OK);
    bool ok = false;
    IFileOpenDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (SUCCEEDED(hr) && pfd) {
        pfd->SetTitle(L"\u9009\u62E9\u8981\u6CE8\u518C\u7684\u6587\u4EF6"); // ????????
        FILEOPENDIALOGOPTIONS opts = 0;
        pfd->GetOptions(&opts);
        pfd->SetOptions(opts | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
        IShellItem* item = nullptr;
        if (SUCCEEDED(pfd->Show(owner)) && SUCCEEDED(pfd->GetResult(&item)) && item) {
            PWSTR p = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &p)) && p) {
                out = p;
                CoTaskMemFree(p);
                ok = true;
            }
            item->Release();
        }
        pfd->Release();
    }
    if (needUninit) CoUninitialize();
    return ok;
}

LRESULT CALLBACK RegDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            if (code == BN_CLICKED) {
                if (id == IDC_BTN_BROWSE) {
                    std::wstring path;
                    if (BrowseForFile(hwnd, path)) {
                        SetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_PATH), path.c_str());
                    }
                    return 0;
                }
                if (id == IDC_BTN_OK) {
                    wchar_t buf[8192];
                    GetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_PATH), buf, 8192);
                    std::wstring path = Trim(buf);
                    if (path.empty()) {
                        MessageBoxW(hwnd, L"\u8BF7\u5148\u9009\u62E9\u6216\u8F93\u5165\u6587\u4EF6\u8DEF\u5F84\u3002", L"\u63D0\u793A", MB_OK | MB_ICONINFORMATION);
                        return 0;
                    }
                    std::wstring full = ac::NormalizePath(path);
                    if (!ac::FileExists(full)) {
                        MessageBoxW(hwnd, (L"\u6587\u4EF6\u4E0D\u5B58\u5728\u6216\u8DEF\u5F84\u65E0\u6548\uFF1A\n" + full).c_str(), L"\u9519\u8BEF", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    int sel = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_COMBO_PERM));
                    int perm = (sel == 1) ? ac::kPermNormal : ac::kPermAdmin;
                    std::wstring err;
                    if (!ac::RegisterEntry(full, perm, &err)) {
                        MessageBoxW(hwnd, err.empty() ? L"\u6CE8\u518C\u5931\u8D25\u3002" : err.c_str(), L"\u9519\u8BEF", MB_OK | MB_ICONERROR);
                        return 0;
                    }
                    g_dlgResult = 1;
                    DestroyWindow(hwnd);
                    return 0;
                }
                if (id == IDC_BTN_CANCEL) {
                    g_dlgResult = 0;
                    DestroyWindow(hwnd);
                    return 0;
                }
            }
            return 0;
        }
        case WM_CLOSE:
            g_dlgResult = 0;
            DestroyWindow(hwnd);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

static HWND CreateRegDlg(HWND parent) {
    RECT rc{};
    GetWindowRect(parent, &rc);
    int w = 560, h = 235;
    int x = rc.left + ((rc.right - rc.left) - w) / 2;
    int y = rc.top + ((rc.bottom - rc.top) - h) / 2;

    HWND dlg = CreateWindowExW(WS_EX_DLGMODALFRAME, kRegClass,
                               L"\u6CE8\u518C\u65B0\u6587\u4EF6", // ?????
                               WS_POPUP | WS_CAPTION | WS_SYSMENU,
                               x, y, w, h, parent, nullptr, g_hInst, nullptr);
    if (!dlg) return nullptr;

    auto mk = [&](const wchar_t* cls, const wchar_t* text, DWORD style,
                  int cx, int cy, int cw, int ch, int id) -> HWND {
        return CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style,
                               cx, cy, cw, ch, dlg,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInst, nullptr);
    };

    mk(L"STATIC", L"\u9009\u62E9\u6587\u4EF6\uFF1A", SS_LEFT, 16, 26, 80, 20, -1);       // ?????
    HWND edit = mk(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 96, 24, 392, 24, IDC_EDIT_PATH);
    SendMessageW(edit, EM_SETLIMITTEXT, 8190, 0);
    mk(L"BUTTON", L"\u25BC", BS_PUSHBUTTON | WS_TABSTOP, 490, 24, 42, 24, IDC_BTN_BROWSE); // ?

    mk(L"STATIC", L"\u8FD0\u884C\u6743\u9650\uFF1A", SS_LEFT, 16, 66, 80, 20, -1);       // ?????
    HWND combo = mk(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, 96, 64, 392, 220, IDC_COMBO_PERM);
    ComboBox_AddString(combo, L"\u7BA1\u7406\u5458\u6743\u9650");  // ?????
    ComboBox_AddString(combo, L"\u666E\u901A\u6743\u9650");        // ????
    ComboBox_SetCurSel(combo, 0);

    mk(L"BUTTON", L"\u6CE8\u518C", BS_PUSHBUTTON | WS_TABSTOP, 96, 118, 100, 32, IDC_BTN_OK);    // ??
    mk(L"BUTTON", L"\u53D6\u6D88", BS_PUSHBUTTON | WS_TABSTOP, 208, 118, 100, 32, IDC_BTN_CANCEL); // ??

    mk(L"STATIC",
       L"\u63D0\u793A\uFF1A\u6CE8\u518C\u540E\u5C06\u5728\u5F00\u673A\u65F6\u81EA\u52A8\u8FD0\u884C\u6240\u9009\u6587\u4EF6\uFF1B\u201C\u7BA1\u7406\u5458\u6743\u9650\u201D\u9879\u9700\u8981\u672C\u7A0B\u5E8F\u4FDD\u7559\u5728\u539F\u4F4D\u7F6E\u3002",
       SS_LEFT, 16, 168, 528, 42, -1);

    return dlg;
}

static bool RunRegisterDialog(HWND parent) {
    g_dlgResult = 0;
    HWND dlg = CreateRegDlg(parent);
    if (!dlg) return false;
    EnableWindow(parent, FALSE);
    ShowWindow(dlg, SW_SHOW);
    UpdateWindow(dlg);
    MSG msg{};
    while (IsWindow(dlg) && GetMessageW(&msg, nullptr, 0, 0)) {
        if (!IsDialogMessageW(dlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    EnableWindow(parent, TRUE);
    SetForegroundWindow(parent);
    return g_dlgResult != 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hMain = hwnd;
            g_hList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_LIST)), g_hInst, nullptr);
            ListView_SetExtendedListViewStyle(g_hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);

            LVCOLUMNW c1{};
            c1.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            c1.fmt = LVCFMT_LEFT;
            c1.cx = 520;
            c1.pszText = const_cast<LPWSTR>(L"\u6587\u4EF6\u7EDD\u5BF9\u8DEF\u5F84"); // ??????
            ListView_InsertColumn(g_hList, 0, &c1);

            LVCOLUMNW c2{};
            c2.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
            c2.fmt = LVCFMT_CENTER;
            c2.cx = 150;
            c2.pszText = const_cast<LPWSTR>(L"\u8FD0\u884C\u6743\u9650"); // ????
            ListView_InsertColumn(g_hList, 1, &c2);

            g_hBtnReg = CreateWindowExW(0, L"BUTTON", L"\u6CE8\u518C\u65B0\u6587\u4EF6", // ?????
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_REG)), g_hInst, nullptr);
            g_hBtnUnreg = CreateWindowExW(0, L"BUTTON", L"\u6CE8\u9500", // ??
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                0, 0, 0, 0, hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_BTN_UNREG)), g_hInst, nullptr);
            EnableWindow(g_hBtnUnreg, FALSE);

            RefreshList();
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lParam);
            int h = HIWORD(lParam);
            const int m = 10, bw = 140, bh = 34;
            MoveWindow(g_hBtnUnreg, w - m - bw, h - m - bh, bw, bh, TRUE);
            MoveWindow(g_hBtnReg, w - 2 * m - 2 * bw, h - m - bh, bw, bh, TRUE);
            MoveWindow(g_hList, m, m, w - 2 * m, h - 2 * m - bh - m, TRUE);
            return 0;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (HIWORD(wParam) == BN_CLICKED) {
                if (id == IDC_BTN_REG) {
                    if (RunRegisterDialog(hwnd)) RefreshList();
                    return 0;
                }
                if (id == IDC_BTN_UNREG) {
                    int sel = ListView_GetNextItem(g_hList, -1, LVNI_SELECTED);
                    if (sel < 0 || sel >= static_cast<int>(g_entries.size())) {
                        MessageBoxW(hwnd, L"\u8BF7\u5148\u9009\u4E2D\u4E00\u4E2A\u6587\u4EF6\u3002", L"\u63D0\u793A", MB_OK | MB_ICONINFORMATION);
                        return 0;
                    }
                    std::wstring msg = L"\u786E\u5B9A\u8981\u6CE8\u9500\u4EE5\u4E0B\u6587\u4EF6\u7684\u5F00\u673A\u81EA\u542F\u52A8\u5417\uFF1F\n\n" + g_entries[sel].path;
                    if (MessageBoxW(hwnd, msg.c_str(), L"\u786E\u8BA4\u6CE8\u9500", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES) {
                        std::wstring err;
                        if (!ac::UnregisterById(g_entries[sel].id, &err)) {
                            MessageBoxW(hwnd, err.empty() ? L"\u6CE8\u9500\u5931\u8D25\u3002" : err.c_str(), L"\u9519\u8BEF", MB_OK | MB_ICONERROR);
                        }
                        RefreshList();
                    }
                    return 0;
                }
            }
            return 0;
        }
        case WM_NOTIFY: {
            NMHDR* nm = reinterpret_cast<NMHDR*>(lParam);
            if (nm->idFrom == IDC_LIST && nm->code == LVN_ITEMCHANGED) {
                UpdateUnregButton();
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR lpCmdLine, int nCmdShow) {
    g_hInst = hInstance;

    // Parse the full command line. argv[0] is always the executable path
    // (CommandLineToArgvW returns it even for an empty command line), so skip it.
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::wstring> args;
    if (argv) {
        for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
        LocalFree(argv);
    }

    // Elevated-launch mode used by admin auto-start entries: --run <path>
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == L"--run") {
            std::wstring err;
            if (!ac::LaunchTarget(args[i + 1], &err)) {
                MessageBoxW(nullptr, err.c_str(), L"\u81EA\u542F\u52A8", MB_OK | MB_ICONERROR); // ???
            }
            return 0;
        }
    }

    // Command line file argument -> register it with default (administrator) permission
    std::wstring fileArg;
    for (const auto& a : args) {
        if (a == L"--run") continue;
        fileArg = a;
        break;
    }
    if (!fileArg.empty()) {
        std::wstring full = ac::NormalizePath(fileArg);
        std::wstring err;
        if (ac::RegisterEntry(full, ac::kPermAdmin, &err)) {
            MessageBoxW(nullptr,
                (L"\u5DF2\u6CE8\u518C\u5F00\u673A\u81EA\u542F\u52A8\uFF1A\n" + full + L"\n\n\uFF08\u9ED8\u8BA4\u7BA1\u7406\u5458\u6743\u9650\uFF09").c_str(),
                L"\u6CE8\u518C\u6210\u529F", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(nullptr, (L"\u6CE8\u518C\u5931\u8D25\uFF1A\n" + err).c_str(), L"\u9519\u8BEF", MB_OK | MB_ICONERROR);
            return 1;
        }
        // continue to open the manager window
    }

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASSW wc{};
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = g_hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.lpszClassName = kMainClass;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&wc);

    WNDCLASSW wc2{};
    wc2.lpfnWndProc = RegDlgProc;
    wc2.hInstance = g_hInst;
    wc2.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc2.lpszClassName = kRegClass;
    wc2.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    RegisterClassW(&wc2);

    HWND hwnd = CreateWindowExW(0, kMainClass,
        L"\u81EA\u542F\u52A8\u7BA1\u7406\u5668", // ??????
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 820, 500,
        nullptr, nullptr, g_hInst, nullptr);
    if (!hwnd) return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
