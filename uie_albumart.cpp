#include "stdafx.h"

DECLARE_COMPONENT_VERSION(
    "Album Art Panel",
    "0.2.7",
    "A Columns UI extension by Nathan Kallus.\n"
    "Modified by David L to fix redrawing issues and add animation\n"
    "Ported to foobar2000 0.9 by G-Lite\n"
    "Various changes including code clean-up and preferences redesign by Holger Stenger\n"
    "Several bug fixes and feature additions by Cheran Shunmugavel\n"
    "Ported to foobar2000 1.0 by Major(Duny). Implemented extracting album art from archives and various tags formats read/write\n"
    "\ncompiled: " __DATE__ "\n"
    "with Panel API version: " UI_EXTENSION_VERSION)

// {E32B3859-B6BC-48e1-8370-3D5F5FA00A60}
const GUID uie_albumart::albumart_guid =
    { 0xE32B3859, 0xB6BC, 0x48e1, { 0x83, 0x70, 0x3D, 0x5F, 0x5F, 0xA0, 0x0A, 0x60 } };

static uie::window_factory<uie_albumart> foo_uie_albumart;

static service_factory_single_t<albumart_ns_manager_impl> g_albumart_ns_manager_factory;
static service_factory_single_t<albumart_node_select_callback_impl> g_albumart_ns_callback_impl_factory;

uie_albumart::uie_albumart() {
    m_animshouldstart=m_animstarted=m_pan_dx=m_pan_dy=0;
    m_animating=false;
    m_panning_enabled = false;
    m_dragging = false;
    b_dblclick = false;
    m_config_changed = false;
    m_image_file_exists = false;
    m_hWnd = NULL;
    reset_config_vars(m_config, m_sources);
}

bool uie_albumart::show_config_popup(HWND wnd_parent)
{
    albumart_config prefs(this);
    return prefs.run(wnd_parent);
}

void uie_albumart::set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort)
{
    // first reset the config, in case there's an error in the reading
    reset_config_vars(m_config, m_sources);

    albumart_vars buf_config;
    list_t<string8> buf_sources;

    version_control vc;
    vc.read_config(p_reader, p_size, p_abort, buf_config, buf_sources);

    m_config = buf_config;
    m_sources = buf_sources;
    
    m_sources_control.set_config_vars(m_config, m_sources);
}

void uie_albumart::get_config(stream_writer * p_writer, abort_callback & p_abort) const
{
    version_control vc;
    vc.write_config(p_writer, p_abort, m_config, m_sources);
}

void uie_albumart::get_config_vars(albumart_vars & p_config, list_t<string8> & p_src_list)
{
    p_config = m_config;
    p_src_list = m_sources;
}

void uie_albumart::set_config_vars(unsigned int change_type, albumart_vars & p_config, list_t<string8> & p_src_list)
{
    // update variables
    m_config = p_config;
    m_sources = p_src_list;

    m_sources_control.set_config_vars(m_config, m_sources);
    m_config_changed = true;

    if (m_hWnd != NULL)
    {
        if ((change_type & VC_MIN_HEIGHT) != 0)
        {
            m_host->on_size_limit_change(m_hWnd,uie::size_limit_minimum_height);
        }
        if ((change_type & VC_MIN_WIDTH) != 0)
        {
            m_host->on_size_limit_change(m_hWnd,uie::size_limit_minimum_width);
        }
        if ((change_type & VC_FOLLOW) != 0)
        {
            // "Follow Cursor" has already been toggled, so we
            // just need to refresh the image
        }
        if ((change_type & VC_CYCLE) != 0)
        {
            // do nothing here since cycle timer
            // is started during a refresh
        }
        if ((change_type & VC_SOURCES) != 0)
        {
            m_sources_control.set_current_source(0);
            m_sources_control.refresh(true);
            return;
        }
        if ((change_type & VC_GENERAL) != 0)
        {
            // do nothing here since the screen and image are
            // refreshed below, and that's all we want to have happen
        }
        if ((change_type & VC_EDGESTYLE) != 0)
        {
            SetWindowLong(m_hWnd, GWL_EXSTYLE, exstyle_from_config(m_config.edge_style));
            SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED);
        }

        func_Refresh(false);
    }
}

