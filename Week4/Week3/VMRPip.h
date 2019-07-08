#pragma once

HRESULT InitPlayerWindow();
HRESULT InitVideoWindow(int nMultiplier, int nDivider);
HRESULT HandleGraphEvent();

BOOL GetClipFileName(LPTSTR szName);
BOOL CheckVideoVisibility();

void MoveVideoWindow();
void CloseInterfaces();
void OpenClip();
void CloseFiles();
void OpenFiles();
void GetFilename(TCHAR * pszFull, TCHAR *pszFile);
void Msg(TCHAR * szFormat, ...);

HRESULT InitializeWindowlessVMR(IBaseFilter **ppVmr9);
HRESULT BlendVideo(LPTSTR szFile1, LPTSTR szFile2);

void SetNextQuadrant(int nStream);
void OnPaint(HWND hwnd);
void EnableMenus(BOOL bEnable);

LRESULT CALLBACK FilesDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

#define FILE_FILTER_TEXT \
	TEXT("Video Files (*.asf; *.avi; *.qt; *.mov; *.mpg; *.mpeg; *.m1v; *.wmv)\0")

#define DEFAULT_MEDIA_PATH	TEXT("\\\0");

#define DEFAULT_PLAYER_WIDTH	260
#define DEFAULT_PLAYER_HEIGHT	120
#define DEFAULT_VIDEO_WIDTH		320
#define DEFAULT_VIDEO_HEIGHT	240
#define MINIMUM_VIDEO_WIDTH		200
#define MINIMUM_VIDEO_HEIGHT	120

#define APPLICATIONNAME TEXT("VMR9 Picture-In-Picture\0")
#define CLASSNAME		TEXT("VMR9PIP\0")

#define WM_GRAPHNOTIFY	WM_USER+1

extern HWND ghApp;
extern HMENU ghMenu;
extern HINSTANCE ghInst;
extern DWORD g_dwGraphRegister;

extern IGraphBuilder *pGB;
extern IMediaControl *pMC;
extern IVMRWindowlessControl9 *pWC;
extern IMediaControl *pMC;
extern IMediaEventEx *pME;
extern IMediaSeeking *pMS;
extern IVMRMixerControl9 *pMix;
