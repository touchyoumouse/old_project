
// RtmpStreamingDlg.cpp : implementation file
//

#include "stdafx.h"
#include "RtmpLiveScreenApp.h"
#include "RtmpLiveScreenDlg.h"
#include "afxdialogex.h"

#include "FlvReader.h"
#include "LibRtmp.h"

#include "dshow/DSCapture.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include <fstream>

// RtmpStreamingDlg dialog



RtmpLiveScreenDlg::RtmpLiveScreenDlg(bool isShowWindow, const std::string& cfgPath, CWnd* pParent /*=NULL*/)
	: CDialog(RtmpLiveScreenDlg::IDD, pParent)
	, edit_cap(_T(""))
{
	is_show_window_ = isShowWindow;
	config_path_ = cfgPath;

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	WORD version;  
	WSADATA wsaData;  
	version = MAKEWORD(1, 1);
	WSAStartup(version, &wsaData);

	rtmp_live_screen_ = NULL;
	webcam_bitrate_ = 80;
	screen_bitrate_ = 350;
	video_change_millsecs_ = 3000;
	//
	i_waittime_ = 0;
	show_window_ = 0;
}

RtmpLiveScreenDlg::~RtmpLiveScreenDlg()
{

    WSACleanup();

    CoUninitialize();
}

void RtmpLiveScreenDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CB_VIDEO, cb_select_video_);
	DDX_Control(pDX, IDC_CB_AUDIO, cb_select_audio_);
	DDX_Control(pDX, IDC_BTN_START, btn_start_);
	DDX_Control(pDX, IDC_BTN_STOP, btn_stop_);
	DDX_Control(pDX, IDC_EDIT_SCREEN_WH, edit_screen_wh_);
	DDX_Control(pDX, IDC_EDIT_X, edit_x_);
	DDX_Control(pDX, IDC_EDIT_Y, edit_y_);
	DDX_Control(pDX, IDC_CB_CAP_WH, cb_cap_wh_);
	DDX_Control(pDX, IDC_CHECK_RECORD, check_record_);
	DDX_Control(pDX, IDC_CHECK_WEBCAM, check_webcam_);
	DDX_Control(pDX, IDC_CHECK_SCREEN, check_screen_);

	DDX_Control(pDX, IDC_BTN_UP, btn_up_);
	DDX_Control(pDX, IDC_BTN_DOWN, btn_down_);
	DDX_Control(pDX, IDC_BTN_LEFT, btn_left_);
	DDX_Control(pDX, IDC_BTN_RIGHT, btn_right_);
	DDX_Control(pDX, IDC_SLIDER_H, slider_ctrl_h_);
	DDX_Control(pDX, IDC_SLIDER_V, slider_ctrl_v_);
	DDX_Text(pDX, IDC_EDIT1, edit_cap);
	DDX_Control(pDX, IDC_EDIT1, edit_Ckbps);
	//  DDX_Text(pDX, IDC_EDIT_WAITTIME, m_edit_waittime);
	DDX_Control(pDX, IDC_EDIT_WAITTIME, m_edit_waittime);
	DDX_Control(pDX, IDC_CHECK_ATUOPUBLISH, m_check_autopublish);
}

BEGIN_MESSAGE_MAP(RtmpLiveScreenDlg, CDialog)
	ON_WM_PAINT()
    ON_WM_NCPAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_HSCROLL()
    ON_WM_VSCROLL()
    ON_BN_CLICKED(IDC_BTN_START, &RtmpLiveScreenDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDC_BTN_STOP, &RtmpLiveScreenDlg::OnBnClickedBtnStop)
    ON_MESSAGE(WM_FRAME, &RtmpLiveScreenDlg::OnNewFrame)
    ON_WM_CTLCOLOR()
    ON_BN_CLICKED(IDC_CHECK_WEBCAM, &RtmpLiveScreenDlg::OnBnClickedCheckWebcam)
    ON_BN_CLICKED(IDC_BTN_UP, &RtmpLiveScreenDlg::OnBnClickedBtnUp)
    ON_BN_CLICKED(IDC_BTN_LEFT, &RtmpLiveScreenDlg::OnBnClickedBtnLeft)
    ON_BN_CLICKED(IDC_BTN_RIGHT, &RtmpLiveScreenDlg::OnBnClickedBtnRight)
    ON_BN_CLICKED(IDC_BTN_DOWN, &RtmpLiveScreenDlg::OnBnClickedBtnDown)
    ON_CBN_SELCHANGE(IDC_CB_CAP_WH, &RtmpLiveScreenDlg::OnCbnSelchangeCbCapWh)
	ON_STN_CLICKED(IDC_STATIC_C_3, &RtmpLiveScreenDlg::OnStnClickedStaticC3)
	ON_BN_CLICKED(IDC_BUTTON_yingyong, &RtmpLiveScreenDlg::OnBnClickedButtonyingyong)
	ON_WM_SYSCOMMAND()
	ON_MESSAGE(WM_SHOWTASK, &RtmpLiveScreenDlg::OnShowtask)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// RtmpStreamingDlg message handlers

