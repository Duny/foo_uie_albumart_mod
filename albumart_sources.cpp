#include "stdafx.h"

const pfc::string8 sources_control::m_embedded_image_search_pattern = "<embedded image>";

const GUID* sources_control::m_cover_ids[] = { 
    &album_art_ids::cover_front,
    &album_art_ids::cover_back,
    &album_art_ids::disc,
    &album_art_ids::icon,
    &album_art_ids::artist
};
const t_size sources_control::m_num_covers = tabsize (sources_control::m_cover_ids);

const pfc::string8 sources_control::m_cover_types[] = { "Front", "Back", "Disc", "Icon", "Artist" };

// this class mostly controls the sources list and cycling through various sources.
// The functions which actually try to match one or more images with a source pattern
// are found in albumart_helpers.cpp

void sources_control::setup_sources_control(albumart_vars & p_config, pfc::list_t<pfc::string8> & p_sources)
{
    m_config = p_config;
    m_sources = p_sources;

    pfc::string8 temp;
    temp.reset();
    m_src_matches.remove_all();
    m_src_matches.add_item(temp);
    m_current_src_match = 0;

    m_current_source = 0;
    m_pattern_history.remove_all();
    set_max_history(m_config.history_enabled, m_config.history_size);

    {
        metadb_handle_ptr track;
        static_api_ptr_t<playback_control>()->get_now_playing(track);
        if(track.is_valid())
            m_playing_track = track;
    }

    {
        metadb_handle_ptr track;
        static_api_ptr_t<playlist_manager>()->activeplaylist_get_focus_item_handle(track);
        if(track.is_valid())
        {
            m_selected_track = track;
        }
        else
        {
            // if there's no focused item, just try to get the first item in the playlist
            static_api_ptr_t<playlist_manager>()->activeplaylist_get_item_handle(track, 0);
            if (track.is_valid())
                m_selected_track = track;
        }
    }
    
    if (m_sources.get_count() > 0)
        find_and_set_image();
}

sources_control::~sources_control()
{
    pfc::dynamic_assert(m_callback==NULL, "Sources control callback registered");
    m_selected_track = m_playing_track = NULL;
}

void sources_control::register_foo_callbacks()
{
    static_api_ptr_t<play_callback_manager> pcm;
    pcm->register_callback(this, play_callback_flags, false);

    static_api_ptr_t<playlist_manager> pm;
    pm->register_callback(this, playlist_callback_flags);

    static_api_ptr_t<albumart_ns_manager> nsm;
    nsm->register_callback(this);
}

void sources_control::unregister_foo_callbacks()
{
    static_api_ptr_t<play_callback_manager> pcm;
    pcm->unregister_callback(this);

    static_api_ptr_t<playlist_manager> pm;
    pm->unregister_callback(this);

    static_api_ptr_t<albumart_ns_manager> nsm;
    nsm->unregister_callback(this);
}

void sources_control::register_sources_control_callback(sources_control_callback * p_callback)
{
    m_callback = p_callback;
}

// p_callback is reserved for future use
void sources_control::unregister_sources_control_callback(sources_control_callback * p_callback)
{
    m_callback = NULL;
}

bool sources_control::get_displayed_track(metadb_handle_ptr & p_track) const
{
    if (get_display_mode() == display_playing)
        p_track = m_playing_track;
    else
        p_track = m_selected_track;
    return p_track.is_valid();
}

int sources_control::get_display_mode() const
{
    if ((!m_config.selected || m_selected_track == m_playing_track) && is_playing())
        return display_playing;
    else
        return display_selected;
}

