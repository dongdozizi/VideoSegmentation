//*****************************************************************************
//
// Main.cpp : Defines the entry point for the application.
// Creates a white RGB image with a black line and displays it.
// Two images are displayed on the screen.
// Left Pane: Input Image, Right Pane: Modified Image
//
// Author - Parag Havaldar
// Code used by students as starter code to display and modify images
//
//*****************************************************************************
#define _FILE_OFFSET_BITS 64

// Include class files
#include "Image.h"
#include "VideoGPU.h"
#include "Video.h"
#include <iostream>
#include <vector>
#include <string>
#include <windows.h>
#include <Mmsystem.h>
#pragma comment(lib, "winmm.lib")

#define MAX_LOADSTRING 100
// Copy from MSDN   
#pragma comment(linker,"\"/manifestdependency:type='win32' "\
						"name='Microsoft.Windows.Common-Controls' "\
						"version='6.0.0.0' processorArchitecture='*' "\
						"publicKeyToken='6595b64144ccf1df' language='*'\"")
HINSTANCE		hInst;							// Current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// The window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

std::vector<MyImage> video;
const char* videoPath;
const int width = 960;
const int height = 540;
const int subUnits = 1;

int inImageType = 0;
int showMotionType = 0;
int segmentType = 0;
MyImage	inImage;
MyVideo	inVideo;						// video objects
bool isPlaying = true;             // Play/Pause state
int currentFrame = 0;              // Current frame index
int totalFrames;    // Total number of frames in the video

int audioLength;

#define ID_PLAY_PAUSE  1001
#define ID_NEXT_FRAME  1002
#define ID_PREV_FRAME  1003
#define ID_SHOW_ORIGN  1004
#define ID_SHOW_SGMEN  1005
#define ID_SHOW_MOVEC  1006

HWND hProgressBar;

// Main entry point for a windows application
int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	MSG msg;
	HACCEL hAccelTable;

	// Create a separate console window to display output to stdout
	AllocConsole();
	freopen("CONOUT$", "w", stdout);

	int argc;
	wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	// Store arguments in a vector of strings
	std::vector<std::string> arguments;

	for (int i = 0; i < argc; i++) {
		// Convert wchar_t* to std::string
		int len = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
		std::string arg(len, '\0');
		WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, &arg[0], len, nullptr, nullptr);
		arguments.push_back(arg);
	}

	// Free memory allocated for argv
	LocalFree(argv);
	// Print out the arguments
	for (size_t i = 0; i < arguments.size(); ++i) {
		std::cout << "Argument[" << i << "]: " << arguments[i] << std::endl;
	}
	int n1 = 0;
	int n2 = 0;
	std::string Filename;
	Filename = arguments[1];
	n1 = std::stoi(arguments[2]);
	n2 = std::stoi(arguments[3]);

	std::cout << "Filename: " << Filename << " " << "n1: " << n1 << " " << "n2: " << n2 << "\n";

	inVideo.setHeight(height);
	inVideo.setWidth(width);
	inVideo.setN1(n1);
	inVideo.setN2(n2);
	inVideo.setVideoPath(Filename);
	inVideo.ReadVideo();
	inVideo.CalcMotion();
	inVideo.writeCoeffToCmp("output.cmp");
	video = inVideo.getVideoData();
//	playAudio(arguments[2].c_str());

	totalFrames = video.size();
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_IMAGE, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, (LPCTSTR)IDC_IMAGE);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage is only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_IMAGE);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = (LPCSTR)IDC_IMAGE;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}


