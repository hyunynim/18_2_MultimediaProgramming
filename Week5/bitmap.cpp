#include <tchar.h>
#include <dshow.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <strsafe.h>

#include "ticker.h"
#include "bitmap.h"
#include "resource.h"

const float EDGE_BUFFER=0.04f; 
                               
								
const float SLIDE_VALUE = 0.05f;
								
const int SLIDE_TIMER   = 2001;	
const int SLIDE_TIMEOUT = 125;  
												
IVMRMixerBitmap9 *pBMP = NULL;
DWORD g_dwTickerFlags=0;
int gnSlideTimer=0;
TCHAR g_szAppText[DYNAMIC_TEXT_SIZE]={0};

float g_fBitmapCompWidth=0;  
int g_nImageWidth=0;         

HFONT g_hFont=0;
LONG g_lFontPointSize   = DEFAULT_FONT_SIZE;
TCHAR g_szFontName[100] = {DEFAULT_FONT_NAME};
TCHAR g_szFontStyle[32] = {DEFAULT_FONT_STYLE};
COLORREF g_rgbColors    = DEFAULT_FONT_COLOR;

VMR9NormalizedRect  g_rDest={0};


HRESULT BlendApplicationImage(HWND hwndApp)
{
    LONG cx, cy;
    HRESULT hr;

    hr = pWC->GetNativeVideoSize(&cx, &cy, NULL, NULL);
    if (FAILED(hr))
    {
        Msg(TEXT("GetNativeVideoSize FAILED!  hr=0x%x\r\n"), hr);
        return hr;
    }

    HBITMAP hbm = LoadBitmap(ghInst, MAKEINTRESOURCE(IDR_TICKER));

    HDC hdc = GetDC(hwndApp);
    HDC hdcBmp = CreateCompatibleDC(hdc);
    ReleaseDC(hwndApp, hdc);

    BITMAP bm;
    HBITMAP hbmOld;
    GetObject(hbm, sizeof(bm), &bm);
    hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

    VMR9AlphaBitmap bmpInfo;
    ZeroMemory(&bmpInfo, sizeof(bmpInfo) );
    bmpInfo.dwFlags = VMRBITMAP_HDC;
    bmpInfo.hdc = hdcBmp;

    g_nImageWidth = bm.bmWidth;

    g_fBitmapCompWidth = (float)g_nImageWidth / (float)cx;

    SetRect(&bmpInfo.rSrc, 0, 0, g_nImageWidth, bm.bmHeight);
    bmpInfo.rDest.left   = 1.0f;
    bmpInfo.rDest.right  = 1.0f + g_fBitmapCompWidth;
    bmpInfo.rDest.top    = (float)(cy - bm.bmHeight) / (float)cy - EDGE_BUFFER;
    bmpInfo.rDest.bottom = 1.0f - EDGE_BUFFER;

    g_rDest = bmpInfo.rDest;

    bmpInfo.fAlpha = TRANSPARENCY_VALUE;

    SetColorRef(bmpInfo);

    hr = pBMP->SetAlphaBitmap(&bmpInfo);
    if (FAILED(hr))
    {
        Msg(TEXT("SetAlphaBitmap FAILED!  hr=0x%x\r\n\r\n%s\0"),
            hr, STR_VMR_DISPLAY_WARNING);
    }

    DeleteObject(SelectObject(hdcBmp, hbmOld));

    DeleteObject(hbm);
    DeleteDC(hdcBmp);

    return hr;
}


void ClearTickerState(void)
{
    if (gnSlideTimer)
    {
        KillTimer(NULL, gnSlideTimer);
        gnSlideTimer = 0;
    }

    g_dwTickerFlags = 0;

    EnableMenuItem(ghMenu, ID_SLIDE,   MF_ENABLED);
    EnableMenuItem(ghMenu, ID_TICKER_STATIC_IMAGE, MF_ENABLED);
    EnableMenuItem(ghMenu, ID_TICKER_DYNAMIC_TEXT, MF_ENABLED);
    EnableMenuItem(ghMenu, ID_SET_FONT, MF_ENABLED);
    EnableMenuItem(ghMenu, ID_SET_TEXT, MF_ENABLED);

    CheckMenuItem(ghMenu, ID_SLIDE,   MF_UNCHECKED);
}