//WigBaM's modified code
// modified heavily by foosion:
//   restructured loop and helper funtions
// modified heavily by gfngfgf:
//   takes advantage of helper functions
t_size sources_control::next_source()
{
    // cycle through any wildcard matches that haven't been displayed yet
    if (m_current_src_match < (m_src_matches.get_count() - 1))
    {
        set_current_match(m_current_src_match+1);
    }
    // otherwise, search the next source group in the list
    else
    {
        // get index of first source in next group
        t_size source_index = get_next_group(m_current_source);

        // use helper function to search for a valid source
        // loops forward until a match is found
        t_size found_source;
        pfc::string8 pattern;
        pfc::list_t<pfc::string8> matches;
        metadb_handle_ptr track;
        get_displayed_track(track);

        find_and_set_image_help(matches, found_source, pattern, track, source_index);
        m_current_source = found_source;
        set_matches(matches, pattern);
    }
    return m_current_source;
}

// Analoguous to func_NextSource(), but scans source list in the opposite direction.
// originally by foosion
// rewrote by gfngfgf
//   takes full advantage of helper functions
//   handles cases where previous source isn't found
t_size sources_control::previous_source()
{
    metadb_handle_ptr track;
    get_displayed_track(track);

    // display any wildcard matches, if present
    if (m_current_src_match > 0)
    {
        set_current_match(m_current_src_match-1);
    }
    else
    {
        // get index of first source in previous group
        t_size group_index = get_prev_group(m_current_source);

        t_size source_count = m_sources.get_count();
        bool first_time = true;

        // search previous groups until we reach the current source again
        for (;;)
        {
            t_size n;
            // search forward within current group for a match
            for (n = 0; n < source_count; n++)
            {
                t_size source_index = (group_index + n) % source_count;

                // if we've reached the original source, cycle to the
                // last wildcard match
                if ((source_index == m_current_source))
                {
                    if (!first_time)
                    {
                        set_current_match(m_src_matches.get_count() - 1);
                        return m_current_source;
                    }
                    else
                    {
                        first_time = false;
                    }
                }

                // test if we've reached the beginning of the next group
                if (n > 0 && strcmp_partial(m_sources[source_index], "-") != 0)
                {
                    // stop searching forward
                    break;
                }
                else
                // we're in the same group
                {
                    pfc::list_t<pfc::string8> matches;
                    pfc::string8 pattern;

                    // test for an image match
                    // we don't want to use the find_and_set_image functions
                    // because those will search forward through all the sources
                    if (test_image_source(matches, pattern, track, source_index))
                    {
                        m_current_source = source_index;

                        // show last match, rather than first
                        set_matches(matches, pattern, matches.get_count() - 1);
                        return m_current_source;
                    }
                }
            }

            // we've searched an entire group and found no match
            // search the previous group
            group_index = get_prev_group(group_index);
        }
    }
    return m_current_source;
}

// gets the index of the first non-hyphenated source seen
// scanning backwards from p_start_source, or the first
// source in the list (index 0), whichever comes first
t_size sources_control::get_first_in_group(t_size p_start_source) const
{
    pfc::dynamic_assert((p_start_source < m_sources.get_count()) && (p_start_source >= 0));

    t_size source_index;

    // work backwards from current source to top of the list
    // (top of the list is always the first source in a group
    for (source_index = p_start_source; source_index > 0; source_index--)
    {
        // test if we've reached the start of the group by checking
        // for a hyphen "-"
        if (strcmp_partial(m_sources[source_index], "-") != 0)
            return source_index;
    }

    // if we reach this point, then we must be at the top of the list
    return 0;
}

// gets the index of the first non-hyphenated source seen
// after p_start_source.  Wraps around from end to beginning
// if necessary
t_size sources_control::get_next_group(t_size p_start_source) const
{
    t_size source_count = m_sources.get_count();

    pfc::dynamic_assert((p_start_source < source_count) && (p_start_source >= 0));

    t_size n;
    // start with next source and avoid current one
    for (n = 1; n < source_count; n++)
    {
        t_size source_index = (p_start_source + n) % source_count;

        // test if we've reach the start of the next group yet by
        // checking for a hyphen
        if (strcmp_partial(m_sources[source_index], "-") != 0)
            return source_index;
    }

    // if we've reached this point, we've tried all sources, so just
    // return the first source in the current group
    return get_first_in_group(p_start_source);
}

