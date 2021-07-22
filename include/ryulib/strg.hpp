#ifndef _RYULIB_STRG_H_
#define	_RYULIB_STRG_H_

#ifdef _WIN32
#include <windows.h>
#include <atlstr.h>
#endif

#include <stdlib.h>
#include <string>
#include <algorithm>

using namespace std;

#ifdef _WIN32
static char* WideCharToChar(wchar_t* src)
{
    int len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
    char* result = new char[len];
    WideCharToMultiByte(CP_ACP, 0, src, -1, result, len, 0, 0);
    return result;
}

static string WideCharToString(wchar_t* src)
{
    char* temp = WideCharToChar(src);
    string result = string(temp);
    delete temp;
    return result;
}

static wchar_t* StringToWideChar(const string str)
{
    int len;
    int slength = str.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, 0, 0);
    wchar_t* result = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, str.c_str(), slength, result, len);
    return result;
}

static string UTF8ToAnsi(const string text)
{
    const char* src = text.c_str();
    int len = MultiByteToWideChar(CP_UTF8, 0, src, strlen(src) + 1, NULL, NULL);
    BSTR bstrWide = SysAllocStringLen(NULL, len);

    MultiByteToWideChar(CP_UTF8, 0, src, strlen(src) + 1, bstrWide, len);

    len = WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    char* ansi_str = new char[len];

    WideCharToMultiByte(CP_ACP, 0, bstrWide, -1, ansi_str, len, NULL, NULL);
    SysFreeString(bstrWide);

    string result(ansi_str);
    delete ansi_str;

    return result;
}

static string AnsiToUTF8(const string text)
{
    const char* src = text.c_str();
    int len = MultiByteToWideChar(CP_ACP, 0, src, strlen(src) + 1, NULL, NULL);
    BSTR bstrWide = SysAllocStringLen(NULL, len);

    MultiByteToWideChar(CP_ACP, 0, src, strlen(src) + 1, bstrWide, len);

    len = WideCharToMultiByte(CP_UTF8, 0, bstrWide, -1, NULL, 0, NULL, NULL);
    char* utf_str = new char[len];

    WideCharToMultiByte(CP_UTF8, 0, bstrWide, -1, utf_str, len, NULL, NULL);
    SysFreeString(bstrWide);

    string result(utf_str);
    delete utf_str;

    return result;
}
#endif

template<typename ... Args>
static string format_string(const string& format, Args ... args)
{
    size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1;
    char* buffer = new char[size];
    memset(buffer, 0, size);
    snprintf(buffer, size, format.c_str(), args ...);
    string result(buffer);
    delete(buffer);
    return result;
}

static string deleteLeft(const string str, const string border, bool ignore_case = false)
{
    string src = string(str);
    string dst = string(border);

    if (ignore_case) {
        transform(src.begin(), src.end(), src.begin(), ::tolower);
        transform(dst.begin(), dst.end(), dst.begin(), ::tolower);
    }

    int pos = src.find(dst);
    if (pos <= 0) {
        return str;
    }

    return str.substr(pos, str.length());
}

static string deleteLeftPlus(const string str, const string border, bool ignore_case = false)
{
    string src = string(str);
    string dst = string(border);

    if (ignore_case) {
        transform(src.begin(), src.end(), src.begin(), ::tolower);
        transform(dst.begin(), dst.end(), dst.begin(), ::tolower);
    }

    int pos = src.find(dst);
    if (pos < 0) {
        return str;
    }

    return str.substr(pos + border.length(), str.length());
}

static string deleteRight(const string str, const string border, bool ignore_case = false)
{
    string src = string(str);
    string dst = string(border);

    if (ignore_case) {
        transform(src.begin(), src.end(), src.begin(), ::tolower);
        transform(dst.begin(), dst.end(), dst.begin(), ::tolower);
    }

    for (int i = src.length() - 1; i >= 0; i--) {
        if (src.substr(i, border.length()) == border) {
            return str.substr(0, i + border.length());
        }
    }

    return str;
}

static string deleteRightPlus(const string str, const string border, bool ignore_case = false)
{
    string src = string(str);
    string dst = string(border);

    if (ignore_case) {
        transform(src.begin(), src.end(), src.begin(), ::tolower);
        transform(dst.begin(), dst.end(), dst.begin(), ::tolower);
    }

    for (int i = src.length() - 1; i >= 0; i--) {
        if (src.substr(i, border.length()) == border) {
            return str.substr(0, i);
        }
    }

    return str;
}

static string setLastString(string str, const string last)
{
    if (str.substr(str.length() - last.length(), last.length()) == last) {
        return str;
    } else {
        return str = str + last;
    }
}

static string getRandomString(int len)
{ 
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; 

    srand(time(NULL));
    char* temp = new char[len+1];
    for (int i = 0; i < len; ++i) { 
        temp[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    } 
    temp[len] = 0;
    return string(temp);
}

#endif // _RYULIB_STRG_H_ 