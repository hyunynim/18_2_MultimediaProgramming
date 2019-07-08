#pragma once
#include<vmr9.h>

#define TRANSPARENCY_VALUE (0.6f)

typedef const float CF;
typedef const int CI;
const int ZORDER_CLOSE = 0;
const int ZORDER_FAR = 1;

const float MOVEVAL = 0.05f;

const int SWAP_NOTHING = 0;
const int SWAP_POSITION = 1;
const int SWAP_ALPHA = 2;

const int UPDATE_TIMER = 2000;
const int POSITION_TIMEOUT = 100;
const int ALPHA_TIMEOUT = 50;

const float NUM_ANIMATION_STEPS = 10.0f;


enum QUAD {
	TOP_LEFT, TOP_RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, NUM_QUADRANTS
};

CI DEFAULT_QUADRANT = BOTTOM_RIGHT;

CF X_EDGE_BUFFER = 0.025f;
CF Y_EDGE_BUFFER = 0.05f;

CF SECONDARY_WIDTH = 0.4f;
CF SECONDARY_HEIGHT = 0.4f;

CI PRIMARY_STREAM = 0;
CI SECONDARY_STREAM = 1;

typedef struct {
	FLOAT xPos, yPos;
	FLOAT xSize, ySize;
	FLOAT fAlapha;
	BOOL bFlipped, bMirrored;
	VMR9NormalizedRect rect;
} STRM_PARAM;

typedef struct {
	FLOAT xPos, yPos;
	FLOAT xSize, ySize;
}QUADRANT;

HRESULT UpdatePinAlpha(int nStreamID);
HRESULT UpdatePinPos(int nStreamID);

void InitStreamParams();
void PositionStream(int nStream, int nQuadrant);

BOOL VerifyVMR9();
void EnableMenus(BOOL bEnable);
LRESULT CALLBACK WndMainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void OnPaint(HWND hwnd);
void CloseFiles();
void CloseInterfaces();
void OpenFiles();
LRESULT CALLBACK FilesDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitStreamParams();
void PositionStream(int nStream, int nQuadrant);
HRESULT UpdatePinPos(int nStreamID);
HRESULT BlendVideo(LPTSTR szFile1, LPTSTR szFile2);
HRESULT ConfigureMultiFileVMR9(WCHAR *wFile1, WCHAR *wFile2);
HRESULT InitializeWindowlessVMR(IBaseFilter **ppVmr9);
HRESULT RenderFileToVMR9(IGraphBuilder *pGB, WCHAR *wFileName, IBaseFilter *pRenderer, BOOL bRenderAudio = TRUE);
BOOL IsWindowsMediaFile(WCHAR *lpszFile);
HRESULT GetUnConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
BOOL CheckVideoVisibility();
HRESULT InitVideoWindow(int nMultiplier, int nDivider);
HRESULT UpdatePinAlpha(int nStreamID);
void SwapStreams();
void StartSwapAnimation(void);
void StartTimer(int nTimeout);
void StopTimer(void);
VOID CALLBACK TimerProc(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void AdjustStreamRect(int nStream, STRM_PARAM * pStream);
void AdjustAlpha(int nStream, STRM_PARAM * pStream);
HRESULT UpdatePinZOrder(int nStreamID, DWORD dw2Order);