// returns the index of the second non-hyphenated source seen
// before p_start_source.  Wraps around from beginning to
// end if necessary
t_size sources_control::get_prev_group(t_size p_start_source) const
{
    t_size source_count = m_sources.get_count();

    pfc::dynamic_assert((p_start_source < source_count) && (p_start_source >= 0));

    // to ignore sources prefixed with "-" while still in same group
    // as start source
    bool first_group = strcmp_partial(m_sources[p_start_source], "-") != 0;

    t_size n;
    // start with previous source and avoid current one
    for (n = 1; n < source_count; n++)
    {
        t_size source_index = (p_start_source + source_count - n) % source_count;

        // skip sources within the same group as the start source
        if (!first_group)
        {
            // check if we've reach the first source of the start group
            if (strcmp_partial(m_sources[source_index], "-") != 0)
                first_group = true;

            // skip to previous source
            continue;
        }
        else
        // we've already passed the starting group
        {
            // now, if we don't see a hyphen, it must be the first source
            // in the previous group
            if (strcmp_partial(m_sources[source_index], "-") != 0)
                return source_index;
        }
    }

    // if we've reached this far, we've checked all the sources
    // so just return the first source in the current group
    return get_first_in_group(p_start_source);
}

// returns the index of the first no-cover source (starts with two hyphens)
// on or after p_start_source.  Wraps from end to beginning if necessary.
// If no "no-cover" source can be found, the return value will be infinite
t_size sources_control::get_next_nocover(t_size p_start_source) const
{
    t_size source_count = m_sources.get_count();

    pfc::dynamic_assert((p_start_source < source_count) && (p_start_source >= 0));

    t_size n;
    for (n = 0; n < source_count; n++)
    {
        t_size source_index = (p_start_source + n) % source_count;

        // test for a no-cover source
        if (strcmp_partial(m_sources[source_index], "--") == 0)
            return source_index;
    }

    // if we've reached this point, we've tried all sources
    return pfc_infinite;
}

void sources_control::set_current_source(t_size p_index)
{
    pfc::dynamic_assert((p_index >= 0) && (p_index < m_sources.get_count()));

	if ((m_current_source == p_index) && (p_index == 0))
		m_current_src_match = 0;

    m_current_source = p_index;
}

void sources_control::refresh(bool p_force_full_search)
{
    if (p_force_full_search)
        clear_history();

    track_change_manual();
}

inline void sources_control::clear_history()
{
    if (m_config.debug_log_sources)
        console::formatter() << "Clearing image match history";
    m_pattern_history.remove_all();
}

inline void sources_control::set_max_history(bool p_enabled, t_size p_max = 0)
{
    if (p_enabled)
        m_pattern_history.set_max(p_max);
    else
        m_pattern_history.set_max(0);
}

// Sets current image matches to p_matches and loads the image at index p_index
// p_pattern is the pattern which resulted in p_matches
// Also starts appropriate animation timers
//
// NOTE: p_matches may be modified by this function
void sources_control::set_matches(pfc::list_t<pfc::string8> & p_matches, const char * p_pattern, t_size p_index)
{
    // save the pattern that generated the search results p_matches
    m_pattern_history.add_entry(p_pattern, p_matches);

    // make sure we have at least one item in p_matches
    if (p_matches.get_count() == 0)
    {
        pfc::string8 temp;
        temp.reset();
        p_matches.add_item(temp);
    }

    // copy matches to m_src_matches
    m_src_matches.remove_all();

    if (m_config.cycle_wildcards && (m_config.cycle_order == WILDCARD_RANDOM))
    {
        if (m_config.debug_log_sources)
            console::formatter() << "Randomizing order of matches...";

        //service_ptr_t<genrand_service> prng = genrand_service::g_create();
        //prng->seed(GetTickCount());

        //while (p_matches.get_count() > 0)
        //{
        //    // generate random integer n such that 0 <= n < matches.get_count()
        //    unsigned int n = prng->genrand(p_matches.get_count());
        //    m_src_matches.add_item(p_matches[n]);
        //    p_matches.remove_by_idx(n);
        //}
        service_ptr_t<genrand_service> prng = genrand_service::g_create();
        prng->seed(GetTickCount());

        unsigned int n = p_matches.get_count();
        while (n > 0)
        {
            unsigned int i = prng->genrand(n);
            m_src_matches.add_item(p_matches[i]);
            p_matches.swap_items(i, n-1);
            n--;
        }
    }
    else
    {
        // alphabetical order
        m_src_matches.add_items(p_matches);
    }

    set_current_match(p_index);
}

