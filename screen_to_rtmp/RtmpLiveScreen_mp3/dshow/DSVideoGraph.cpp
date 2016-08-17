#include "stdafx.h"
#include "DSVideoGraph.h"
#include "baseclasses/mtype.h"

DSVideoGraph::DSVideoGraph()
{
    active_video_window_ = NULL;
}

DSVideoGraph::~DSVideoGraph()
{
    Destroy();
}

HRESULT DSVideoGraph::Create(DSVideoCaptureDevice* videoCaptureDevice)
{
    HRESULT hr;

    video_cap_device_ = videoCaptureDevice;

    if (video_cap_device_->GetID().IsEmpty())
    {
        return 0;
    }

    SetCapDevice(CLSID_VideoInputDeviceCategory, video_cap_device_->GetID());

    SetVideoFormat(video_cap_device_->GetPreferredVideoWidth(), 
        video_cap_device_->GetPreferredVideoHeight(),
        video_cap_device_->GetPreferredVideoFPS());

    if (FAILED(graph_->AddFilter(source_filter_, L"Source" )))
        return -1;

    if (FAILED(graph_->AddFilter(grabber_filter_, L"Grabber")))
        return -1;

    CMediaType GrabType;
    GrabType.SetType(&MEDIATYPE_Video);
    GrabType.SetSubtype(&MEDIASUBTYPE_RGB24);
    if (FAILED(sample_grabber_->SetMediaType( &GrabType)))
        return -1;

    IPin* source_pin = GetOutPin(source_filter_, 0);
    IPin* grabber_pin = GetInPin(grabber_filter_, 0);

    if (FAILED(graph_->Connect(source_pin, grabber_pin)))
        return -1;

    AM_MEDIA_TYPE mt;
    if (FAILED(sample_grabber_->GetConnectedMediaType(&mt)))
        return -1;

    VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*)mt.pbFormat;
    video_cap_device_->SetVideoWidth(vih->bmiHeader.biWidth);
    video_cap_device_->SetVideoHeight(vih->bmiHeader.biHeight);
    //video_cap_device_->SetVideoFPS(10000000/vih->AvgTimePerFrame);
    FreeMediaType(mt);

    IPin *grabber_out_pin = GetOutPin(grabber_filter_, 0);
    if (FAILED(graph_->Render( grabber_out_pin )))
        return -1;

    if (FAILED(sample_grabber_->SetBufferSamples(FALSE)))
        return -1;
    sample_grabber_->SetOneShot(FALSE);

    if (FAILED(sample_grabber_->SetCallback((ISampleGrabberCB*)&grabber_callback_, 1)))
        return -1;

    if (FAILED(graph_->QueryInterface(IID_IVideoWindow, (void **)&active_video_window_)))
        return -1;

    if (active_video_window_)
    {    
        if (FAILED(active_video_window_->put_AutoShow(OAFALSE)))
            return -1;
    }

    if (FAILED(graph_->QueryInterface(IID_IMediaControl, (void **)&media_control_)))
        return 0;

    if (FAILED(graph_->QueryInterface(IID_IMediaEvent, (void **)&media_event_)))
        return 0;

    SAFE_RELEASE(source_pin);
    SAFE_RELEASE(grabber_pin);
    SAFE_RELEASE(grabber_out_pin);

    is_create_ok_ = true;

    return 0;
}

void DSVideoGraph::Destroy()
{
    if (active_video_window_) active_video_window_->Release();
}

HRESULT DSVideoGraph::SetVideoFormat(UINT preferredImageWidth, UINT preferredImageHeight, 
    REFERENCE_TIME preferredFPS)
{
    IAMStreamConfig *pConfig = NULL;

    HRESULT hRet;
    if (FAILED(hRet = capture_builder_->FindInterface(&PIN_CATEGORY_CAPTURE, &MEDIATYPE_Video, 
        source_filter_, IID_IAMStreamConfig, (void**)&pConfig)))
    {
        return -1;
    }

    int iCount = 0, iSize  = 0;
    if (FAILED(pConfig->GetNumberOfCapabilities(&iCount, &iSize)))
        return -1;

    bool find_fmt = false;
    if (iSize == sizeof(VIDEO_STREAM_CONFIG_CAPS))
    {
        for (int iFormat = 0; iFormat < iCount; iFormat++)
        {
            VIDEO_STREAM_CONFIG_CAPS scc;
            AM_MEDIA_TYPE *pmtConfig;
            HRESULT hr = pConfig->GetStreamCaps(iFormat, &pmtConfig, (BYTE*)&scc);

            if (SUCCEEDED(hr))
            {
                if ((pmtConfig->majortype == MEDIATYPE_Video) &&
                    /*(pmtConfig->subtype == MEDIASUBTYPE_RGB24) &&*/
                    (pmtConfig->formattype == FORMAT_VideoInfo) &&
                    (pmtConfig->cbFormat >= sizeof (VIDEOINFOHEADER)) &&
                    (pmtConfig->pbFormat != NULL))
                {
                    VIDEOINFOHEADER *pVih = (VIDEOINFOHEADER*)pmtConfig->pbFormat;

                    LONG lWidth  = pVih->bmiHeader.biWidth;
                    LONG lHeight = pVih->bmiHeader.biHeight;

                    if ((lWidth == preferredImageWidth) && (lHeight == preferredImageHeight))
                    {
                        find_fmt = true;

                        hr = pConfig->SetFormat(pmtConfig);
                        break;
                    }
                }

                DeleteMediaType(pmtConfig);
            }
        }
    }

    SAFE_RELEASE(pConfig);

    return 0;
}

void DSVideoGraph::AdjustVideoWindow(OAHWND owner, unsigned int width, unsigned int height)
{
    if (active_video_window_)
    {
        active_video_window_->put_Visible(OAFALSE);
        active_video_window_->put_Owner(NULL);

        active_video_window_->put_Owner(owner);
        active_video_window_->put_WindowStyle(WS_CHILD | WS_CLIPCHILDREN);
        active_video_window_->put_Visible(OATRUE);
        active_video_window_->SetWindowPosition(0, 0, width, height);
    }
}
