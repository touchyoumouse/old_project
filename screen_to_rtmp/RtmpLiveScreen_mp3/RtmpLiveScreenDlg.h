#pragma once

#include <vector>

#include "RtmpLiveScreen.h"
#include "BtnST.h"
#include "MySliderControl.h"
#include "SimpleConfig.h"
#include "afxwin.h"

class RtmpLiveScreenDlg : public CDialog
{
public:
	RtmpLiveScreenDlg(bool isShowWindow, const std::string& cfgPath, CWnd* pParent = NULL);

    ~RtmpLiveScreenDlg();

	enum { IDD = IDD_RTMPSTREAMING_DIALOG };

    void LoadConfig();

    void SaveConfig();

    void UpdateGui();

    bool UpdateValue();

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    virtual void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);

protected:
	HICON m_hIcon;
	BOOL  ShowInTaskbar(HWND hWnd, BOOL bShow);
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
    afx_msg void OnNcPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg LRESULT OnNewFrame(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
    // UI controls
    CComboBox cb_select_video_;
    CComboBox cb_select_audio_;
    CButton btn_start_;
    CButton btn_stop_;
    CEdit edit_screen_wh_;
    CEdit edit_x_;
    CEdit edit_y_;
    CComboBox cb_cap_wh_;
    CButton check_record_;
    CButton check_webcam_;
    CButton check_screen_;
    CFont label_font_;

    CButtonST btn_up_;
    CButtonST btn_down_;
    CButtonST btn_left_;
    CButtonST btn_right_;
    CMySliderControl slider_ctrl_v_;
    CMySliderControl slider_ctrl_h_;

private:
    std::vector<CString> audio_device_index_;
    std::vector<CString> video_device_index_;

    bool is_cap_webcam_;
    int webcam_x_;
    int webcam_y_;
    int webcam_width_;
    int webcam_height_;

    bool is_cap_screen_;
    bool is_need_record_;

    std::string rtmp_url_;
    int screen_bitrate_;
    int webcam_bitrate_;

    int screen_width_;
    int screen_height_;
    HANDLE hloc_;
    LPBITMAPINFO bmpinfo_;	//位图信息头指针

    int video_change_millsecs_;

    SimpleConfig config_;
    RtmpLiveScreen* rtmp_live_screen_;

    bool is_show_window_;
    std::string config_path_;

	int i_waittime_;
	bool show_window_;
public:
    afx_msg void OnBnClickedBtnStart();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnBnClickedCheckWebcam();
    afx_msg void OnBnClickedBtnUp();
    afx_msg void OnBnClickedBtnLeft();
    afx_msg void OnBnClickedBtnRight();
    afx_msg void OnBnClickedBtnDown();
    afx_msg void OnCbnSelchangeCbCapWh();
	afx_msg void OnStnClickedStaticC3();
	CString edit_cap;
	CEdit edit_Ckbps;
	afx_msg void OnBnClickedButtonyingyong();
//	int m_edit_waittime;
	CEdit m_edit_waittime;
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
protected:
	afx_msg LRESULT OnShowtask(WPARAM wParam, LPARAM lParam);
public:
	afx_msg void OnClose();
	CButton m_check_autopublish;
};
