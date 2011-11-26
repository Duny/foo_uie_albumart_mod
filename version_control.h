#ifndef __FOO_UIE_ALBUMART__VERSION_CONTROL_H__
#define __FOO_UIE_ALBUMART__VERSION_CONTROL_H__

// make sure to update this with the correct albumart_version enum
#define VERSION_CURRENT VERSION_027

// used in uie_albumart::set_config to ensure importing
// is done correctly, in cases where variables
// change between versions
enum albumart_version
{
    VERSION_020=0x20,
    VERSION_021,
    VERSION_022,
    VERSION_023,
    VERSION_024,
    VERSION_0241,
    VERSION_025,
    VERSION_0251,
    VERSION_026,
    VERSION_027,
};

// structure containing all vars for albumart panel, except the source strings
// for version 0.2.4.1/0.2.5/0.2.5.1: left here for compatibility
struct albumart_vars_0241
{
    bool centeralbum, expandalbum, shrinkalbum, aspectratio;
    resizing_quality interpolationmode;
    int minheight;
    int padding;
    edgestyle edge_style;
    bool draw_pixel_border;
    COLORREF bordercol;
    COLORREF bgcol;

    bool selected;
    mouse_function lftclickfunc, mdlclickfunc, dblclickfunc;
    reset_option resetsource;
    bool skip_nocovers;
    int animtime;
    int old_animtime;
    int cycletime;
    int old_cycletime;

    bool debug_log_sources;

    int last_tab;

    bool cycle_wildcards;
    wildcard_order cycle_order;
};

// structure containing all vars for albumart panel, except the source strings
// for version 0.2.4: left here for compatibility
struct albumart_vars_024
{
    bool centeralbum, expandalbum, shrinkalbum, aspectratio;
    resizing_quality interpolationmode;
    int minheight;
    int padding;
    edgestyle edge_style;
    bool draw_pixel_border;
    COLORREF bordercol;
    COLORREF bgcol;

    bool selected;
    mouse_function lftclickfunc, mdlclickfunc, dblclickfunc;
    reset_option resetsource;
    bool skip_nocovers;
    int animtime;
    int old_animtime;
    int cycletime;
    int old_cycletime;

    bool debug_log_sources;

    //** removed as of version 0.2.4, since sources are now stored before
    //** albumart_vars structure
    //int num_sources;

    int last_tab;
};

class version_control
{
public:
    version_control() { m_version = VERSION_CURRENT; }
    version_control(albumart_version p_version) { m_version = p_version; }

    void read_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort,
                     albumart_vars & p_config, pfc::list_t<pfc::string8> & p_sources);
    void write_config(stream_writer * p_writer, abort_callback & p_abort,
                      const albumart_vars & p_config, const pfc::list_t<pfc::string8> & p_sources) const;

protected:
    unsigned m_version;
};

#endif //__FOO_UIE_ALBUMART__VERSION_CONTROL_H__