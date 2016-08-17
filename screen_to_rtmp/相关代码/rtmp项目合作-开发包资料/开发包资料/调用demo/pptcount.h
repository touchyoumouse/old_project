// Machine generated IDispatch wrapper class(es) created with ClassWizard
/////////////////////////////////////////////////////////////////////////////
// _pCount wrapper class

#include "Afxdisp.h"

class _pCount : public COleDispatchDriver
{
public:
	_pCount() {}		// Calls COleDispatchDriver default constructor
	_pCount(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
	_pCount(const _pCount& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

// Attributes
public:

// Operations
public:
	LPDISPATCH NewPPTApp();
	BOOL OpenPPT(LPCTSTR sPath, BOOL bRedOnly);
	short GetPlayNumber();
	short GetNPlayNumber();
	CString GetPptCount(LPCTSTR sPath);
};