BOOL RtmpLiveScreenDlg::OnInitDialog()
{
	
	CDialog::OnInitDialog();
	//this->ModifyStyleEx(WS_EX_APPWINDOW, 0); 
	PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	ShowInTaskbar(*this,false);
	//::SetWindowLong ( GetSafeHwnd () , GWL_EXSTYLE , WS_EX_TOOLWINDOW );
	//ModifyStyleEx(WS_EX_APPWINDOW,WS_EX_TOOLWINDOW,1);
	
	//ModifyStyleEx(0, WS_EX_APPWINDOW); 
	//ModifyStyle(WS_CAPTION, 0);

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

    SetWindowText(_T("Rtmp桌面直播台"));

    // 字体设置

    LOGFONT logFont;
    memset(&logFont,0,sizeof(LOGFONT));
    logFont.lfHeight = 13;
    logFont.lfWeight = FW_BOLD; 
    logFont.lfCharSet = GB2312_CHARSET;
    wcscpy(logFont.lfFaceName, _T("幼圆"));
    label_font_.CreateFontIndirect(&logFont);
    GetDlgItem(IDC_STATIC_1)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_2)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_3)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_4)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_5)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_9)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_10)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_C_1)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_C_2)->SetFont(&label_font_);
    GetDlgItem(IDC_STATIC_C_3)->SetFont(&label_font_);
	GetDlgItem(IDC_STATIC_C_4)->SetFont(&label_font_);

	GetDlgItem(IDC_STATIC_F1)->SetFont(&label_font_);
	GetDlgItem(IDC_STATIC_F2)->SetFont(&label_font_);

    btn_start_.SetFont(&label_font_);
    btn_stop_.SetFont(&label_font_);
    check_record_.SetFont(&label_font_);
    check_webcam_.SetFont(&label_font_);
    check_screen_.SetFont(&label_font_);
	m_check_autopublish.SetFont(&label_font_);

    LoadConfig();

    // 音视频设备
	
    std::map<CString, CString> a_devices, v_devices;
    DSCapture::ListAudioCapDevices(a_devices);
    DSCapture::ListVideoCapDevices(v_devices);

    for (std::map<CString, CString>::iterator it = a_devices.begin();
        it != a_devices.end(); ++it)
    {
        cb_select_audio_.AddString(it->second);
        audio_device_index_.push_back(it->first);
    }
    cb_select_audio_.SetCurSel(0);

    for (std::map<CString, CString>::iterator it = v_devices.begin();
        it != v_devices.end(); ++it)
    {
        cb_select_video_.AddString(it->second);
        video_device_index_.push_back(it->first);
    }
    cb_select_video_.SetCurSel(0);

    // 屏幕分辨率
    HDC hscreen = ::GetDC(NULL);
    screen_width_ = ::GetDeviceCaps(hscreen, HORZRES);
    screen_height_ = ::GetDeviceCaps(hscreen, VERTRES);
    ::DeleteDC(hscreen);
    CString whstr;
    whstr.Format(_T("宽:%d  高:%d"), screen_width_, screen_height_);
    edit_screen_wh_.SetWindowText(whstr);

    //
    cb_cap_wh_.AddString(_T("704x576"));
    cb_cap_wh_.AddString(_T("352x288"));
    cb_cap_wh_.AddString(_T("320x240"));
    cb_cap_wh_.AddString(_T("176x144"));

    btn_start_.SetFocus();
    btn_stop_.EnableWindow(FALSE);

    //btn_start_.DrawTransparent();
    //btn_stop_.DrawTransparent();
    btn_start_.SetWindowText(_T("开始"));
    btn_stop_.SetWindowText(_T("结束"));

    //check_webcam_.DrawTransparent();
    //check_record_.DrawTransparent();
    //check_screen_.DrawTransparent();
    slider_ctrl_h_.DrawTransparent(TRUE);
    slider_ctrl_v_.DrawTransparent(TRUE);

    // 控制按钮
	//CString cs_waittime_ = cs_waittime_.Format(_T("%d"),i_waittime_);
	char c_waittime[128] = {0};
	sprintf(c_waittime,"%d",i_waittime_);
	CString cs_waittime_ = CString(c_waittime); 
	m_edit_waittime.SetWindowTextW(cs_waittime_);
    //btn_up_.SetTooltipText(_T("向上移动"));
    //btn_up_.SetFlat(FALSE);
    btn_up_.DrawTransparent();
    //btn_up_.SetBitmaps(IDB_BITMAP_LEFT, RGB(255, 255, 255));
    btn_up_.SetIcon(IDI_ICON_UP);
    btn_down_.DrawTransparent();
    btn_down_.SetIcon(IDI_ICON_DOWN);
    btn_left_.DrawTransparent();
    btn_left_.SetIcon(IDI_ICON_LEFT);
    btn_right_.DrawTransparent();
    btn_right_.SetIcon(IDI_ICON_RIGHT);

    gFrameWnd = this->GetSafeHwnd();

    UpdateGui();

    if (1)
    {
        hloc_ = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE,
            sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * 256));
        bmpinfo_ = (LPBITMAPINFO) GlobalLock(hloc_);

        HANDLE hloc1;
        RGBQUAD *argbq;

        hloc1 = LocalAlloc(LMEM_ZEROINIT | LMEM_MOVEABLE,(sizeof(RGBQUAD) * 256));
        argbq = (RGBQUAD *) LocalLock(hloc1);

        for(int i=0;i<256;i++) 
		{
            argbq[i].rgbBlue=i;
            argbq[i].rgbGreen=i;
            argbq[i].rgbRed=i;
            argbq[i].rgbReserved=0;
        }

        bmpinfo_->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmpinfo_->bmiHeader.biPlanes = 1;
        bmpinfo_->bmiHeader.biBitCount = 24;
        bmpinfo_->bmiHeader.biCompression = BI_RGB;
        bmpinfo_->bmiHeader.biWidth = 320;
        bmpinfo_->bmiHeader.biHeight = 240;

        memcpy(bmpinfo_->bmiColors, argbq, sizeof(RGBQUAD) * 256);

        LocalUnlock(hloc1);
        LocalFree(hloc1);
    }

	//@feng AUTO publishing;
	//is_show_window_ = m_check_autopublish.GetCheck();
	//show_window_ = m_check_autopublish.GetCheck();

    if (show_window_/*false == is_show_window_*/)
    {
      //  ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);
        OnBnClickedBtnStart();
    }

	
	//SetTimer();
	return FALSE;  // return TRUE  unless you set the focus to a control
}

