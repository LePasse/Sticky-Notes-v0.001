// Notes.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "shellapi.h"
#include "resource.h"
#include <vector>
#include <algorithm>
#include <fstream>
#include <string> 
#include "atlstr.h"

#define MAX_LOADSTRING 100
#define APPWM_ICONNOTIFY (WM_APP + 1)
#define ID_EDITCHILD 100

#pragma warning(disable : 4996)

struct note {
    HWND window, edit;
    WORD x, y, width, height;
    BOOL locked;
};

struct Config {
    RECT rc;
    BOOL locked;
};

// Глобальные переменные:
HINSTANCE hInst, hNote, hEdit;                  // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];
HWND hwndForFind;
HFONT hFont;

std::vector<note> noteWindow;



// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK    LoadProc(HWND, UINT, WPARAM, LPARAM);

int cmd;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    setlocale(LC_ALL, "Russian");
    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOTES, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);


    // Выполнить инициализацию приложения:
    HWND hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, szWindowClass, szTitle,
        WS_CHILD | WS_CAPTION | WS_SYSMENU,
        50, 50, 200, 100, GetDesktopWindow(), nullptr, hInstance, nullptr);
    EnableWindow(hWnd, TRUE);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOTES));

    MSG msg;

    /////
    HICON hIcon = static_cast<HICON>(LoadImage(NULL,
        TEXT("sticker.ico"),
        //MAKEINTRESOURCE(IDC_NOTES),
        IMAGE_ICON,
        0, 0,
        LR_DEFAULTCOLOR | LR_SHARED | LR_DEFAULTSIZE | LR_LOADFROMFILE));

    SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    //Notification
    NOTIFYICONDATA nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = APPWM_ICONNOTIFY;
    nid.hIcon = hIcon;
    // This text will be shown as the icon's tooltip.

    // Show the notification.
    Shell_NotifyIconW(NIM_ADD, &nid);

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = LoadProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hNote;
    wcex.hIcon = 0;
    wcex.hCursor = 0;
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = 0;
    wcex.lpszClassName = L"Note";
    wcex.hIconSm = 0;

    RegisterClassExW(&wcex);

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = 0;   

    return RegisterClassExW(&wcex);
}

void resize(HWND hwnd) {
    RECT rect;
    if (GetWindowRect(hwnd, &rect))
    {
        WORD width = rect.right - rect.left;
        WORD height = rect.bottom - rect.top;
        LPARAM lparam = MAKELPARAM(width, height);
        SendMessage(hwnd, WM_SIZE, 0, lparam);
    }
}

void LoadSubWindow(char* buff, Config cfg) {

    //UnregisterClass(L"Note", hNote);

    USES_CONVERSION;
    TCHAR* b = A2T(buff);

    RECT rc = cfg.rc;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        L"Note",
        L"Note",
        WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE,
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        GetDesktopWindow(),
        nullptr,
        hNote,
        nullptr);
    EnableWindow(hwnd, TRUE);
    SetLayeredWindowAttributes(hwnd, 0, (255 * 90) / 100, LWA_ALPHA);

    note n;
    n.window = hwnd;
    n.edit = CreateWindowExW(0, L"EDIT", 0,
        WS_VISIBLE | WS_CHILD  |
        ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL,
        0, 0, 0, 0,
        hwnd, (HMENU)ID_EDITCHILD, hEdit, NULL);

    n.x = rc.left; n.y = rc.top; n.width = rc.right - rc.left; n.height = rc.bottom - rc.top;

    n.locked = cfg.locked;
    EnableWindow(n.edit, n.locked);

    noteWindow.emplace_back(n);

    resize(hwnd);   

    SendMessageW(n.edit, WM_SETFONT, WPARAM(hFont), TRUE);
    SendMessageW(n.edit, WM_SETTEXT, 0, (LPARAM)b);
}

std::string createString(int i) {
    std::string s = "..\\Configs\\Note";
    //std::string s = "Configs\\Note";
    s += std::to_string(i);
    s += ".txt";
    return s;
}

std::string createConfigString(int i) {
    std::string s = "..\\Configs\\Note";
    //std::string s = "Configs\\Note";
    s += std::to_string(i);
    s += "Config.txt";
    return s;
}

