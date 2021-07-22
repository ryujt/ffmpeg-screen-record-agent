#ifndef _RYULIB_DISK_H_
#define	_RYULIB_DISK_H_

#ifdef _WIN32
#include <windows.h>
#include <shlobj_core.h>
#include <shellapi.h>
#endif

#include <string>
#include <boost/filesystem.hpp>
#include <ryulib/strg.hpp>

using namespace std;

#ifdef _WIN32
static string getExecFilename()
{
    wchar_t filename[_MAX_PATH] = { 0 };
    GetModuleFileName(NULL, filename, _MAX_PATH);
    return WideCharToString(filename);
}

static string getExecPath()
{
    string result = getExecFilename();
    result = deleteRight(result, "\\");
    return setLastString(result, "\\");
}

static string getCommonAppDataPath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
        return setLastString(WideCharToString(szPath), "\\");
    }
    return "";
}

static string getMyDocumentsPath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
        return setLastString(WideCharToString(szPath), "\\");
    }
    return "";
}

static string getWindowsPath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_WINDOWS | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
        return setLastString(WideCharToString(szPath), "\\");
    }
    return "";
}

static string GetSystemPath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_SYSTEM | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
        return setLastString(WideCharToString(szPath), "\\");
    }
    return "";
}

static string getProgramsPath()
{
    TCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES | CSIDL_FLAG_CREATE, NULL, 0, szPath))) {
        return setLastString(WideCharToString(szPath), "\\");
    }
    return "";
}

static string getTempPath()
{
    TCHAR szPath[MAX_PATH];
    GetTempPath(MAX_PATH, szPath);
    return setLastString(WideCharToString(szPath), "\\");
}

static void executeFile(string filename, string params, string path)
{
    wchar_t* buf = StringToWideChar(filename);
    wstring filename_(buf);
    delete buf;

    buf = StringToWideChar(params);
    wstring params_(buf);
    delete buf;

    buf = StringToWideChar(path);
    wstring path_(buf);
    delete buf;

    ShellExecute(NULL, L"open", filename_.c_str(), params_.c_str(), path_.c_str(), SW_SHOWNORMAL);
}
#endif

static void forceDirectories(string path)
{
    boost::filesystem::path dir(path);
    boost::filesystem::create_directory(dir);
}

static void saveTextToFile(string filename, string text)
{
    ofstream out;
    out.open(filename);
    if (out.is_open()) {
        out << text;
        out.close();
    }
}

static string loadFileAsText(string filename)
{
    ifstream in(filename);
    if (in.is_open()) {
        string content((istreambuf_iterator<char>(in)), (istreambuf_iterator<char>()));
        in.close();
        return content;
    }
    else {
        return "";
    }
}

#endif // _RYULIB_DISK_H_ 