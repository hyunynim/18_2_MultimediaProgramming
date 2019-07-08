#include<dshow.h>
#include<control.h>
#include<tchar.h>
#include<CommCtrl.h>
#include<commdlg.h>
#include<atlbase.h>
#include<d3d9.h>
#include<vmr9.h>

#include"VMRPip.h"
#include"blend.h"
#include"resource.h"

#define SAFE_RELEASE(x) { if(x) x->Release(); x = NULL; }
#define JIF(x) if (FAILED(hr=(x))){return hr;}
#define LIF(x) if (FAILED(hr=(x))){}
#define F(x) FAILED(x)
#define S(x) SUCCEEDED(x)
HWND ghApp = 0;
HMENU ghMenu = 0;
HINSTANCE ghInst = 0;
DWORD g_dwGraphRegister = 0;
RECT g_rcDest = { 0 };
BOOL g_bFilesSelected = FALSE;

IGraphBuilder *pGB = 0;
IMediaControl *pMC = 0;
IVMRWindowlessControl9 *pWC = 0;
IMediaEventEx *pME = 0;
IMediaSeeking *pMS = 0;
IVMRMixerControl9 *pMix = 0;

TCHAR g_szFile1[MAX_PATH] = { 0 }, g_szFile2[MAX_PATH] = { 0 };

const STRM_PARAM strParamInit[1] = { {0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0, 0, {0,0,0,0}} };

STRM_PARAM strParam[2] = { 0 }, strDelta[2] = { 0 };

const QUADRANT quad[4] = {
	X_EDGE_BUFFER, Y_EDGE_BUFFER,
	SECONDARY_WIDTH, SECONDARY_HEIGHT,
	(1.0f - SECONDARY_WIDTH - X_EDGE_BUFFER), Y_EDGE_BUFFER,
	SECONDARY_WIDTH, SECONDARY_HEIGHT,
	(1.0f - SECONDARY_WIDTH - X_EDGE_BUFFER), (1.0f - SECONDARY_HEIGHT - Y_EDGE_BUFFER),
	SECONDARY_WIDTH, SECONDARY_HEIGHT,
	X_EDGE_BUFFER, (1.0f, -SECONDARY_HEIGHT - Y_EDGE_BUFFER),
	SECONDARY_WIDTH, SECONDARY_HEIGHT };

int g_nCurrentQuadrant[2] = { 0 }, g_nSubpictureID = SECONDARY_STREAM;

BOOL VerifyVMR9() {
	HRESULT hr;

	IBaseFilter * pBF = 0;
	hr = CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC, IID_IBaseFilter, (LPVOID *)&pBF);

	if (S(hr)) {
		pBF->Release();
		return 1;
	}
	else return 0;
}

void EnableMenus(BOOL bEnable) {
	int nState = (UINT)((bEnable) ? MF_ENABLED : MF_GRAYED) | MF_BYPOSITION;

	EnableMenuItem(ghMenu, 1, nState);
	EnableMenuItem(ghMenu, 2, nState);
	DrawMenuBar(ghApp);
}

LRESULT CALLBACK WndMainProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_PAINT: OnPaint(hWnd);
		break;
	case WM_COMMAND:
		switch (wParam) {
		case ID_FILE_OPENCLIPS:
			CloseFiles();
			OpenFiles();
			break;
		case ID_FILE_EXITS:
			CloseFiles();
			PostQuitMessage(0);
			break;
		case ID_FILE_CLOSES:
			CloseFiles();
			break;
		}
		break;
	case WM_CLOSE:
		SendMessage(ghApp, WM_COMMAND, ID_FILE_EXITS, 0);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

void OnPaint(HWND hwnd) {
	HRESULT hr;
	PAINTSTRUCT ps;
	HDC hdc;
	RECT rcClient;

	GetClientRect(hwnd, &rcClient);
	hdc = BeginPaint(hwnd, &ps);

	if (pWC)
		hr = pWC->RepaintVideo(hwnd, hdc);
	else
		FillRect(hdc, &rcClient, (HBRUSH)(COLOR_BTNFACE + 1));

	EndPaint(hwnd, &ps);
}

void CloseFiles() {
	HRESULT hr;
	if (pMC) hr = pMC->Stop();

	CloseInterfaces();
	EnableMenus(0);

	RECT rect;
	GetClientRect(ghApp, &rect);
	InvalidateRect(ghApp, &rect, 1);

	InitPlayerWindow();
}

void CloseInterfaces() {
	if (pME) pME->SetNotifyWindow((OAHWND)NULL , 0, 0);

	//SAFE_RELEASE(pMF);
	SAFE_RELEASE(pMS);
	SAFE_RELEASE(pMC);
	SAFE_RELEASE(pWC);
	SAFE_RELEASE(pMix);
	SAFE_RELEASE(pGB);
}