// loads the image match at p_index
void sources_control::set_current_match(t_size p_index)
{
    pfc::dynamic_assert(p_index < m_src_matches.get_count());

    m_current_src_match = p_index;

    // notify any callbacks that image search is complete
    if (m_callback != NULL)
        m_callback->on_sources_control_new_image();
}

bool sources_control::get_current_bitmap(pfc::rcptr_t<Bitmap> & p_bmp)
{
    abort_callback_impl abort;
    pfc::string8 path = m_src_matches[m_current_src_match];

	if (path.is_empty())
		return false;

	if (m_config.debug_log_sources)
        console::formatter() << "Loading album art: \"" << path << "\"";

    t_size len, img_id;
    if ((img_id = get_album_art_id (path, len)) != ~0) {
        abort_callback_impl p_abort;
        album_art_extractor::ptr ptr;
        
        bool res = false;

        path.truncate (len);
        if (get_extractor (ptr, path, p_abort)) {
            album_art_extractor_instance_ptr p_extractor;
            try {
                p_extractor = ptr->open (NULL, path, p_abort);
                album_art_data_ptr pic = p_extractor->query (*m_cover_ids[img_id], p_abort);
                bitmap_file bmp;
                res = bmp.get_gdiplus_bitmap_from_album_art_data(pic, p_bmp);
                if (res == false) {
                    pfc::string8 temp;
                    if (m_config.debug_log_sources)
                    {
                        temp.add_string("  File");
                    }
                    else
                    {
                        temp.add_string("Error (foo_uie_albumart): File ");
                        temp.add_string(path);
                        temp.add_char('/');
                        temp.add_string(m_cover_types[img_id]);
                    }
                    console::formatter() << temp << " is not a valid image!";
                }
            }
            catch (...) { }

            p_extractor.release ();
            ptr.release ();
            
            return res;
        }
    }
    else if (filesystem::g_exists(path, abort))
    {
        bitmap_file bmp(path);
        if (bmp.get_gdiplus_bitmap(p_bmp))
        {
            return true;
        }
        else
        {
            pfc::string8 temp;
            if (m_config.debug_log_sources)
            {
                temp.add_string("  File");
            }
            else
            {
                temp.add_string("Error (foo_uie_albumart): File ");
                temp.add_string(path);
            }
            console::formatter() << temp << " is not a valid image!";
            return false;
        }
    }
    
    pfc::string8 temp;
    if (m_config.debug_log_sources)
    {
        temp.add_string("  File");
    }
    else
    {
        temp.add_string("Error (foo_uie_albumart): File ");
        temp.add_string(path);
    }
    console::formatter() << temp << " not found!";
    return false;
}

//! p_nocover [in] true iff the source being displayed before the search
//!                is a no-cover source
void sources_control::find_and_set_image(bool p_nocover)
{
    find_and_set_image(m_current_source, p_nocover);
}

