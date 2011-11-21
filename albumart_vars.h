#ifndef __FOO_UIE_ALBUMART__VARS_H__
#define __FOO_UIE_ALBUMART__VARS_H__

// mouse-related declarations
enum mouse_function : int
{
    FUNC_NULL=0,
    FUNC_OPENEX,
    FUNC_OPENDIR,
    FUNC_FOLLOW,
    FUNC_FOCUSNP,
    FUNC_REFRESH,
    FUNC_NEXTSRC,
    FUNC_PREVSRC,
    FUNC_FIRSTSRC,
    FUNC_COPYNAME,
    //FUNC_CUSTOM,
};

extern t_size get_function_count();
extern const char * get_function_name(t_size p_index);

// edge style-related declarations
enum edgestyle : int
{
    NO_EDGE,
    SUNKEN_EDGE,
    GREY_EDGE,
};

extern DWORD exstyle_from_config(edgestyle edge_style);

extern bool GUIDtoExt (GUID &guid, string_base & p_out);

// reset sources and resize quality declarations
enum reset_option : int
{
    RESET_NEVER=0,
    RESET_ALWAYS,
    RESET_ONCHANGE,
};

enum resizing_quality : int
{
    RESIZE_LOW,
    RESIZE_MEDIUM,
    RESIZE_HIGH,
    RESIZE_HIGHEST,
};

enum wildcard_order : int
{
    WILDCARD_ALPHA,
    WILDCARD_RANDOM,
};

// used to notify main window of preferences changes
enum var_change : unsigned int
{
    VC_GENERAL          = 1<<0,
    VC_MIN_HEIGHT       = 1<<1,
    VC_FOLLOW           = 1<<2,
    VC_CYCLE            = 1<<3,
    VC_SOURCES          = 1<<4,
    VC_EDGESTYLE        = 1<<5,
    VC_MIN_WIDTH        = 1<<6,
};

// default source list
extern t_size get_defsrclist_count();
extern const char * get_def_source(t_size p_index);

// structure containing all vars for current version of albumart panel, except the source strings
struct albumart_vars
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

    bool history_enabled;
    int history_size;

    bool bg_enabled;

    int minwidth;
};

// used to reset config variables
extern void reset_config_vars(albumart_vars & p_config, list_t<string8> & p_src_list);

#endif