HRESULT InitPlayerWindow() {
	SetWindowPos(ghApp, 0, 0, 0,
		DEFAULT_PLAYER_WIDTH, DEFAULT_PLAYER_HEIGHT,
		SWP_NOMOVE | SWP_NOOWNERZORDER);
	return S_OK;
}

void OpenFiles() {
	HRESULT hr = 0;

	DialogBox(ghInst, MAKEINTRESOURCE(IDD_DIALOG1), ghApp, (DLGPROC)FilesDlgProc);

	if (g_bFilesSelected) {
		g_bFilesSelected = 0;

		InitStreamParams();
		hr = BlendVideo(g_szFile1, g_szFile2);
		if (F(hr)) {
			CloseFiles();
			return;
		}
		UpdatePinPos(0);
		UpdatePinPos(1);
		UpdatePinAlpha(0);
		UpdatePinAlpha(1);
	}
}

LRESULT CALLBACK FilesDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	static TCHAR szFile[MAX_PATH] = { 0 };

	switch (message) {
	case WM_INITDIALOG:
		SetDlgItemText(hWnd, IDC_EDIT_FILE1, g_szFile1);
		SetDlgItemText(hWnd, IDC_EDIT_FILE2, g_szFile2);
		return 1;
	case WM_COMMAND:
		switch (wParam) {
		case IDCANCEL:
			g_bFilesSelected = 0;
			EndDialog(hWnd, 1);
			break;
		case IDC_BUTTON_OK:
			GetDlgItemText(hWnd, IDC_EDIT_FILE1, g_szFile1, MAX_PATH);
			GetDlgItemText(hWnd, IDC_EDIT_FILE2, g_szFile2, MAX_PATH);
			g_bFilesSelected = 1;
			EndDialog(hWnd, TRUE);
			break;
		case IDC_BUTTON_FILE1:
			if (GetClipFileName(szFile))
				SetDlgItemText(hWnd, IDC_EDIT_FILE1, szFile);
			break;
		case IDC_BUTTON_FILE2:
			if (GetClipFileName(szFile))
				SetDlgItemText(hWnd, IDC_EDIT_FILE2, szFile);
			break;
		}
		break;
	}
	return 0;
}

BOOL GetClipFileName(LPTSTR szName) {
	static OPENFILENAME ofn = { 0 };
	static BOOL bSetInitialDir = FALSE;

	*szName = 0;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = ghApp;
	ofn.lpstrFilter = FILE_FILTER_TEXT;
	ofn.lpstrCustomFilter = 0;
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = szName;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = TEXT("Open Video File...\0");
	ofn.lpstrFileTitle = 0;
	ofn.lpstrDefExt = TEXT("*\0");
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_READONLY;

	if (bSetInitialDir == 0) {
		ofn.lpstrInitialDir = DEFAULT_MEDIA_PATH;
		bSetInitialDir = 1;
	}
	else ofn.lpstrInitialDir = 0;

	return GetOpenFileName((LPOPENFILENAME)&ofn);
}

void InitStreamParams() {
	CopyMemory(&strParam[PRIMARY_STREAM], strParamInit, sizeof(strParamInit));
	CopyMemory(&strParam[SECONDARY_STREAM], strParamInit, sizeof(strParamInit));

	strParam[1].fAlapha = TRANSPARENCY_VALUE;

	PositionStream(SECONDARY_STREAM, DEFAULT_QUADRANT);
}

void PositionStream(int nStream, int nQuadrant) {
	strParam[nStream].xPos = quad[nQuadrant].xPos;
	strParam[nStream].yPos = quad[nQuadrant].yPos;
	strParam[nStream].xSize = quad[nQuadrant].xSize;
	strParam[nStream].ySize = quad[nQuadrant].ySize;

	UpdatePinPos(nStream);

	g_nCurrentQuadrant[nStream] = nQuadrant;
}

HRESULT UpdatePinPos(int nStreamID) {
	HRESULT hr = S_OK;
	STRM_PARAM * p = &strParam[nStreamID];

	VMR9NormalizedRect r = { p->xPos, p->yPos, p->xPos + p->xSize, p->yPos + p->ySize };

	if (pMix) hr = pMix->SetOutputRect(nStreamID, &r);

	return hr;
}