void sources_control::find_and_set_image(t_size p_start_source, bool p_nocover)
{
    pfc::list_t<pfc::string8> matches;
    pfc::string8 pattern;
    metadb_handle_ptr track;

    get_displayed_track(track);
    t_size start_source = (m_config.resetsource == RESET_ALWAYS)
                          ?0:p_start_source;
    t_size found_source = start_source;


    find_and_set_image_help(matches, found_source, pattern, track, start_source);
    m_current_source = found_source;

    pfc::string8 prev_pattern;
    if (m_pattern_history.get_last_pattern(prev_pattern))
    {
        if (stricmp_utf8(pattern, prev_pattern) == 0)
        {
            // if the old source was a no-cover source, and we've
            // matched the same source, reset to the first source
            // and try the search again
            if (p_nocover && m_config.resetsource == RESET_ONCHANGE)
            {
                start_source = 0;
                find_and_set_image_help(matches, found_source, pattern, track, start_source);
                m_current_source = found_source;
            }
            else
            {
                // We don't need to call set_matches because we
                // already have our results loaded.
                // just call our callback(s)
                if (m_callback != NULL)
                    m_callback->on_sources_control_new_image();
                return;
            }
        }
        else if (m_config.resetsource == RESET_ONCHANGE)
        {
            // if there is a new pattern, reset to the first source
            // and do the search again
            start_source = 0;
            find_and_set_image_help(matches, found_source, pattern, track, start_source);
            m_current_source = found_source;
        }
    }

    set_matches(matches, pattern);
}

// loops forward once through the source list, starting at p_start_source, searching for the
// first match found.  This function will be called at least once for every call to
// a find_and_set_image function
void sources_control::find_and_set_image_help(pfc::list_t<pfc::string8> & p_matches_out, t_size & p_found_source,
                                           pfc::string_base & p_pattern_out,
                                           metadb_handle_ptr p_track, t_size p_start_source)
{
    t_size source_count = m_sources.get_count();

    pfc::dynamic_assert((p_start_source < source_count) && (p_start_source >= 0));

    pfc::list_t<pfc::string8> matches;

    t_size n;
    t_size source_index;
    for (n = 0; n < source_count; n++)
    {
        source_index = (n + p_start_source) % source_count;

        // if we're cycling, skip 'no-cover' arts
        if (m_skip_no_covers &&
            strcmp_partial(m_sources[source_index], "--") == 0)
            continue;

        if (test_image_source(matches, p_pattern_out, p_track, source_index))
        {
            p_matches_out = matches;
            p_found_source = source_index;
            return;
        }

        // [1] if no match was found, make sure matches is reset for the next loop
        matches.remove_all();
    }

    // we've checked all sources and we're back at p_start_source

    // if we're cycling and skipping covers, just show the first no-cover
    if (m_skip_no_covers)
    {
        for (n = 0; n < source_count; n++)
        {
            source_index = (n + p_start_source) % source_count;

            if (strcmp_partial(m_sources[source_index], "--") == 0 &&
                test_image_source(matches, p_pattern_out, p_track, source_index))
            {
                p_matches_out = matches;
                p_found_source = source_index;
                return;
            }

            // see [1]
            matches.remove_all();
        }
    }

    // at this point, we have exhausted all our options and still found
    // no match
    p_found_source = p_start_source;
    return;
}

