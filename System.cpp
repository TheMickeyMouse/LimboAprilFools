#include "System.h"

#include <filesystem>

#include "Utils/Debug/Logger.h"
#include <windows.h>
#include <ShlObj.h>
#undef ERROR

namespace System {
    template <class T>
    class _NoAddRefReleaseOnCComPtr : public T
    {
    private:
        STDMETHOD_(ULONG, AddRef)()=0;
        STDMETHOD_(ULONG, Release)()=0;
    };

    template<class T>
    class CComPtr
    {
    public:
        T *p;
    public:
        CComPtr()
        {
            p = NULL;
        }

        CComPtr(T *lp)
        {
            p = lp;
            if (p != NULL)
                p->AddRef();
        }

        CComPtr(const CComPtr<T> &lp)
        {
            p = lp.p;
            if (p != NULL)
                p->AddRef();
        }

        ~CComPtr()
        {
            if (p != NULL)
                p->Release();
        }

        HRESULT CoCreateInstance(REFCLSID rclsid, REFIID riid, LPUNKNOWN pOuter = NULL, DWORD ClsCtx = CLSCTX_ALL)
        {
            return ::CoCreateInstance(rclsid, pOuter, ClsCtx, riid, (void**)&p);
        }

        HRESULT CoCreateInstance(LPCOLESTR ProgID, REFIID riid, LPUNKNOWN pOuter = NULL, DWORD ClsCtx = CLSCTX_ALL)
        {
            CLSID clsid;
            HRESULT hr = CLSIDFromProgID(ProgID, &clsid);
            return FAILED(hr) ? hr : CoCreateInstance(clsid, riid, pOuter, ClsCtx);
        }

        void Release()
        {
            if (p != NULL)
            {
                p->Release();
                p = NULL;
            }
        }

        void Attach(T *lp)
        {
            if (p != NULL)
                p->Release();
            p = lp;
        }

        T *Detach()
        {
            T *saveP;

            saveP = p;
            p = NULL;
            return saveP;
        }

        T **operator & ()
        {
            return &p;
        }

        operator T * ()
        {
            return p;
        }

        _NoAddRefReleaseOnCComPtr<T>* operator->() const throw()
        {
            return (_NoAddRefReleaseOnCComPtr<T>*)p;
        }
    };


    template <class T, const IID* piid = &__uuidof(T)>
    class CComQIPtr : public CComPtr<T>
    {
    public:
        CComQIPtr() {}
        CComQIPtr(T* lp) : CComPtr<T>(lp) {}
        CComQIPtr(const CComQIPtr<T,piid>& lp) : CComPtr<T>(lp.p) {}
        CComQIPtr(IUnknown* lp) {
            if (lp != NULL)
                lp->QueryInterface(*piid, (void **)&(this->p));
        }
    };

    class CComVariant : public tagVARIANT {
    // Constructors
    public:
    	CComVariant() {
    		::VariantInit(this);
    	}
    	~CComVariant() {
    	    ::VariantClear(this);
    	}
    	CComVariant(int nSrc, VARTYPE vtSrc = VT_I4) {
    		vt = vtSrc;
    		intVal = nSrc;
    	}
    };

    void Try(HRESULT hr, Quasi::Str msg) {
        if (FAILED(hr))
           Quasi::Debug::QError$(msg);
    }

    // Query an interface from the desktop shell view.
    void FindDesktopFolderView(REFIID riid, void **ppv) {
        CComPtr<IShellWindows> spShellWindows;
        Try(spShellWindows.CoCreateInstance(CLSID_ShellWindows, IID_IShellWindows),
            "Failed to create IShellWindows instance" );

        CComVariant vtLoc( CSIDL_DESKTOP );
        CComVariant vtEmpty;
        long lhwnd;
        CComPtr<IDispatch> spdisp;
        Try(
            spShellWindows->FindWindowSW(
                &vtLoc, &vtEmpty, SWC_DESKTOP, &lhwnd, SWFO_NEEDDISPATCH, &spdisp ),
            "Failed to find desktop window" );

        CComQIPtr<IServiceProvider> spProv( spdisp );
        if(!spProv)
            Try(E_NOINTERFACE, "Failed to get IServiceProvider interface for desktop");

        CComPtr<IShellBrowser> spBrowser;
        Try(
            spProv->QueryService( SID_STopLevelBrowser, IID_PPV_ARGS( &spBrowser ) ),
            "Failed to get IShellBrowser for desktop" );

        CComPtr<IShellView> spView;
        Try(
            spBrowser->QueryActiveShellView( &spView ),
            "Failed to query IShellView for desktop" );

        Try(spView->QueryInterface( riid, ppv ),
            "Could not query desktop IShellView for interface ");
    }