HBRUSH RtmpLiveScreenDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
    HBRUSH   hbr   =   CDialog::OnCtlColor(pDC,   pWnd,   nCtlColor);

    switch(pWnd->GetDlgCtrlID()) 
    { 
    case IDC_STATIC_1:
    case IDC_STATIC_2:
    case IDC_STATIC_3:
    case IDC_STATIC_4:
    case IDC_STATIC_5:
    case IDC_STATIC_6:
    case IDC_STATIC_9:
    case IDC_STATIC_10:
	case IDC_STATIC_11:
    case IDC_STATIC_C_1:
    case IDC_STATIC_C_2:
    case IDC_STATIC_C_3:
	case IDC_STATIC_C_4:
	case IDC_STATIC_F1:
	case IDC_STATIC_F2:
	
        pDC->SetBkMode(TRANSPARENT); 
        //pDC->SetTextColor(RGB(0,0,0)); 
        return (HBRUSH)GetStockObject(HOLLOW_BRUSH); 
    default: break; 
    } 

    //   TODO:   Return   a   different   brush   if   the   default   is   not   desired 
    return   hbr; 
}

void RtmpLiveScreenDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
    CDialog::OnHScroll(nSBCode, nPos, pScrollBar);

    webcam_x_ = slider_ctrl_h_.GetPos();
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_x_);
    edit_x_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }
}

