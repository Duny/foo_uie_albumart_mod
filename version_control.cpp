#include "stdafx.h"

// for now, p_config should be the most recent type of albumart_vars.  This may change in the future.
void version_control::read_config(stream_reader * p_reader, t_size p_size, abort_callback & p_abort,
                                  albumart_vars & p_config, pfc::list_t<pfc::string8> & p_sources)
{
    unsigned read_version;

    // reset the config now; in case anything goes wrong later,
    // we should still have a valid config
    reset_config_vars(p_config, p_sources);

    if (p_size < sizeof(unsigned))
        return;

    try
    {
        t_size albumart_var_size = 0;
        p_reader->read_lendian_t(read_version, p_abort);
        switch (read_version)
        {
        case VERSION_024:
            if (albumart_var_size == 0)
                albumart_var_size = sizeof(albumart_vars_024);
            // fall-through
        case VERSION_0241:
        case VERSION_025:
        case VERSION_0251:
            if (albumart_var_size == 0)
                albumart_var_size = sizeof(albumart_vars_0241);
            // fall-through
        case VERSION_026:
        case VERSION_027:
            {
                if (albumart_var_size == 0)
                    albumart_var_size = sizeof(albumart_vars);

                if (p_size < (sizeof(unsigned) + albumart_var_size))
                    return;

                int num_sources;
                p_reader->read_lendian_t(num_sources, p_abort);
                p_sources.remove_all();
                for (t_size n = 0; n < num_sources; n++)
                {
                    pfc::string8 temp;
                    p_reader->read_string(temp, p_abort);
                    p_sources.add_item(temp);
                }

                // read only the variables that were in previous versions and the rest
                // should be already set from the previous reset
                p_reader->read_object(&p_config, albumart_var_size, p_abort);
            }
            break;
        case VERSION_022:
        case VERSION_021:
        case VERSION_020:
            // in older versions, the number of sources was part of the
            // albumart_vars structure, and the sources list was stored
            // after the rest of the variables.  This code converts old
            // configs to the new format, where the number of sources is
            // not part of the albumart_vars structure and the sources
            // list is stored before other variables.
            {
                p_reader->read_object(&p_config, sizeof(albumart_vars_024), p_abort);

                // p_config is a NEW format albumart_vars structure,
                // so the number of sources is read into the variable
                // last_tab, and the real last_tab variable is still
                // on the stream
                int num_sources = p_config.last_tab;
                p_reader->read_lendian_t(p_config.last_tab, p_abort);

                // get source strings
                p_sources.remove_all();
                for (t_size n = 0; n < num_sources; n++)
                {
                    pfc::string8 temp;
                    p_reader->read_string(temp, p_abort);
                    p_sources.add_item(temp);
                }
            }
            break;
        case VERSION_023:
        default:
            // if there's no recognized version stored, it's either a newer
            // version or some other weird thing...just stay with the config defaults
            //    Also reset the config if the old version was the OpenGL
            //    version (0.2.3)
            return;
            break;
        }
    }
    catch (exception_aborted)
    {
        return;
    }
}

// Preferences are stored as follows (all variables are stored as little endian)
//
// * VERSION_CURRENT constant
// * Number of source strings
// * source strings, each string is preceded by a 32-bit header with the length of the
//   string in bytes (see declaration of p_writer::write_string)
// * albumart_vars structure
void version_control::write_config(stream_writer * p_writer, abort_callback & p_abort, 
                                   const albumart_vars & p_config, const pfc::list_t<pfc::string8> & p_sources) const
{
    int num_sources = p_sources.get_count();

    p_writer->write_lendian_t(m_version, p_abort);
    p_writer->write_lendian_t(num_sources, p_abort);
    for (t_size n = 0; n < num_sources; n++)
        p_writer->write_string(p_sources[n], p_abort);
    p_writer->write_object(&p_config, sizeof(albumart_vars), p_abort);
}