    void ChangeWallpaper(const wchar_t* filepath) {
        const wchar_t* absolutePath = std::filesystem::current_path().append(filepath).c_str();

        // Call SystemParametersInfoW to set the desktop wallpaper
        // Parameters:
        // SPI_SETDESKWALLPAPER: Sets the desktop wallpaper
        // 0: Not used for this action
        // (void*)wallpaperPath: Pointer to the path string
        // SPIF_UPDATEINIFILE | SPIF_SENDCHANGE: Updates the user profile and broadcasts a message
        //                                         to all windows to inform them of the change.
        BOOL result = SystemParametersInfoW(
            SPI_SETDESKWALLPAPER,
            0,
            (PVOID)absolutePath,
            SPIF_UPDATEINIFILE | SPIF_SENDCHANGE
        );
        if (!result) {
            Quasi::Debug::QError$("Failed to set wallpaper!");
        }
    }

    void HideIcons() {
        // SHELLSTATE ss;
        // ss.fHideIcons = true;
        // SHGetSetSettings(&ss, SSF_HIDEICONS, true);

        CComPtr<IFolderView2> spView;
        FindDesktopFolderView(IID_PPV_ARGS(&spView));

        Try(
            spView->SetCurrentFolderFlags(FWF_NOICONS, FWF_NOICONS),
            "SetCurrentFolderFlags failed" );
    }

    void HideAllWindows() {
        HWND lHwnd = FindWindow("Shell_TrayWnd", NULL);
        SendMessage(lHwnd, WM_COMMAND, 419, 0);
    }

    void Shutdown() {
        // kinda unsafe. if you dont want this, turn safe mode on in cmakelists.
        std::system("shutdown /s /t 0");
    }

    void HideTaskbar() {
        static HWND hShellWnd = FindWindow("Shell_TrayWnd", NULL);
        ShowWindow(hShellWnd, SW_HIDE);
    }

    void ShowTaskbar() {
        static HWND hShellWnd = FindWindow("Shell_TrayWnd", NULL);
        ShowWindow(hShellWnd, SW_SHOW);
    }

    void KillRainmeter() {
        std::system("taskkill /IM Rainmeter.exe /T /F");
    }

    Image CaptureScreen() {
        static constexpr int w = 1920, h = 1080;
        HDC     hScreen = GetDC(NULL);
        HDC     hDC     = CreateCompatibleDC(hScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hScreen, w, h);
        HGDIOBJ old_obj = SelectObject(hDC, hBitmap);
        if (!BitBlt(hDC, 0, 0, w, h, hScreen, 0, 0, SRCCOPY))
            Quasi::Debug::QError$("Failed to capture screen!");

        BITMAP Bmp = {};
        BITMAPINFO Info = {};

        GetObject(hBitmap, sizeof(Bmp), &Bmp);

        int width, height;
        Info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        Info.bmiHeader.biWidth = width = Bmp.bmWidth;
        Info.bmiHeader.biHeight = height = Bmp.bmHeight;
        Info.bmiHeader.biPlanes = 1;
        Info.bmiHeader.biBitCount = Bmp.bmBitsPixel;
        Info.bmiHeader.biCompression = BI_RGB;
        Info.bmiHeader.biSizeImage = ((width * Bmp.bmBitsPixel + 31) / 32) * 4 * height;

        using namespace Quasi::Graphics;
        Image screenCapture = Image::New(width, height);
        GetDIBits(hDC, hBitmap, 0, height, screenCapture.Data(), &Info, DIB_RGB_COLORS);

        // clean up
        SelectObject(hDC, old_obj);
        DeleteDC(hDC);
        ReleaseDC(NULL, hScreen);
        DeleteObject(hBitmap);

        return screenCapture;
    }
}
