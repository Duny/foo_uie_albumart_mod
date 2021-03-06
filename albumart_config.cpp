#include "stdafx.h"

INT_PTR WINAPI edit_view::EditViewProc(HWND hWnd,UINT msg,WPARAM wp,LPARAM lp)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        {
            SetWindowLongPtr(hWnd, DWL_USER, lp);

            pfc::string8 * ptr = reinterpret_cast<pfc::string8*>(lp);
            uSetDlgItemText(hWnd,IDC_VALUE,*ptr);

            pfc::string8 preview;
            preview_string(preview, *ptr);
            uSetDlgItemText(hWnd,IDC_PREVIEW,preview);
        }
        break;
    case WM_COMMAND:
        switch(LOWORD(wp))
        {
        case IDCANCEL:
            EndDialog(hWnd,0);
            break;
        case IDOK:
            {
                pfc::string8 * ptr = reinterpret_cast<pfc::string8*>(GetWindowLongPtr(hWnd,DWL_USER));
                uGetDlgItemText(hWnd,IDC_VALUE,*ptr);
                EndDialog(hWnd,1);
            }
            break;
        case IDC_VALUE:
            {
                if (HIWORD(wp) == EN_CHANGE)
                {
                    pfc::string8 pattern, preview;
                    uGetDlgItemText(hWnd,IDC_VALUE,pattern);
                    preview_string(preview, pattern);
                    uSetDlgItemText(hWnd,IDC_PREVIEW,preview);
                }
            }
            break;
        }
        break;
    }
    return FALSE;
}

//! gets an appropriate track and evaluates the titleformatting pattern p_in
//! @param [out] p_out will contain formatted string
//! @param [in] p_in formatting pattern to be evaluated
void edit_view::preview_string(pfc::string8 & p_out, pfc::string8 & p_in)
{
    // first, try to find a valid track
    static_api_ptr_t<playlist_manager> pm;
    t_size playlist;
    t_size index;

    if (!pm->get_playing_item_location(&playlist, &index))
    {
        playlist = pm->get_active_playlist();
        if (playlist == pfc_infinite)
        {
            playlist = 0;
        }

        index = pm->playlist_get_focus_item(playlist);
        if (index == pfc_infinite)
        {
            if (pm->playlist_get_item_count(playlist) > 0)
            {
                index = 0;
            }
            else
            {
                playlist = 0;
                index = pm->playlist_get_focus_item(0);
                if (index == pfc_infinite)
                {
                    if (pm->playlist_get_item_count(0) > 0)
                    {
                        index = 0;
                    }
                    else
                    {
                        p_out.reset();
                        return;
                    }
                }
            }
        }
    }

    // Now we have a valid track; format the title
    static_api_ptr_t<titleformat_compiler> compiler;
    service_ptr_t<titleformat_object> pattern_obj;

    compiler->compile(pattern_obj, p_in);
    pm->playlist_item_format_title(playlist, index, NULL, p_out, pattern_obj, NULL, playback_control::display_level_titles);
}

bool edit_view::run_edit_view(pfc::string8 & param,HWND parent)
{
    return !!uDialogBox(IDD_EDIT_VIEW,parent,EditViewProc,reinterpret_cast<LPARAM>(&param));
}

INT_PTR WINAPI command_select::CommandSelectProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    command_select * p_this;
    
    if (msg == WM_INITDIALOG)
    {
        p_this = (command_select*)(lp); //retrieve pointer to class
        uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
    }
    else
    {
        // if isnt wm_create, retrieve pointer to class
        p_this = reinterpret_cast<command_select*>(uGetWindowLong(hWnd,GWL_USERDATA));
    }
    return p_this ? p_this->OnCommandSelectMessage(hWnd, msg, wp, lp) : FALSE;
}

INT_PTR WINAPI command_select::OnCommandSelectMessage(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch(msg)
    {
    case WM_INITDIALOG:
        {
            HWND list = uGetDlgItem(hWnd,IDC_COMMAND_VALUE);

            service_enum_t<mainmenu_commands> e;
            service_ptr_t<mainmenu_commands> ptr;
            int idx = 0;
            int command_idx = 0;
            while (e.next(ptr))
            {
                for (t_uint32 n = 0; n < ptr->get_command_count(); n++)
                {
                    idx++;
                    pfc::string8 name;
                    ptr->get_name(n, name);
                    m_commands.add_item(ptr->get_command(n));
                    uSendMessageText(list, CB_ADDSTRING, 0, name.get_ptr());
                    if (ptr->get_command(n) == *(m_command_ptr))
                        command_idx = idx-1;
                }
            }

            uSendMessage(list,CB_SETCURSEL,command_idx,0);
        }
        break;
    case WM_COMMAND:
        switch(LOWORD(wp))
        {
        case IDCANCEL:
            EndDialog(hWnd,0);
            break;
        case IDOK:
            {
                t_uint32 value = uSendMessage(uGetDlgItem(hWnd,IDC_COMMAND_VALUE),CB_GETCURSEL,0,0);
                *(m_command_ptr) = m_commands[value];
                EndDialog(hWnd,1);
            }
            break;
        }
        break;
    }
    return FALSE;
}

bool command_select::run_command_select(GUID & selected_command, HWND parent)
{
    m_command_ptr = &selected_command;
    return !!uDialogBox(IDD_COMMAND_SELECT,parent,CommandSelectProc,reinterpret_cast<LPARAM>(this));
}

