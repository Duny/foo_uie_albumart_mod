#ifndef __FOO_UIE_ALBUMART__H__
#define __FOO_UIE_ALBUMART__H__

class uie_albumart : public uie::window, public sources_control_callback
{
public:
    static const GUID extension_guid;

    uie_albumart();

    virtual const GUID & get_extension_guid() const {return albumart_guid;}
    virtual void get_name(pfc::string_base & out) const { out = "Album Art"; }
    virtual void get_category(pfc::string_base & out) const {out = "Panels";    }
    virtual void get_menu_items (uie::menu_hook_t & p_hook);

    virtual bool have_config_popup() const {return true;}
    virtual bool show_config_popup(HWND wnd_parent);
    virtual void set_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort);
    virtual void get_config(stream_writer * p_writer, abort_callback & p_abort) const;

    virtual unsigned get_type() const { return uie::type_panel; }
    virtual bool is_available(const uie::window_host_ptr & p_host) const {return true;}
    virtual HWND create_or_transfer_window(HWND wnd_parent, const uie::window_host_ptr & p_host, const ui_helpers::window_position_t & p_position);
    virtual void destroy_window();
    virtual HWND get_wnd() const {return m_hWnd;}

    virtual void on_sources_control_new_image();

    static LRESULT WINAPI host_proc(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);
    virtual LRESULT on_message(HWND wnd,UINT msg,WPARAM wp,LPARAM lp);

    void paint(HDC hdc);
    void redraw();

    void get_config_vars(albumart_vars & p_config, pfc::list_t<pfc::string8> & p_src_list);
    void set_config_vars(unsigned int change_type, albumart_vars & p_config, pfc::list_t<pfc::string8> & p_src_list);

    void click_func(mouse_function func);

    sources_control m_sources_control;

protected:
    enum {
        animation_timer_id = 7,
        cycle_timer_id,
    };

    // albumart configuration
    albumart_vars m_config;
    pfc::list_t<pfc::string8> m_sources;
    bool m_config_changed;

    pfc::rcptr_t<Bitmap> m_bmp;
    pfc::rcptr_t<Bitmap> m_bmpnew;   // should be empty unless new image needs to be drawn
    pfc::string8 m_path;
    bool m_image_file_exists;
    pfc::ptrholder_t<Bitmap> m_bufnew, m_bufold, m_bufanim;
    bool m_animating;
    DWORD m_animstarted,m_animshouldstart;
    bool b_dblclick;

    bool m_panning_enabled;
	bool m_dragging;
	POINT m_drag_start;
	int m_pan_dx, m_pan_dy;
	int m_orig_dx, m_orig_dy;

	// accumulator for mouse wheel
    int m_wheel_accum;

    HWND m_hWnd;
    static const GUID albumart_guid;
    uie::window_host_ptr m_host;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;

    void set_anim_timer();

    // checks whether cycling should be enabled before
    // starting timer, and disables timer if cycling
    // isn't necessary
    void start_cycle_timer();

    void stop_cycle_timer();

    void func_OpenEx();
    void func_OpenDir();
    void func_ToggleFollow();
    void func_FocusPlaying();
    void func_Refresh(bool p_full_refresh);
    void func_NextSource(bool user_action);
    void func_PreviousSource();
    void func_FirstSource();
    void func_CopyName();
};

#endif