void RtmpLiveScreenDlg::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
    CDialog::OnVScroll(nSBCode, nPos, pScrollBar);

    webcam_y_ = slider_ctrl_v_.GetPos();
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_y_);
    edit_y_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }
}

void RtmpLiveScreenDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		//CDialog::OnPaint();
        CPaintDC dc(this);
        CRect rc;
        GetClientRect(&rc);
        CDC dcMem;
        dcMem.CreateCompatibleDC(&dc);
        CBitmap bmpBackground;
        bmpBackground.LoadBitmap(IDB_BITMAP1);

        BITMAP bitmap;
        bmpBackground.GetBitmap(&bitmap);
        CBitmap* pbmpPri = dcMem.SelectObject(&bmpBackground);
        //dc.StretchBlt(0,0,rc.Width(), rc.Height(), &dcMem,0,0,bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
        dc.BitBlt(0,0,rc.Width(),rc.Height(),&dcMem,0,0,SRCCOPY);
        ReleaseDC(&dcMem);
	}
}

void RtmpLiveScreenDlg::OnNcPaint()
{
    if (is_show_window_)
    {
        CDialog::OnNcPaint();
    }
    else
    {
        this->ShowWindow(SW_HIDE);
    }
}

LRESULT RtmpLiveScreenDlg::OnNewFrame(WPARAM wParam, LPARAM lParam)
{
    unsigned char* frame_buf = (unsigned char*)lParam;
    unsigned int* buf_len = (unsigned int*)wParam;
    if (frame_buf && buf_len)
    {
        CDC* video_dc = GetDlgItem(IDC_VIDEO_PRIVIEW)->GetDC();
        video_dc->SetStretchBltMode(STRETCH_DELETESCANS);

        CRect video_rect;
        GetDlgItem(IDC_VIDEO_PRIVIEW)->GetClientRect(video_rect);
        video_dc->SetStretchBltMode(STRETCH_DELETESCANS);
        StretchDIBits(video_dc->m_hDC,
            0, 0, video_rect.Width(), video_rect.Height(),
            0, 0, bmpinfo_->bmiHeader.biWidth, bmpinfo_->bmiHeader.biHeight,
            frame_buf, bmpinfo_, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(video_dc);

        GetDlgItem(IDC_VIDEO_PRIVIEW)->Invalidate(FALSE);
    }
    delete buf_len;
    delete[] frame_buf;
    return 0;
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR RtmpLiveScreenDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static std::string CStringToString(const CString& mfcStr)
{
    CT2CA pszConvertedAnsiString(mfcStr);
    return (pszConvertedAnsiString);
}

void RtmpLiveScreenDlg::OnBnClickedBtnStart()
{
    if (false == UpdateValue())
    {
        return;
    }

    if (false == is_cap_screen_)
    {
        check_webcam_.EnableWindow(FALSE);
        edit_x_.EnableWindow(FALSE);
        edit_y_.EnableWindow(FALSE);
        slider_ctrl_h_.EnableWindow(FALSE);
        slider_ctrl_v_.EnableWindow(FALSE);
		//edit_Ckbps.EnableWindow(FALSE);
    }

    cb_select_audio_.EnableWindow(FALSE);
    cb_select_video_.EnableWindow(FALSE);
    cb_cap_wh_.EnableWindow(FALSE);
    check_record_.EnableWindow(FALSE);
    check_screen_.EnableWindow(FALSE);

    CString audio_device_id = _T("");
    if (audio_device_index_.size())
    {
        audio_device_id = audio_device_index_[cb_select_audio_.GetCurSel()];
    }
    CString video_device_id = _T("");
    if (video_device_index_.size() && check_webcam_.GetCheck())
    {
        video_device_id = video_device_index_[cb_select_video_.GetCurSel()];
    }

    CWnd *pWnd = GetDlgItem(IDC_VIDEO_PRIVIEW);
    CRect rc;
    pWnd->GetClientRect(&rc);

    btn_start_.EnableWindow(FALSE);
    btn_stop_.EnableWindow(TRUE);

    rtmp_live_screen_ = new RtmpLiveScreen(audio_device_id, video_device_id, webcam_width_, webcam_height_, 
        (OAHWND)pWnd->GetSafeHwnd(), rc.right, rc.bottom, rtmp_url_,
        is_cap_screen_, is_need_record_, webcam_x_, webcam_y_, webcam_width_, webcam_height_,
        webcam_bitrate_, screen_bitrate_, video_change_millsecs_,i_waittime_);
    rtmp_live_screen_->SetCaptureWebcam(is_cap_webcam_);
    rtmp_live_screen_->Start();
}


void RtmpLiveScreenDlg::OnBnClickedBtnStop()
{
    btn_start_.EnableWindow(TRUE);
    btn_stop_.EnableWindow(FALSE);
    check_screen_.EnableWindow(TRUE);
    if (false == video_device_index_.empty())
    {
        check_webcam_.EnableWindow(TRUE);
    }
    check_record_.EnableWindow(TRUE);
    cb_cap_wh_.EnableWindow(TRUE);
    cb_select_audio_.EnableWindow(TRUE);
    cb_select_video_.EnableWindow(TRUE);

    if (check_webcam_.GetCheck())
    {
        edit_x_.EnableWindow(TRUE);
        edit_y_.EnableWindow(TRUE);
		//edit_Ckbps.EnableWindow(TRUE);
        slider_ctrl_h_.EnableWindow(TRUE);
        slider_ctrl_v_.EnableWindow(TRUE);
    }

    rtmp_live_screen_->Stop();
    rtmp_live_screen_->Join();
    delete rtmp_live_screen_;
    rtmp_live_screen_ = 0;
}

void RtmpLiveScreenDlg::LoadConfig()
{
    config_.Load(config_path_);

    is_cap_webcam_ = config_.GetInt("is_capure_webcam", 0) ? true: false;
    webcam_x_ = config_.GetInt("webcam_x", 0);
    webcam_y_ = config_.GetInt("webcam_y", 0);
    webcam_width_ = config_.GetInt("webcam_width", 320);
    webcam_height_ = config_.GetInt("webcam_height", 240);

    is_cap_screen_ = config_.GetInt("is_cap_screen", 1) ? true: false;
    is_need_record_ = config_.GetInt("is_need_record", 0) ? true: false;

    rtmp_url_ = config_.GetProperty("rtmpurl");
    screen_bitrate_ = config_.GetInt("bitrate", 300);
    webcam_bitrate_ = config_.GetInt("webcam_bitrate", 80);

    video_change_millsecs_ = config_.GetInt("video_change_millsecs", 3000);

	//@feng read 'wait_time' from file; 
	i_waittime_  = config_.GetInt("wait_time",0);
	show_window_ = config_.GetInt("is_auto_publish",0) ? true: false;
	
}

void RtmpLiveScreenDlg::SaveConfig()
{
    config_.SetInt("is_capure_webcam", is_cap_webcam_ ? 1 : 0);
    config_.SetInt("webcam_x", webcam_x_);
    config_.SetInt("webcam_y", webcam_y_);
    config_.SetInt("webcam_width", webcam_width_);
    config_.SetInt("webcam_height", webcam_height_);

    config_.SetInt("is_cap_screen", is_cap_screen_ ? 1 : 0);
    config_.SetInt("is_need_record", is_need_record_ ? 1 : 0);

    config_.SetProperty("rtmpurl", rtmp_url_);
    config_.SetInt("bitrate", screen_bitrate_);
    config_.SetInt("webcam_bitrate", webcam_bitrate_);

    config_.SetInt("video_change_millsecs", video_change_millsecs_);

	//@feng save wait_time;
	CString cs_getwaittime_;
	m_edit_waittime.GetWindowText(cs_getwaittime_);
	i_waittime_ = _ttoi(cs_getwaittime_);
	config_.SetInt("wait_time", i_waittime_);
	config_.SetInt("is_auto_publish", show_window_ ? 1 : 0);

    config_.Save(config_path_);
}

void RtmpLiveScreenDlg::UpdateGui()
{
    switch (webcam_width_)
    {
    case 704:
        cb_cap_wh_.SetCurSel(0);
        break;
    case 352:
        cb_cap_wh_.SetCurSel(1);
        break;
    case 320:
        cb_cap_wh_.SetCurSel(2);
        break;
    case 176:
        cb_cap_wh_.SetCurSel(3);
        break;
    }

    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_x_);
    edit_x_.SetWindowText(tmpstr);
    tmpstr.Format(_T("%d"), webcam_y_);
    edit_y_.SetWindowText(tmpstr);
	tmpstr.Format(_T("%d"), screen_bitrate_);
	edit_Ckbps.SetWindowText(tmpstr);
	

    slider_ctrl_h_.SetRange(0, screen_width_-webcam_x_);
    slider_ctrl_h_.SetPos(0);
    slider_ctrl_v_.SetRange(0, screen_height_-webcam_y_);
    slider_ctrl_v_.SetPos(0);

    if (video_device_index_.empty())
    {
        check_webcam_.SetCheck(FALSE);
        check_webcam_.EnableWindow(FALSE);
        is_cap_webcam_ = false;
    }
    else
    {
        check_webcam_.SetCheck(is_cap_webcam_);
    }
    check_screen_.SetCheck(is_cap_screen_);
    check_record_.SetCheck(is_need_record_);
	m_check_autopublish.SetCheck(show_window_);

    if (false == is_cap_webcam_)
    {
        cb_cap_wh_.EnableWindow(FALSE);
        edit_x_.EnableWindow(FALSE);
        edit_y_.EnableWindow(FALSE);
		//edit_Ckbps.EnableWindow(FALSE);
        slider_ctrl_h_.EnableWindow(FALSE);
        slider_ctrl_v_.EnableWindow(FALSE);

    }
}

bool RtmpLiveScreenDlg::UpdateValue()
{
    CString tmpstr;

    is_cap_webcam_ = check_webcam_.GetCheck() ? true : false;
    edit_x_.GetWindowText(tmpstr);  
	webcam_x_ = _ttoi(tmpstr);

    edit_y_.GetWindowText(tmpstr);  
	webcam_y_ = _ttoi(tmpstr);

	edit_Ckbps.GetWindowText(tmpstr);  
	screen_bitrate_ = _ttoi(tmpstr);

	edit_Ckbps.EnableWindow(TRUE);
    switch (cb_cap_wh_.GetCurSel())
    {
    case 0:
        webcam_width_ = 704; webcam_height_ = 576;
        break;
    case 1:
        webcam_width_ = 352; webcam_height_ = 288;
        break;
    case 2:
        webcam_width_ = 320; webcam_height_ = 240;
        break;
    case 3:
        webcam_width_ = 176; webcam_height_ = 144;
        break;
    }

    is_cap_screen_ = check_screen_.GetCheck() ? true : false;
    is_need_record_ = check_record_.GetCheck() ? true : false;
	show_window_    = m_check_autopublish.GetCheck() ? true : false;

    if (is_cap_screen_)
    {
        bmpinfo_->bmiHeader.biWidth = screen_width_;
        bmpinfo_->bmiHeader.biHeight = screen_height_;
    }
    else
    {
        if (is_cap_webcam_)
        {
            bmpinfo_->bmiHeader.biWidth = webcam_width_;
            bmpinfo_->bmiHeader.biHeight = webcam_height_;
        }
        else
        {
            // 不能一个都不选择
            MessageBox(_T("摄像头和桌面必须选择一个"), _T("错误设置"), MB_OK);
            return false;
        }
    }

    return true;
}

void RtmpLiveScreenDlg::OnBnClickedCheckWebcam()
{
    bool is_webcam_check = check_webcam_.GetCheck();
    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetCaptureWebcam(is_webcam_check);
    }
    if (is_webcam_check)
    {
        cb_cap_wh_.EnableWindow(TRUE);
        edit_x_.EnableWindow(TRUE);
        edit_y_.EnableWindow(TRUE);
		//edit_Ckbps.EnableWindow(TRUE);
        slider_ctrl_h_.EnableWindow(TRUE);
        slider_ctrl_v_.EnableWindow(TRUE);
    }
    else
    {
        cb_cap_wh_.EnableWindow(FALSE);
        edit_x_.EnableWindow(FALSE);
        edit_y_.EnableWindow(FALSE);
		//edit_Ckbps.EnableWindow(FALSE);
        slider_ctrl_h_.EnableWindow(FALSE);
        slider_ctrl_v_.EnableWindow(FALSE);
    }

    UpdateValue();
    SaveConfig();
}