// parent: pointer to the calling instance of uie_albumart
albumart_config::albumart_config(uie_albumart * parent)
{
    m_albumart_config_initialized = false;
    m_tab_table.add_item(tab_entry("Display",   DisplayTabDialogProc,   IDD_CONFIG_TAB_DISPLAY));
    m_tab_table.add_item(tab_entry("Behaviour", BehaviourTabDialogProc, IDD_CONFIG_TAB_BEHAVIOUR));
    m_tab_table.add_item(tab_entry("Sources",   SourcesTabDialogProc,   IDD_CONFIG_TAB_SOURCES));
    m_refreshing = false;

    m_parent = parent;
    parent->get_config_vars(m_config, m_sources);

    m_dirty = false;
    m_vars_changed = 0;

    m_hUxtheme = LoadLibrary(_T("uxtheme.dll"));
    m_EnableThemeDialog = NULL;
    if (m_hUxtheme != NULL)
        m_EnableThemeDialog = (DIALOGTHEMEPROC)GetProcAddress(m_hUxtheme, "EnableThemeDialogTexture");
}

albumart_config::~albumart_config()
{
    if (m_hUxtheme != NULL)
        FreeLibrary(m_hUxtheme);
}

INT_PTR WINAPI albumart_config::DisplayTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    albumart_config * p_this;
    
    if (msg == WM_INITDIALOG)
    {
        p_this = (albumart_config*)(lParam); //retrieve pointer to class
        uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
    }
    else
    {
        // if isnt wm_create, retrieve pointer to class
        p_this = reinterpret_cast<albumart_config*>(uGetWindowLong(hWnd,GWL_USERDATA));
    }
    return p_this ? p_this->OnDisplayTabMessage(hWnd, msg, wParam, lParam) : FALSE;
}

INT_PTR WINAPI albumart_config::OnDisplayTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool redraw_art = false;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            HWND list = uGetDlgItem(hWnd,IDC_Quality);
            uSendMessageText(list,CB_ADDSTRING,0,"Low");
            uSendMessageText(list,CB_ADDSTRING,0,"Medium");
            uSendMessageText(list,CB_ADDSTRING,0,"High");
            uSendMessageText(list,CB_ADDSTRING,0,"Highest");
            uSendMessage(list,CB_SETCURSEL,m_config.interpolationmode,0);

            uSetDlgItemInt(hWnd,IDC_MinHeight,m_config.minheight,FALSE);
            uSetDlgItemInt(hWnd,IDC_MinWidth,m_config.minwidth,FALSE);
            uSetDlgItemInt(hWnd,IDC_Padding,m_config.padding,FALSE);
            uSendDlgItemMessage(hWnd,IDC_MinHeight_Spin,UDM_SETRANGE32,0,999);
            uSendDlgItemMessage(hWnd,IDC_MinWidth_Spin,UDM_SETRANGE32,0,999);
            uSendDlgItemMessage(hWnd,IDC_Padding_Spin,UDM_SETRANGE32,0,999);

            uButton_SetCheck(hWnd, IDC_Center, m_config.centeralbum);
            uButton_SetCheck(hWnd, IDC_Expand, m_config.expandalbum);
            uButton_SetCheck(hWnd, IDC_Shrink, m_config.shrinkalbum);
            
            uButton_SetCheck(hWnd, IDC_AspectRatio, m_config.aspectratio);
            uButton_SetCheck(hWnd, IDC_EnableBG, m_config.bg_enabled);
            uEnableWindow(uGetDlgItem(hWnd,IDC_ChooseBG),m_config.bg_enabled);
            uButton_SetCheck(hWnd, IDC_PixelBorder, m_config.draw_pixel_border);
            uEnableWindow(uGetDlgItem(hWnd,IDC_ChooseBorder),m_config.draw_pixel_border);

            uSendDlgItemMessageText(hWnd,IDC_EdgeStyle,CB_ADDSTRING,0,"None");
            uSendDlgItemMessageText(hWnd,IDC_EdgeStyle,CB_ADDSTRING,0,"Sunken");
            uSendDlgItemMessageText(hWnd,IDC_EdgeStyle,CB_ADDSTRING,0,"Grey");
            uSendDlgItemMessage(hWnd,IDC_EdgeStyle,CB_SETCURSEL,(int)m_config.edge_style,0);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_Center:
            m_config.centeralbum = uButton_GetCheck(hWnd, IDC_Center);
            redraw_art = true;
            break;
        case IDC_Expand:
            m_config.expandalbum = uButton_GetCheck(hWnd, IDC_Expand);
            redraw_art = true;
            break;
        case IDC_Shrink:
            m_config.shrinkalbum = uButton_GetCheck(hWnd, IDC_Shrink);
            redraw_art = true;
            break;
        case IDC_AspectRatio:
            m_config.aspectratio = uButton_GetCheck(hWnd, IDC_AspectRatio);
            redraw_art = true;
            break;
        case IDC_EnableBG:
            m_config.bg_enabled = uButton_GetCheck(hWnd, IDC_EnableBG);
            uEnableWindow(uGetDlgItem(hWnd,IDC_ChooseBG),m_config.bg_enabled);
            InvalidateRect(uGetDlgItem(hWnd,IDC_BGColor), NULL, TRUE);
            redraw_art = true;
            break;
        case IDC_ChooseBG:
            g_color_picker(hWnd,m_config.bgcol);
            InvalidateRect(uGetDlgItem(hWnd,IDC_BGColor), NULL, TRUE);
            redraw_art = true;
            break;
        case IDC_PixelBorder:
            m_config.draw_pixel_border = uButton_GetCheck(hWnd, IDC_PixelBorder);
            uEnableWindow(uGetDlgItem(hWnd,IDC_ChooseBorder),m_config.draw_pixel_border);
            InvalidateRect(uGetDlgItem(hWnd,IDC_BorderColor), NULL, TRUE);
            redraw_art = true;
            break;
        case IDC_ChooseBorder:
            g_color_picker(hWnd,m_config.bordercol);
            InvalidateRect(uGetDlgItem(hWnd,IDC_BorderColor), NULL, TRUE);
            redraw_art = true;
            break;
        case IDC_MinHeight:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (m_albumart_config_initialized)
                {
                    BOOL result;
                    int new_height = uGetDlgItemInt(hWnd, IDC_MinHeight, &result, FALSE);
                    if (result)
                        m_config.minheight = pfc::min_t<int>(new_height, 999);
                    make_dirty(VC_MIN_HEIGHT);
                }
            }
            break;
        case IDC_MinWidth:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (m_albumart_config_initialized)
                {
                    BOOL result;
                    int new_height = uGetDlgItemInt(hWnd, IDC_MinWidth, &result, FALSE);
                    if (result)
                        m_config.minwidth = pfc::min_t<int>(new_height, 999);
                    make_dirty(VC_MIN_WIDTH);
                }
            }
            break;
        case IDC_Padding:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (m_albumart_config_initialized)
                {
                    BOOL result;
                    int new_padding = uGetDlgItemInt(hWnd, IDC_Padding, &result, FALSE);
                    if (result)
                        m_config.padding = pfc::min_t(new_padding, 999);
                    redraw_art = true;
                }
            }
            break;
        case IDC_EdgeStyle:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.edge_style = (edgestyle)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                make_dirty(VC_EDGESTYLE);
            }
            break;
        case IDC_Quality:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.interpolationmode = (resizing_quality)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                redraw_art = true;
            }
            break;
        }

        if (redraw_art) make_dirty(VC_GENERAL);
        break;

    case WM_DRAWITEM:
        {
            DRAWITEMSTRUCT * item_struct = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
            switch (wParam)
            {
            case IDC_BGColor:
                {
                HBRUSH bg_brush = CreateSolidBrush((m_config.bg_enabled)
                                                   ?m_config.bgcol
                                                   :GetSysColor(COLOR_3DFACE));
                FillRect(item_struct->hDC, &item_struct->rcItem, bg_brush);
                break;
                }

            case IDC_BorderColor:
                {
                HBRUSH border_brush = CreateSolidBrush((m_config.draw_pixel_border)
                                                        ?m_config.bordercol
                                                        :GetSysColor(COLOR_3DFACE));
                FillRect(item_struct->hDC, &item_struct->rcItem, border_brush);
                break;
                }
            }
        }
        break;
    }
    return FALSE;
}

