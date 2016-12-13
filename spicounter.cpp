/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "stdafx.h"
#include "spicounter.h"
#include <stdio.h>
#include <assert.h>
#include <ShellAPI.h>

#define MAX_LOADSTRING 100

#define SPICOUNTERMODE_COUNTUP		0
#define SPICOUNTERMODE_COUNTDOWN	1
#define SPICOUNTERMODE_CLOCK		2

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

int global_x=0;
int global_y=0;
int global_spicountermode=SPICOUNTERMODE_CLOCK;
HFONT global_hFont;
int global_fontheight=480;
int global_fontwidth=-1; //will be computed within WM_PAINT handler

int global_starttime_sec=-1; //user specified, -1 for not specified
int global_endtime_sec=-1; //user specified, -1 for not specified

int global_timetodisplay_sec; //calculated
DWORD global_startstamp_ms;
DWORD global_nowstamp_ms;

char charbuffer[1024]={""};
char charbuffer_prev[1024]={""};

BYTE global_alpha=220;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);


PCHAR*
    CommandLineToArgvA(
        PCHAR CmdLine,
        int* _argc
        )
    {
        PCHAR* argv;
        PCHAR  _argv;
        ULONG   len;
        ULONG   argc;
        CHAR   a;
        ULONG   i, j;

        BOOLEAN  in_QM;
        BOOLEAN  in_TEXT;
        BOOLEAN  in_SPACE;

        len = strlen(CmdLine);
        i = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

        argv = (PCHAR*)GlobalAlloc(GMEM_FIXED,
            i + (len+2)*sizeof(CHAR));

        _argv = (PCHAR)(((PUCHAR)argv)+i);

        argc = 0;
        argv[argc] = _argv;
        in_QM = FALSE;
        in_TEXT = FALSE;
        in_SPACE = TRUE;
        i = 0;
        j = 0;

        while( a = CmdLine[i] ) {
            if(in_QM) {
                if(a == '\"') {
                    in_QM = FALSE;
                } else {
                    _argv[j] = a;
                    j++;
                }
            } else {
                switch(a) {
                case '\"':
                    in_QM = TRUE;
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    in_SPACE = FALSE;
                    break;
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                    if(in_TEXT) {
                        _argv[j] = '\0';
                        j++;
                    }
                    in_TEXT = FALSE;
                    in_SPACE = TRUE;
                    break;
                default:
                    in_TEXT = TRUE;
                    if(in_SPACE) {
                        argv[argc] = _argv+j;
                        argc++;
                    }
                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
                }
            }
            i++;
        }
        _argv[j] = '\0';
        argv[argc] = NULL;

        (*_argc) = argc;
        return argv;
    }

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);


	global_startstamp_ms = GetTickCount();

	LPSTR *szArgList;
	int nArgs;
	szArgList = CommandLineToArgvA(GetCommandLineA(), &nArgs);
	if( NULL == szArgList )
	{
		//wprintf(L"CommandLineToArgvW failed\n");
		return FALSE;
	}
	if(nArgs>1)
	{
		if(strstr(szArgList[1], "COUNTUP")) 
		{
			global_spicountermode = SPICOUNTERMODE_COUNTUP;
		}
		else if(strstr(szArgList[1], "COUNTDOWN"))
		{
			global_spicountermode = SPICOUNTERMODE_COUNTDOWN;
		}
		else if(strstr(szArgList[1], "CLOCK"))
		{
			global_spicountermode = SPICOUNTERMODE_CLOCK;
		}
		else 
		{
			assert(false);
			global_spicountermode = SPICOUNTERMODE_CLOCK;
		}
	}
	if(nArgs>2)
	{
		global_starttime_sec = atoi(szArgList[2]);
	}
	if(nArgs>3)
	{
		global_endtime_sec = atoi(szArgList[3]);
	}
	if(nArgs>4)
	{
		global_x = atoi(szArgList[4]);
	}
	if(nArgs>5)
	{
		global_y = atoi(szArgList[5]);
	}
	if(nArgs>6)
	{
		global_fontheight = atoi(szArgList[6]);
	}
	LocalFree(szArgList);

	if(global_starttime_sec>-1)
	{
		global_timetodisplay_sec = global_starttime_sec;
	}
	else
	{
		global_timetodisplay_sec = 0;
	}


	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_CPPMFC_TRANSPARENTTXT, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CPPMFC_TRANSPARENTTXT));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CPPMFC_TRANSPARENTTXT));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= L""; //MAKEINTRESOURCE(IDC_CPPMFC_TRANSPARENTTXT);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	/*
	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
	*/
	DWORD Flags1 = WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOPMOST | WS_EX_TRANSPARENT;
	DWORD Flags2 = WS_POPUP;

	//hWnd = CreateWindowEx(Flags1, szWindowClass, szTitle, Flags2, global_x, global_y, 1920, 1200, 0, 0, hInstance, 0);
	hWnd = CreateWindowEx(Flags1, szWindowClass, szTitle, Flags2, 0, 0, 100, 100, 0, 0, hInstance, 0);
	if (!hWnd)
	{
		return FALSE;
	}
	//global_hFont=CreateFontW(global_fontheight,0,0,0,FW_NORMAL,0,0,0,0,0,0,2,0,L"SYSTEM_FIXED_FONT");
	global_hFont=CreateFontW(global_fontheight,0,0,0,FW_BOLD,0,0,0,0,0,0,2,0,L"Segoe Script");
	
	SIZE mySIZE;
	HDC myHDC = GetDC(hWnd);
	HGDIOBJ prevHGDIOBJ = SelectObject(myHDC, global_hFont);
	GetTextExtentPoint32A(myHDC, "88:88:88", strlen("88:88:88"), &mySIZE);
	SetWindowPos(hWnd, NULL, global_x, global_y, mySIZE.cx, mySIZE.cy, SWP_NOZORDER);
	SelectObject(myHDC, prevHGDIOBJ);
	ReleaseDC(hWnd, myHDC);
	/*
	HRGN GGG = CreateRectRgn(0, 0, 1920, 1200);
	InvertRgn(GetDC(hWnd), GGG);
	SetWindowRgn(hWnd, GGG, false);
	*/
	COLORREF RRR = RGB(255, 0, 255);
	SetLayeredWindowAttributes(hWnd, RRR, (BYTE)0, LWA_COLORKEY);

	/*
	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);
	*/
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	/*
	DeleteObject(GGG);
	*/
	SetTimer(hWnd, 1, 1000, NULL);
	return TRUE;
}

