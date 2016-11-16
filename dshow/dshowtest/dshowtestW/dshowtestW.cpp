// dshowtestW.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <dshow.h>




int _tmain(int argc, _TCHAR* argv[])
{
	//playing the file;
	HRESULT hr = CoInitialize(NULL);
	if (FAILED(hr))
	{
		return 0;
	}

	IGraphBuilder *pGraph;
	hr = CoCreateInstance(CLSID_FilterGraph, NULL,
		CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&pGraph);

	IMediaControl *pControl;
	IMediaEvent   *pEvent;
	hr = pGraph->QueryInterface(IID_IMediaControl, (void **)&pControl);
	hr = pGraph->QueryInterface(IID_IMediaEvent, (void **)&pEvent);
	hr = pGraph->RenderFile(L"E:\\VMBak\\courseSection0000100527_av.avi", NULL);
	if (SUCCEEDED(hr))
	{
		// Run the graph.
		hr = pControl->Run();
		if (SUCCEEDED(hr))
		{
			// Wait for completion.
			long evCode;
			pEvent->WaitForCompletion(INFINITE, &evCode);
			// Note: Do not use INFINITE in a real application, because it
			// can block indefinitely.
		}
	}
	pControl->Release();
	pEvent->Release();
	pGraph->Release();
	CoUninitialize();
	return 0;
}

