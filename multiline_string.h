#ifndef __FOO_UIE_ALBUMART__MULTILINE_STRING_H__
#define __FOO_UIE_ALBUMART__MULTILINE_STRING_H__

/*   multiline_string.h
 *
 * Cheran Shunmugavel - September 2006
 *
 * Contains a class for manipulating multiline strings,
 * especially as it relates to reading and writing text files.
 * Handles creating a multiline string from a collection of
 * strings and separating a string into individual lines.
 *
 * NOTE: I haven't completely tested all the functions yet.
 *
 * Requires: foobar2000 0.9 SDK
*/

#include "stdafx.h"

class multiline_string
{
protected:
    list_t<string8> m_buffer;
    t_size m_cur_line;

public:
    multiline_string() { m_buffer.remove_all(); m_cur_line = 0;}
    multiline_string(const char * p_source) { set_string(p_source); }

    // would be nice to be able to use string_base instead...
    multiline_string(list_t<string8> & p_source)
    {
        m_buffer.remove_all();
        for (t_size n = 0; n < p_source.get_count(); n++)
            m_buffer.add_item(p_source.get_item(n));
        m_cur_line = 0;
    }

    // parses a string which contains multiple lines
    // separated by CR and/or LF combinations
    void set_string(const char * p_source)
    {
        string8 temp(p_source);
        t_size eol;

        m_buffer.remove_all();

        eol = 0;

        while (eol != pfc_infinite)
        {
            eol = temp.find_first('\r');
            if (eol != pfc_infinite)
            {
                string8 line(temp.get_ptr(), eol);
                m_buffer.add_item(line);

                // remove the line just read, including the
                // ending carriage-return
                temp.remove_chars(0, eol + 1);

                // if there's a line-feed, too, skip it
                if (strcmp_partial(temp, "\n") == 0)
                    temp.remove_chars(0, 1);
            }
            else
            {
                eol = temp.find_first('\n');
                if (eol != pfc_infinite)
                {
                    string8 line(temp.get_ptr(), eol);
                    m_buffer.add_item(line);
                    temp.remove_chars(0, eol + 1);
                }
                else
                {
                    m_buffer.add_item(temp);
                }
            }
        }
        m_cur_line = 0;
    }

    // outputs a string with each line terminated by a
    // CR/LF sequence.  The last line does not end with
    // a CR/LF.
    void get_string(string_base & p_out) const
    {
        p_out.reset();
        t_size num_strings = m_buffer.get_count();
        for (t_size n = 0; n < num_strings-1; n++)
        {
            p_out.add_string(m_buffer[n]);
            p_out.add_string("\r\n");
        }
        p_out.add_string(m_buffer[num_strings-1]);
    }

    inline t_size get_count() { return m_buffer.get_count(); }

    // adds line to the end of the list if p_after is not specified
    inline void add_line(const char * p_source, t_size p_after = ~0) { m_cur_line = m_buffer.insert_item(p_source, p_after); }

    // removes current line if n is not specified
    void remove_line(t_size n = ~0) { if (n == ~0) n = m_cur_line; m_buffer.remove_by_idx(n); }

    // returns line at current cursor location if n is not specified
    void get_line(string_base & p_out, t_size n = ~0) const { if (n == ~0) n = m_cur_line; p_out = m_buffer.get_item(n); }

    // reads line at the current cursor location and advances
    // cursor to next line
    // returns false if the cursor is at end of string
    bool read_line(string_base & p_out)
    {
        if (is_cursor_valid())
        {
            p_out = m_buffer[m_cur_line];
            seek_next();
            return true;
        }
        else
        {
            return false;
        }
    }

    inline const char * operator[](t_size n) const { string8 temp; get_line(temp, n); return temp.get_ptr(); }

    bool is_cursor_valid() { return ((m_cur_line != ~0) && (m_cur_line < m_buffer.get_count())); }

    inline void seek_first() { m_cur_line = 0; }
    inline void seek_last() { m_cur_line = m_buffer.get_count() - 1; }

    void seek(t_size n) { if (n >= m_buffer.get_count()) n = m_buffer.get_count() - 1; m_cur_line = n; }

    // returns false if end of list is reached
    bool seek_next()
    {
        if (m_cur_line >= m_buffer.get_count())
        {
            return false;
        }
        else
        {
            m_cur_line++;
            return (m_cur_line < m_buffer.get_count());
        }
    }

    // returns false if beginning of list is reached
    bool seek_prev()
    {
        if (m_cur_line == ~0)
        {
            return false;
        }
        else if (m_cur_line == 0)
        {
            m_cur_line = ~0;
            return false;
        }
        else
        {
            m_cur_line--;
            return true;
        }
    }
};

#endif