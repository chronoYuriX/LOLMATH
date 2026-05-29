#include <windows.h>
#include <wchar.h>
#include <stdio.h>

bool cmpheaderW(const wchar_t* a, const wchar_t* b) {
	size_t la = wcslen(a), lb = wcslen(b);
	return memcmp(a, b, ((la < lb) ? la : lb) * sizeof(wchar_t)) == 0;
}
bool cmpheaderA(const char* a, const char* b) {
	size_t la = strlen(a), lb = strlen(b);
	return memcmp(a, b, (la < lb) ? la : lb) == 0;
}
bool cmpheader(const void* a, const wchar_t* bW, const char* bA, bool* matchAflag = NULL) {
	bool matchA = cmpheaderA((char*)a, bA);
	if (matchAflag) if (*matchAflag == 0) *matchAflag = matchA;
	return cmpheaderW((wchar_t*)a, bW) || matchA;
}

bool ceksourceflW(const wchar_t* name) {
    size_t len = wcslen(name);
    const wchar_t ceklst[6][5] = { L".cpp", L".c++", L".cxx", L".cp", L".cc", L".c" };
    for (BYTE i = 0; i < 6; i++) {
    	BYTE taillen = wcslen(ceklst[i]);
    	if (len >= taillen) if (wcscmp(name + len - taillen, ceklst[i]) == 0) return 1;
	}
    return 0;
}

int main(int argc, char *argv[]) {
	bool ischar = 0, replace_space = 0;
	wchar_t pathW[MAX_PATH];
	for (int i = 0; i < argc; i++) {
	    if (cmpheader((void*)(argv[i]), L"-path", "-path", &ischar)) {
	    	if (ischar) {
	    		char* div = strchr(argv[i], '=');
	    		char pathA[MAX_PATH];
	    		strcpy(pathA, div + 1);
	    		mbstowcs(pathW, pathA, strlen(pathA) + 1);
			} else {
				wchar_t* div = wcschr((wchar_t*)(argv[i]), L'=');
        		wcscpy(pathW, div + 1);
			}
			int pathWlen = wcslen(pathW);
			for (int j = 0; j < pathWlen; j++) if (pathW[j] == L'/') pathW[j] = L'\\';
			if (replace_space) for (int j = 0; j < pathWlen; j++) if (pathW[j] == L'~') pathW[j] = L' ';
		} else if (cmpheader((void*)(argv[i]), L"-rep~", "-rep~", &ischar)) replace_space = 1;
	}
	int pathWlen = wcslen(pathW);
	bool noquotation = 1;
	for (int i = pathWlen; i >= 0; i--) {
		if (pathW[i] == L'\"') {
			noquotation = 0;
			pathW[i] = L'\\'; pathW[i + 1] = L'*'; pathW[i + 2] = L'\"'; pathW[i + 3] = '\0';
			break;
		}
	}
	if (noquotation) {
		pathW[pathWlen] = '\\'; pathW[pathWlen + 1] = L'*'; pathW[pathWlen + 2] = L'\0';
	}
	wprintf(L"Find: %ls\n", pathW);
    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(pathW, &ffd);
    if (hFind == INVALID_HANDLE_VALUE) {
        wprintf(L"\n\n\nERROR: %lu\n", GetLastError());
        return 1;
    }
	wchar_t* pathWf = pathW;
    if (noquotation) pathW[pathWlen + 1] = L'\0';
	else {
		for (int i = pathWlen; i >= 0; i--) {
			if (pathW[i] == L'\"') {
                pathW[i - 1] = L'\0';
			    break;
            }
	    }
		pathWf++;
	}
	wchar_t cmdln[16384] = L"g++ ";
    do {
        if (wcscmp(ffd.cFileName, L".") == 0 || wcscmp(ffd.cFileName, L"..") == 0) continue;
        if (ceksourceflW(ffd.cFileName)) {
        	wprintf(L"Detected: %ls\n", ffd.cFileName);
        	wcscat(cmdln, L"\""); wcscat(cmdln, pathWf); wcscat(cmdln, ffd.cFileName); wcscat(cmdln, L"\" ");
		}
    } while (FindNextFileW(hFind, &ffd));
    FindClose(hFind);
    wprintf(L"Command: %ls\n", cmdln);
	STARTUPINFOW siW = { 0 };
	PROCESS_INFORMATION pi = { 0 };
	bool success = CreateProcessW(NULL, cmdln, NULL, NULL, TRUE, 0, NULL, NULL, &siW, &pi);
	if (!success) {
		wprintf(L"CreateProcess failed: %lu\n", GetLastError());
		return 2;
	}
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exit_code = 0;
    GetExitCodeProcess(pi.hProcess, &exit_code);
    CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	wprintf(L"Done!");
	Sleep(1000);
    return exit_code;
}

