// 112.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "112.h"

#include "pptcount.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMy112App

BEGIN_MESSAGE_MAP(CMy112App, CWinApp)
	//{{AFX_MSG_MAP(CMy112App)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMy112App construction

CMy112App::CMy112App()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CMy112App object

CMy112App theApp;
#include <objbase.h>



/////////////////////////////////////////////////////////////////////////////
// CMy112App initialization

BOOL CMy112App::InitInstance()
{
	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif
	CoInitialize(NULL);

	COleException* pError = NULL;

	 //char bstrOne[40]="c:\\演示文稿1.ppt";
	CString owow;
	 char* iik=new char[40];
	 memcpy(iik,"c:\\演示文稿1.ppt",40);

	_pCount	 *oo=new _pCount;
	//oo=NULL;

	oo->CreateDispatch("PPTCount.pCount");

	owow=oo->GetPptCount(iik);
	AfxMessageBox(owow, MB_OK,0);


	oo->ReleaseDispatch();




	CoUninitialize();
	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
