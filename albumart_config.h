#ifndef __FOO_UIE_ALBUMART__CONFIG_H__
#define __FOO_UIE_ALBUMART__CONFIG_H__

#define NUM_TABS        3

typedef HRESULT (CALLBACK* DIALOGTHEMEPROC)(HWND,DWORD);

class edit_view
{
public:
    bool run_edit_view(pfc::string8 & param,HWND parent);
    static INT_PTR WINAPI EditViewProc(HWND hWnd,UINT msg,WPARAM wp,LPARAM lp);
private:
    static void preview_string(pfc::string8 & p_out, pfc::string8 & p_in);
};

class command_select
{
public:
    bool run_command_select(GUID & selected_command, HWND parent);

    static INT_PTR WINAPI CommandSelectProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
    INT_PTR WINAPI OnCommandSelectMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
private:
    GUID * m_command_ptr;
    pfc::list_t<GUID> m_commands;
};

class tab_entry
{
public:
    tab_entry(LPSTR pszName,DLGPROC lpDialogFunc, WORD nID)
    {
        m_pszName = pszName;
        m_lpDialogFunc = lpDialogFunc;
        m_nID = nID;
    }
    tab_entry() { m_pszName = ""; m_lpDialogFunc = NULL; m_nID = 0;}

    HWND CreateTabDialog(HWND hWndParent, LPARAM lInitParam = 0)
    {
        PFC_ASSERT(m_lpDialogFunc != NULL);
        return uCreateDialog(m_nID, hWndParent, m_lpDialogFunc, lInitParam);
    }
public:
    LPSTR m_pszName;
    DLGPROC m_lpDialogFunc;
    WORD m_nID;
};

class albumart_config
{
public:
    albumart_config(uie_albumart * parent);
    ~albumart_config();

    static INT_PTR WINAPI DisplayTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR WINAPI OnDisplayTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR WINAPI BehaviourTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR WINAPI OnBehaviourTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static INT_PTR WINAPI SourcesTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR WINAPI OnSourcesTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static INT_PTR WINAPI ConfigProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
    INT_PTR WINAPI OnConfigMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    bool run(HWND parent);
private:
    bool m_albumart_config_initialized;
    HWND m_hWnd;
    pfc::list_t<tab_entry> m_tab_table;
    uie_albumart * m_parent;
    albumart_vars m_config;
    pfc::list_t<pfc::string8> m_sources;
    bool m_refreshing;
    bool m_dirty;
    unsigned int m_vars_changed;

    HINSTANCE m_hUxtheme;
    DIALOGTHEMEPROC m_EnableThemeDialog;

    void update_config(int last_tab);
    void make_dirty(var_change change_type);
    bool confirm_lost_changes(HWND hWnd, int last_tab);

    void export_sources(HWND hWnd);
    bool import_sources(HWND hWnd);
};


#endif
