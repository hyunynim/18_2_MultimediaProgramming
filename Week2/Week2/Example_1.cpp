#include <stdio.h>
#include <dshow.h>
#define F(x) FAILED(x)
#define S(x) SUCCEEDED(x)
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

HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath) {
	const WCHAR wszStreamName[] = L"ActiveMovieGraph";
	HRESULT hr;
	IStorage *pStorage = NULL;

	hr = StgCreateDocfile(wszPath, STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStorage);

	if (F(hr)) {
		return hr;
	}

	IStream *pStream;

	hr = pStorage->CreateStream(wszStreamName, STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE, 0, 0, &pStream);

	if (F(hr)) {
		pStorage->Release();

		return hr;
	}

	IPersistStream *pPersist = NULL;

	pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void **>(&pPersist));

	hr = pPersist->Save(pStream, TRUE);

	pStream->Release();
	pPersist->Release();

	if (S(hr)) {
		hr = pStorage->Commit(STGC_DEFAULT);
	}

	pStorage->Release();

	return hr;
}

IPin *GetPin(IBaseFilter *pFilter, PIN_DIRECTION PinDir) {
	BOOL bFound = FALSE;
	IEnumPins *pEnum;
	IPin *pPin;

	HRESULT hr = pFilter->EnumPins(&pEnum);

	if (F(hr)) {
		return NULL;
	}

	while (pEnum->Next(1, &pPin, 0) == S_OK) {
		PIN_DIRECTION PinDirThis;
		pPin->QueryDirection(&PinDirThis);

		if (bFound = (PinDir == PinDirThis)) {
			break;
		}

		pPin->Release();
	}

	pEnum->Release();

	return (bFound ? pPin : NULL);
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

int main() {
	IGraphBuilder *pGraph = 0;
	IMediaControl *pControl = 0;
	IMediaEvent *pEvent = 0;

	IBaseFilter *InputFileFilter = 0;
	IBaseFilter *StreamSplitterFilter = 0;
	IBaseFilter *VideoDecoderFilter = 0;
	IBaseFilter *AudioDecoderFilter = 0;
	IBaseFilter *VideoRendererFilter = 0;
	IBaseFilter *AudioRendererFilter = 0;

	IPin *FileOutputPin = 0;
	IPin *StreamSplitterInputPin = 0;
	IPin *StreamSplitterOutputPin = 0;
	IPin *VideoDecoderInputPin = 0;
	IPin *VideoDecoderOutputPin = 0;
	IPin *AudioDecoderInputPin = 0;
	IPin *AudioDecoderOutputPin = 0;
	IPin *VideoRendererInputPin = 0;
	IPin *AudioRendererInputPin = 0;

	HRESULT hr = CoInitialize(0);

	if (F(hr)) {
		printf("ERROR - Could not initialize COM library.\n");
		return hr;
	}

	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);

	if (F(hr)) {
		printf("ERROR - Could not create the Filter Graph Manager.\n");
		return hr;
	}

	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);

	if (F(hr)) {
		printf("ERROR - Could not create the Media Control object.\n");

		pGraph->Release();
		CoUninitialize();
		return hr;
	}

	hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);

	if (F(hr)) {
		printf("ERROR - Could not create the Media Event object.\n");

		pGraph->Release();
		pControl->Release();
		CoUninitialize();

		return hr;
	}

	if (!GetMediaFileName()) return 0;

#ifndef UNICODE
	WCHAR wFileName[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, g_pathFileName, -1, wFileName, MAX_PATH);
	hr = pGraph->AddSourceFilter(wFileName, wFileName, &InputFileFilter);

#else
	hr = pGraph->AddSourceFilter(wFileName, wFileName, &InputFileFilter);
