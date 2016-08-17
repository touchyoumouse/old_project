// Machine generated IDispatch wrapper class(es) created with Add Class from Typelib Wizard

//#import "C:\\Windows\\System32\\pptCount.dll" no_namespace
// CpCount wrapper class

class CpCount : public COleDispatchDriver
{
public:
    CpCount(){} // Calls COleDispatchDriver default constructor
    CpCount(LPDISPATCH pDispatch) : COleDispatchDriver(pDispatch) {}
    CpCount(const CpCount& dispatchSrc) : COleDispatchDriver(dispatchSrc) {}

    // Attributes
public:

    // Operations
public:


    // _pCount methods
public:
    LPDISPATCH NewPPTApp()
    {
        LPDISPATCH result;
        InvokeHelper(0x60030001, DISPATCH_METHOD, VT_DISPATCH, (void*)&result, NULL);
        return result;
    }
    BOOL OpenPPT(LPCTSTR sPath, BOOL bRedOnly)
    {
        BOOL result;
        static BYTE parms[] = VTS_BSTR VTS_BOOL ;
        InvokeHelper(0x60030002, DISPATCH_METHOD, VT_BOOL, (void*)&result, parms, sPath, bRedOnly);
        return result;
    }
    short GetPlayNumber()
    {
        short result;
        InvokeHelper(0x60030003, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
        return result;
    }
    short GetNPlayNumber()
    {
        short result;
        InvokeHelper(0x60030004, DISPATCH_METHOD, VT_I2, (void*)&result, NULL);
        return result;
    }
    CString GetPptCount(LPCTSTR sPath)
    {
        CString result;
        static BYTE parms[] = VTS_BSTR ;
        InvokeHelper(0x60030005, DISPATCH_METHOD, VT_BSTR, (void*)&result, parms, sPath);
        return result;
    }

    // _pCount properties
public:

};
