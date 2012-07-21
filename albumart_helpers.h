#ifndef __FOO_UIE_ALBUMART__HELPER_H__
#define __FOO_UIE_ALBUMART__HELPER_H__

extern bool g_color_picker(HWND parent, COLORREF & p_value);

extern void findFirstImageMatch(pfc::list_t<pfc::string8> & p_path_out, const char * p_pattern, bool p_process_all);

void fix_filename(pfc::string8 & p_out, const char * p_path);

bool skip_prefix(pfc::string8 & p_string, const char * p_prefix);

album_art_extractor::ptr get_extractor_for_path(const char * path);

// For archive listing
typedef boost::function<bool (archive * owner, const char * url, const t_filestats & p_stats, const file_ptr & p_reader)> archive_callback_func;

class archive_callback_helper : public archive_callback
{
    archive_callback_func m_func;
    abort_callback_impl m_abort;

    bool is_aborting () const override { return m_abort.is_aborting (); }

    abort_callback_event get_abort_event() const override { return m_abort.get_abort_event (); }


    bool on_entry (archive * owner, const char * url, const t_filestats & p_stats, const service_ptr_t<file> & p_reader) override
    {
        return m_func (owner, url, p_stats, p_reader);
    }
public:
    archive_callback_helper (archive_callback_func p_func) : m_func (p_func) {}
};
#endif