HRESULT BlendVideo(LPTSTR szFile1, LPTSTR szFile2) {
	HRESULT hr;
	USES_CONVERSION;
	WCHAR wFile1[MAX_PATH], wFile2[MAX_PATH];

	if ((szFile1 == NULL) || (szFile2 == NULL)) return E_POINTER;

	UpdateWindow(ghApp);

	wcsncpy(wFile1, T2W(szFile1), NUMELMS(wFile1) - 1);
	wcsncpy(wFile2, T2W(szFile2), NUMELMS(wFile2) - 1);
	wFile1[MAX_PATH - 1] = wFile2[MAX_PATH - 1] = 0;

	JIF(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGB));

	if (S(hr)) {
		if (F(ConfigureMultiFileVMR9(wFile1, wFile2))) return hr;

		JIF(pGB->QueryInterface(IID_IMediaControl, (void **)&pMC));
		JIF(pGB->QueryInterface(IID_IMediaEventEx, (void **)&pME));
		JIF(pGB->QueryInterface(IID_IMediaSeeking, (void **)&pMS));

		if (CheckVideoVisibility()) {
			JIF(InitVideoWindow(1, 1));
		}
		else
			return E_FAIL;

		JIF(pME->SetNotifyWindow((OAHWND)ghApp, WM_GRAPHNOTIFY, 0));

		ShowWindow(ghApp, SW_SHOWNORMAL);
		UpdateWindow(ghApp);
		SetForegroundWindow(ghApp);
		SetFocus(ghApp);

		MoveVideoWindow();
		JIF(pMC->Run());

		EnableMenus(TRUE);
	}

	return hr;
}

HRESULT ConfigureMultiFileVMR9(WCHAR *wFile1, WCHAR *wFile2) {
	HRESULT hr = S_OK;
	CComPtr <IBaseFilter> pVmr;

	JIF(InitializeWindowlessVMR(&pVmr));

	if (S(hr = RenderFileToVMR9(pGB, wFile1, pVmr)))
		hr = RenderFileToVMR9(pGB, wFile2, pVmr);

	return hr;
}

void MoveVideoWindow() {
	HRESULT hr;

	if (pWC) {
		GetClientRect(ghApp, &g_rcDest);

		hr = pWC->SetVideoPosition(NULL, &g_rcDest);
	}
}

HRESULT InitializeWindowlessVMR(IBaseFilter **ppVmr9) {
	IBaseFilter *pVmr = 0;
	if (!ppVmr9) return E_POINTER;
	*ppVmr9 = 0;

	HRESULT hr = CoCreateInstance(CLSID_VideoMixingRenderer9, 0, CLSCTX_INPROC, IID_IBaseFilter, (void**)&pVmr);

	if (S(hr)) {
		hr = pGB->AddFilter(pVmr, (LPCWSTR)"Video Mixing Renderer 9");
		if (S(hr)) {
			CComPtr<IVMRFilterConfig9> pConfig;

			JIF(pVmr->QueryInterface(IID_IVMRFilterConfig9, (void **)&pConfig));
			JIF(pConfig->SetRenderingMode(VMR9Mode_Windowless));
			JIF(pConfig->SetNumberOfStreams(2));

			JIF(pVmr->QueryInterface(IID_IVMRWindowlessControl9, (void**)&pWC));
			JIF(pWC->SetVideoClippingWindow(ghApp));
			JIF(pWC->SetBorderColor(RGB(0, 0, 0)));

			JIF(pVmr->QueryInterface(IID_IVMRMixerControl9, (void**)&pMix));
		}
		*ppVmr9 = pVmr;
	}
	return hr;
}

HRESULT RenderFileToVMR9(IGraphBuilder *pGB, WCHAR *wFileName, IBaseFilter *pRenderer, BOOL bRenderAudio) {
	HRESULT hr = S_OK;
	CComPtr <IPin> pOutputPin;
	CComPtr <IBaseFilter> pSource;
	CComPtr <IFilterGraph2> pFG;
	IBaseFilter * pAudioRenderer;
	if (!IsWindowsMediaFile(wFileName)) {
		if (F(hr = pGB->AddSourceFilter(wFileName, (LPCWSTR)"SOURCE", &pSource))) {
			USES_CONVERSION;
			TCHAR szMsg[MAX_PATH + 128];

			wsprintf(szMsg, TEXT("Failed to add the source filter to the graph! hr=0x%x\r\n\r\n"), TEXT("Filename: %s\0"), hr, W2T(wFileName));
			MessageBox(0, szMsg, TEXT("Failed to render file to VMR9"), MB_OK | MB_ICONERROR);

			return hr;
		}
		JIF(GetUnConnectedPin(pSource, PINDIR_OUTPUT, &pOutputPin));
	}
	else
		return E_FAIL;

	if (bRenderAudio) {
		if (S(CoCreateInstance(CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&pAudioRenderer))) {
			JIF(pGB->AddFilter(pAudioRenderer, (LPCWSTR)"Audio Renderer"));
		}
	}

	JIF(pGB->QueryInterface(IID_IFilterGraph2, (void **)&pFG));

	JIF(pFG->RenderEx(pOutputPin, AM_RENDEREX_RENDERTOEXISTINGRENDERERS, 0));

	if (pAudioRenderer != 0) {
		IPin *pUnconnectedPin = 0;
		HRESULT hrPin = GetUnConnectedPin(pAudioRenderer, PINDIR_INPUT, &pUnconnectedPin);
		if (S(hrPin) && (pUnconnectedPin != NULL)) {
			pUnconnectedPin->Release();

			hrPin = pGB->RemoveFilter(pAudioRenderer);
		}
	}
	return hr;
}