void DrawTextXOR(HDC hdc, const char* charbuffer, int charbufferlength)
{

		//spi, begin
		HDC myMemHDC = CreateCompatibleDC(hdc);
		HFONT hOldFont_memhdc=(HFONT)SelectObject(myMemHDC,global_hFont);
		SIZE mySIZE;
		GetTextExtentPoint32A(myMemHDC, charbuffer, charbufferlength, &mySIZE);

		HBITMAP myHBITMAP = CreateCompatibleBitmap(hdc, mySIZE.cx, mySIZE.cy);
		HGDIOBJ prevHBITMAP = SelectObject(myMemHDC, myHBITMAP);
		//COLORREF crOldBkColor = SetBkColor(myMemHDC, RGB(0xFF, 0xFF, 0xFF));
		//COLORREF crOldBkColor = SetBkColor(myMemHDC, RGB(0x00, 0x00, 0xFF));
		//COLORREF crOldBkColor = SetBkColor(myMemHDC, RGB(0x00, 0x00, 0x00));
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xFF, 0x00, 0x00));
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xFF, 0x00, 0xFF)); //white
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xFF, 0xFF, 0xFF)); //black
		COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0x00, 0xFF, 0xFF)); //blue
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xFF, 0xFF, 0x00)); //red
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0x00, 0xFF, 0x00)); //black
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xFF, 0x00, 0x00)); //yellow
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0xA0, 0x00, 0x20)); //green lime
		//COLORREF crOldTextColor_memhdc = SetTextColor(myMemHDC, RGB(0x00, 0x00, 0x00)); //green

		//int nOldDrawingMode_memhdc = SetROP2(myMemHDC, R2_NOTXORPEN); //XOR mode, always have to erase what's drawn.
		//int iOldBkMode_memhdc = SetBkMode(myMemHDC, TRANSPARENT);
		//HFONT hOldFont_memhdc=(HFONT)SelectObject(myMemHDC,global_hFont);
		//TextOutA(myMemHDC, 1, 1, "test string", 11);
		TextOutA(myMemHDC, 0, 0, charbuffer, charbufferlength);
		strcpy(charbuffer_prev, charbuffer);
		//Rectangle(myMemHDC, 0, 0, 1000, 800);
		//BitBlt(hdc, 0, 0, 1000, 800, myMemHDC, 0, 0, SRCCOPY); 
		BitBlt(hdc, 0, 0, mySIZE.cx, mySIZE.cy, myMemHDC, 0, 0, 0x00990066); //XOR mode, always have to erase what's drawn.
		//BitBlt(hdc, global_x, global_y, mySIZE.cx, mySIZE.cy, myMemHDC, 0, 0, 0x00990066); //XOR mode, always have to erase what's drawn.
		SelectObject(myMemHDC, prevHBITMAP);
		DeleteDC(myMemHDC);
		DeleteObject(myHBITMAP);
		//DeleteDC(myMemHDC2);
		
		//spi, end
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
		
    case WM_ERASEBKGND:
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			FillRect((HDC)wParam, &rect, CreateSolidBrush(RGB(255, 0, 255))); //keying color
			//FillRect((HDC)wParam, &rect, CreateSolidBrush(RGB(0, 0, 0)));
		}
		break;
		
	case WM_TIMER:
		{
			InvalidateRect(hWnd, NULL, FALSE);
			global_nowstamp_ms = GetTickCount();

			if(global_spicountermode==SPICOUNTERMODE_CLOCK)
			{
			}
			else if(global_spicountermode==SPICOUNTERMODE_COUNTUP)
			{
				//1) calculate time to display
				if(global_starttime_sec<0) global_starttime_sec=0;
				int elapsed_sec = (global_nowstamp_ms-global_startstamp_ms)/1000;
				global_timetodisplay_sec = global_starttime_sec + elapsed_sec;

				//2) check for end condition
				if(global_endtime_sec>-1 && ((global_endtime_sec-global_starttime_sec)-elapsed_sec)<1)
				{
					int nShowCmd = false;
					ShellExecuteA(NULL, "open", "c:\\temp\\batch.bat", "", NULL, nShowCmd);
					PostMessage(hWnd, WM_DESTROY, 0, 0);
				}
			}
			else if(global_spicountermode==SPICOUNTERMODE_COUNTDOWN)
			{
				//calculate time to display
				if(global_starttime_sec<0) global_starttime_sec=0;
				int elapsed_sec = (global_nowstamp_ms-global_startstamp_ms)/1000;
				global_timetodisplay_sec = global_starttime_sec - elapsed_sec;

				//2) check for end condition
				if(global_endtime_sec>-1 && ((global_starttime_sec-global_endtime_sec)-elapsed_sec)<1)
				{
					int nShowCmd = false;
					ShellExecuteA(NULL, "open", "c:\\temp\\batch.bat", "", NULL, nShowCmd);
					PostMessage(hWnd, WM_DESTROY, 0, 0);
				}
			}
			else
			{
				assert(false);
			}
		}
		break;
	case WM_PAINT:
		{
			hdc = BeginPaint(hWnd, &ps);

			//SaveDC(hdc);
			//int nOldDrawingMode = SetROP2(hdc, R2_NOTXORPEN); //XOR mode, always have to erase what's drawn.

			//int iOldBkMode = SetBkMode(hdc, TRANSPARENT);
			COLORREF crOldTextColor = SetTextColor(hdc, RGB(0xFF, 0x00, 0x00));
			HGDIOBJ hOldFont=(HFONT)SelectObject(hdc,global_hFont);
			
			if(global_spicountermode==SPICOUNTERMODE_CLOCK)
			{
				SYSTEMTIME st;
				//GetSystemTime(&st);
				GetLocalTime(&st);
				//sprintf(charbuffer, "%02d:%02d:%02d", st.wHour, st.wMinute, st.wSecond);
				sprintf(charbuffer, "%02d.%02d:%02d", st.wHour, st.wMinute, st.wSecond);
			}
			else if(global_spicountermode==SPICOUNTERMODE_COUNTUP)
			{
				int hh = global_timetodisplay_sec / 3600;
				int mm = (global_timetodisplay_sec % 3600) / 60;
				int ss = global_timetodisplay_sec % 60;
				//sprintf(charbuffer, "%02d:%02d:%02d", hh, mm, ss);
				sprintf(charbuffer, "%02d.%02d:%02d", hh, mm, ss);
			}
			else if(global_spicountermode==SPICOUNTERMODE_COUNTDOWN)
			{
				int hh = global_timetodisplay_sec / 3600;
				int mm = (global_timetodisplay_sec % 3600) / 60;
				int ss = global_timetodisplay_sec % 60;
				//sprintf(charbuffer, "%02d:%02d:%02d", hh, mm, ss);
				sprintf(charbuffer, "%02d.%02d:%02d", hh, mm, ss);
			}
			else
			{
				//ERROR
				assert(false);
				int hh=99;
				int mm=99;
				int ss=99;
				//sprintf(charbuffer, "%02d:%02d:%02d", hh, mm, ss);
				sprintf(charbuffer, "%02d.%02d:%02d", hh, mm, ss);
			}
			//TextOutA(hdc, 50, 50, charbuffer, charbufferlength);
			int charbufferlength = strlen(charbuffer);
			if(strcmp(charbuffer_prev, "")) DrawTextXOR(hdc, charbuffer_prev, strlen(charbuffer));
			DrawTextXOR(hdc, charbuffer, charbufferlength);
			

			SetTextColor(hdc, crOldTextColor);
			//SetBkMode(hdc, iOldBkMode);
			SelectObject(hdc,hOldFont);

			//SetROP2(hdc, nOldDrawingMode); 
			//RestoreDC(hdc, -1);

			EndPaint(hWnd, &ps);
		}
		break;
	case WM_DESTROY:
		DeleteObject(global_hFont);
		KillTimer(hWnd, 1);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
