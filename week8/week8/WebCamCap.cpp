#include<cstdio>
#include<dshow.h>

#define F(x) FAILED(x)
#define S(x) SUCCEEDED(x)

HRESULT GetAudioInputFilter(IBaseFilter ** gottaFilter, const wchar_t* matchName) {
	BOOL done = false;

	ICreateDevEnum *pSysDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);

	if (F(hr)) return hr;
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK) {
		IMoniker *pMoniker = 0;
		ULONG cFetched;
		while ((pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) && (!done)) {
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (S(hr)) {
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (S(hr)) {
					wprintf(L"Testing Audio Input Device: %s\n", varName.bstrVal);

					if (wcsncmp(varName.bstrVal, matchName, wcslen(matchName)) == 0) {
						hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)gottaFilter);
						done = true;
					}
				}
				VariantClear(&varName);
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();
	if (done) return hr;
	else return VFW_E_NOT_FOUND;
}

HRESULT GetVideoInputFilter(IBaseFilter** gottaFilter, const wchar_t * matchName) {
	BOOL done = false;

	ICreateDevEnum *pSysDevEnum = 0;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void **)&pSysDevEnum);

	if (F(hr)) return hr;

	IEnumMoniker *pEnumCat = 0;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumCat, 0);

	if (hr == S_OK) {
		IMoniker *pMoniker = 0;
		ULONG cFetched;
		while ((pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) && (!done)) {
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pPropBag);
			if (S(hr)) {
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (S(hr)) {
					wprintf(L"Testing Video Input Device: %s\n", varName.bstrVal);

					if (wcsncmp(varName.bstrVal, matchName, wcslen(matchName)) == 0) {
						hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)gottaFilter);
						done = true;
					}
				}
				VariantClear(&varName);
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();
	if (done) return hr;
	else return VFW_E_NOT_FOUND;
}

HRESULT SaveGraphFile(IGraphBuilder *pGraph, const WCHAR *wszPath) {
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;
	IStorage *pStorage = NULL;
	hr = StgCreateDocfile(
		wszPath,
		STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
		0, &pStorage);
	if (FAILED(hr)) {
		return hr;
	}
	IStream *pStream;
	hr = pStorage->CreateStream(
		wszStreamName,
		STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
		0, 0, &pStream);
	if (FAILED(hr)) {
		pStorage->Release();
		return hr;
	}
	IPersistStream *pPersist = NULL;
	pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersist));
	hr = pPersist->Save(pStream, TRUE);
	pStream->Release();
	pPersist->Release();
	if (SUCCEEDED(hr)) {
		hr = pStorage->Commit(STGC_DEFAULT);
	}
	pStorage->Release();
	return hr;
}

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, const GUID& clsid, LPCWSTR wszName, IBaseFilter **ppF) {
	if (!pGraph || !ppF) return E_POINTER;
	*ppF = 0;
	IBaseFilter *pF = 0;
	HRESULT hr = CoCreateInstance(clsid, 0, CLSCTX_INPROC_SERVER,
		IID_IBaseFilter,
		reinterpret_cast<void**>(&pF));
	if (SUCCEEDED(hr)) {
		hr = pGraph->AddFilter(pF, wszName);
		if (SUCCEEDED(hr))
			*ppF = pF;
		else
			pF->Release();
	}
	return hr;
}

int main(int argc, char * argv[]) {
	ICaptureGraphBuilder2 *pCaptureGraph = 0;
	IGraphBuilder *pGraph = 0;
	IMediaControl *pControl = 0;
	IFileSinkFilter *pSink = 0;
	IBaseFilter *pAudioInputFilter = 0;
	IBaseFilter *pVideoInputFilter = 0;
	IBaseFilter *pAVIWriter = 0;

	HRESULT hr = CoInitialize(0);

	if (F(hr)) {
		printf("ERROR - Could not initialize COM library");
		return hr;
	}

	hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, 0, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void **)&pCaptureGraph);

	if (F(hr)) {
		printf("ERROR - Could not create the Filter Graph Manager.");
		return hr;
	}

	hr = pCaptureGraph->SetOutputFileName(&MEDIASUBTYPE_Avi, L"MyWebcam.AVI", &pAVIWriter, &pSink);

	hr = pCaptureGraph->GetFiltergraph(&pGraph);

	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);

	if (F(hr)) {
		printf("ERROR - Could not create the Media Control object.");
		pGraph->Release();
		CoUninitialize();
		return hr;
	}

	hr = GetAudioInputFilter(&pAudioInputFilter, L"¸¶ÀÌÅ©(Realtek High Definition Audio)");
	if (S(hr)) 
		hr = pGraph->AddFilter(pAudioInputFilter, L"Webcam Audio Capture");

	hr = GetVideoInputFilter(&pVideoInputFilter, L"BisonCam, NB Pro");
	if (S(hr)) 
		hr = pGraph->AddFilter(pVideoInputFilter, L"Webcam Video Capture");

	hr = pCaptureGraph->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pVideoInputFilter, 0, 0);
	hr = pCaptureGraph->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Audio, pAudioInputFilter, 0, pAVIWriter);
	hr = pCaptureGraph->RenderStream(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, pVideoInputFilter, 0, pAVIWriter);

	if (S(hr)) {
		hr = pControl->Run();

		if (S(hr)) {
			wprintf(L"Starated recording...press Enter to stop recording.\n");

			char ch = getchar();
		}

		hr = pControl->Stop();
		wprintf(L"Stopped recording.\n");

		SaveGraphFile(pGraph, L"MyGraph.grf");

	}

	pSink->Release();
	pAVIWriter->Release();
	pAudioInputFilter->Release();
	pVideoInputFilter->Release();
	pControl->Release();
	pGraph->Release();
	pCaptureGraph->Release();
	CoUninitialize();
	return 0;
}