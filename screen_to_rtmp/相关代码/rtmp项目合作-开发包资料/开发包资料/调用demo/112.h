// 112.h : main header file for the 112 application
//

#if !defined(AFX_112_H__A9BCA29F_1EA0_4192_A7ED_5BBA0F4A4F2F__INCLUDED_)
#define AFX_112_H__A9BCA29F_1EA0_4192_A7ED_5BBA0F4A4F2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include <objbase.h>


/////////////////////////////////////////////////////////////////////////////
// CMy112App:
// See 112.cpp for the implementation of this class
//

class CMy112App : public CWinApp
{
public:
	CMy112App();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMy112App)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMy112App)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_112_H__A9BCA29F_1EA0_4192_A7ED_5BBA0F4A4F2F__INCLUDED_)
