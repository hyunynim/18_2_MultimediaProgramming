#include<cstdio>
#include<dshow.h>
#include<string>
using namespace std;
const string filepath = "D:\\Education\\Media\\passat.mpg";

int main() {
	IGraphBuilder *pGraph = 0;
	IMediaControl *pControl = 0;
	IMediaEvent *pEvent = 0;

	HRESULT hr = CoInitialize(0);
	if (FAILED(hr)) {
		printf("ERROR - Coould not initialize COM library");
		return 0;
	}
	hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
		IID_IGraphBuilder, (void **)&pGraph);
	if (FAILED(hr)) {
		printf("ERROR - Could not create the Filter Graph Manager.\n");
		return 0;
	}
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);

	wstring stemp = wstring(filepath.begin(), filepath.end());
	LPCWSTR path = stemp.c_str();
	hr = pGraph->RenderFile(path, 0);

	if (SUCCEEDED(hr)) {
		hr = pControl->Run();
		if (SUCCEEDED(hr)) {
			long evCode;
			pEvent->WaitForCompletion(INFINITE, &evCode);
		}
	}
	else {
		printf("ERROR - Could not find the media file.\n");
		return 0;
	}
	pControl->Release();
	pEvent->Release();
	pGraph->Release();
	CoUninitialize();
	return 0;
}