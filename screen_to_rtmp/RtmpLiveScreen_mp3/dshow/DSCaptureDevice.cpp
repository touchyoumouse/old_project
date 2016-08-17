#include "stdafx.h"
#include "DSCaptureDevice.h"

void DSCaptureDevice::ListCapDevices(const IID& deviceIID, std::map<CString, CString>& deviceList)
{
    CComPtr<IEnumMoniker> enum_moniker;

    CComPtr<ICreateDevEnum> create_dev_enum;
    create_dev_enum.CoCreateInstance(CLSID_SystemDeviceEnum);
    create_dev_enum->CreateClassEnumerator(deviceIID, &enum_moniker, 0);

    if (!enum_moniker)
    {
        return;
    }

    enum_moniker->Reset();
    deviceList.clear();

    UINT index = 0;
    while (true)
    {
        CComPtr<IMoniker> moniker;

        ULONG ulFetched = 0;
        HRESULT hr = enum_moniker->Next(1, &moniker, &ulFetched);
        if(hr != S_OK)
        {
            break;
        }

        CComPtr< IPropertyBag > pBag;
        hr = moniker->BindToStorage( 0, 0, IID_IPropertyBag, (void**) &pBag );
        if( hr != S_OK )
        {
            continue;
        }

        CComVariant var;
        var.vt = VT_BSTR;
        pBag->Read( L"FriendlyName", &var, NULL );

        LPOLESTR wszDeviceID;
        moniker->GetDisplayName( 0, NULL, &wszDeviceID );

        deviceList[wszDeviceID] = var.bstrVal;

        index++;
    }

    create_dev_enum.Release();
    enum_moniker.Release();
}

DSCaptureDevice::DSCaptureDevice()
{

}

DSCaptureDevice::~DSCaptureDevice()
{

}
