// -- focusback.c starts from here --

// To build release executable(mingw/gcc example):
//gcc focusback.c -o focusback.exe -lpsapi -mwindows -s

// To build debug executable(mingw/gcc example):
//gcc focusback.c -o focusback-debug.exe -lpsapi -DDEBUG

#define PROGNAME "FOCUSBACK"
#define PROGVERSION "0.8"

#include <windows.h>
#include <psapi.h>

#ifdef DEBUG
#include <stdio.h>
#endif

// Foreground: Currently foreground application. May point WiseScopeKatre background process.
// Previous: Previously foreground application. Saved every time when switched to forgound window which has a valid WindowText.
// This: This focusback application.

// Store some variables in global scope to reduce memory / stack allocation overhead.

// hForegroundWnd initialised in WinEventProc()
HANDLE hForegroundProcess = NULL;
DWORD  dwForegroundProcessID = 0;
DWORD  dwForegroundThreadID = 0;
char   sForegroundImageFileName[ MAX_PATH ] = {};
char   sForegroundWindowText[8] = {};
RECT   tagForegroundWindowRect;

HWND   hPreviousWnd = NULL;
DWORD  dwPreviousProcessID = 0;
DWORD  dwPreviousThreadID = 0;

DWORD  dwThisProcessID = 0;
DWORD  dwThisThreadID = 0;

// WinEventProc called every time when forground window changes.
void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event, HWND hForegroundWnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime){
	if (idObject == OBJID_WINDOW && idChild == CHILDID_SELF)
	{
		GetWindowText(hForegroundWnd, sForegroundWindowText, 7);
		#ifdef DEBUG
			printf("Got Focus: %s\n", sForegroundWindowText);
		#endif
		if(strlen(sForegroundWindowText) == 0){
			dwForegroundThreadID = GetWindowThreadProcessId(hForegroundWnd, &dwForegroundProcessID);
			GetWindowRect(hForegroundWnd, &tagForegroundWindowRect);
			hForegroundProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwForegroundProcessID);
			if(hForegroundProcess){
				GetProcessImageFileName(hForegroundProcess, sForegroundImageFileName, MAX_PATH);
				#ifdef DEBUG
					printf("Window has no name. So we check ImageFileName: %s\n", sForegroundImageFileName);
					printf("WindowPosition: %d, %d, %d, %d \n", tagForegroundWindowRect.left, tagForegroundWindowRect.top, tagForegroundWindowRect.right, tagForegroundWindowRect.bottom);
				#endif
				// We check three portions to determine WiseScope backgroud window:
				// 1: Windows has no name.
				// 2: ImageName contains Wise.Scope.Karte.exe
				// 3: Window positions outside of the screen.
				if (strstr(sForegroundImageFileName, "Wise.Scope.Karte.exe") && 
					(tagForegroundWindowRect.left < 0)&&(tagForegroundWindowRect.top < 0)&&(tagForegroundWindowRect.right < 0)&&(tagForegroundWindowRect.bottom < 0)){
					#ifdef DEBUG
						printf("WiseScope background window got focus.\n");
						printf("Hide hForegroundWnd: %d\n", hForegroundWnd);
					#endif
					ShowWindow(hForegroundWnd, SW_HIDE);
					Sleep(100);
					SetForegroundWindow(hPreviousWnd);
					if(GetForegroundWindow() != hPreviousWnd){
						Sleep(100);
						SetForegroundWindow(hPreviousWnd);
					}
				}
			}
		}else{
			// If foreground window has a valid WindowName, save hWnd in hPreviousWnd.
			hPreviousWnd = hForegroundWnd;
			#ifdef DEBUG
				printf("Saved current hWnd: %d\n", hPreviousWnd);
			#endif
		}
	}
}

int WINAPI WinMain(HINSTANCE hInstance , HINSTANCE hPrevInstance , PSTR lpCmdLine , int nCmdShow ) {
	
	#ifdef DEBUG
		printf(PROGNAME);
		printf(" v");
		printf(PROGVERSION);
		printf("\n");
	#endif
	
	HANDLE hMSP = CreateMutex(NULL, TRUE, PROGNAME);
	if(GetLastError() == ERROR_ALREADY_EXISTS){
		ReleaseMutex(hMSP);
		CloseHandle(hMSP);
		return 0;
	}
	
	// Store proc and thread id to reduce API overhead.
	dwThisProcessID = GetCurrentProcessId();
	dwThisThreadID = GetCurrentThreadId();
	
	hPreviousWnd = GetForegroundWindow();
	
	// VIQ of this thread allocated at this point.
	SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, 0, WinEventProc, 0, 0, WINEVENT_SKIPOWNTHREAD | WINEVENT_SKIPOWNPROCESS);
	
	MSG tagMsg;
	while (GetMessage(&tagMsg, NULL, 0, 0)){
		if (tagMsg.message == WM_ENDSESSION) PostQuitMessage(0);
		DispatchMessage(&tagMsg);
	}
	return tagMsg.wParam;
	
}

// -- focusback.c ends here --