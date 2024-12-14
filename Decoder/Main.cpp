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

// Include class files
#include "Image.h"
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
const int frameRate = 1000;
MyVideo			inVideo;						// video objects
bool isPlaying = true;             // Play/Pause state
int currentFrame = 0;              // Current frame index
int totalFrames;    // Total number of frames in the video

int audioLength;

#define ID_PLAY_PAUSE  1001
#define ID_NEXT_FRAME  1002
#define ID_PREV_FRAME  1003

#define WM_REDRAW 1234

HWND hProgressBar;
UINT timerId;





void playAudio(const char* filePath) {
	// Open the audio file
	char command[256];
	sprintf_s(command, "open \"%s\" type waveaudio alias myAudio", filePath);
	if (mciSendString(command, NULL, 0, NULL) != 0) {
		std::cerr << "Unable to open file: " << filePath << std::endl;
		return;
	}

	char lengthBuffer[128];
	if (mciSendString("status myAudio length", lengthBuffer, sizeof(lengthBuffer), NULL) != 0) {
		std::cerr << "Failed to retrieve audio length!" << std::endl;
	}
	else {
		audioLength = std::stoi(lengthBuffer);
		std::cout << "Audio length: " << audioLength << " milliseconds" << std::endl;
	}

	// Play the audio
	if (mciSendString("play myAudio", NULL, 0, NULL) != 0) {
		std::cerr << "Playback failed!" << std::endl;
		return;
	}

	std::cout << "Playing: " << filePath << std::endl;
}

void seekAudio() {
	int milliseconds = currentFrame / (float)totalFrames * audioLength;
	
	char command[256];
	//std::cout << milliseconds << std::endl;


	sprintf_s(command, "seek myAudio to %d", milliseconds);
	if (mciSendString(command, NULL, 0, NULL) != 0) {
		std::cerr << "Seek failed!" << std::endl;
		std::cout << "Failed!" << std::endl;
	}
	else {
		//std::cout << "Seek to: " << milliseconds << " milliseconds." << std::endl;
	}
}

void pauseAudio() {
	if (mciSendString("pause myAudio", NULL, 0, NULL) != 0) {
		std::cerr << "Pause failed!" << std::endl;
	}
	else {
		std::cout << "Paused." << std::endl;
	}
}

void resumeAudio() {
	seekAudio();
	if (mciSendString("play myAudio", NULL, 0, NULL) != 0) {
		std::cerr << "Resume playback failed!" << std::endl;
	}
	else {
		std::cout << "Resuming playback." << std::endl;
	}
}

void stopAudio() {
	if (mciSendString("stop myAudio", NULL, 0, NULL) != 0) {
		std::cerr << "Stop playback failed!" << std::endl;
	}
	else {
		std::cout << "Playback stopped." << std::endl;
	}
	mciSendString("close myAudio", NULL, 0, NULL); // Close the file
}

void UpdateFrameWithSync(HWND hWnd) {
	// Update the current frame
	currentFrame = (currentFrame + 1) % video.size(); // Loop playback
	printf("%d\n", currentFrame);
	if (currentFrame % 33 == 0)
	{
		//mciSendString("stop myAudio", NULL, 0, NULL);
		//mciSendString("seek myAudio to start", NULL, 0, NULL);
		//pauseAudio();
		printf("Seek\n");
		seekAudio();
	}
	// Update slider position
	SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame * subUnits);

	InvalidateRect(hWnd, NULL, FALSE); // Notify the window to redraw
}
bool isPaused = false;
// Timer callback function
void CALLBACK TimerCallback(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2) {
	if (!isPlaying)
		return;
	HWND hWnd = (HWND)dwUser; // Retrieve the window handle passed as user data
	PostMessage(hWnd, WM_REDRAW, 0, 0);
	//UpdateFrameWithSync(hWnd); // Call the frame update function
}

// Set up a high-precision multimedia timer
void StartHighPrecisionTimer(HWND hWnd, UINT frameRate) {
	UINT interval = 1000 / frameRate; // Calculate the time interval for each frame in milliseconds
	timeBeginPeriod(1); // Set the minimum timer resolution to 1 millisecond
	timerId = timeSetEvent(interval, 1, TimerCallback, (DWORD_PTR)hWnd, TIME_PERIODIC); // Start the periodic timer
}

// Stop the high-precision multimedia timer
void StopHighPrecisionTimer() {
	if (timerId) {
		timeKillEvent(timerId); // Stop the timer
		timeEndPeriod(1); // Restore the default timer resolution
		timerId = 0; // Reset the timer ID
	}
}


bool LoadVideoToMemory() {
	inVideo.setHeight(height);
	inVideo.setWidth(width);
	inVideo.setVideoPath(videoPath);
	inVideo.readCmp(videoPath);
	video = inVideo.getVideoData();
	return true;
}

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
	for (const auto& arg : arguments) {
		std::cout << "Argument: " << arg << std::endl;
	}

	videoPath = arguments[1].c_str();
	LoadVideoToMemory();
	playAudio(arguments[2].c_str());

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

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_IMAGE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCSTR)IDC_IMAGE;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

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

	//TODO: 1. You can't pause the video
	//TODO: 2. No audio when second plays
	//TODO: 3. 
	switch (message)
	{
	case WM_REDRAW: {
		char positionBuffer[128];
		if (mciSendString("status myAudio position", positionBuffer, sizeof(positionBuffer), NULL) != 0) {
			std::cerr << "Failed to retrieve current position!" << std::endl;
		}
		int currentPosition = std::stoi(positionBuffer);
		int newFrame = 1.0 * currentPosition * totalFrames / audioLength;
		if (newFrame != currentFrame) {
			currentFrame = (currentFrame + 1) % video.size(); // Loop playback
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame * subUnits);
			InvalidateRect(hWnd, NULL, FALSE); // Notify the window to redraw
		}
		// Update the current frame

		//if (currentFrame % 33 == 0) {
		//	resumeAudio();
		//}
		// Update slider position

		break;
	}
	case WM_CREATE:
	{
		StartHighPrecisionTimer(hWnd, 30);
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
				resumeAudio();
			}
			else {
				pauseAudio();
			}
			InvalidateRect(hWnd, NULL, FALSE); // Redraw the window to update state
			break;

		case ID_NEXT_FRAME:
			isPlaying = false; // Pause playback

				KillTimer(hWnd, 1); // Pause the timer
				pauseAudio();
			currentFrame = (currentFrame + 1) % totalFrames; // Switch to the next frame
			// Update the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame);
			InvalidateRect(hWnd, NULL, FALSE);
			break;

		case ID_PREV_FRAME:
			isPlaying = false; // Pause playback
				KillTimer(hWnd, 1); // Pause the timer
				pauseAudio();
			currentFrame = (currentFrame - 1 + totalFrames) % totalFrames; // Switch to the previous frame
			// Update the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, currentFrame);
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
		MyImage& inImage = video[currentFrame]; // Use the current frame
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
			std::cout << "Slider Clicked: "<< clickedPos << std::endl;
			// Set the slider position
			SendMessage(hProgressBar, TBM_SETPOS, TRUE, clickedPos);
			currentFrame = clickedPos; // Set the frame to play
			currentFrame = min(currentFrame, totalFrames - 1);
			currentFrame = max(0, currentFrame);
			if (isPlaying)
				resumeAudio();
			InvalidateRect(hWnd, NULL, FALSE);
		}
		break;

	case WM_TIMER:
	{
		
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