INT_PTR WINAPI albumart_config::BehaviourTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    albumart_config * p_this;
    
    if (msg == WM_INITDIALOG)
    {
        p_this = (albumart_config*)(lParam); //retrieve pointer to class
        uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
    }
    else
    {
        // if isnt wm_create, retrieve pointer to class
        p_this = reinterpret_cast<albumart_config*>(uGetWindowLong(hWnd,GWL_USERDATA));
    }
    return p_this ? p_this->OnBehaviourTabMessage(hWnd, msg, wParam, lParam) : FALSE;
}

INT_PTR WINAPI albumart_config::OnBehaviourTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool vars_changed = false;

    switch (msg)
    {
    case WM_INITDIALOG:
        {
            HWND list = uGetDlgItem(hWnd,IDC_LftClick);
            for(t_size i = 0; i < get_function_count(); i++)
                uSendMessageText(list, CB_ADDSTRING, 0, get_function_name(i));
            uSendMessage(list,CB_SETCURSEL,m_config.lftclickfunc,0);
        }
        {
            HWND list = uGetDlgItem(hWnd,IDC_MdlClick);
            for(t_size i = 0; i < get_function_count(); i++)
                uSendMessageText(list, CB_ADDSTRING, 0, get_function_name(i));
            uSendMessage(list,CB_SETCURSEL,m_config.mdlclickfunc,0);
        }
        {
            HWND list = uGetDlgItem(hWnd,IDC_DblClick);
            for(t_size i = 0; i < get_function_count(); i++)
                uSendMessageText(list, CB_ADDSTRING, 0, get_function_name(i));
            uSendMessage(list,CB_SETCURSEL,m_config.dblclickfunc,0);
        }
        {
            HWND list = uGetDlgItem(hWnd,IDC_ResetSource);
            uSendMessageText(list,CB_ADDSTRING,0,"Never");
            uSendMessageText(list,CB_ADDSTRING,0,"Always");
            uSendMessageText(list,CB_ADDSTRING,0,"Only if display would change");
            uSendMessage(list,CB_SETCURSEL,(int)m_config.resetsource,0);
        }
        {
            uButton_SetCheck(hWnd, IDC_Selected, m_config.selected);

            bool anim_enabled = (m_config.animtime != 0);
            uButton_SetCheck(hWnd, IDC_EnableAnim, anim_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_AnimTime), anim_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_ANIMTIME_CAPTION), anim_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_AnimTime_Spin), anim_enabled);

            uSetDlgItemInt(hWnd,IDC_AnimTime,m_config.old_animtime,FALSE);
            uSendDlgItemMessage(hWnd,IDC_AnimTime_Spin,UDM_SETRANGE32,0,999);
            
            UDACCEL anim_steps = {0, 10};
            uSendDlgItemMessage(hWnd,IDC_AnimTime_Spin,UDM_SETACCEL,1,reinterpret_cast<LPARAM>(&anim_steps));

            bool cycle_enabled = (m_config.cycletime != 0);
            uButton_SetCheck(hWnd, IDC_EnableCycle, cycle_enabled);
            uButton_SetCheck(hWnd, IDC_SKIP_NOCOVERS, m_config.skip_nocovers);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CycleTime), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_SKIP_NOCOVERS), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CYCLETIME_CAPTION), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CycleTime_Spin), cycle_enabled);

            uSetDlgItemInt(hWnd,IDC_CycleTime,m_config.old_cycletime,FALSE);
            uSendDlgItemMessage(hWnd,IDC_CycleTime_Spin,UDM_SETRANGE32,0,999);

            uButton_SetCheck(hWnd, IDC_CYCLE_WILDCARDS, m_config.cycle_wildcards);
            uButton_SetCheck(hWnd, IDC_WILDCARD_ALPHA, m_config.cycle_order==WILDCARD_ALPHA);
            uButton_SetCheck(hWnd, IDC_WILDCARD_RANDOM, m_config.cycle_order==WILDCARD_RANDOM);
            uEnableWindow(uGetDlgItem(hWnd, IDC_ORDER_CAPTION), m_config.cycle_wildcards);
            uEnableWindow(uGetDlgItem(hWnd, IDC_WILDCARD_ALPHA), m_config.cycle_wildcards);
            uEnableWindow(uGetDlgItem(hWnd, IDC_WILDCARD_RANDOM), m_config.cycle_wildcards);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_Selected:
            m_config.selected = uButton_GetCheck(hWnd, IDC_Selected);
            make_dirty(VC_FOLLOW);
            break;
        case IDC_ResetSource:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.resetsource = (reset_option)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                vars_changed = true;
            }
            break;
        case IDC_EnableAnim:
            {
            bool anim_enabled = uButton_GetCheck(hWnd, IDC_EnableAnim);
            uEnableWindow(uGetDlgItem(hWnd, IDC_AnimTime), anim_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_ANIMTIME_CAPTION), anim_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_AnimTime_Spin), anim_enabled);
            if (!anim_enabled)
            {
                m_config.old_animtime = m_config.animtime;
                m_config.animtime = 0;
            }
            else
                m_config.animtime = m_config.old_animtime;
            }
            vars_changed = true;
            break;
        case IDC_EnableCycle:
            {
            bool cycle_enabled = uButton_GetCheck(hWnd, IDC_EnableCycle);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CycleTime), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_SKIP_NOCOVERS), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CYCLETIME_CAPTION), cycle_enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_CycleTime_Spin), cycle_enabled);
            if (!cycle_enabled)
            {
                m_config.old_cycletime = m_config.cycletime;
                m_config.cycletime = 0;
            }
            else
                m_config.cycletime = m_config.old_cycletime;
            }

            make_dirty(VC_CYCLE);
            break;
        case IDC_SKIP_NOCOVERS:
            m_config.skip_nocovers = uButton_GetCheck(hWnd, IDC_SKIP_NOCOVERS);
            vars_changed = true;
            break;
        case IDC_MdlClick:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.mdlclickfunc = (mouse_function)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                /* if (m_config.mdlclickfunc == FUNC_CUSTOM)
                {
                    GUID command = m_config.custom_menu_command;
                    command_select selector;
                    if (selector.run_command_select(command, hWnd))
                        m_config.custom_menu_command = command;
                }*/
                vars_changed = true;
            }
            break;
        case IDC_LftClick:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.lftclickfunc = (mouse_function)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                /*if (m_config.lftclickfunc == FUNC_CUSTOM)
                {
                    GUID command = m_config.custom_menu_command;
                    command_select selector;
                    if (selector.run_command_select(command, hWnd))
                        m_config.custom_menu_command = command;
                }*/
                vars_changed = true;
            }
            break;
        case IDC_DblClick:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                m_config.dblclickfunc = (mouse_function)uSendMessage((HWND)lParam,CB_GETCURSEL,0,0);
                /*if (m_config.dblclickfunc == FUNC_CUSTOM)
                {
                    GUID command = m_config.custom_menu_command;
                    command_select selector;
                    if (selector.run_command_select(command, hWnd))
                        m_config.custom_menu_command = command;
                }*/
                vars_changed = true;
            }
            break;
        case IDC_AnimTime:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (m_albumart_config_initialized)
                {
                    BOOL result=FALSE;
                    unsigned new_time = uGetDlgItemInt(hWnd, IDC_AnimTime, &result, FALSE);
                    if (result)
                    {
                        m_config.old_animtime = new_time;
                        m_config.animtime = new_time;
                        vars_changed = true;
                    }
                }
            }
            break;
        case IDC_CycleTime:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                if (m_albumart_config_initialized)
                {
                    BOOL result = FALSE;
                    unsigned new_time = uGetDlgItemInt(hWnd, IDC_CycleTime, &result, FALSE);
                    if (result)
                    {
                        m_config.old_cycletime = new_time;
                        m_config.cycletime = new_time;
                        make_dirty(VC_CYCLE);
                    }
                }
            }
            break;
        case IDC_CYCLE_WILDCARDS:
            {
                m_config.cycle_wildcards = uButton_GetCheck(hWnd, IDC_CYCLE_WILDCARDS);
                uEnableWindow(uGetDlgItem(hWnd, IDC_ORDER_CAPTION), m_config.cycle_wildcards);
                uEnableWindow(uGetDlgItem(hWnd, IDC_WILDCARD_ALPHA), m_config.cycle_wildcards);
                uEnableWindow(uGetDlgItem(hWnd, IDC_WILDCARD_RANDOM), m_config.cycle_wildcards);
                make_dirty(VC_SOURCES);
            }
            break;
        case IDC_WILDCARD_ALPHA:
            {
                m_config.cycle_order = WILDCARD_ALPHA;
                vars_changed = true;
            }
            break;
        case IDC_WILDCARD_RANDOM:
            {
                m_config.cycle_order = WILDCARD_RANDOM;
                vars_changed = true;
            }
            break;
        }

        if (vars_changed) make_dirty(VC_GENERAL);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR WINAPI albumart_config::SourcesTabDialogProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    albumart_config * p_this;
    
    if (msg == WM_INITDIALOG)
    {
        p_this = (albumart_config*)(lParam); //retrieve pointer to class
        uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
    }
    else
    {
        // if isnt wm_create, retrieve pointer to class
        p_this = reinterpret_cast<albumart_config*>(uGetWindowLong(hWnd,GWL_USERDATA));
    }
    return p_this ? p_this->OnSourcesTabMessage(hWnd, msg, wParam, lParam) : FALSE;
}