HWND uie_albumart::create_or_transfer_window(HWND wnd_parent, const uie::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position)
{
    if (m_hWnd == NULL) {
        static const TCHAR g_class_name[] = _T("foo_uie_albumart.dll window class");
        static ATOM g_class_atom = 0;
        if (g_class_atom == 0)
        {
            WNDCLASS wc;
            memset(&wc,0,sizeof(wc));
            wc.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
            wc.lpfnWndProc = host_proc;
            wc.hInstance = core_api::get_my_instance();
            wc.hCursor = ::LoadCursor(0,IDC_ARROW);
            wc.hbrBackground = NULL;
            wc.lpszClassName = g_class_name;
            g_class_atom = RegisterClass(&wc);
        }

        m_hWnd = ::CreateWindowEx(exstyle_from_config(m_config.edge_style),
            MAKEINTATOM(g_class_atom),_T("Album Art"),
            WS_CHILD,
            CW_USEDEFAULT,CW_USEDEFAULT,
            0,0,
            wnd_parent,0,core_api::get_my_instance(),this);
    } else {
        ::ShowWindow(m_hWnd, SW_HIDE);
        ::SetParent(m_hWnd, wnd_parent);
        ::SetWindowPos(m_hWnd, NULL, p_position.x, p_position.y, p_position.cx, p_position.cy, SWP_NOZORDER);
        m_host->relinquish_ownership(m_hWnd);
        m_host.release();
    }
    m_host = p_host;
    return m_hWnd;
}

void uie_albumart::destroy_window()
{
    if (m_hWnd != NULL) {
        DestroyWindow(m_hWnd);
        m_hWnd = NULL;
    }
    m_host.release();
}

LRESULT WINAPI uie_albumart::host_proc(HWND wnd1,UINT msg,WPARAM wp,LPARAM lp)
{
    uie_albumart * p_this;
    
    if(msg == WM_NCCREATE)
    {
        p_this = (uie_albumart *)((CREATESTRUCT *)(lp))->lpCreateParams; //retrieve pointer to class
        SetWindowLongPtr(wnd1, GWL_USERDATA, (LPARAM)p_this);//store it for future use
    }else{
        p_this = reinterpret_cast<uie_albumart*>(GetWindowLongPtr(wnd1,GWL_USERDATA));//if isnt wm_create, retrieve pointer to class
    }
    return p_this ? p_this->on_message(wnd1, msg, wp, lp) : uDefWindowProc(wnd1, msg, wp, lp);
}

inline void uie_albumart::set_anim_timer()
{
    SetTimer(m_hWnd, animation_timer_id, 10, NULL);
}

// checks whether cycle timer should start
// if so, starts the cycle timer, otherwise
// stops the cycle timer
void uie_albumart::start_cycle_timer()
{
    if (m_config.cycletime > 0
        && m_sources_control.get_display_mode() == sources_control::display_playing)
    {
        SetTimer(m_hWnd, cycle_timer_id, m_config.cycletime*1000, NULL);
        m_sources_control.set_skip_nocovers(m_config.skip_nocovers);
    }
    else
    {
        stop_cycle_timer();
    }
}

inline void uie_albumart::stop_cycle_timer()
{
    KillTimer(m_hWnd, cycle_timer_id);
    m_sources_control.set_skip_nocovers(false);
}