void deleteOldFiles() {
    for (int i = 1; i <= 64; i++) {
        try {
            std::remove(createString(i).c_str());
            std::remove(createConfigString(i).c_str());
        }
        catch (const std::exception& e) {
            continue;
        }
    }
}

Config loadConfig( int i) {
    std::ifstream fin(createConfigString(i));
    int arr[5]; int index = 0;
    while (!fin.eof())
    {
        fin >> arr[index];
        index++;
    }
    fin.close();
    RECT n;
    n.left = arr[0];
    n.top = arr[1];
    n.right = arr[0] + arr[2];
    n.bottom = arr[1] + arr[3];
    return { n, arr[4]};
}

void loadFiles() {
    for (int i = 1; i <= 64; i++) {
        try {
            std::ifstream fin;
            int length;
            fin.open(createString(i));
            fin.seekg(0, std::ios::end);
            length = fin.tellg();
            fin.seekg(0, std::ios::beg);

            char* buffer = new char[length];
            ZeroMemory(buffer, length);
            fin.read(buffer, length);
            ZeroMemory(buffer + length, 1);

            Config cfg = loadConfig(i);

            LoadSubWindow(buffer, cfg);

            fin.close();

        }
        catch (const std::exception& e) {
            continue;
        }
    }
}

void saveConfig(note n, int i) {
    RECT rect;
    if (GetWindowRect(n.window, &rect))
    {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        int x = rect.left;
        int y = rect.top;

        std::ofstream fout(createConfigString(i));
        fout << x << " " << y << " " << width << " " << height << " " << n.locked;
        fout.close();
    }
}

void saveNewFiles() {
    int id = 0;
    for (std::vector<note>::iterator i = noteWindow.begin(); i < noteWindow.end(); i++) {
        HWND edit = (*i).edit;
        TCHAR buff[256];
        GetWindowText(edit, buff, 256);

        id++;
        std::string path = createString(id);
        std::ofstream fout(path);

        char c_buff[256];
        _wcstombs_l(c_buff, buff, wcslen(buff) + 1, LC_ALL);

        fout << c_buff;
        fout.close();

        saveConfig(*i,id);
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        hFont = CreateFont(18, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS, L"Arial");
        loadFiles();
        break;
    case APPWM_ICONNOTIFY:
    {
        switch (lParam)
        {
        case WM_LBUTTONUP:
            LoadSubWindow((char*)"Note", { {100,100,300,300},true });
            break;
        case WM_RBUTTONUP:
            
            deleteOldFiles();

            saveNewFiles();

            SendMessage(hWnd, WM_DESTROY, 0, 0);
            break;
        }
        return 0;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool isWnd(note val) {
    return (val.window == hwndForFind);
}

LRESULT CALLBACK LoadProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    note n;
    std::vector<note>::iterator it;
    if (message == WM_PAINT || message == WM_KEYDOWN || message == WM_SIZE || message == WM_DESTROY || message == WM_MOVE) {
        hwndForFind = hwnd;
        it = std::find_if(noteWindow.begin(), noteWindow.end(), isWnd);
        if (it < noteWindow.end()) { n = *it; }
    }
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        EndPaint(hwnd, &ps);
    }
    break;
    case WM_KEYDOWN:
    {
        if (wParam == 0x4C && GetKeyState(VK_SHIFT)) {

            n.locked = !n.locked;
            EnableWindow(n.edit, n.locked);

            RECT rc;
            GetWindowRect(n.window, &rc);
            n.x = rc.left;
            n.y = rc.top;
            n.width = rc.right - rc.left;
            n.height = rc.bottom - rc.top;

            noteWindow.erase(it);
            noteWindow.emplace_back(n);
        }
    }
    break;
    case WM_MOVE:
        if (!n.locked) {
            SetWindowPos(n.window, 0, n.x, n.y, n.width, n.height, SWP_NOREDRAW);  
        }
        break;
    case WM_SIZE:
        // Make the edit control the size of the window's client area. 
            MoveWindow(n.edit,
                0, 0,                  // starting x- and y-coordinates 
                LOWORD(lParam),        // width of client area 
                HIWORD(lParam),        // height of client area 
                TRUE);                  // repaint window                
        return 0;
    case WM_DESTROY:
        //PostQuitMessage(0);
        noteWindow.erase(it);
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return 0;
}