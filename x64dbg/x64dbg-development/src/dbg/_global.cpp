/**
\file _global.cpp
\brief Implements the global class.
*/

#include "_global.h"
#include <objbase.h>
#include <shlobj.h>
#include <psapi.h>
#include "DeviceNameResolver/DeviceNameResolver.h"

/**
\brief x64dbg library instance.
*/
HINSTANCE hInst;

/**
\brief Number of allocated buffers by emalloc(). This should be 0 when x64dbg ends.
*/
static int emalloc_count = 0;
#ifdef ENABLE_MEM_TRACE
/**
\brief Path for debugging, used to create an allocation trace file on emalloc() or efree(). Not used.
*/
static char alloctrace[MAX_PATH] = "";
static std::unordered_map<void*, int> alloctracemap;
static CRITICAL_SECTION criticalSection;
#endif

/**
\brief Allocates a new buffer.
\param size The size of the buffer to allocate (in bytes).
\param reason The reason for allocation (can be used for memory allocation tracking).
\return Always returns a valid pointer to the buffer you requested. Will quit the application on errors.
*/
void* emalloc(size_t size, const char* reason)
{
    ASSERT_NONZERO(size);
#ifdef ENABLE_MEM_TRACE
    unsigned char* a = (unsigned char*)GlobalAlloc(GMEM_FIXED, size + sizeof(void*));
#else
    unsigned char* a = (unsigned char*)GlobalAlloc(GMEM_FIXED, size);
#endif //ENABLE_MEM_TRACE
    if(!a)
    {
        wchar_t size[25];
        swprintf_s(size, L"%p bytes", size);
        MessageBoxW(0, L"Could not allocate memory (minidump will be created)", size, MB_ICONERROR);
        __debugbreak();
        ExitProcess(1);
    }
    emalloc_count++;
#ifdef ENABLE_MEM_TRACE
    EnterCriticalSection(&criticalSection);
    memset(a, 0, size + sizeof(void*));
    FILE* file = fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:  alloc:%p:%p:%s:%p\n", emalloc_count, a, _ReturnAddress(), reason, size);
    fclose(file);
    alloctracemap[_ReturnAddress()]++;
    *(void**)a = _ReturnAddress();
    LeaveCriticalSection(&criticalSection);
    return a + sizeof(void*);
#else
    memset(a, 0, size);
    return a;
#endif //ENABLE_MEM_TRACE
}

/**
\brief Reallocates a buffer allocated with emalloc().
\param [in] Pointer to memory previously allocated with emalloc(). When NULL a new buffer will be allocated by emalloc().
\param size The new memory size.
\param reason The reason for allocation (can be used for memory allocation tracking).
\return Always returns a valid pointer to the buffer you requested. Will quit the application on errors.
*/
void* erealloc(void* ptr, size_t size, const char* reason)
{
    ASSERT_NONZERO(size);

    // Free the memory if the pointer was set (as per documentation).
    if(ptr)
        efree(ptr, reason);

    return emalloc(size, reason);
}

/**
\brief Free memory previously allocated with emalloc().
\param [in] Pointer to the memory to free.
\param reason The reason for freeing, should be the same as the reason for allocating.
*/
void efree(void* ptr, const char* reason)
{
    emalloc_count--;
#ifdef ENABLE_MEM_TRACE
    EnterCriticalSection(&criticalSection);
    char* ptr2 = (char*)ptr - sizeof(void*);
    FILE* file = fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:   free:%p:%p:%s\n", emalloc_count, ptr, *(void**)ptr2, reason);
    fclose(file);
    if(alloctracemap.find(*(void**)ptr2) != alloctracemap.end())
    {
        if(--alloctracemap.at(*(void**)ptr2) < 0)
        {
            String str = StringUtils::sprintf("address %p, reason %s", *(void**)ptr2, reason);
            MessageBoxA(0, str.c_str(), "Freed memory more than once", MB_OK);
            __debugbreak();
        }
    }
    else
    {
        String str = StringUtils::sprintf("address %p, reason %s", *(void**)ptr2, reason);
        MessageBoxA(0, str.c_str(), "Trying to free const memory", MB_OK);
        __debugbreak();
    }
    LeaveCriticalSection(&criticalSection);
    GlobalFree(ptr2);
#else
    GlobalFree(ptr);
#endif //ENABLE_MEM_TRACE
}

void* json_malloc(size_t size)
{
#ifdef ENABLE_MEM_TRACE
    return emalloc(size, "json:ptr");
#else
    return emalloc(size);
#endif
}