LRESULT uie_albumart::on_message(HWND wnd1,UINT msg,WPARAM wp,LPARAM lp)
{
    switch(msg)
    {
    case WM_CREATE:
        {
            GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

            m_sources_control.register_foo_callbacks();
            m_sources_control.register_sources_control_callback(this);
            m_sources_control.setup_sources_control(m_config, m_sources);

            m_wheel_accum = 0;
        }
        break;

    case WM_DESTROY:
        {
            m_sources_control.unregister_sources_control_callback(this);
            m_sources_control.unregister_foo_callbacks();

            m_bufold.release();
            m_bufnew.release();
            m_bufanim.release();

            m_bmpnew.release();
            m_bmp.release();

            GdiplusShutdown(gdiplusToken);
        }
        break;

    case WM_LBUTTONDOWN:
        {
            POINTS pts = MAKEPOINTS(lp);
            POINT pt;
            POINTSTOPOINT(pt, pts);
            if (m_panning_enabled)
            {
                m_dragging = true;
                SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                SetCapture(wnd1);

                m_drag_start = pt;
                m_orig_dx = m_pan_dx;
                m_orig_dy = m_pan_dy;
            }
        }
        break;
    case WM_MOUSEMOVE:
        {
            if (m_dragging)
            {
                POINTS pts = MAKEPOINTS(lp);
                m_pan_dx = m_orig_dx + pts.x - m_drag_start.x;
                m_pan_dy = m_orig_dy + pts.y - m_drag_start.y;
                redraw();
            }
        }
        break;
    case WM_CANCELMODE:
        if (m_dragging)
        {
            ReleaseCapture();
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            m_dragging = false;
        }
        break;

    case WM_LBUTTONDBLCLK:
        click_func(m_config.dblclickfunc);
        b_dblclick = true;
        break;
    case WM_LBUTTONUP:
        if (m_dragging)
        {
            ReleaseCapture();
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            m_dragging = false;

            // if the mouse didn't move much, it probably wasn't
            // a drag, so trigger the mouse action
            POINTS pts = MAKEPOINTS(lp);
            POINT pt;
            POINTSTOPOINT(pt, pts);

            int cxdrag = GetSystemMetrics(SM_CXDRAG);
            int cydrag = GetSystemMetrics(SM_CYDRAG);

            RECT rect;
            SetRect(&rect, -1*cxdrag/2, -1*cydrag/2, cxdrag/2, cydrag/2);
            OffsetRect(&rect, m_drag_start.x, m_drag_start.y);

            if (PtInRect(&rect, pt)) click_func(m_config.lftclickfunc);
        }
        else if (!b_dblclick)
        {
            click_func(m_config.lftclickfunc);
        }
        b_dblclick = false;
        break;
    case WM_MBUTTONUP:
        click_func(m_config.mdlclickfunc);
        break;

    case WM_MOUSEWHEEL:
        {
            short delta = GET_WHEEL_DELTA_WPARAM(wp);
            m_wheel_accum += delta;
            while (m_wheel_accum <= -WHEEL_DELTA)
            {
                m_wheel_accum += WHEEL_DELTA;
                func_NextSource(true);
            }
            while (m_wheel_accum >= WHEEL_DELTA)
            {
                m_wheel_accum -= WHEEL_DELTA;
                func_PreviousSource();
            }
        }
        break;

    case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;
            hdc = BeginPaint(m_hWnd, &ps);
            paint(hdc);
            EndPaint(m_hWnd, &ps);
            return 0;
        }
        break;
    case WM_TIMER:
        switch (wp)
        {
        case animation_timer_id:
            KillTimer(m_hWnd, animation_timer_id);
            redraw();
            return 0;
            break;
        case cycle_timer_id:
            if (static_api_ptr_t<ui_control>()->is_visible())
            {
                func_NextSource(false);
            }
            else
            {
                start_cycle_timer();
            }
            return 0;
            break;
        }
        break;
    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO mmi = LPMINMAXINFO(lp);
            mmi->ptMinTrackSize.y = m_config.minheight;
            mmi->ptMinTrackSize.x = m_config.minwidth;
        }
        break;
    }
    return uDefWindowProc(wnd1, msg, wp, lp);
}