INT_PTR WINAPI albumart_config::OnSourcesTabMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool redraw_art = false;
    switch (msg)
    {
    case WM_INITDIALOG:
        {
            HWND list = uGetDlgItem(hWnd, IDC_List);
            t_size source_count = m_sources.get_count();
            for(t_size n = 0; n < source_count; n++)
            {
                uSendMessageText(list, LB_ADDSTRING, 0, m_sources[n]);
            }

            bool enabled = m_config.history_enabled;
            uButton_SetCheck(hWnd, IDC_HISTORY_ENABLE, enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_HISTORY_SIZE), enabled);
            uEnableWindow(uGetDlgItem(hWnd, IDC_HISTORY_SPIN), enabled);

            uSetDlgItemInt(hWnd, IDC_HISTORY_SIZE, m_config.history_size, false);

            uSendDlgItemMessage(hWnd,IDC_HISTORY_SPIN,UDM_SETRANGE32,0,10);

            uButton_SetCheck(hWnd, IDC_DEBUG_LOG_SOURCES, m_config.debug_log_sources);

            HWND remove_btn = uGetDlgItem(hWnd,IDC_Remove);
            LRESULT count = uSendMessage(list,LB_GETCOUNT,0,0);
            if (count == 1)
            {
                uEnableWindow(remove_btn,false);
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_List:
            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                HWND list = (HWND)lParam;
                LRESULT idx = uSendMessage(list,LB_GETCURSEL,0,0);
                if (idx != LB_ERR)
                {
                    pfc::string8 p = m_sources[idx];
                    edit_view editor;
                    if (editor.run_edit_view(p,hWnd))
                    {
                        m_sources[idx] = p;
                        uSendMessage(list,LB_DELETESTRING,idx,0);
                        uSendMessageText(list,LB_INSERTSTRING,idx,p);
                        uSendMessageText(list,LB_SETCURSEL,idx,0);
                        redraw_art = true;
                    }
                }
            }
            break;
        case IDC_Up:
            {
                HWND list = uGetDlgItem(hWnd,IDC_List);
                LRESULT idx = uSendMessage(list,LB_GETCURSEL,0,0);
                if (idx != LB_ERR && idx > 0)
                {
                    uSendMessage(list,LB_DELETESTRING,idx,0);
                    m_sources.swap_items(idx,idx-1);
                    uSendMessageText(list,LB_INSERTSTRING,idx-1,m_sources[idx-1]);
                    uSendMessage(list,LB_SETCURSEL,idx-1,0);
                    redraw_art = true;
                }
            }
            break;
        case IDC_Down:
            {
                HWND list = uGetDlgItem(hWnd,IDC_List);
                LRESULT idx = uSendMessage(list,LB_GETCURSEL,0,0);
                if (idx != LB_ERR && (t_size)(idx+1) < m_sources.get_count())
                {
                    uSendMessage(list,LB_DELETESTRING,idx,0);
                    m_sources.swap_items(idx,idx+1);
                    uSendMessageText(list,LB_INSERTSTRING,idx+1,m_sources[idx+1]);
                    uSendMessage(list,LB_SETCURSEL,idx+1,0);
                    redraw_art = true;
                }
            }
            break;
        case IDC_Remove:
            {
                HWND list = uGetDlgItem(hWnd,IDC_List);
                LRESULT idx = uSendMessage(list,LB_GETCURSEL,0,0);
                if (idx != LB_ERR)
                {
                    m_sources.remove_by_idx(idx);
                    uSendDlgItemMessage(hWnd,IDC_List,LB_DELETESTRING,idx,0);

                    redraw_art = true;

                    // disable the "Remove" button if there's only one source left in the list
                    LRESULT count = uSendMessage(list,LB_GETCOUNT,0,0);
                    if (count == 1)
                    {
                        HWND remove_btn = uGetDlgItem(hWnd,IDC_Remove);
                        EnableWindow(remove_btn,false);
                    }
                }
            }
            break;
        case IDC_Add:
            {
                pfc::string8 p;
                edit_view editor;
                if (editor.run_edit_view(p,hWnd))
                {
                    HWND list = uGetDlgItem(hWnd,IDC_List);
                    t_size n = m_sources.add_item(p);
                    uSendMessageText(list,LB_ADDSTRING,0,p);
                    uSendMessage(list,LB_SETCURSEL,n,0);
                    redraw_art = true;
                    
                    // make sure the "Remove" button is enabled
                    HWND remove_btn = uGetDlgItem(hWnd,IDC_Remove);
                    EnableWindow(remove_btn,true);
                }
            }
            break;
        case IDC_Reset:
            {
                if (uMessageBox(hWnd,"Reset sources to default?", "Reset",
                                MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK)
                {
                    t_size source_count = get_defsrclist_count();
                    m_sources.remove_all();
                    HWND list = uGetDlgItem(hWnd,IDC_List);
                    uSendMessage(list,LB_RESETCONTENT,0,0);
                    for (t_size n = 0; n < source_count; n++)
                    {
                        m_sources.add_item(get_def_source(n));
                        uSendMessageText(list,LB_ADDSTRING,0,get_def_source(n));
                    }

                    // make sure the "Remove button is enabled
                    if (source_count > 1)
                        EnableWindow(uGetDlgItem(hWnd, IDC_Remove),true);

                    redraw_art = true;
                }
            }
            break;
        case IDC_HISTORY_ENABLE:
            {
                bool enabled = uButton_GetCheck(hWnd, IDC_HISTORY_ENABLE);
                uEnableWindow(uGetDlgItem(hWnd, IDC_HISTORY_SIZE), enabled);
                uEnableWindow(uGetDlgItem(hWnd, IDC_HISTORY_SPIN), enabled);

                m_config.history_enabled = enabled;
                make_dirty(VC_GENERAL);
            }
            break;
        case IDC_HISTORY_SIZE:
            {
                if (HIWORD(wParam) == EN_CHANGE)
                {
                    if (m_albumart_config_initialized)
                    {
                        BOOL result = FALSE;
                        unsigned new_size = uGetDlgItemInt(hWnd, IDC_HISTORY_SIZE, &result, FALSE);

                        if (result)
                        {
                            m_config.history_size = new_size;
                            make_dirty(VC_GENERAL);
                        }
                    }
                }
            }
            break;
        case IDC_EXPORT:
            {
                export_sources(hWnd);
            }
            break;
        case IDC_IMPORT:
            {
                if (import_sources(hWnd))
                    redraw_art = true;
            }
            break;
        case IDC_DEBUG_LOG_SOURCES:
            {
                m_config.debug_log_sources = uButton_GetCheck(hWnd, IDC_DEBUG_LOG_SOURCES);
                make_dirty(VC_GENERAL);
            }
            break;
        }

        if (redraw_art) make_dirty(VC_SOURCES);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR WINAPI albumart_config::ConfigProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    albumart_config * p_this;
    
    if (msg == WM_INITDIALOG)
    {
        p_this = (albumart_config*)(lParam); //retrieve pointer to class
        uSetWindowLong(hWnd, GWL_USERDATA, (LPARAM)p_this); //store it for future use
    }
    else
    {
        // if isnt wm_create, retrieve pointer to class
        p_this = reinterpret_cast<albumart_config*>(uGetWindowLong(hWnd,GWL_USERDATA));
    }
    return p_this ? p_this->OnConfigMessage(hWnd, msg, wParam, lParam) : FALSE;
}