void RtmpLiveScreenDlg::OnBnClickedBtnUp()
{
    if (!check_webcam_.GetCheck() || !check_screen_.GetCheck()) return;

    webcam_y_+= 5;
    if (webcam_y_ >= screen_height_ - webcam_height_)
        webcam_y_ = screen_height_ - webcam_height_;
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_y_);
    edit_y_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }

    UpdateValue();
    SaveConfig();
}


void RtmpLiveScreenDlg::OnBnClickedBtnLeft()
{
    if (!check_webcam_.GetCheck() || !check_screen_.GetCheck()) return;

    webcam_x_-= 5;
    if (webcam_x_ < 0) webcam_x_ = 0;
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_x_);
    edit_x_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }

    UpdateValue();
    SaveConfig();
}


void RtmpLiveScreenDlg::OnBnClickedBtnRight()
{
    if (!check_webcam_.GetCheck() || !check_screen_.GetCheck()) return;

    webcam_x_+= 5;
    if (webcam_x_ >= screen_width_ - webcam_width_)
        webcam_x_ = screen_width_ - webcam_width_;
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_x_);
    edit_x_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }

    UpdateValue();
    SaveConfig();
}


void RtmpLiveScreenDlg::OnBnClickedBtnDown()
{
    if (!check_webcam_.GetCheck() || !check_screen_.GetCheck()) return;

    webcam_y_-= 5;
    if (webcam_y_ < 0) webcam_y_ = 0;
    CString tmpstr;
    tmpstr.Format(_T("%d"), webcam_y_);
    edit_y_.SetWindowText(tmpstr);

    if (rtmp_live_screen_)
    {
        rtmp_live_screen_->SetWebcamPosition(webcam_x_, webcam_y_);
    }

    UpdateValue();
    SaveConfig();
}