class albumart_menu_functions : public uie::menu_node_command_t
{
private:
    int func_idx;
    bool * checked;
    uie_albumart * parent;
public:
    bool get_display_data(string_base & p_out,unsigned & p_state) const
    {
        p_out = get_function_name(func_idx);
        if (checked != NULL && *checked)
            p_state = state_checked;
        else
            p_state = 0;
        return true;
    }
    bool get_description(string_base & p_out) const
    {
        return false;
    }
    virtual void execute()
    {
        parent->click_func((mouse_function)func_idx);
    }

    albumart_menu_functions(int p_func_idx, uie_albumart * p_parent = NULL, bool * p_checked = NULL)
    {
        func_idx = p_func_idx;
        checked = p_checked;
        parent = p_parent;
    }
};

class albumart_menu_preferences : public uie::menu_node_command_t
{
private:
    HWND m_hWnd;
    uie_albumart * m_parent;
public:
    albumart_menu_preferences(HWND wnd, uie_albumart * parent)
    {
        m_hWnd = wnd;
        m_parent = parent;
    }
    bool get_display_data(string_base & p_out,unsigned & p_state) const
    {
        p_out = "Preferences...";
        p_state = 0;
        return true;
    }
    bool get_description(string_base & p_out) const
    {
        return false;
    }
    virtual void execute()
    {
        albumart_config prefs(m_parent);
        prefs.run(m_hWnd);
    }
};


void uie_albumart::get_menu_items (uie::menu_hook_t & p_hook)
{
    uie::menu_node_ptr item;
    for(t_size i = 1; i < get_function_count(); i++)
    {
        if (i == FUNC_FOLLOW)
            item = new albumart_menu_functions(i, this, &m_config.selected);
        else
            item = new albumart_menu_functions(i, this);
        p_hook.add_node(item);
    }
    
    item = new uie::menu_node_separator_t();
    p_hook.add_node(item);
    item = new albumart_menu_preferences(m_hWnd, this);
    p_hook.add_node(item);
}

inline void uie_albumart::redraw()
{
    RedrawWindow(m_hWnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
}

void uie_albumart::click_func(mouse_function func)
{
    switch(func)
    {
    case FUNC_NULL:
        // do nothing
        break;
    case FUNC_OPENEX:
        func_OpenEx();
        break;
    case FUNC_OPENDIR:
        func_OpenDir();
        break;
    case FUNC_FOLLOW:
        func_ToggleFollow();
        break;
    case FUNC_FOCUSNP:
        func_FocusPlaying();
        break;
    case FUNC_REFRESH:
        func_Refresh(true);
        break;
    case FUNC_NEXTSRC:
        func_NextSource(true);
        break;
    case FUNC_PREVSRC:
        func_PreviousSource();
        break;
    case FUNC_FIRSTSRC:
        func_FirstSource();
        break;
    case FUNC_COPYNAME:
        func_CopyName();
        break;
    }
}

void uie_albumart::func_OpenEx()
{
    string8 path;
    m_sources_control.get_current_match(path);
    if (!path.is_empty())
        uShellExecute(0,"open",path,0,0,SW_SHOW);
}

void uie_albumart::func_OpenDir()
{
    metadb_handle_ptr track;
    if (m_sources_control.get_displayed_track(track))
    {
        list_t<metadb_handle_ptr> list;
        list.add_item(track);
        standard_commands::context_file_open_directory(list);
    }
}

void uie_albumart::func_ToggleFollow()
{
    m_config.selected = !m_config.selected;
    m_sources_control.set_config_vars(m_config, m_sources);
    func_Refresh(false);
}

void uie_albumart::func_FocusPlaying()
{
    static_api_ptr_t<playlist_manager>()->highlight_playing_item();
}

void uie_albumart::func_Refresh(bool p_full_refresh)
{
    m_sources_control.refresh(p_full_refresh);
}

void uie_albumart::func_FirstSource()
{
    m_sources_control.set_current_source(0);
    m_sources_control.refresh(false);
}

// - user_action: true if user initiated NextSource (eg, by mouse-click)
void uie_albumart::func_NextSource(bool user_action)
{
    // reset cycle timer to 0 by stopping the timer
    if (user_action)
        stop_cycle_timer();

    // use helper function to search for a valid source
    // loops forward until a match is found
    m_sources_control.next_source();
}

void uie_albumart::func_PreviousSource()
{
    m_sources_control.previous_source();
}

void uie_albumart::func_CopyName()
{
    if (!OpenClipboard(m_hWnd))
        return;

    if (!EmptyClipboard())
    {
        CloseClipboard();
        return;
    }

    t_size len = m_path.get_length();
    HGLOBAL hglbCopyMem = GlobalAlloc(GMEM_MOVEABLE,
                                      (len+1)*sizeof(TCHAR));
    if (hglbCopyMem == NULL)
    {
        CloseClipboard();
        return;
    }

    LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hglbCopyMem);
    stringcvt::string_wide_from_utf8 path_wide(m_path);
    _tcscpy_s(lptstrCopy, len+1, path_wide);
    lptstrCopy[len] = 0;
    GlobalUnlock(hglbCopyMem);

    if (SetClipboardData(CF_UNICODETEXT, hglbCopyMem) == NULL)
    {
        GlobalFree(hglbCopyMem);
        CloseClipboard();
        return;
    }

    CloseClipboard();
}

