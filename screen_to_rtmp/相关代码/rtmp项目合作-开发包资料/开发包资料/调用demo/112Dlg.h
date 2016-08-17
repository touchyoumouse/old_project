// 112Dlg.h : header file
//

#if !defined(AFX_112DLG_H__7CEA3584_0A5A_4AFE_864D_3B9E4EB67689__INCLUDED_)
#define AFX_112DLG_H__7CEA3584_0A5A_4AFE_864D_3B9E4EB67689__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMy112Dlg dialog

class CMy112Dlg : public CDialog
{
// Construction
public:
	CMy112Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMy112Dlg)
	enum { IDD = IDD_MY112_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMy112Dlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMy112Dlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_112DLG_H__7CEA3584_0A5A_4AFE_864D_3B9E4EB67689__INCLUDED_)