// compiles the source pattern at index p_source, formats the pattern using info from p_track, and
// returns true if an image exists at that path.  p_matches_out will be filled with the paths of
// all images that match the source pattern.
bool sources_control::test_image_source(pfc::list_t<pfc::string8> & p_matches_out, pfc::string_base & p_pattern_out,
                                     metadb_handle_ptr p_track, t_size p_source)
{
    pfc::string8 pattern, file;
    static_api_ptr_t<titleformat_compiler> compiler;
    service_ptr_t<titleformat_object> pattern_obj;

    // for Columns UI global variables script
    pfc::string8 globals, temp;
    service_ptr_t<titleformat_object> globals_obj;
    columns_ui::global_variable_list vars;

    pattern = m_sources[p_source];

    skip_prefix(pattern, "-");

    // for no-cover images
    skip_prefix(pattern, "-");

    // for backwards compatibility
    skip_prefix(pattern, "match:");

    if (pattern == m_embedded_image_search_pattern) {
        if (p_track.is_valid ()) {
            file = p_track->get_path ();
            file.add_string ("/*");
        }
    }
    else {
        try {
            // compile columns UI global variable string
            columns_ui::static_control_ptr()->get_string(columns_ui::strings::guid_global_variables, globals);
            compiler->compile(globals_obj, globals);

            // compile source pattern
            compiler->compile(pattern_obj, pattern.get_ptr());

            // run titleformat scripts
            // use appropriate helpers depending on whether
            // currently playing track is being tested, and
            // whether there's a valid track
            if (!p_track.is_valid())
            {
                globals_obj->run(&columns_ui::titleformat_hook_global_variables<true,false>(vars), temp, NULL);
                pattern_obj->run(&columns_ui::titleformat_hook_global_variables<false,true>(vars), file,
                    &titleformat_text_filter_impl_reserved_chars_ex("*?<>:/|" "\"" "\\", '?'));
            }
            else if (get_display_mode() == display_playing)
            {
                // set global variables
                static_api_ptr_t<playback_control>()->playback_format_title(&columns_ui::titleformat_hook_global_variables<true,false>(vars),
                    temp, globals_obj, NULL, playback_control::display_level_titles);
                // format filename to search
                static_api_ptr_t<playback_control>()->playback_format_title(&columns_ui::titleformat_hook_global_variables<false,true>(vars),
                    file, pattern_obj, &titleformat_text_filter_impl_reserved_chars_ex("*?<>:/|" "\"" "\\", '?'),
                    playback_control::display_level_titles);
            }
            else
            {
                // set global variables
                p_track->format_title(&columns_ui::titleformat_hook_global_variables<true,false>(vars),
                    temp, globals_obj, NULL);
                //format filename to search
                p_track->format_title(&columns_ui::titleformat_hook_global_variables<false,true>(vars),
                    file, pattern_obj, &titleformat_text_filter_impl_reserved_chars_ex("*?<>:/|" "\"" "\\", '?'));
            }
        }
        catch (...) { 
            return false; 
        }
    }

    globals_obj.release();
    pattern_obj.release();

    if (m_config.debug_log_sources)
        console::formatter() << "searching album art, preprocessed pattern: \"" << file << "\"";

    // only perform the image search if the pattern is not in the history
    t_size history_idx = m_pattern_history.find_entry(file);
    if (history_idx == pfc_infinite)
    {
        if (pattern == m_embedded_image_search_pattern) {
            if (p_track.is_valid ()) {
                album_art_extractor::ptr ptr;
                abort_callback_impl p_abort;
                
                const char *path = p_track->get_path ();
                if (get_extractor (ptr, path, p_abort)) {
                    album_art_extractor_instance_ptr p_extractor;

                    try {
                        p_extractor = ptr->open (NULL, path, p_abort);
                    }
                    catch (...) {
                        p_extractor = NULL;
                    }
                        
                    if (p_extractor != NULL) {
                        for (t_size i = 0; i < m_num_covers; i++) {
                            try {
                                album_art_data_ptr pic = p_extractor->query (*m_cover_ids[i], p_abort);
                                pic.release ();

                                temp.reset ();
                                temp.add_string (p_track->get_path ());
                                temp.add_char ('/');
                                temp += m_cover_types[i];
                                console::formatter() << temp << "\n";
                                p_matches_out.add_item (temp);
                            }
                            catch (...) { }
                        }
                        p_extractor.release ();
                    }
                    ptr.release ();
                }
            }
        }
        else
            findFirstImageMatch(p_matches_out, file, m_config.cycle_wildcards);

        if (m_config.debug_log_sources)
        {
            char count[10];
            _itoa_s(p_matches_out.get_count(), count, 10, 10);
            console::formatter() << "  " << count << " match(es) found";
        }
    }
    else
    {
        if (m_config.debug_log_sources)
            console::formatter() << "  Pattern found in history; previous search results used";

        m_pattern_history.get_matches(history_idx, p_matches_out);
    }

    if (p_matches_out.get_count() != 0)
    {
        p_pattern_out = file;
        return true;
    }
    else
    {
        p_pattern_out.reset();
        return false;
    }
}