void uie_albumart::on_sources_control_new_image()
{
    m_bmp.release();
    m_bmpnew.release();

    if (m_sources_control.get_current_bitmap(m_bmp))
    {
        m_image_file_exists = true;
        m_bmpnew = m_bmp;
        m_sources_control.get_current_match(m_path);
    }
    else
    {
        m_image_file_exists = false;
    }

    redraw();

    if (!GetSystemMetrics(SM_REMOTESESSION))
    {
        m_animshouldstart = GetTickCount();
        start_cycle_timer();
    }
}

void uie_albumart::paint(HDC hdc)
{
    RECT wndrect_temp;
    if(GetClientRect(m_hWnd,&wndrect_temp))
    {
        Graphics gfx(hdc);
        Rect wndrect(
            wndrect_temp.left,
            wndrect_temp.top,
            wndrect_temp.right-wndrect_temp.left,
            wndrect_temp.bottom-wndrect_temp.top
            );

        Color bg;
        bg.SetFromCOLORREF((m_config.bg_enabled)
                           ?m_config.bgcol
                           :GetSysColor(COLOR_3DFACE));

        bool noimage = m_bufnew.is_empty();
        bool image_changed = !m_image_file_exists || m_bmpnew.is_valid();
        bool size_changed = m_bufnew.is_valid()
            ? (m_bufnew->GetWidth() != wndrect.Width || m_bufnew->GetHeight() != wndrect.Height)
            : true;
        if (noimage || image_changed || size_changed || m_config_changed || m_dragging)
        {
            if (!m_dragging)
            {
                m_pan_dx = m_pan_dy = 0;
            }
            else
            {
                m_bmpnew = m_bmp;
            }

            if (size_changed || noimage)
            {
                // create bitmaps with new size
                m_animating = false;
                m_bufold.release();
                m_bufanim.release();

                m_bufold = new Bitmap(wndrect.Width,wndrect.Height,&gfx);
                m_bufnew = new Bitmap(wndrect.Width,wndrect.Height,&gfx);

                // make sure currently displayed image is redrawn
                // during the resize
                m_bmpnew = m_bmp;
            }
            if (m_config.animtime == 0)
                m_bufanim.release();
            else if (m_bufanim.is_empty())
                m_bufanim=new Bitmap(wndrect.Width,wndrect.Height,&gfx);
            
            m_config_changed = false;

            Bitmap *temp = m_bufold.detach();
            m_bufold.set(m_bufnew.detach());
            m_bufnew.set(temp);

            Graphics bufgfx(m_bufnew.get_ptr());
            bufgfx.Clear(bg);

            if (m_bmpnew.is_valid())
            {
                // adjust for padding
                if(m_config.padding > 0)
                {
                    wndrect.Height-=2*m_config.padding;
                    wndrect.Width-=2*m_config.padding;
                    wndrect.X+=m_config.padding;
                    wndrect.Y+=m_config.padding;
                }

                Size bmp_dim(m_bmpnew->GetWidth(), m_bmpnew->GetHeight());

                // rectangle that will be drawn on the screen
                RectF rect(wndrect.X, wndrect.Y, bmp_dim.Width, bmp_dim.Height);

                // shrink album art
                if (m_config.shrinkalbum)
                {
                    if (bmp_dim.Width > wndrect.Width)
                        rect.Width = wndrect.Width;

                    if (bmp_dim.Height > wndrect.Height)
                        rect.Height = wndrect.Height;
                }

                // expand album art
                if (m_config.expandalbum)
                {
                    if (bmp_dim.Width < wndrect.Width)
                        rect.Width = wndrect.Width;

                    if (bmp_dim.Height < wndrect.Height)
                        rect.Height = wndrect.Height;
                }

                // correct aspect ratio
                if (m_config.aspectratio)
                {
                    REAL new_width = bmp_dim.Width*(rect.Height/bmp_dim.Height);
                    REAL new_height = bmp_dim.Height*(rect.Width/bmp_dim.Width);

                    if (m_config.expandalbum && !m_config.shrinkalbum)
                    {
                        // try to fill the entire window, even if parts of the
                        // image are cut off (enlarge image)
                        if (new_height >= wndrect.Height)
                        {
                            rect.Height = new_height;
                        }
                        else
                        {
                            if (new_width >= wndrect.Width)
                            {
                                rect.Width = new_width;
                            }
                        }
                    }
                    else
                    {
                        // do not cut off any part of the image while
                        // maintaining aspect ratio (shrink image)
                        if (new_height <= wndrect.Height)
                        {
                            rect.Height = new_height;
                        }
                        else
                        {
                            if (new_width <= wndrect.Width)
                            {
                                rect.Width = new_width;
                            }
                        }
                    }
                }

                // Center album
                // uses integers for the offsets because if the offset
                // is a fraction of a pixel, there's lowered quality
                if (m_config.centeralbum)
                {
                    int center_dx = (wndrect.GetRight() - rect.GetRight()) / 2;
                    int center_dy = (wndrect.GetBottom() - rect.GetBottom()) / 2;
                    rect.Offset((REAL)center_dx,
                                (REAL)center_dy);
                }

                if (m_config.draw_pixel_border)
                {
                    Gdiplus::Color border;
                    border.SetFromCOLORREF(m_config.bordercol);

                    rect.Width--;
                    rect.Height--;
                    bufgfx.DrawRectangle(& Pen(border),rect);
                    rect.X++;
                    rect.Y++;
                    rect.Width--;
                    rect.Height--;
                }

                m_panning_enabled = ((rect.Width > wndrect.Width) ||
                                     (rect.Height > wndrect.Height));

                // panning
                REAL dx = m_pan_dx;
                REAL dy = m_pan_dy;
                if (m_dragging)
                {
                    // Make sure the image doesn't pan out of bounds
                    if (m_panning_enabled)
                    {
                        if (rect.Width > wndrect.Width)
                        {
                            if ((rect.GetLeft()+dx) > wndrect.GetLeft())
                                dx = wndrect.GetLeft() - rect.GetLeft();
                            if ((rect.GetRight()+dx) < wndrect.GetRight())
                                dx = wndrect.GetRight() - rect.GetRight();
                        }

                        if (rect.Height > wndrect.Height)
                        {
                            if ((rect.GetTop()+dy) > wndrect.GetTop())
                                dy = wndrect.GetTop() - rect.GetTop();
                            if ((rect.GetBottom()+dy) < wndrect.GetBottom())
                                dy = wndrect.GetBottom() - rect.GetBottom();
                        }
                    }
                    else
                    {
                        dx = 0;
                        dy = 0;
                    }

                    if (rect.Width <= wndrect.Width) dx = 0;
                    if (rect.Height <= wndrect.Height) dy = 0;

                    m_pan_dx = dx;
                    m_pan_dy = dy;
                }
                rect.Offset(dx,dy);

                bufgfx.SetInterpolationMode(m_config.interpolationmode == RESIZE_HIGH
                    ? InterpolationModeHighQuality
                    : m_config.interpolationmode == RESIZE_MEDIUM
                        ? InterpolationModeDefault
                        : m_config.interpolationmode == RESIZE_LOW
                            ? InterpolationModeLowQuality
                            // m_config.interpolationmode == RESIZE_HIGHEST
                            : InterpolationModeHighQualityBicubic);

                if (m_dragging) bufgfx.SetInterpolationMode(InterpolationModeLowQuality);

                REAL srcWidth = m_bmpnew->GetWidth();
                REAL srcHeight = m_bmpnew->GetHeight();
                REAL srcX = 0;
                REAL srcY = 0;

                /*if (m_config.padding < 0)
                {
                    if(abs(m_config.padding) < m_bmpnew->GetWidth() &&
                       abs(m_config.padding) < m_bmpnew->GetHeight())
                    {
                        srcWidth+=2*m_config.padding;
                        srcHeight+=2*m_config.padding;
                        srcX-=m_config.padding;
                        srcY-=m_config.padding;
                    }
                }*/

                bufgfx.SetClip(wndrect);    // ensures that padding is respected
                bufgfx.DrawImage(
                    &(*m_bmpnew),
                    rect,         // destination rectangle 
                    srcX, srcY,   // upper-left corner of source rectangle
                    srcWidth,     // width of source rectangle
                    srcHeight,    // height of source rectangle
                    UnitPixel);
            }

            // m_bufnew updated. time to fade in the changes.
            DWORD now = GetTickCount();
            if (m_config.animtime != 0 && !size_changed && (m_animshouldstart+700) > now)
            {
                m_animating = true;
                m_animstarted=GetTickCount();
                set_anim_timer();
            }
        }



        Rect dest(wndrect_temp.left,
            wndrect_temp.top,
            wndrect_temp.right-wndrect_temp.left,
            wndrect_temp.bottom-wndrect_temp.top);
        if (m_bufnew.is_valid() && !m_animating)
        {
            gfx.DrawImage(m_bufnew.get_ptr(), 0, 0);
        }
        else if (m_animating && m_bufnew.is_valid() && m_bufanim.is_valid())
        {
            float opacityold=0.0f;
            float opacitynew=1.0f;
            int doneness=GetTickCount();
            if (doneness > (m_animstarted+m_config.animtime))
            // animation complete
            {
                m_animating=false;
                opacityold=0.0f;
                opacitynew=1.0f;
            }
            else if (doneness >= m_animstarted && m_config.animtime != 0)
            {
                opacitynew=(float)(doneness-m_animstarted)/(float)m_config.animtime;
                opacityold=1.0f-0.0f;
            }
            {
                Graphics animbuf(m_bufanim.get_ptr());
                animbuf.Clear(bg);
                ColorMatrix cm;
                ZeroMemory(&cm,sizeof(ColorMatrix));
                cm.m[0][0] = 1.0f;
                cm.m[1][1] = 1.0f;
                cm.m[2][2] = 1.0f;
                cm.m[3][3] = opacityold;
                
                ImageAttributes ia;
                ia.SetColorMatrix(&cm);
                animbuf.DrawImage(m_bufold.get_ptr(),dest,0,0,dest.Width,dest.Height,UnitPixel,&ia);
                cm.m[3][3] = opacitynew;
                ia.SetColorMatrix(&cm);
                animbuf.DrawImage(m_bufnew.get_ptr(),dest,0,0,dest.Width,dest.Height,UnitPixel,&ia);
            }
            gfx.DrawImage(m_bufanim.get_ptr(), 0, 0);
            if (m_animating)
                set_anim_timer();
            else
                m_bufanim.release();
        }
        else gfx.Clear(bg);

        m_bmpnew.release();
    }
}