INT_PTR WINAPI albumart_config::OnConfigMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static HWND g_hWndTabDialog[NUM_TABS] = {NULL};
    static HWND g_hWndCurrentTab = NULL;
    static t_uint32 g_current_tab;

    switch(msg)
    {
    case WM_DESTROY:
        {
            m_albumart_config_initialized = false;
            pfc::fill_array_t(g_hWndTabDialog, (HWND)NULL);
            g_hWndCurrentTab = NULL;
        }
        break;

    case WM_INITDIALOG:
        {
            // disable "Apply" button
            uEnableWindow(uGetDlgItem(hWnd, IDC_APPLY), FALSE);

            // get handle of tab control
            HWND hWndTab = uGetDlgItem(hWnd, IDC_TAB);

            // set up tabs and create (invisible) subdialogs
            uTCITEM item= {0};
            item.mask = TCIF_TEXT;

            for (t_size n = 0; n < NUM_TABS; n++)
            {
                PFC_ASSERT(m_tab_table[n].m_pszName != NULL);

                item.pszText = m_tab_table[n].m_pszName;
                ::uTabCtrl_InsertItem(hWndTab, n, &item);

                g_hWndTabDialog[n] = m_tab_table[n].CreateTabDialog(hWnd, (LPARAM)this);
            }

            // get the size of the inner part of the tab control
            RECT rcTab;
            GetWindowRect (hWndTab, &rcTab);
            POINT p;
            p.x = rcTab.left;
            p.y = rcTab.top;
            ScreenToClient (hWnd, &p);
            rcTab.left = p.x;
            rcTab.top  = p.y;
            p.x = rcTab.right;
            p.y = rcTab.bottom;
            ScreenToClient (hWnd, &p);
            rcTab.right = p.x;
            rcTab.bottom  = p.y;
            //GetChildRect(hWnd, IDC_TAB, &rcTab);
            uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTab);

            // now resize tab control and entire prefs window to fit all controls
            // (it is necessary to manually move the buttons and resize the dialog
            //  to ensure correct display on large font (120dpi) screens)

            // Tab Control
            RECT rcTabDialog;
            GetClientRect(g_hWndTabDialog[0], &rcTabDialog);
            OffsetRect(&rcTabDialog, rcTab.left, rcTab.top);
            rcTabDialog.bottom = (rcTabDialog.bottom>rcTab.bottom)
                                 ?rcTabDialog.bottom:rcTab.bottom;
            rcTabDialog.right = (rcTabDialog.right>rcTab.right)
                                ?rcTabDialog.right:rcTab.right;

            uSendMessage(hWndTab, TCM_ADJUSTRECT, TRUE, (LPARAM)&rcTabDialog);
            ::SetWindowPos(hWndTab, NULL,
                rcTabDialog.left, rcTabDialog.top,
                rcTabDialog.right - rcTabDialog.left, rcTabDialog.bottom - rcTabDialog.top,
                SWP_NOZORDER | SWP_NOACTIVATE);

            RECT rcCtrl;

            // Reset All Button
            rcCtrl.left = 0;
            rcCtrl.top = 3;
            rcCtrl.right = rcCtrl.left + 1;
            rcCtrl.bottom = rcCtrl.top + 1;
            ::MapDialogRect(hWnd, &rcCtrl);
            ::OffsetRect(&rcCtrl, rcTabDialog.left, rcTabDialog.bottom);
            ::SetWindowPos(uGetDlgItem(hWnd, IDC_RESET_ALL), NULL,
                rcCtrl.left, rcCtrl.top, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

            // Help Button
            rcCtrl.left = -41;
            rcCtrl.top = 3;
            ::MapDialogRect(hWnd, &rcCtrl);
            ::OffsetRect(&rcCtrl, rcTabDialog.right, rcTabDialog.bottom);
            ::SetWindowPos(uGetDlgItem(hWnd, IDC_WIKIHELP), NULL,
                rcCtrl.left, rcCtrl.top, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

            // Apply Button
            rcCtrl.left = -86;
            rcCtrl.top = 3;
            ::MapDialogRect(hWnd, &rcCtrl);
            ::OffsetRect(&rcCtrl, rcTabDialog.right, rcTabDialog.bottom);
            ::SetWindowPos(uGetDlgItem(hWnd, IDC_APPLY), NULL,
                rcCtrl.left, rcCtrl.top, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

            // Cancel Button
            rcCtrl.left = -131;
            rcCtrl.top = 3;
            ::MapDialogRect(hWnd, &rcCtrl);
            ::OffsetRect(&rcCtrl, rcTabDialog.right, rcTabDialog.bottom);
            ::SetWindowPos(uGetDlgItem(hWnd, IDCANCEL), NULL,
                rcCtrl.left, rcCtrl.top, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

            // OK Button
            rcCtrl.left = -176;
            rcCtrl.top = 3;
            ::MapDialogRect(hWnd, &rcCtrl);
            ::OffsetRect(&rcCtrl, rcTabDialog.right, rcTabDialog.bottom);
            ::SetWindowPos(uGetDlgItem(hWnd, IDOK), NULL,
                rcCtrl.left, rcCtrl.top, 0, 0,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);

            // now resize the entire window
            ::SetRect(&rcCtrl, 0, 0, 6, 26);
            ::MapDialogRect(hWnd, &rcCtrl);
            rcCtrl.right += rcTabDialog.right;
            rcCtrl.bottom += rcTabDialog.bottom + GetSystemMetrics(SM_CYCAPTION);
            ::SetWindowPos(hWnd, NULL,
                rcCtrl.left, rcCtrl.top, rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top,
                SWP_NOZORDER | SWP_NOMOVE);

            // position the subdialogs in the inner part of the tab control
            uSendMessage(hWndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rcTabDialog);

            for (t_size n = 0; n < tabsize(g_hWndTabDialog); n++)
            {
                if (g_hWndTabDialog[n] != NULL)
                {
                    ::SetWindowPos(g_hWndTabDialog[n], NULL,
                        rcTabDialog.left, rcTabDialog.top,
                        rcTabDialog.right - rcTabDialog.left, rcTabDialog.bottom - rcTabDialog.top,
                        SWP_NOZORDER | SWP_NOACTIVATE);
                    if (m_hUxtheme != NULL && m_EnableThemeDialog != NULL)
                        m_EnableThemeDialog(g_hWndTabDialog[n], ETDT_ENABLETAB);
                }
            }

            // select last used tab and show corresponding subdialog
            g_current_tab = m_config.last_tab;
            if (g_current_tab >= tabsize(g_hWndTabDialog))
                g_current_tab = 0;
            uSendMessage(hWndTab, TCM_SETCURSEL, g_current_tab, 0);

            g_hWndCurrentTab = g_hWndTabDialog[g_current_tab];
            ShowWindow(g_hWndCurrentTab, SW_SHOW);

            // we are done setting up the dialog
            m_albumart_config_initialized = true;
            m_hWnd = hWnd;
            m_refreshing = false;
        }
        break;

    case WM_NOTIFY:
        if (((LPNMHDR)lParam)->idFrom == IDC_TAB)
        {
            switch (((LPNMHDR)lParam)->code)
            {
            case TCN_SELCHANGE:
                {
                    if (g_hWndCurrentTab != NULL)
                        ::ShowWindow(g_hWndCurrentTab, SW_HIDE);
                    g_hWndCurrentTab = NULL;

                    g_current_tab = (t_uint32)::SendDlgItemMessage(hWnd, IDC_TAB, TCM_GETCURSEL, 0, 0);
                    if (g_current_tab < tabsize(g_hWndTabDialog))
                    {
                        g_hWndCurrentTab = g_hWndTabDialog[g_current_tab];
                        ::ShowWindow(g_hWndCurrentTab, SW_SHOW);
                    }
                }
                break;
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            update_config(g_current_tab);
            EndDialog(hWnd, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hWnd, FALSE);
            break;
        case IDC_APPLY:
            update_config(g_current_tab);
            m_dirty = false;
            m_vars_changed = 0;
            uEnableWindow(uGetDlgItem(hWnd, IDC_APPLY), FALSE);
            break;
        case IDC_WIKIHELP:
            uShellExecute(0,"open","http://wiki.hydrogenaudio.org/index.php?title="
                          "Foobar2000:Components_0.9/Album_Art_Panel_%28foo_uie_albumart%29",0,0,SW_SHOW);
            break;
        case IDC_RESET_ALL:
            if (uMessageBox(hWnd, "Reset all settings to default?", "Reset",
                            MB_OKCANCEL | MB_ICONQUESTION | MB_DEFBUTTON2) == IDOK)
            {
                reset_config_vars(m_config, m_sources);
                update_config(g_current_tab);
                m_refreshing = true;
                EndDialog(hWnd, TRUE);
            }
            break;
        }
        break;

    case WM_SYSCOMMAND:
        // handler for user pressing "X" button in upper-right corner
        if (wParam == SC_CLOSE)
        {
            if (confirm_lost_changes(hWnd, g_current_tab)) return TRUE;
        }
        break;

    default:
        return FALSE;
    }
    return FALSE;
}

// returns TRUE if user cancels close operation
bool albumart_config::confirm_lost_changes(HWND hWnd, int last_tab)
{
    if (m_dirty)
    {
        int msg_box_val;
        msg_box_val = uMessageBox(hWnd, "Save changes?", "Question",
                                  MB_YESNOCANCEL | MB_ICONQUESTION | MB_DEFBUTTON1);

        switch (msg_box_val)
        {
        case IDYES:
            update_config(last_tab);
            EndDialog(hWnd, TRUE);
            break;
        case IDNO:
            EndDialog(hWnd, FALSE);
            break;
        case IDCANCEL:
            return TRUE;
            break;
        }
    }
    else
    {
        EndDialog(hWnd, FALSE);
    }

    return FALSE;
}

void albumart_config::export_sources(HWND hWnd)
{
    pfc::string8 file_path;
    pfc::string8 current_dir;

    if (!uGetCurrentDirectory(current_dir))
    {
        current_dir = core_api::get_profile_path();
    }

    if (!uGetOpenFileName(hWnd,"Text Files (*.txt)|*.txt|All Files (*.*)|*.*", 0 , "*.txt",
                          "Save As", current_dir, file_path, true))
    {
        return;
    }

    multiline_string out_text;

    // add identifier
    out_text.add_line("[foo_uie_albumart]");

    // add source strings
    t_size source_count = m_sources.get_count();
    for (t_size n = 0; n < source_count; n++)
    {
        out_text.add_line(m_sources[n]);
    }

    out_text.add_line("[End]");

    pfc::string8 temp;
    out_text.get_string(temp);

    // write the text to file
    abort_callback_impl p_abort;
    try
    {
        text_file_loader::write(file_path, p_abort, temp, !is_lower_ascii(temp));
    }
    catch (exception_io)
    {
        uMessageBox(hWnd, "Error writing file", "Error", MB_OK);
        return;
    }

    uMessageBox(hWnd, "Export complete.", "Message", MB_OK);
}

bool albumart_config::import_sources(HWND hWnd)
{
    pfc::string8 file_path;
    pfc::string8 current_dir;

    if (!uGetCurrentDirectory(current_dir))
    {
        current_dir = core_api::get_profile_path();
    }

    if (!uGetOpenFileName(hWnd, "Text Files (*.txt)|*.txt|All Files (*.*)|*.*", 0 , "*.txt",
                          "Open", current_dir, file_path, false))
    {
        return false;
    }

    // try to open the file
    abort_callback_impl p_abort;
    pfc::string8 temp;
    bool is_utf8;

    try
    {
        text_file_loader::read(file_path, p_abort, temp, is_utf8);
    }
    catch (exception_io)
    {
        uMessageBox(hWnd, "Error opening file", "Error", MB_OK);
        return false;
    }

    multiline_string in_text(temp);

    pfc::string8 line;
    if (!in_text.read_line(line))
    {
        uMessageBox(hWnd, "Error reading from file", "Error", MB_OK);
        return false;
    }
    if (stricmp_utf8(line, "[foo_uie_albumart]") != 0)
    {
        uMessageBox(hWnd, "Source list must begin with \"[foo_uie_albumart]\"", "Error", MB_OK);
        return false;
    }

    if (uMessageBox(hWnd, "Replace all sources with the contents of text file?",
        "Confirm", MB_OKCANCEL) == IDCANCEL)
    {
        return false;
    }

    HWND list = uGetDlgItem(hWnd, IDC_List);
    uSendMessage(list,LB_RESETCONTENT,0,0);
    m_sources.remove_all();

    while (in_text.read_line(line))
    {
        if (stricmp_utf8(line, "[End]") == 0)
            break;

        uSendMessageText(list,LB_ADDSTRING,0,line);
        m_sources.add_item(line);
    }

    return true;
}

inline void albumart_config::update_config(int last_tab)
{
    m_config.last_tab = last_tab;
    m_parent->set_config_vars(m_vars_changed, m_config, m_sources);
}

void albumart_config::make_dirty(var_change change_type)
{
    m_vars_changed |= change_type;
    m_dirty = true;
    uEnableWindow(uGetDlgItem(m_hWnd, IDC_APPLY), TRUE);
}

bool albumart_config::run(HWND parent)
{
    bool dialog_box_return_val = false;

    do
    {
        dialog_box_return_val = !!uDialogBox(IDD_ALBUMART_CONFIG,parent,ConfigProc, (LPARAM)this);
    } while (m_refreshing);

    return dialog_box_return_val;
}
