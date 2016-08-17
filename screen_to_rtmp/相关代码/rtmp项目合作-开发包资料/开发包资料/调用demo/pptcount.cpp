// Machine generated IDispatch wrapper class(es) created with ClassWizard

#include "stdafx.h"
#include "pptcount.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// _pCount properties

/////////////////////////////////////////////////////////////////////////////
// _pCount operations

LPDISPATCH _pCount::NewPPTApp()
{
	LPDISPATCH result;
	InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, NULL);
	return result;
}

BOOL _pCount::OpenPPT(LPCTSTR sPath, BOOL bRedOnly)
{
	BOOL result;
	static BYTE parms[] =
		VTS_BSTR VTS_BOOL;
	InvokeHelper(0x60030002, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms,
		sPath, bRedOnly);
	return result;
}

short _pCount::GetPlayNumber()
{
	short result;
	InvokeHelper(0x60030003, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
	return result;
}

short _pCount::GetNPlayNumber()
{
	short result;
	InvokeHelper(0x60030004, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
	return result;
}

CString _pCount::GetPptCount(LPCTSTR sPath)
{
	CString result;
	static BYTE parms[] =
		VTS_BSTR;
	InvokeHelper(0x60030005, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms,
		sPath);
	return result;
}