HRESULT DisableTicker(DWORD dwFlags)
{
    HRESULT hr;
    VMR9AlphaBitmap bmpInfo={0};

    hr = pBMP->GetAlphaBitmapParameters(&bmpInfo);
    if (FAILED(hr))
        Msg(TEXT("GetAlphaBitmapParameters FAILED!  hr=0x%x\r\n"), hr);

    if (dwFlags & MARK_DISABLE)
    {
        bmpInfo.dwFlags = VMRBITMAP_DISABLE;

        EnableMenuItem(ghMenu, ID_SLIDE,   MF_GRAYED);
        EnableMenuItem(ghMenu, ID_TICKER_STATIC_IMAGE, MF_GRAYED);
        EnableMenuItem(ghMenu, ID_TICKER_DYNAMIC_TEXT, MF_GRAYED);
        EnableMenuItem(ghMenu, ID_SET_FONT, MF_GRAYED);
        EnableMenuItem(ghMenu, ID_SET_TEXT, MF_GRAYED);

        hr = pBMP->UpdateAlphaBitmapParameters(&bmpInfo);
        if (FAILED(hr))
            Msg(TEXT("UpdateAlphaBitmapParameters FAILED to disable ticker!  hr=0x%x\r\n"), hr);
    }
    else
    {
        if (g_dwTickerFlags & MARK_STATIC_IMAGE)
            hr = BlendApplicationImage(ghApp);
        else
            hr = BlendApplicationText(ghApp, g_szAppText);

        EnableMenuItem(ghMenu, ID_SLIDE,   MF_ENABLED);
        EnableMenuItem(ghMenu, ID_TICKER_STATIC_IMAGE, MF_ENABLED);
        EnableMenuItem(ghMenu, ID_TICKER_DYNAMIC_TEXT, MF_ENABLED);
        EnableMenuItem(ghMenu, ID_SET_FONT, MF_ENABLED);
        EnableMenuItem(ghMenu, ID_SET_TEXT, MF_ENABLED);
     }

    return hr;
}


void FlipFlag(DWORD dwFlag)
{
    if (g_dwTickerFlags & dwFlag)
        g_dwTickerFlags &= ~dwFlag;

    else
        g_dwTickerFlags |= dwFlag;
}


void SlideTicker(DWORD dwFlags)
{
    if (dwFlags & MARK_SLIDE)
    {
        gnSlideTimer = (int) SetTimer(NULL, SLIDE_TIMER, SLIDE_TIMEOUT, TimerProc);
        CheckMenuItem(ghMenu, ID_SLIDE, MF_CHECKED);
    }
    else
    {
        if (gnSlideTimer)
        {
            KillTimer(NULL, gnSlideTimer);
            gnSlideTimer=0;
        }
        CheckMenuItem(ghMenu, ID_SLIDE, MF_UNCHECKED);
        ResetBitmapPosition();
    }
}


VOID CALLBACK TimerProc(
  HWND hwnd,        
  UINT uMsg,        
  UINT_PTR idEvent, 
  DWORD dwTime      
)
{
    HandleSlide();
}


void HandleSlide(void)
{
    HRESULT hr;
    VMR9AlphaBitmap bmpInfo={0};

    hr = pBMP->GetAlphaBitmapParameters(&bmpInfo);
    if (FAILED(hr))
        Msg(TEXT("GetAlphaBitmapParameters FAILED!  hr=0x%x\r\n"), hr);

    bmpInfo.rDest.left  -= SLIDE_VALUE;   
    bmpInfo.rDest.right -= SLIDE_VALUE;

    if ((bmpInfo.rDest.right <= EDGE_BUFFER))
    {
        bmpInfo.rDest.left = 1.0f;
        bmpInfo.rDest.right = 1.0f + g_fBitmapCompWidth;
    }

    SetColorRef(bmpInfo);

    hr = pBMP->UpdateAlphaBitmapParameters(&bmpInfo);
}


void ResetBitmapPosition(void)
{
    HRESULT hr;
    VMR9AlphaBitmap bmpInfo={0};

    hr = pBMP->GetAlphaBitmapParameters(&bmpInfo);
    if (FAILED(hr))
        Msg(TEXT("GetAlphaBitmapParameters FAILED!  hr=0x%x\r\n"), hr);

    bmpInfo.rDest.left  = g_rDest.left;
    bmpInfo.rDest.right = g_rDest.right;

    SetColorRef(bmpInfo);

    hr = pBMP->UpdateAlphaBitmapParameters(&bmpInfo);
}


void SetColorRef(VMR9AlphaBitmap& bmpInfo)
{
    bmpInfo.clrSrcKey = RGB(255, 255, 255);
    bmpInfo.dwFlags |= VMRBITMAP_SRCCOLORKEY;
}


