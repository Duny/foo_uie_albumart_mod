#include "stdafx.h"
#include "bitmap_file.h"
#include "albumart_helpers.h"
#include "foo_unpack_7z_dll/archive.h"

static const char * image_exts = "jpg;jpeg;gif;bmp;png;tif;tiff;ico;emf;wmf";

struct t_custom_colors
{
    DWORD m_table[16];

    t_custom_colors()
    {
        fill_array_t(m_table, RGB(255, 255, 255));
    }
};

bool g_color_picker(HWND parent, COLORREF & p_value)
{
    t_custom_colors custom_colors;
    COLORREF color = p_value;
    if (uChooseColor(&color, parent, custom_colors.m_table))
    {
        p_value = color;
        return true;
    }
    return false;
}

//! Scan recursively for image file matching pattern
//! @param [out] p_path_out will be filled with found matches
//! @param [in] p_path already determind portion of the path, contains no wildcards
//! @param [in] p_pattern pattern with wildcards, specifies path relative to p_path
//! @param [in] p_process_all true to process all wildcard matches, false to stop at first match
static void findFirstImageMatch_internal(list_t<string8> & p_path_out, const char * p_path,
                                         const char * p_pattern, bool p_process_all)
{
#ifdef _DEBUG
    uDebugLog() << "image match recursion:\n path = \"" << p_path << "\"\n pattern = \"" << p_pattern << "\"";
#endif

    if(!wildcard_helper::has_wildcards(p_pattern))
    {
        string8 fullpath;
        fullpath << p_path;
        if (p_pattern[0] != 0)
        {
            // Make sure we don't add a directory separator for wildcard-less patterns
            if (!fullpath.is_empty())
                fullpath << "\\";

            fullpath << p_pattern;
        }

        abort_callback_impl file_abort;

        // Check if the file has an image file extension
        string_extension file_ext(fullpath);
        if (pfc::string_find_first(image_exts, string_lower(file_ext.get_ptr())) == pfc_infinite)
            return;

        // check if the file exists and is valid
        bool file_exists;
        try
        {
            // if the match is a directory, we don't want to try to open it,
            // or else Bad Things will happen
            if (filesystem::g_is_valid_directory(file_path_canonical(fullpath), file_abort))
                return;

            // if the file path is not valid, we don't want to try opening it
            if (filesystem::g_is_remote_or_unrecognized(file_path_canonical(fullpath)))
                return;

            file_exists = filesystem::g_exists(fullpath, file_abort);

        }
        catch (...)
        {
            return;
        }

        if (file_exists)
            p_path_out.add_item(fullpath);

        return;
    }

    // move next directory level(s) from p_pattern to p_path

    // position of first wildcard
    t_size first_wildcard = min_t(
        string_find_first(p_pattern, '*'),
        string_find_first(p_pattern, '?'));

    t_size pattern_length = strlen(p_pattern);

    // length of the prefix of path that "specifies a valid directory or path, 
    // and file name that can contain wildcard characters (* and ?)."
    // (see FindFirstFile in the Platform SDK)
    t_size base_length = min_t(pattern_length, string_find_first(p_pattern, '\\', first_wildcard));

    string8 base_path;
    base_path.set_string(p_path);
    base_path.add_string(p_pattern, base_length);

    const char * remaining_pattern = &p_pattern[base_length];

#ifdef _DEBUG
    uDebugLog() << "image match:\n search path = " << base_path << "\n remaining pattern = \"" << remaining_pattern << "\"";
#endif

    // pointer will be automatically deleted when 'find' goes out of scope
    ptrholder_t<uFindFile> find = uFindFirstFile(base_path);
    if(find.is_empty())
    {
        return;
    }

    // Remove part with wildcards
    base_path.truncate(base_path.scan_filename());

    do
    {
        if(strcmp(find->GetFileName(), ".") != 0 && strcmp(find->GetFileName(), "..") !=0)
        {
            findFirstImageMatch_internal(p_path_out,
                string8() << base_path << find->GetFileName(),
                remaining_pattern, p_process_all);

            // if we've found a match and caller specified to stop here, then stop
            if (!p_process_all && p_path_out.get_count() > 0)
                return;
        }
    }
    while (find->FindNext());
}

class my_archive_callback : public archive_callback {
public:
	my_archive_callback() {}

	bool on_entry(archive * owner,const char * url,const t_filestats & p_stats,const service_ptr_t<file> & p_reader)
	{
		return true;
	}
};

