#include<dshow.h>
#include <stdio.h>
HRESULT EnumerateAudioInputFilters(void** gottaFilter);
HRESULT EnumerateAudioInputPins(IBaseFilter *pFilter);
HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin);
HRESULT ConnectFilters(IGraphBuilder *pGraph, IPin *pOut, IBaseFilter *pDest);
HRESULT ConnectFilters(IGraphBuilder *pGraph, IBaseFilter *pSrc, IBaseFilter *pDest);
HRESULT AddFilterByCLSID(IGraphBuilder *pGraph, const GUID& clsid, LPCWSTR wszName, IBaseFilter **ppF);
HRESULT SaveGraphFile(IGraphBuilder *pGraph, const WCHAR *wszPath);
int main(int argc, char* argv[]) {
	IGraphBuilder *pGraph = NULL; 
	IMediaControl *pControl = NULL;
	IFileSinkFilter *pSink = NULL; 
	IBaseFilter *pAudioInputFilter = NULL; 
	IBaseFilter *pFileWriter = NULL;
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		printf("ERROR - Could not initialize COM library");
		return hr;
	}
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (void **)&pGraph);
	if (FAILED(hr)) { 
		printf("ERROR - Could not create the Filter Graph Manager.");
		return hr;
	}

	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	if (FAILED(hr)) {
		printf("ERROR - Could not create the Media Control object.");
		pGraph->Release();
		CoUninitialize(); 
		return hr;
	}
	hr = EnumerateAudioInputFilters((void**)&pAudioInputFilter);
	hr = EnumerateAudioInputPins(pAudioInputFilter);
	hr = pGraph->AddFilter(pAudioInputFilter, L"Capture");
	IBaseFilter *pAVIMux = NULL;
	hr = AddFilterByCLSID(pGraph, CLSID_AviDest, L"AVI Mux", &pAVIMux);
	hr = ConnectFilters(pGraph, pAudioInputFilter, pAVIMux);
	hr = AddFilterByCLSID(pGraph, CLSID_FileWriter,
		L"File Writer", &pFileWriter);
	hr = pFileWriter->QueryInterface(IID_IFileSinkFilter, (void**)&pSink);
	pSink->SetFileName(L"MyWAVFile.AVI", NULL);
	hr = ConnectFilters(pGraph, pAVIMux, pFileWriter);
	if (SUCCEEDED(hr)) {
		hr = pControl->Run();
		if (SUCCEEDED(hr)) {
			wprintf(L"Started recording...press Enter to stop recording.\n");
			char ch;
			ch = getchar();
		}
		hr = pControl->Stop();
		wprintf(L"Stopped recording.\n");
		SaveGraphFile(pGraph, L"MyGraph.GRF");
	}
	pSink->Release();
	pAVIMux->Release();
	pFileWriter->Release();
	pAudioInputFilter->Release();
	pControl->Release();
	pGraph->Release();
	CoUninitialize();
	return 0;
}

HRESULT EnumerateAudioInputFilters(void** gottaFilter) {
	ICreateDevEnum *pSysDevEnum = NULL;
	HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL,
		CLSCTX_INPROC_SERVER, IID_ICreateDevEnum,
		(void **)&pSysDevEnum);
	if (FAILED(hr)) {
		return hr;
	}
	IEnumMoniker *pEnumCat = NULL;
	hr = pSysDevEnum->CreateClassEnumerator(CLSID_AudioInputDeviceCategory,
		&pEnumCat, 0);
	if (hr == S_OK) {
		IMoniker *pMoniker = NULL;
		ULONG cFetched;
		if (pEnumCat->Next(1, &pMoniker, &cFetched) == S_OK) {
			IPropertyBag *pPropBag;
			hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag,
				(void **)&pPropBag);
			if (SUCCEEDED(hr)) {
				VARIANT varName;
				VariantInit(&varName);
				hr = pPropBag->Read(L"FriendlyName", &varName, 0);
				if (SUCCEEDED(hr)) {
					wprintf(L"Selecting Audio Input Device: %s\n",
						varName.bstrVal);
				}
				VariantClear(&varName);
				hr = pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter,
					gottaFilter);
				pPropBag->Release();
			}
			pMoniker->Release();
		}
		pEnumCat->Release();
	}
	pSysDevEnum->Release();
	return hr;
}

HRESULT EnumerateAudioInputPins(IBaseFilter *pFilter) {
	IEnumPins *pEnum = NULL;
	IPin *pPin = NULL;
	PIN_DIRECTION PinDirThis;
	PIN_INFO pInfo;
	IAMAudioInputMixer *pAMAIM = NULL;
	BOOL pfEnable = FALSE;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr)) {
		return NULL;
	}
	while (pEnum->Next(1, &pPin, 0) == S_OK) {
		pPin->QueryDirection(&PinDirThis);
		if (PinDirThis == PINDIR_INPUT) {
			hr = pPin->QueryPinInfo(&pInfo);
			if (SUCCEEDED(hr)) {
				wprintf(L"Input pin: %s\n", pInfo.achName);
				hr = pPin->QueryInterface(IID_IAMAudioInputMixer,
					(void**)&pAMAIM);
				if (SUCCEEDED(hr)) {
					hr = pAMAIM->get_Enable(&pfEnable);
					if (SUCCEEDED(hr)) {
						if (pfEnable) {
							wprintf(L"\tENABLED\n");
						}
					}
					pAMAIM->Release();
				}
				pInfo.pFilter->Release();
			}
		}
		pPin->Release();
	}
	pEnum->Release();
	return hr;
}

HRESULT GetUnconnectedPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir, IPin **ppPin) {
	*ppPin = 0;
	IEnumPins *pEnum = 0;
	IPin *pPin = 0;
	HRESULT hr = pFilter->EnumPins(&pEnum);
	if (FAILED(hr)) {
		return hr;
	}
	while (pEnum->Next(1, &pPin, NULL) == S_OK) {
		PIN_DIRECTION ThisPinDir;
		pPin->QueryDirection(&ThisPinDir);
		if (ThisPinDir == PinDir) {
			IPin *pTmp = 0;
			hr = pPin->ConnectedTo(&pTmp);
			if (SUCCEEDED(hr)) { 
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

HRESULT ConnectFilters(
	IGraphBuilder *pGraph, 
	IPin *pOut, 
	IBaseFilter *pDest) { 
	if ((pGraph == NULL) || (pOut == NULL) || (pDest == NULL)) {
		return E_POINTER;
	}
	IPin *pIn = 0;
	HRESULT hr = GetUnconnectedPin(pDest, PINDIR_INPUT, &pIn);
	if (FAILED(hr)) {
		return hr;
	}
	hr = pGraph->Connect(pOut, pIn);
	pIn->Release();
	return hr;
}

HRESULT ConnectFilters(	IGraphBuilder *pGraph,	IBaseFilter *pSrc,	IBaseFilter *pDest) {
	if ((pGraph == NULL) || (pSrc == NULL) || (pDest == NULL)) {
		return E_POINTER;
	}
	IPin *pOut = 0;
	HRESULT hr = GetUnconnectedPin(pSrc, PINDIR_OUTPUT, &pOut);
	if (FAILED(hr)) {
		return hr;
	}
	hr = ConnectFilters(pGraph, pOut, pDest);
	pOut->Release();
	return hr;
}

HRESULT AddFilterByCLSID(IGraphBuilder *pGraph,	const GUID& clsid,	LPCWSTR wszName,	IBaseFilter **ppF) {
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