//
//   FUNCTION: InitInstance(HANDLE, int)
//
//   PURPOSE: Saves instance handle and creates the main window
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

	hWnd = CreateWindow(
		szWindowClass,          // Window class name
		szTitle,                // Window title
		WS_OVERLAPPEDWINDOW,    // Window style
		CW_USEDEFAULT,          // Initial x-coordinate of the window (use default value)
		CW_USEDEFAULT,          // Initial y-coordinate of the window (use default value)
		1280,                   // Window width (in pixels)
		960,                    // Window height (in pixels)
		NULL,                   // Parent window handle
		NULL,                   // Menu handle
		hInstance,              // Instance handle
		NULL                    // Additional parameters
	);

	if (!hWnd)
	{
		return FALSE;
	}
	HWND hPlayPauseButton, hNextFrameButton, hPrevFrameButton;
	HFONT hFont = CreateFont(
		20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");



	// Create the "Previous Frame" button
	hPrevFrameButton = CreateWindowEx(
		0, "BUTTON", "Prev Frame",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		10, 800, 100, 30,
		hWnd, (HMENU)ID_PREV_FRAME, hInst, NULL);
	SendMessage(hPrevFrameButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create the "Play/Pause" button
	hPlayPauseButton = CreateWindowEx(
		0, "BUTTON", "Play/Pause",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		120, 800, 100, 30,
		hWnd, (HMENU)ID_PLAY_PAUSE, hInst, NULL);
	SendMessage(hPlayPauseButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create the "Next Frame" button
	hNextFrameButton = CreateWindowEx(
		0, "BUTTON", "Next Frame",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		230, 800, 100, 30,
		hWnd, (HMENU)ID_NEXT_FRAME, hInst, NULL);
	SendMessage(hNextFrameButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create the "Show Original" button
	hNextFrameButton = CreateWindowEx(
		0, "BUTTON", "Show Original",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		340, 800, 100, 30,
		hWnd, (HMENU)ID_SHOW_ORIGN, hInst, NULL);
	SendMessage(hNextFrameButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create the "Show Segmentation" button
	hNextFrameButton = CreateWindowEx(
		0, "BUTTON", "Show Segmentation",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		450, 800, 100, 30,
		hWnd, (HMENU)ID_SHOW_SGMEN, hInst, NULL);
	SendMessage(hNextFrameButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create the "Show Motion Vector" button
	hNextFrameButton = CreateWindowEx(
		0, "BUTTON", "Show Motion Vec",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
		560, 800, 100, 30,
		hWnd, (HMENU)ID_SHOW_MOVEC, hInst, NULL);
	SendMessage(hNextFrameButton, WM_SETFONT, (WPARAM)hFont, TRUE);
	// Create a progress bar (slider)
	hProgressBar = CreateWindowEx(
		0, TRACKBAR_CLASS, NULL,
		WS_CHILD | WS_VISIBLE | TBS_HORZ,  // Horizontal slider
		0, 750, 960, 30,                  // Position and size
		hWnd, NULL, hInst, NULL);

	// Set the range of the slider (assuming the total number of frames is `totalFrames`)
	SendMessage(hProgressBar, TBM_SETRANGE, TRUE, MAKELPARAM(0, totalFrames * subUnits - 1));
	//SendMessage(hProgressBar, TBM_SETTICFREQ, 1, 0);
	//SendMessage(hProgressBar, TBM_SETPAGESIZE, 0, 1); // Set the step size to 1
	SendMessage(hProgressBar, TBM_SETPOS, TRUE, 0); // Initial position is frame 0

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

//
//  FUNCTION: WndProc(HWND, unsigned, WORD, LONG)
//
//  PURPOSE:  Processes messages for the main window
//
//  WM_COMMAND	- Processes application menu
//  WM_PAINT	- Paints the main window
//  WM_DESTROY	- Posts a quit message and returns
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// TO DO: Part useful to render video frames; you may place your own code here
	// You are free to modify the following code to display the video

	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;
	TCHAR szHello[MAX_LOADSTRING];
	LoadString(hInst, IDS_HELLO, szHello, MAX_LOADSTRING);
	RECT rt;
	GetClientRect(hWnd, &rt);

	switch (message)
	{
	case WM_CREATE:
	{
		SetTimer(hWnd, 1, 33, NULL); // Set a timer for 33ms (approximately 30 FPS)
		return 0;
	}
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, (LPCTSTR)IDD_ABOUTBOX, hWnd, (DLGPROC)About);
			break;
		case ID_MODIFY_IMAGE:
			InvalidateRect(hWnd, &rt, false);
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		case ID_PLAY_PAUSE:
			isPlaying = !isPlaying; // Toggle play/pause state
			if (isPlaying) {
				SetTimer(hWnd, 1, 33, NULL); // Resume the timer
			}
			else {
				KillTimer(hWnd, 1); // Pause the timer
			}
			InvalidateRect(hWnd, NULL, FALSE); // Redraw the window to update state
			break;

		case ID_NEXT_FRAME:
			isPlaying = false; // Pause playback
			if (isPlaying) {
				SetTimer(hWnd, 1, 33, NULL); // Resume the timer
			}
			else {
				KillTimer(hWnd, 1); // Pause the timer
			}
			currentFrame = (currentFrame + 1) % totalFrames; // Switch to the next frame
			// Update the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case ID_PREV_FRAME:
			isPlaying = false; // Pause playback
			if (isPlaying) {
				SetTimer(hWnd, 1, 33, NULL); // Resume the timer
			}
			else {
				KillTimer(hWnd, 1); // Pause the timer
			}
			currentFrame = (currentFrame - 1 + totalFrames) % totalFrames; // Switch to the previous frame
			// Update the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame);
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case ID_SHOW_ORIGN:
			inImageType = 0;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case ID_SHOW_SGMEN:
			segmentType = 1 - segmentType;
			printf("Segment Type: %d\n",segmentType);
			inImageType = 1;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		case ID_SHOW_MOVEC:
			showMotionType = 1 - showMotionType;
			printf("Show Motion Type: %d\n", showMotionType);
			inImageType = 2;
			InvalidateRect(hWnd, NULL, FALSE);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case WM_PAINT:
	{
		hdc = BeginPaint(hWnd, &ps);

		// Display text
		char text[1000];
		strcpy(text, "   \n");
		DrawText(hdc, text, strlen(text), &rt, DT_LEFT);
		strcpy(text, "\n   \n");
		DrawText(hdc, text, strlen(text), &rt, DT_LEFT);

		// Configure Bitmap information
		BITMAPINFO bmi;
		memset(&bmi, 0, sizeof(bmi));
		inImage = video[currentFrame]; // Use the current frame
		switch (inImageType) {
		case 1: {
			if (segmentType == 1) inImage.Segmentation2();
			else inImage.Segmentation();
			inImage.ShowSegmentation(); 
			break;
		}
		case 2: {
			if (showMotionType == 1) inImage.ShowMotionPlot();
			else inImage.ShowMotionVec(); 
			break;
		}
		}

		bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
		bmi.bmiHeader.biWidth = inImage.getWidth();
		bmi.bmiHeader.biHeight = -inImage.getHeight(); // Negative height indicates top-down
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 24;
		bmi.bmiHeader.biCompression = BI_RGB;
		bmi.bmiHeader.biSizeImage = inImage.getWidth() * inImage.getHeight();

		// Draw the current frame
		SetDIBitsToDevice(hdc,
			0, 100, inImage.getWidth(), inImage.getHeight(),
			0, 0, 0, inImage.getHeight(),
			inImage.getImageData(), &bmi, DIB_RGB_COLORS);

		EndPaint(hWnd, &ps);
	}
	break;
	case WM_HSCROLL:
		if ((HWND)lParam == hProgressBar) {
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hWnd, &pt); // Convert mouse position to client area coordinates

			// Get the client rectangle of the progress bar
			RECT trackRect;
			GetClientRect(hProgressBar, &trackRect);
			int trackWidth = trackRect.right - trackRect.left;

			// Calculate the relative position on the progress bar based on the x-coordinate of the mouse
			if (pt.x > 480) {
				pt.x = 480 + (pt.x - 480.0) * 1.035;
			}
			else {
				pt.x = 480 - (-pt.x + 480.0) * 1.035;
			}
			int clickedPos = (pt.x * (totalFrames - 1)) / trackWidth;
			std::cout << "Slider Clicked: " << clickedPos << std::endl;
			// Set the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, clickedPos);
			currentFrame = clickedPos; // Set the frame to play
			currentFrame = min(currentFrame, totalFrames - 1);
			currentFrame = max(0, currentFrame);
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_TIMER:
	{
		// Update the current frame
		currentFrame = (currentFrame + 1) % video.size(); // Loop playback
		// Update slider position
		SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame * subUnits);

		InvalidateRect(hWnd, NULL, FALSE); // Notify the window to redraw
		break;
	}

	case WM_DESTROY:
	{
		KillTimer(hWnd, 1);
		PostQuitMessage(0);
		return 0;
	}

	default:

		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


// Message handler for about box.
LRESULT CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		return TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}