void json_free(void* ptr)
{
#ifdef ENABLE_MEM_TRACE
    return efree(ptr, "json:ptr");
#else
    return efree(ptr);
#endif
}

/**
\brief Gets the number of memory leaks. This number is only valid in _dbg_dbgexitsignal().
\return The number of memory leaks.
*/
int memleaks()
{
#ifdef ENABLE_MEM_TRACE
    EnterCriticalSection(&criticalSection);
    auto leaked = false;
    for(auto & i : alloctracemap)
    {
        if(i.second != 0)
        {
            String str = StringUtils::sprintf("memory leak at %p : count %d", i.first, i.second);
            MessageBoxA(0, str.c_str(), "memory leak", MB_OK);
            leaked = true;
        }
    }
    if(leaked)
        __debugbreak();
    LeaveCriticalSection(&criticalSection);
#endif
    return emalloc_count;
}

#ifdef ENABLE_MEM_TRACE
/**
\brief Sets the path for the allocation trace file.
\param file UTF-8 filepath.
*/
void setalloctrace(const char* file)
{
    InitializeCriticalSection(&criticalSection);
    strcpy_s(alloctrace, file);
}
#endif //ENABLE_MEM_TRACE

/**
\brief Compares two strings without case-sensitivity.
\param a The first string.
\param b The second string.
\return true if the strings are equal (case-insensitive).
*/
bool scmp(const char* a, const char* b)
{
    if(!a || !b)
        return false;
    return !_stricmp(a, b);
}

/**
\brief Formats a string to hexadecimal format (removes all non-hex characters).
\param [in,out] String to format.
*/
void formathex(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formathex:new_string");
    for(int i = 0, j = 0; i < len; i++)
        if(isxdigit(string[i]))
            j += sprintf(new_string() + j, "%c", string[i]);
    strcpy_s(string, len + 1, new_string());
}

/**
\brief Formats a string to decimal format (removed all non-numeric characters).
\param [in,out] String to format.
*/
void formatdec(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formatdec:new_string");
    for(int i = 0, j = 0; i < len; i++)
        if(isdigit(string[i]))
            j += sprintf(new_string() + j, "%c", string[i]);
    strcpy_s(string, len + 1, new_string());
}

/**
\brief Queries if a given file exists.
\param file Path to the file to check (UTF-8).
\return true if the file exists on the hard drive.
*/
bool FileExists(const char* file)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(file).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

/**
\brief Queries if a given directory exists.
\param dir Path to the directory to check (UTF-8).
\return true if the directory exists.
*/
bool DirExists(const char* dir)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(dir).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

/**
\brief Gets file path from a file handle.
\param hFile File handle to get the path from.
\param [in,out] szFileName Buffer of size MAX_PATH.
\return true if it succeeds, false if it fails.
*/
bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    if(!hFile)
        return false;
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!PathFromFileHandleW(hFile, wszFileName, _countof(wszFileName)))
        return false;
    strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return true;
}

bool GetFileNameFromProcessHandle(HANDLE hProcess, char* szFileName)
{
    wchar_t wszDosFileName[MAX_PATH] = L"";
    wchar_t wszFileName[MAX_PATH] = L"";
    auto result = false;
    if(GetProcessImageFileNameW(hProcess, wszDosFileName, _countof(wszDosFileName)))
    {
        if(!DevicePathToPathW(wszDosFileName, wszFileName, _countof(wszFileName)))
            result = !!GetModuleFileNameExW(hProcess, 0, wszFileName, _countof(wszFileName));
        else
            result = true;
    }
    else
        result = !!GetModuleFileNameExW(hProcess, 0, wszFileName, _countof(wszFileName));
    if(result)
        strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return result;
}

bool GetFileNameFromModuleHandle(HANDLE hProcess, HMODULE hModule, char* szFileName)
{
    wchar_t wszDosFileName[MAX_PATH] = L"";
    wchar_t wszFileName[MAX_PATH] = L"";
    auto result = false;
    if(GetMappedFileNameW(hProcess, hModule, wszDosFileName, _countof(wszDosFileName)))
    {
        if(!DevicePathToPathW(wszDosFileName, wszFileName, _countof(wszFileName)))
            result = !!GetModuleFileNameExW(hProcess, hModule, wszFileName, _countof(wszFileName));
        else
            result = true;
    }
    else
        result = !!GetModuleFileNameExW(hProcess, hModule, wszFileName, _countof(wszFileName));
    if(result)
        strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return result;
}

