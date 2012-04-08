#ifndef __FOO_UIE_ALBUMART__SOURCES_H__
#define __FOO_UIE_ALBUMART__SUORCES_H__

// Only stores the path to image files that correspond to a
// particular search pattern
class search_cache_entry
{
public:
    search_cache_entry() { m_pattern.reset(); m_matches.remove_all(); }
    search_cache_entry(const char * p_pattern, pfc::list_t<pfc::string8> & p_matches)
    {
        set_pattern(p_pattern);
        set_matches(p_matches);
    }

    void set_pattern(const char * p_pattern) { m_pattern.set_string(p_pattern); }
    const char * get_pattern() { return m_pattern.get_ptr(); }

    void set_matches(pfc::list_t<pfc::string8> & p_in) { m_matches.remove_all(); m_matches.add_items(p_in); }
    void get_matches(pfc::list_t<pfc::string8> & p_out) { p_out.add_items(m_matches); }

protected:
    pfc::string8 m_pattern;
    pfc::list_t<pfc::string8> m_matches;
};

class search_cache
{
public:
    search_cache() { m_buffer.remove_all(); set_max(5); }
    search_cache(t_size p_max) { m_buffer.remove_all(); set_max(p_max); }

    void set_max(t_size p_max)
    {
        m_max_items = p_max;
        m_buffer.truncate(m_max_items);
    }

    void remove_all() { m_buffer.remove_all(); }

    void add_entry(const char * p_pattern, pfc::list_t<pfc::string8> & p_matches)
    {
        if (m_max_items == 0)
            return;

        t_size idx = find_entry(p_pattern);
        if (idx == pfc_infinite)
        {
            if (m_buffer.get_count() == m_max_items)
            {
                m_buffer.remove_by_idx(0);
            }
            search_cache_entry item(p_pattern, p_matches);
            m_buffer.add_item(item);
        }
        else
        {
            // move this entry to the top of the list
            for (t_size n=idx; n<m_buffer.get_count()-1; n++)
                m_buffer.swap_items(n, n+1);
        }
    }

    t_size find_entry(const char * p_pattern)
    {
        for (t_size n=0; n<m_buffer.get_count(); n++)
        {
            if (stricmp_utf8(p_pattern, m_buffer[n].get_pattern()) == 0)
            {
                return n;
            }
        }

        // no matches found
        return pfc_infinite;
    }

    // False if there is no previous entry
    bool get_last_pattern(pfc::string_base & p_out)
    {
        t_size count = m_buffer.get_count();
        if (count == 0)
        {
            return false;
        }
        else
        {
            get_pattern(count-1, p_out);
            return true;
        }
    }

    // False if there is no previous entry
    bool get_last_matches(pfc::list_t<pfc::string8> & p_out)
    {
        t_size count = m_buffer.get_count();
        if (count == 0)
        {
            return false;
        }
        else
        {
            get_matches(count-1, p_out);
        }
    }

    void get_pattern(t_size n, pfc::string_base & p_out)
    {
        t_size count = m_buffer.get_count();
        if (count == 0)
            return;
        else if (n >= count)
            n = count-1;

        p_out = m_buffer[n].get_pattern();
    }

    void get_matches(t_size n, pfc::list_t<pfc::string8> & p_out)
    {
        t_size count = m_buffer.get_count();
        if (count == 0)
            return;
        else if (n >= count)
            n = count-1;

        m_buffer[n].get_matches(p_out);
    }

    bool get_matches(const char * p_pattern, pfc::list_t<pfc::string8> & p_out)
    {
        t_size idx = find_entry(p_pattern);
        if (idx == pfc_infinite)
            return false;

        get_matches(idx, p_out);
        return true;
    }

protected:
    t_size m_max_items;
    pfc::list_t<search_cache_entry> m_buffer;
};

class sources_control_callback
{
public:
    // Use sources_control::get_image_* functions to get more information
    // about found images
    virtual void on_sources_control_new_image() = 0;
};

class sources_control : private play_callback, private playlist_callback_single, public albumart_ns_callback
{
public:
    enum {
        play_callback_flags = flag_on_playback_new_track | flag_on_playback_stop | flag_on_playback_dynamic_info_track,
        playlist_callback_flags = flag_on_item_focus_change | flag_on_playlist_switch |
                                  flag_on_items_added | flag_on_items_removed,
    };

    enum {
        display_playing=0,
        display_selected,
    };

    sources_control() { m_skip_no_covers = false; m_callback = NULL; }
    
    void setup_sources_control(albumart_vars & p_config, pfc::list_t<pfc::string8> & p_sources);

    ~sources_control();

    void register_foo_callbacks();
    void unregister_foo_callbacks();

    void register_sources_control_callback(sources_control_callback * p_callback);
    void unregister_sources_control_callback(sources_control_callback * p_callback);

    void clear_history();
    void set_max_history(bool p_enabled, t_size p_max);