static void findArchiveImageMatch_internal(list_t<string8> & p_path_out, const char * p_pattern, bool p_process_all)
{
    string8 full_path, archive_path, file_path, file_out;
    abort_callback_impl file_abort;

    filesystem::g_get_canonical_path(p_pattern, full_path);

    t_size pipe_location = string_find_first(full_path, '|');

    archive_path = full_path;
    archive_path.truncate(pipe_location);

    file_path = full_path;
    skip_prefix (file_path, archive_path);
    skip_prefix (file_path, "|");

    if (wildcard_helper::has_wildcards (p_pattern))
	{
        try {
		    unpack_7z::archive archive (archive_path, abort_callback_dummy ());

            archive.list ([&] (const pfc::string8 &item_path, const t_filestats) ->bool
            {
                if (wildcard_helper::test (item_path, file_path)) {
                    archive_impl::g_make_unpack_path (file_out, archive_path, item_path, "7");
                    p_path_out.add_item (file_out);
                }
                return true;
            });

        } catch (...) {}
		
		//	new service_impl_t<my_archive_callback>, true);
    //    if (!C7Z::IsSupportedType (archive_type))
    //        return;

    //    c7z_open archive (archive_type);
    //    file_ptr m_file;
    //    pfc::string8_fastalloc m_url;

    //    try {
    //        filesystem::g_open (m_file, archive_path, filesystem::open_mode_read, file_abort);
    //        archive.open_archive (m_file, file_abort);
    //    }
    //    catch (...) {
    //        return;
    //    }

    //    t_size num_items = archive.files.get_size (), i;
    //    for (i = 0; i < num_items; i++) {
    //        if (wildcard_helper::test (archive.files[i].name, file_path)) {
    //            archive_impl::g_make_unpack_path (file_out, archive_path, archive.files[i].name, archive_type);
    //            p_path_out.add_item (file_out);
    //        }
    //    }
    }
    else {
       archive_impl::g_make_unpack_path(file_out, archive_path, file_path, string_extension(archive_path));

        try
        {
            if (archive::g_exists (file_out.get_ptr (), file_abort))
            {
                bitmap_file test_bitmap (file_out.get_ptr ());
                if (test_bitmap.is_bitmap ())
                    p_path_out.add_item (file_out);
            }
        }
        catch (exception_io_no_handler_for_path)
        {
            // do nothing
        }
    }
}

void findFirstImageMatch(list_t<string8> & p_path_out, const char * p_pattern, bool p_process_all)
{
#ifdef _DEBUG
    uDebugLog() << "image match:\n pattern = \"" << p_pattern << "\"";
#endif

    if (strcmp_partial(p_pattern, "http://") == 0)
        return;

    string8 pattern;
    fix_filename(pattern, p_pattern);

    // use the appropriate find function
    //   right now, archives are detected by searching for a '|' in the path,
    //   since the pipe is not a legal character for a filename.  Not sure
    //   if this is the best way to do it, though.
    if (string_find_first(p_pattern, '|') == pfc_infinite)
    {
        // Normal path
        findFirstImageMatch_internal(p_path_out, "", pattern, p_process_all);

        if ((p_path_out.get_count() == 0) && (pattern.length() >= 2))
        {
            bool is_unc = (pfc::strcmp_partial(pattern, "\\\\") == 0);
            if ((pattern[1] != ':') && (!is_unc))
            {
                // append the current directory to the path and test again
                // to allow for paths relative to components dir
                string8 app_name(core_api::get_my_file_name());
                string8 current_dir(core_api::get_my_full_path());
                current_dir.truncate(current_dir.length()-app_name.length()-strlen(".dll"));
#ifdef _DEBUG
    uDebugLog() << " appending current dir: \"" << current_dir << "\"";
#endif
                pattern.insert_chars(0, current_dir);

                findFirstImageMatch_internal(p_path_out, "", pattern, p_process_all);
            }
        }
    }
    else
    {
        // Archive path
        findArchiveImageMatch_internal(p_path_out, p_pattern, p_process_all);
    }
}


// adjusts a given path for certain discrepancies between how foobar2000
// and GDI+ handle paths, and other oddities
//
// Currently fixes:
//   - User might use a forward-slash instead of a
//     backslash for the directory separator
//   - GDI+ ignores trailing periods '.' in directory names
//   - GDI+ and FindFirstFile ignore double-backslashes
static void fix_filename(string8 & p_out, const char * p_path)
{
    p_out.set_string(p_path);

    if (p_out.get_length() == 0)
        return;

    // fix directory separators
    p_out.replace_char('/', '\\');

    // fix double-backslashes and trailing periods in directory names
    string8 temp(p_out);
    t_size temp_len = temp.get_length();
    p_out.reset();
    p_out.add_byte(temp[0]);
    for (t_size n = 1; n < temp_len-1; n++)
    {
        if (temp[n] == '\\')
        {
            if (temp[n+1] == '\\')
                continue;
        }
        else if (temp[n] == '.')
        {
            if ((temp[n-1] != '.' && temp[n-1] != '\\') &&
                temp[n+1] == '\\')
                continue;
        }

        p_out.add_byte(temp[n]);
    }
    if (temp_len > 1)
        p_out.add_byte(temp[temp_len-1]);
}


bool skip_prefix(string8 & p_string, const char * p_prefix)
{
    if (strcmp_partial(p_string.get_ptr(), p_prefix) == 0)
    {
        p_string.remove_chars(0,strlen(p_prefix));
        return true;
    }
    return false;
}

bool get_extractor(album_art_extractor::ptr & out,const char * path, abort_callback & p_abort) {
    service_enum_t<album_art_extractor> e;
	pfc::string_extension ext(path);
	while(e.next(out)) {
		if (out->is_our_path(path,ext)) {
            return true;
		}
	}
	return false;
}