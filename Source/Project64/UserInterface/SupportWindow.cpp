#include "stdafx.h"
#include "SupportEnterCode.h"
#include <time.h>

CSupportWindow * CSupportWindow::m_this = nullptr;

CSupportWindow::CSupportWindow(CProjectSupport & Support) :
    m_Support(Support),
    m_TimeOutTime(30),
    m_hParent(nullptr),
    m_Delay(false)
{
}

CSupportWindow::~CSupportWindow(void)
{
}

void CALLBACK CSupportWindow::TimerProc(HWND, UINT, UINT_PTR idEvent, DWORD)
{
    ::KillTimer(nullptr, idEvent);
    m_this->DoModal(m_this->m_hParent);
}

void CSupportWindow::Show(HWND hParent, bool Delay)
{
    m_Delay = Delay;
    if (Delay)
    {
        if (m_Support.Validated())
        {
            return;
        }

        m_Support.IncrementRunCount();
        if (m_Support.RunCount() < 7 || !m_Support.ShowSuppotWindow())
        {
            return;
        }
        m_hParent = hParent;
        m_this = this;
        UISettingsSaveBool(UserInterface_ShowingNagWindow, true);
        ::SetTimer(nullptr, 0, 2500, TimerProc);
    }
    else
    {
        UISettingsSaveBool(UserInterface_ShowingNagWindow, true);
        DoModal(hParent);
    }
}

void CSupportWindow::EnableContinue()
{
    GetSystemMenu(true);
    SetWindowLong(GWL_STYLE, GetWindowLong(GWL_STYLE) & ~CS_NOCLOSE);
    ::EnableWindow(GetDlgItem(IDCANCEL), true);
}

LRESULT CSupportWindow::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    return TRUE;
}

LRESULT CSupportWindow::OnColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CDCHandle hdcStatic = (HDC)wParam;
    hdcStatic.SetTextColor(RGB(0, 0, 0));
    hdcStatic.SetBkMode(TRANSPARENT);
    return (LONG)(LRESULT)((HBRUSH)GetStockObject(NULL_BRUSH));
}

LRESULT CSupportWindow::OnEraseBackground(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    static HPEN Outline = CreatePen(PS_SOLID, 1, 0x00FFFFFF);
    static HBRUSH Fill = CreateSolidBrush(0x00FFFFFF);

    CDCHandle hdc = (HDC)wParam;
    hdc.SelectPen(Outline);
    hdc.SelectBrush(Fill);

    RECT rect;
    GetClientRect(&rect);
    hdc.Rectangle(&rect);
    return TRUE;
}

LRESULT CSupportWindow::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    m_TimeOutTime -= 1;
    if (m_TimeOutTime == 0)
    {
        KillTimer(wParam);
        EnableContinue();
    }
    stdstr_f Continue_txt(m_TimeOutTime > 0 ? "%s (%d)" : "%s", GS(MSG_SUPPORT_CONTINUE), m_TimeOutTime);
    GetDlgItem(IDCANCEL).SetWindowText(Continue_txt.ToUTF16().c_str());
    return true;
}

LRESULT CSupportWindow::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    UISettingsSaveBool(UserInterface_ShowingNagWindow, false);
    EndDialog(wID);
    return TRUE;
}

LRESULT CSupportWindow::OnSupportProject64(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    stdstr SupportURL = stdstr_f("https://www.pj64-emu.com/support-project64.html?ver=%s&machine=%s", VER_FILE_VERSION_STR, m_Support.MachineID());
    ShellExecute(nullptr, L"open", SupportURL.ToUTF16().c_str(), nullptr, nullptr, SW_SHOWMAXIMIZED);
    return TRUE;
}

LRESULT CSupportWindow::OnEnterCode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CSupportEnterCode EnterCodeWindow(m_Support);
    EnterCodeWindow.DoModal(m_hWnd);
    if (m_Support.Validated())
    {
        UISettingsSaveBool(UserInterface_ShowingNagWindow, false);
        EndDialog(wID);
    }
    return TRUE;
}