    void set_config_vars(albumart_vars & p_config, pfc::list_t<pfc::string8> p_sources)
    {
        m_config = p_config;
        m_sources = p_sources;
        set_max_history(m_config.history_enabled, m_config.history_size);
    }

    // returns true if currently displayed track is valid
    bool get_displayed_track(metadb_handle_ptr & p_track) const;

    static bool is_playing() {return static_api_ptr_t<playback_control>()->is_playing();}
    int get_display_mode() const;

    t_size next_source();
    t_size previous_source();

    void find_and_set_image(bool p_nocover = false);
    void find_and_set_image(t_size p_start_source, bool p_nocover = false);

    void set_current_source(t_size p_index);
    void refresh(bool p_force_full_search = true);

    void set_skip_nocovers(bool p_skip) { m_skip_no_covers = p_skip; }

    // Used to query sources_control about the current search results
    bool get_current_bitmap(pfc::rcptr_t<Bitmap> & p_bmp);
    void get_current_match(pfc::string_base & p_out) { p_out = m_src_matches[m_current_src_match]; }

    static const pfc::string8 m_source_embedded;
    static const GUID*   m_cover_ids[];
    static const pfc::string8 m_cover_types[];
    static const t_size  m_num_covers;

    // playlist tree callback
    virtual void on_node_select(const callback_node * p_node);

    // Playback callbacks
    virtual void on_playback_new_track(metadb_handle_ptr p_track);
    virtual void on_playback_stop(play_control::t_stop_reason p_reason);
    virtual void on_playback_dynamic_info_track(const file_info & p_info);

    virtual void on_playback_starting(play_control::t_track_command p_command,bool p_paused) {}
    virtual void on_playback_seek(double p_time) {}
    virtual void on_playback_pause(bool p_state) {}
    virtual void on_playback_edited(metadb_handle_ptr p_track) {}
    virtual void on_playback_dynamic_info(const file_info & p_info) {}
    virtual void on_playback_time(double p_time) {}
    virtual void on_volume_change(float p_new_val) {}

    // Playlist callbacks
    virtual void on_item_focus_change(t_size p_from,t_size p_to);
    virtual void on_items_added(t_size start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const pfc::bit_array & p_selection);
    virtual void on_items_removed(const pfc::bit_array & p_mask,t_size p_old_count,t_size p_new_count);

    virtual void on_items_reordered(const t_size * order,t_size count) {}
    virtual void on_items_removing(const pfc::bit_array & p_mask,t_size p_old_count,t_size p_new_count) {}
    virtual void on_items_selection_change(const pfc::bit_array & p_affected,const pfc::bit_array & p_state) {}
    virtual void on_items_modified(const pfc::bit_array & p_mask) {}
    virtual void on_items_modified_fromplayback(const pfc::bit_array & p_mask,play_control::t_display_level p_level) {}
    virtual void on_items_replaced(const pfc::bit_array & p_mask,const pfc::list_base_const_t<playlist_callback::t_on_items_replaced_entry> & p_data) {}
    virtual void on_item_ensure_visible(t_size p_idx) {}

    virtual void on_playlist_switch();

    virtual void on_playlist_renamed(const char * p_new_name,t_size p_new_name_len) {}
    virtual void on_playlist_locked(bool p_locked) {}

    virtual void on_default_format_changed() {}
    virtual void on_playback_order_changed(t_size p_new_index) {}

protected:
    t_size get_album_art_id (pfc::string_base & p_path, t_size & filename_len);

    // functions that retrieve the index of the first source in a group
    t_size get_first_in_group(t_size p_start_source) const;
    t_size get_next_group(t_size p_start_source) const;
    t_size get_prev_group(t_size p_start_source) const;

    t_size get_next_nocover(t_size p_start_source) const;

    void set_matches(pfc::list_t<pfc::string8> & p_matches, const char * p_pattern, t_size p_index = 0);
    void set_current_match(t_size p_index);

    // loops forward through the source list and finds the first matched source
    // p_path_out holds the path to the found image
    void find_and_set_image_help(pfc::list_t<pfc::string8> & p_matches_out, t_size & p_found_source, pfc::string_base & p_pattern_out,
                                 metadb_handle_ptr p_track, t_size p_start_source);

    bool test_image_source(pfc::list_t<pfc::string8> & p_matches_out, pfc::string_base & p_pattern_out, metadb_handle_ptr p_track,
                           t_size p_source);

    void focus_change_manual();
    void focus_change_manual(t_size p_new);
    void track_change_manual();

    albumart_vars m_config;
    pfc::list_t<pfc::string8> m_sources;
    t_size m_current_source;

    // for cycling through multiple wildcard matches
    pfc::list_t<pfc::string8> m_src_matches;
    t_size m_current_src_match;
    search_cache m_pattern_history;

    metadb_handle_ptr m_selected_track, m_playing_track;

    bool m_skip_no_covers;

    sources_control_callback * m_callback;
};

#endif