void RtmpLiveScreenDlg::OnCbnSelchangeCbCapWh()
{
    switch (cb_cap_wh_.GetCurSel())
    {
    case 0:
        slider_ctrl_h_.SetRange(0, screen_width_-704);
        slider_ctrl_v_.SetRange(0, screen_height_-576);
        break;
    case 1:
        slider_ctrl_h_.SetRange(0, screen_width_-352);
        slider_ctrl_v_.SetRange(0, screen_height_-288);
        break;
    case 2:
        slider_ctrl_h_.SetRange(0, screen_width_-320);
        slider_ctrl_v_.SetRange(0, screen_height_-240);
        break;
    case 3:
        slider_ctrl_h_.SetRange(0, screen_width_-176);
        slider_ctrl_v_.SetRange(0, screen_height_-144);
        break;
    }

    slider_ctrl_h_.SetPos(0);
    slider_ctrl_v_.SetPos(0);
}


void RtmpLiveScreenDlg::OnStnClickedStaticC3()
{
	// TODO: 在此添加控件通知处理程序代码
}


void RtmpLiveScreenDlg::OnBnClickedButtonyingyong()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	//CString strTemp,strBuff;
	//GetDlgItemText(IDC_EDIT1,strTemp);
	UpdateValue();
    SaveConfig();

}


void RtmpLiveScreenDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if(nID==SC_MINIMIZE) 
		
	{
		NOTIFYICONDATA nid; 
		nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA); 
		nid.hWnd=this->m_hWnd; 
		nid.uID=IDR_MAINFRAME; 
		nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP ; 
		nid.uCallbackMessage=WM_SHOWTASK;//自定义的消息名称 
		nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)); 
		WCHAR nid_sztip[128] ={0};

		wcscpy(nid.szTip,_T("桌面推流"));
		//strcpy(nid.szTip,(WCHAR)"screen publish"); //信息提示条 
		Shell_NotifyIcon(NIM_ADD,&nid); //在托盘区添加图标 
		ShowWindow(SW_HIDE); //隐藏主窗口 

	}
	CDialog::OnSysCommand(nID, lParam);


}


afx_msg LRESULT RtmpLiveScreenDlg::OnShowtask(WPARAM wParam, LPARAM lParam)
{
	if(wParam!=IDR_MAINFRAME) 
		return 1; 
	switch(lParam) 
	{ 
	case WM_RBUTTONUP://右键起来时弹出快捷菜单，这里只有一个“关闭” 
		{ 
			LPPOINT lpoint=new tagPOINT; 
			::GetCursorPos(lpoint);//得到鼠标位置 
			CMenu menu; 
			menu.CreatePopupMenu();//声明一个弹出式菜单 

			menu.AppendMenu(MF_STRING,WM_DESTROY,_T("关闭")); //增加菜单项“关闭”，点击则发送消息WM_DESTROY给主窗口（已隐藏），将程序结束。 
			menu.TrackPopupMenu(TPM_LEFTALIGN,lpoint->x,lpoint->y,this); //确定弹出式菜单的位置 
			HMENU hmenu=menu.Detach(); 
			menu.DestroyMenu(); //资源回收 
			delete lpoint; 
		} break; 
	case WM_LBUTTONDBLCLK: //双击左键的处理 
		{ 
			this->ShowWindow(SW_SHOWNORMAL);//简单的显示主窗口完事儿 
			//DeleteTray(); 
			//NOTIFYICONDATA nid; 
			//nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA); 
			//nid.hWnd=this->m_hWnd; 
			//nid.uID=IDR_MAINFRAME; 
			//nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP ; 
			//nid.uCallbackMessage=WM_SHOWTASK; //自定义的消息名称 
			//nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME)); 
			//wcscpy(nid.szTip,_T("程序名称")); //信息提示条为“计划任务提醒” 
			//Shell_NotifyIcon(NIM_DELETE,&nid); //在托盘区删除图标 
		
		} break; 
	default: break; 
	} 
	return 0; 
}


void RtmpLiveScreenDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//int result = MessageBox(_T("是否关闭程序？点击取消最小化"),_T("提示"),MB_OKCANCEL);
	//if(result == 1)
	//{
	//	//if(result == ID_YES)
	//	NOTIFYICONDATA nid;
	//	nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA);
	//	nid.hWnd=this->m_hWnd;
	//	nid.uID=IDR_MAINFRAME;
	//	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP ;
	//	nid.uCallbackMessage=WM_SHOWTASK;//自定义的消息名称
	//	nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME));
	//	wcscpy(nid.szTip,L"程序名称");    //信息提示条
	//	Shell_NotifyIcon(NIM_DELETE,&nid);    //在托盘区删除图标
	//	CDialog::OnClose();
	//}
	//else if(result == 2)
	//{
	//	PostMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
	//	return;
	//}
	NOTIFYICONDATA nid;
	nid.cbSize=(DWORD)sizeof(NOTIFYICONDATA);
	nid.hWnd=this->m_hWnd;
	nid.uID=IDR_MAINFRAME;
	nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP ;
	nid.uCallbackMessage=WM_SHOWTASK;//自定义的消息名称
	nid.hIcon=LoadIcon(AfxGetInstanceHandle(),MAKEINTRESOURCE(IDR_MAINFRAME));
	wcscpy(nid.szTip,L"程序名称");    //信息提示条
	Shell_NotifyIcon(NIM_DELETE,&nid);    //在托盘区删除图标
	CDialog::OnClose();
}

BOOL RtmpLiveScreenDlg::ShowInTaskbar(HWND hWnd, BOOL bShow)
{
	HRESULT hr; 
	ITaskbarList* pTaskbarList;
	hr = CoCreateInstance( CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER,  
		IID_ITaskbarList, (void**)&pTaskbarList );

	if(SUCCEEDED(hr))
	{
		pTaskbarList->HrInit();
		if(bShow)
			pTaskbarList->AddTab(hWnd);
		else
			pTaskbarList->DeleteTab(hWnd);

		pTaskbarList->Release();
		return TRUE;
	}

	return FALSE;
}