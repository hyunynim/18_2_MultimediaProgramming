#include <stdio.h>
#include <dshow.h>

char g_fileName[256];
char g_pathFileName[512];

BOOL GetMediaFileName(void) {
	OPENFILENAME ofn;

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = NULL;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = NULL;
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter = NULL;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = (char*)calloc(1, 512);
	ofn.nMaxFile = 512;
	ofn.lpstrFileTitle = (char*)calloc(1, 512);
	ofn.nMaxFileTitle = 255;
	ofn.lpstrInitialDir = NULL;
	ofn.lpstrTitle = "Select file to render...";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = NULL;
	ofn.lCustData = NULL;

	if (!GetOpenFileName(&ofn)) {
		free(ofn.lpstrFile);
		free(ofn.lpstrFileTitle);

		return false;
	}

	else {
		strcpy(g_pathFileName, ofn.lpstrFile);
		strcpy(g_fileName, ofn.lpstrFileTitle);
		free(ofn.lpstrFile);
		free(ofn.lpstrFileTitle);
	}

	return true;
}

HRESULT SaveGraphFile(IGraphBuilder *pGraph, const WCHAR *wszPath) {
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;
	IStorage *pStorage = NULL;

	hr = StgCreateDocfile(wszPath, STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStorage);

	if (FAILED(hr)) {
		return hr;
	}

	IStream *pStream;

	hr = pStorage->CreateStream(wszStreamName, STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream);

	if (FAILED(hr)) {
		pStorage->Release();

		return hr;
	}

	IPersistStream *pPersist = NULL;

	pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void **>(&pPersist));

	hr = pPersist->Save(pStream, TRUE);

	pStream->Release();
	pPersist->Release();

	if (SUCCEEDED(hr))
		hr = pStorage->Commit(STGC_DEFAULT);


	pStorage->Release();

	return hr;
}

int main() {
	IGraphBuilder *pGraph = NULL;
	IMediaControl *pControl = NULL;
	IMediaEvent *pEvent = NULL;
	HRESULT hr = CoInitialize(NULL);

	if (FAILED(hr)) {
		printf("ERROR - Could not initialize COM library.\n");
		return hr;
	}

	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);

	if (FAILED(hr)) {
		printf("ERROR - Could not create the Filter Graph Manager.\n");
		return hr;
	}

	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);

	if (FAILED(hr)) {
		printf("ERROR - Could not create the Media Control object.\n");

		pGraph->Release();
		CoUninitialize();
		return hr;
	}

	hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);

	if (FAILED(hr)) {
		printf("ERROR - Could not create the Media Event object.\n");

		pGraph->Release();
		pControl->Release();
		CoUninitialize();
		return hr;
	}

	if (!GetMediaFileName())
		return 0;

#ifndef UNICODE
	WCHAR wFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, g_pathFileName, -1, wFileName, MAX_PATH);
	hr = pGraph->RenderFile((LPCWSTR)wFileName, NULL);
#else
	hr = pGraph->RenderFile((LPCWSTR)g_pathFileName, NULL);
#endif

	if (SUCCEEDED(hr)) {
		hr = pControl->Run();
		if (SUCCEEDED(hr)) {
			long evCode;
			pEvent->WaitForCompletion(INFINITE, &evCode);
		}
		hr = pControl->Stop();
		const WCHAR savePath[] = L"D:\\Education\\(18-2)MultiMedia_Programming\\Media\\MyGraph.grf";
		SaveGraphFile(pGraph, savePath);
	}
	
	else
		printf("ERROR - Could not find the media fila.\n");

	pControl->Release();
	pEvent->Release();
	pGraph->Release();
	CoUninitialize();
}