t_size sources_control::get_album_art_id (pfc::string_base & p_path, t_size & filename_len)
{
    t_size slash = p_path.find_last ('/');
    if (slash != ~0) {
        pfc::string8 album_art_name = p_path.get_ptr () + slash + 1;
        t_size img_index;
        for (img_index = 0; img_index < m_num_covers; img_index++) {
            if (m_cover_types[img_index] == album_art_name) {
                filename_len = slash;
                return img_index;
            }
        }
    }
    return ~0;
}

void sources_control::on_node_select( const callback_node * node )
{
    if (node->get_entry_count() == 0)
        return;

    pfc::list_t<metadb_handle_ptr> track_list;
    node->get_entries(track_list);
    if (track_list.get_count() == 0)
        return;

    m_selected_track = track_list.get_item(0);
    if (get_display_mode() == display_selected || m_selected_track == m_playing_track)
    {
        track_change_manual();
    }
}
void sources_control::on_playback_new_track(metadb_handle_ptr p_track)
{
    m_playing_track = p_track;
    if (get_display_mode() == display_playing)
    {
        track_change_manual();
    }
}

void sources_control::on_playback_stop(play_control::t_stop_reason p_reason)
{
    if (p_reason != playback_control::stop_reason_starting_another &&
        p_reason != playback_control::stop_reason_shutting_down)
    {
        // set image to currently selected track
        track_change_manual();
    }
}

void sources_control::on_playback_dynamic_info_track(const file_info & p_info)
{
    // just need to refresh current track, if we're in now-playing mode
    // (possibly; I don't have any streams to test on)
    if (get_display_mode() == display_playing)
    {
        track_change_manual();
    }
}

void sources_control::on_item_focus_change(t_size p_from, t_size p_to)
{
    focus_change_manual(p_to);
}

void sources_control::on_playlist_switch()
{
    focus_change_manual();
}

void sources_control::on_items_added(t_size start, const pfc::list_base_const_t<metadb_handle_ptr> & p_data,const pfc::bit_array & p_selection)
{
    focus_change_manual(start);
}

void sources_control::on_items_removed(const pfc::bit_array & p_mask,t_size p_old_count,t_size p_new_count)
{
    focus_change_manual();
}

void sources_control::focus_change_manual()
{
    // get currently focused track
    t_size focus_track = static_api_ptr_t<playlist_manager>()->activeplaylist_get_focus_item();

    // if there's no currently focused item,
    // just show the first item in the playlist
    if (focus_track == pfc_infinite)
        focus_track = 0;

    focus_change_manual(focus_track);
}

void sources_control::focus_change_manual(t_size p_new)
{
    // get handle to track p_new
    metadb_handle_ptr track;
    static_api_ptr_t<playlist_manager>()->activeplaylist_get_item_handle(track, p_new);
    m_selected_track = track;

    if (get_display_mode() == display_selected || track == m_playing_track)
    {
        track_change_manual();
    }
}

void sources_control::track_change_manual()
{
    // check if current source is a "no-cover" image
    bool b_nocover = (strcmp_partial(m_sources[m_current_source], "--") == 0);
    t_size old_source = m_current_source;

    //reset to first source in current group
    m_current_source = get_first_in_group(m_current_source);

    find_and_set_image(b_nocover);
}