#endif

	if (S(hr)) {
		hr = CoCreateInstance(CLSID_MPEG1Splitter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&StreamSplitterFilter);

		if (S(hr)) {
			hr = pGraph->AddFilter(StreamSplitterFilter, L"Stream Splitter");
			
			if (S(hr)
				&& S(GetUnConnectedPin(InputFileFilter, PINDIR_OUTPUT, &FileOutputPin))
				&& S(GetUnConnectedPin(StreamSplitterFilter, PINDIR_INPUT, &StreamSplitterInputPin))) {
					hr = pGraph->Connect(FileOutputPin, StreamSplitterInputPin);
				

				hr = CoCreateInstance(CLSID_CMpegVideoCodec, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&VideoDecoderFilter);

				if (S(hr)) {
					hr = pGraph->AddFilter(VideoDecoderFilter, L"Video Decoder");

					if (S(hr)
						&& S(GetUnConnectedPin(StreamSplitterFilter, PINDIR_OUTPUT, &StreamSplitterOutputPin)) 
						&& S(GetUnConnectedPin(VideoDecoderFilter, PINDIR_INPUT, &VideoDecoderInputPin))) {
							hr = pGraph->Connect(StreamSplitterOutputPin, VideoDecoderInputPin);

						hr = CoCreateInstance(CLSID_VideoRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&VideoRendererFilter);

						if (S(hr)) {
							hr = pGraph->AddFilter(VideoRendererFilter, L"Video Renderer");

							if (S(hr) 
								&& S(GetUnConnectedPin(VideoDecoderFilter, PINDIR_OUTPUT, &VideoDecoderOutputPin)) 
								&& S(GetUnConnectedPin(VideoRendererFilter, PINDIR_INPUT, &VideoRendererInputPin))) {
									hr = pGraph->Connect(VideoDecoderOutputPin, VideoRendererInputPin);
							}
						}
					}
				}

				hr = CoCreateInstance(CLSID_CMpegAudioCodec, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&AudioDecoderFilter);

				if (S(hr)) {
					hr = pGraph->AddFilter(AudioDecoderFilter, L"Audio Decoder");

					if (S(hr)
						&& S(GetUnConnectedPin(StreamSplitterFilter, PINDIR_OUTPUT, &StreamSplitterOutputPin)) 
						&& S(GetUnConnectedPin(AudioDecoderFilter, PINDIR_INPUT, &AudioDecoderInputPin))) {
							hr = pGraph->Connect(StreamSplitterOutputPin, AudioDecoderInputPin);

						hr = CoCreateInstance(CLSID_AudioRender, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void **)&AudioRendererFilter);

						if (S(hr)) {
							hr = pGraph->AddFilter(AudioRendererFilter, L"Video Renderer");

							if (S(hr)
								&& S(GetUnConnectedPin(AudioDecoderFilter, PINDIR_OUTPUT, &AudioDecoderOutputPin)) 
								&& S(GetUnConnectedPin(AudioRendererFilter, PINDIR_INPUT, &AudioRendererInputPin))) {
									hr = pGraph->Connect(AudioDecoderOutputPin, AudioRendererInputPin);
							}
						}
					}
				}
			}
		}

		hr = pControl->Run();

		if (S(hr)) {
			long evCode;
			pEvent->WaitForCompletion(INFINITE, &evCode);
		}

		hr = pControl->Stop();
		WCHAR sPath[] = L"D:\\Develop\\18_2_MultimediaProgramming\\Week2\\MyGraph.grf";
		SaveGraphFile(pGraph, sPath);
	}

	else 
		printf("ERROR - Could not find the media fila.\n");

	if (FileOutputPin) 
		FileOutputPin->Release();

	if (StreamSplitterFilter) 
		StreamSplitterFilter->Release();
	

	if (VideoDecoderFilter) 
		VideoDecoderFilter->Release();
	

	if (AudioDecoderFilter) 
		AudioDecoderFilter->Release();
	
	if (VideoRendererFilter) 
		VideoRendererFilter->Release();
	

	if (AudioRendererFilter) 
		AudioRendererFilter->Release();
	
	if (FileOutputPin) 
		FileOutputPin->Release();
	
	if (StreamSplitterInputPin) 
		StreamSplitterInputPin->Release();
	

	if (StreamSplitterOutputPin) 
		StreamSplitterOutputPin->Release();

	if (VideoDecoderInputPin) 
		VideoDecoderInputPin->Release();
	

	if (VideoDecoderOutputPin) 
		VideoDecoderOutputPin->Release();

	if (AudioDecoderInputPin) 
		AudioDecoderInputPin->Release();

	if (AudioDecoderOutputPin) 
		AudioDecoderOutputPin->Release();

	if (VideoRendererInputPin) 
		VideoRendererInputPin->Release();
	
	if (AudioRendererInputPin) 
		AudioRendererInputPin->Release();
	
	pControl->Release();
	pEvent->Release();
	pGraph->Release();
	CoUninitialize();
}