BOOL IsWindowsMediaFile(WCHAR *lpszFile){
	USES_CONVERSION;
	TCHAR szFilename[MAX_PATH];

	_tcsncpy(szFilename, W2T(lpszFile), NUMELMS(szFilename));
	szFilename[MAX_PATH - 1] = 0;
	_tcslwr(szFilename);

	if (_tcsstr(szFilename, TEXT(".asf"))
		|| _tcsstr(szFilename, TEXT(".wma"))
		|| _tcsstr(szFilename, TEXT(".wmv")))
		return 1;
	else
		return 0;
}
HRESULT GetUnConnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin) {
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;

	if (!ppPin) {
		return E_POINTER;
	}

	*ppPin = 0;

	HRESULT hr = pFilter->EnumPins(&pEnum);

	if (F(hr)) {
		return hr;
	}

	while (pEnum->Next(1, &pPin, NULL) == S_OK) {
		PIN_DIRECTION ThisPinDir;

		pPin->QueryDirection(&ThisPinDir);

		if (ThisPinDir == PinDir) {
			IPin *pTmp = 0;

			hr = pPin->ConnectedTo(&pTmp);

			if (S(hr)) {
				pTmp->Release();
			}

			else {
				pEnum->Release();
				*ppPin = pPin;
				return S_OK;
			}
		}

		pPin->Release();
	}

	pEnum->Release();

	return E_FAIL;
}

BOOL CheckVideoVisibility() {
	HRESULT hr;
	LONG lWidth = 0, lHeight = 0;

	if (!pWC)
		return FALSE;
	hr = pWC->GetNativeVideoSize(&lWidth, &lHeight, 0, 0);
	if (F(hr))
		return 0;
	if ((lWidth == 0) && (lHeight == 0)) return 0;
	return 1;
}

HRESULT InitVideoWindow(int nMultiplier, int nDivider) {
	LONG lHeight, lWidth;
	HRESULT hr = S_OK;
	if (!pWC) return S_OK;

	hr = pWC->GetNativeVideoSize(&lWidth, &lHeight, 0, 0);
	if (hr == E_NOINTERFACE) return S_OK;

	lWidth = lWidth * nMultiplier / nDivider;
	lHeight = lHeight * nMultiplier / nDivider;

	int nTitleHeight = GetSystemMetrics(SM_CYCAPTION);
	int nBorderWidth = GetSystemMetrics(SM_CXBORDER);
	int nBorderHeight = GetSystemMetrics(SM_CYBORDER);

	SetWindowPos(ghApp, 0, 0, 0, lWidth + 2 * nBorderWidth, lHeight + nTitleHeight + 2 * nBorderHeight, SWP_NOMOVE | SWP_NOOWNERZORDER);

	GetClientRect(ghApp, &g_rcDest);
	hr = pWC->SetVideoPosition(0, &g_rcDest);

	return hr;
}

HRESULT UpdatePinAlpha(int nStreamID) {
	HRESULT hr = S_OK;

	STRM_PARAM * p = &strParam[nStreamID];

	if (pMix) hr = pMix->SetAlpha(nStreamID, p->fAlapha);

	return hr;

}


int PASCAL WinMain(HINSTANCE hInstC, HINSTANCE hInstP, LPSTR lpCmdLine, int nCmdShow) {
	MSG msg{ 0 };
	WNDCLASS wc;

	if (F(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED))) return 0;

	if (!VerifyVMR9()) return 0;

	ZeroMemory(&wc, sizeof wc);
	ghInst = wc.hInstance = hInstC;
	wc.lpfnWndProc = WndMainProc;
	wc.lpszClassName = CLASSNAME;
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);

	if (!RegisterClass(&wc)) {
		CoUninitialize();
		return 1;
	}

	ghApp = CreateWindow(CLASSNAME, APPLICATIONNAME, WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_CLIPCHILDREN | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, DEFAULT_PLAYER_WIDTH, DEFAULT_PLAYER_HEIGHT, 0, 0, ghInst, 0);

	if (ghApp) {
		ghMenu = GetMenu(ghApp);
		EnableMenus(FALSE);

		while (GetMessage(&msg, 0, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CoUninitialize();

	return (int)msg.wParam;
}