/**
\brief Get a boolean setting from the configuration file.
\param section The section of the setting (UTF-8).
\param name The name of the setting (UTF-8).
\return true if the setting was set and equals to true, otherwise returns false.
*/
bool settingboolget(const char* section, const char* name)
{
    duint setting;
    if(!BridgeSettingGetUint(section, name, &setting))
        return false;
    if(setting)
        return true;
    return false;
}

/**
\brief Gets file architecture.
\param szFileName UTF-8 encoded file path.
\return The file architecture (::arch).
*/
arch GetFileArchitecture(const char* szFileName)
{
    arch retval = notfound;
    HANDLE hFile = CreateFileW(StringUtils::Utf8ToUtf16(szFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        IMAGE_DOS_HEADER idh;
        DWORD read = 0;
        if(ReadFile(hFile, &idh, sizeof(idh), &read, nullptr))
        {
            if(idh.e_magic == IMAGE_DOS_SIGNATURE)
            {
                IMAGE_NT_HEADERS inth;
                memset(&inth, 0, sizeof(inth));
                PIMAGE_NT_HEADERS pnth = nullptr;
                if(SetFilePointer(hFile, idh.e_lfanew, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER)
                {
                    if(ReadFile(hFile, &inth, sizeof(inth), &read, nullptr))
                        pnth = &inth;
                    else if(ReadFile(hFile, &inth, sizeof(DWORD) + sizeof(WORD), &read, nullptr))
                        pnth = &inth;
                }
                if(pnth && pnth->Signature == IMAGE_NT_SIGNATURE)
                {
                    if(pnth->OptionalHeader.DataDirectory[15].VirtualAddress != 0 && pnth->OptionalHeader.DataDirectory[15].Size != 0 && (pnth->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0)
                        retval = dotnet;
                    else if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_I386)
                        retval = x32;
                    else if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
                        retval = x64;
                }
            }
        }
        CloseHandle(hFile);
    }
    return retval;
}

/**
\brief Query if x64dbg is running in Wow64 mode.
\return true if running in Wow64, false otherwise.
*/
bool IsWow64()
{
    BOOL bIsWow64Process = FALSE;
    //x64dbg supports WinXP SP3 and later only, so ignore the GetProcAddress crap :D
    IsWow64Process(GetCurrentProcess(), &bIsWow64Process);
    return !!bIsWow64Process;
}

//Taken from: http://www.cplusplus.com/forum/windows/64088/
bool ResolveShortcut(HWND hwnd, const wchar_t* szShortcutPath, char* szResolvedPath, size_t nSize)
{
    if(szResolvedPath == NULL)
        return SUCCEEDED(E_INVALIDARG);

    //Initialize COM stuff
    if(!SUCCEEDED(CoInitialize(NULL)))
        return false;

    //Get a pointer to the IShellLink interface.
    IShellLink* psl = NULL;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if(SUCCEEDED(hres))
    {
        //Get a pointer to the IPersistFile interface.
        IPersistFile* ppf = NULL;
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if(SUCCEEDED(hres))
        {
            //Load the shortcut.
            hres = ppf->Load(szShortcutPath, STGM_READ);

            if(SUCCEEDED(hres))
            {
                //Resolve the link.
                hres = psl->Resolve(hwnd, 0);

                if(SUCCEEDED(hres))
                {
                    //Get the path to the link target.
                    char szGotPath[MAX_PATH] = {0};
                    hres = psl->GetPath(szGotPath, _countof(szGotPath), NULL, SLGP_SHORTPATH);

                    if(SUCCEEDED(hres))
                    {
                        strcpy_s(szResolvedPath, nSize, szGotPath);
                    }
                }
            }

            //Release the pointer to the IPersistFile interface.
            ppf->Release();
        }

        //Release the pointer to the IShellLink interface.
        psl->Release();
    }

    //Uninitialize COM stuff
    CoUninitialize();
    return SUCCEEDED(hres);
}

void WaitForThreadTermination(HANDLE hThread, DWORD timeout)
{
    WaitForSingleObject(hThread, timeout);
    CloseHandle(hThread);
}

void WaitForMultipleThreadsTermination(const HANDLE* hThread, int count, DWORD timeout)
{
    WaitForMultipleObjects(count, hThread, TRUE, timeout);
    for(int i = 0; i < count; i++)
        CloseHandle(hThread[i]);
}
