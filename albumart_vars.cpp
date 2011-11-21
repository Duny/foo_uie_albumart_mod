#include "stdafx.h"

inline DWORD exstyle_from_config(edgestyle edge_style)
{
    switch (edge_style)
    {
        case SUNKEN_EDGE: return WS_EX_CLIENTEDGE;
        case GREY_EDGE: return WS_EX_STATICEDGE;
        default: return 0;
    }
}

static const char * def_src_list[] =
{
    "$replace(%path%,%filename_ext%,)folder.*",
    "-$replace(%path%,%filename_ext%,)cover.*",
    "-$replace(%path%,%filename_ext%,)front.*",
    "--default.*",
};

inline t_size get_defsrclist_count()
{
    return tabsize(def_src_list);
}

inline const char * get_def_source(t_size p_index)
{
    dynamic_assert(p_index < tabsize(def_src_list));

    return def_src_list[p_index];
}

static const char * func_name[] =
{
    "Disabled",
    "Open in external viewer",
    "Open directory",
    "Toggle follow cursor",
    "Focus playing",
    "Refresh",
    "Next source",
    "Previous source",
    "First source",
    "Copy image file name"
    //"Custom...",
};

inline t_size get_function_count()
{
    return tabsize(func_name);
}

inline const char * get_function_name(t_size p_index)
{
    dynamic_assert(p_index < tabsize(func_name));

    return func_name[p_index];
}

inline bool GUIDtoExt (GUID &guid, string_base & p_out)
{
    p_out.reset ();
    if (guid == ImageFormatJPEG)
        p_out = "jpeg";
    else if (guid == ImageFormatPNG)
        p_out = "png";
    else if (guid == ImageFormatGIF)
        p_out = "gif";
    else if (guid == ImageFormatTIFF)
        p_out = "tiff";
    else if (guid == ImageFormatBMP)
        p_out = "bmp";
    return p_out.length () > 0;
}

// sets p_config and p_src_list to default values
void reset_config_vars(albumart_vars & p_config, list_t<string8> & p_src_list)
{
    // configuration defaults
    p_config.last_tab = 0;

    // display vars
    p_config.centeralbum = true;
    p_config.expandalbum = true;
    p_config.shrinkalbum = true;
    p_config.aspectratio = true;
    p_config.minheight = 0;
    p_config.minwidth = 0;
    p_config.padding = 0;
    p_config.edge_style = GREY_EDGE;
    p_config.interpolationmode = RESIZE_HIGHEST;
    p_config.draw_pixel_border = false;
    p_config.bg_enabled = true;
    p_config.bordercol = RGB(0,0,0);   // black
    p_config.bgcol = RGB(255,255,255); // white

    // behaviour vars
    p_config.selected = false;
    p_config.lftclickfunc = FUNC_REFRESH;
    p_config.mdlclickfunc = FUNC_OPENEX;
    p_config.dblclickfunc = FUNC_FOLLOW;
    p_config.resetsource = RESET_ONCHANGE;
    p_config.skip_nocovers = true;
    p_config.animtime = 300;
    p_config.old_animtime = p_config.animtime;
    p_config.cycletime = 10;
    p_config.old_cycletime = p_config.cycletime;
    p_config.cycle_wildcards = false;
    p_config.cycle_order = WILDCARD_ALPHA;

    // sources vars
    p_config.debug_log_sources = false;
    p_config.history_enabled = true;
    p_config.history_size = 5;

    int num_sources = get_defsrclist_count();
    p_src_list.remove_all();
    for (t_size i = 0; i < num_sources; i++)
        p_src_list.add_item(get_def_source(i));
}