void EnableTickerMenu(BOOL bEnable)
{
    CheckMenuItem( ghMenu, ID_SLIDE,    bEnable ? MF_CHECKED : MF_UNCHECKED);

    EnableMenuItem(ghMenu, ID_SLIDE,    bEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(ghMenu, ID_DISABLE,  bEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(ghMenu, ID_SET_FONT, bEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(ghMenu, ID_SET_TEXT, bEnable ? MF_ENABLED : MF_GRAYED);

    EnableMenuItem(ghMenu, ID_TICKER_STATIC_IMAGE, bEnable ? MF_ENABLED : MF_GRAYED);
    EnableMenuItem(ghMenu, ID_TICKER_DYNAMIC_TEXT, bEnable ? MF_ENABLED : MF_GRAYED);
}


HFONT UserSelectFont( void ) 
{
    return (SetTextFont(TRUE));
}


HFONT SetTextFont(BOOL bShowDialog) 
{ 
    CHOOSEFONT cf={0}; 
    LOGFONT lf={0}; 
    HFONT hfont; 
    HDC hdc;
    LONG lHeight;

    hdc = GetDC( ghApp );
    lHeight = -MulDiv( g_lFontPointSize, GetDeviceCaps(hdc, LOGPIXELSY), 72 );
    ReleaseDC( ghApp, hdc );

    StringCchCopy(lf.lfFaceName, LF_FACESIZE, g_szFontName);
    lf.lfHeight = lHeight; 

    lf.lfQuality = NONANTIALIASED_QUALITY;

    cf.lStructSize = sizeof(CHOOSEFONT); 
    cf.hwndOwner   = ghApp; 
    cf.hDC         = (HDC)NULL; 
    cf.lpLogFont   = &lf; 
    cf.iPointSize  = g_lFontPointSize * 10; 
    cf.rgbColors   = g_rgbColors; 
    cf.lCustData   = 0L; 
    cf.lpfnHook    = (LPCFHOOKPROC)NULL; 
    cf.hInstance   = (HINSTANCE) NULL; 
    cf.lpszStyle   = g_szFontStyle; 
    cf.nFontType   = SCREEN_FONTTYPE; 
    cf.nSizeMin    = 0; 
    cf.lpTemplateName = NULL; 
    cf.Flags = CF_SCREENFONTS | CF_SCALABLEONLY | CF_INITTOLOGFONTSTRUCT | 
               CF_EFFECTS     | CF_USESTYLE     | CF_LIMITSIZE; 

    cf.nSizeMax = MAX_FONT_SIZE; 
 
    if (cf.rgbColors == ALMOST_WHITE)
        cf.rgbColors = PURE_WHITE;

    if (bShowDialog)
        ChooseFont(&cf); 

    (void)StringCchCopy(g_szFontName, NUMELMS(g_szFontName), lf.lfFaceName);
    g_lFontPointSize = cf.iPointSize / 10;
    g_rgbColors = cf.rgbColors;

    if (g_rgbColors == PURE_WHITE)
        g_rgbColors = ALMOST_WHITE;

    hfont = CreateFontIndirect(cf.lpLogFont); 
    return (hfont); 
} 


HRESULT BlendApplicationText(HWND hwndApp, TCHAR *szNewText)
{
    LONG cx, cy;
    HRESULT hr;

    hr = pWC->GetNativeVideoSize(&cx, &cy, NULL, NULL);
    if (FAILED(hr))
      return hr;

    HDC hdc = GetDC(hwndApp);
    HDC hdcBmp = CreateCompatibleDC(hdc);

    HFONT hOldFont = (HFONT) SelectObject(hdcBmp, g_hFont);

    int nLength, nTextBmpWidth, nTextBmpHeight;
    SIZE sz={0};
    nLength = (int) _tcslen(szNewText);
    GetTextExtentPoint32(hdcBmp, szNewText, nLength, &sz);
    nTextBmpHeight = sz.cy;
    nTextBmpWidth  = sz.cx;

    HBITMAP hbm = CreateCompatibleBitmap(hdc, nTextBmpWidth, nTextBmpHeight);
    ReleaseDC(hwndApp, hdc);

    BITMAP bm;
    HBITMAP hbmOld;
    GetObject(hbm, sizeof(bm), &bm);
    hbmOld = (HBITMAP)SelectObject(hdcBmp, hbm);

    RECT rcText;
    SetRect(&rcText, 0, 0, nTextBmpWidth, nTextBmpHeight);
    SetBkColor(hdcBmp, RGB(255, 255, 255)); 
    SetTextColor(hdcBmp, g_rgbColors);      

    TextOut(hdcBmp, 0, 0, szNewText, nLength);

    VMR9AlphaBitmap bmpInfo;
    ZeroMemory(&bmpInfo, sizeof(bmpInfo) );
    bmpInfo.dwFlags = VMRBITMAP_HDC;
    bmpInfo.hdc = hdcBmp;

    g_nImageWidth = bm.bmWidth;

    g_fBitmapCompWidth = (float)g_nImageWidth / (float)cx;

    bmpInfo.rDest.left  = 1.0f;
    bmpInfo.rDest.right = 1.0f + g_fBitmapCompWidth;
    bmpInfo.rDest.top = (float)(cy - bm.bmHeight) / (float)cy - EDGE_BUFFER;
    bmpInfo.rDest.bottom = 1.0f - EDGE_BUFFER;
    bmpInfo.rSrc = rcText;

    g_rDest = bmpInfo.rDest;

    bmpInfo.fAlpha = TRANSPARENCY_VALUE;

    SetColorRef(bmpInfo);

    hr = pBMP->SetAlphaBitmap(&bmpInfo);
    if (FAILED(hr))
        Msg(TEXT("SetAlphaBitmap FAILED!  hr=0x%x\r\n\r\n%s\0"), hr,
            STR_VMR_DISPLAY_WARNING);

    DeleteObject(SelectObject(hdcBmp, hbmOld));
    SelectObject(hdc, hOldFont);

    DeleteObject(hbm);
    DeleteDC(hdcBmp);

    return hr;
}


