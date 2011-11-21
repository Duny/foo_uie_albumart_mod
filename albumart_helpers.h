#ifndef __FOO_UIE_ALBUMART__HELPER_H__
#define __FOO_UIE_ALBUMART__HELPER_H__

extern bool g_color_picker(HWND parent, COLORREF & p_value);

extern void findFirstImageMatch(list_t<string8> & p_path_out, const char * p_pattern, bool p_process_all);

void fix_filename(string8 & p_out, const char * p_path);

bool skip_prefix(string8 & p_string, const char * p_prefix);

bool get_extractor(album_art_extractor::ptr & out,const char * path, abort_callback & p_abort);

#endif