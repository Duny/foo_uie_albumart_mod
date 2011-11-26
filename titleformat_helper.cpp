#include "stdafx.h"
#include "titleformat_helper.h"

titleformat_text_filter_impl_reserved_chars_ex::titleformat_text_filter_impl_reserved_chars_ex(const char * p_reserved_chars, const char * p_replacement_chars)
{
    if (p_reserved_chars == NULL || p_reserved_chars[0] == 0) return;
    if (p_replacement_chars == NULL)
        p_replacement_chars = "";
    while (true)
    {
        unsigned c1, c2;
        t_size len1 = pfc::utf8_decode_char(p_reserved_chars, c1);
        if (len1 == 0) break;
        t_size len2 = pfc::utf8_decode_char(p_replacement_chars, c2);
        m_entries.append_single(replace_entry(c1, c2));
        p_reserved_chars += len1;
        p_replacement_chars += len2;
    }
}

titleformat_text_filter_impl_reserved_chars_ex::titleformat_text_filter_impl_reserved_chars_ex(const char * p_reserved_chars, unsigned p_replacement_char)
{
    if (p_reserved_chars == NULL || p_reserved_chars[0] == 0) return;
    while (true)
    {
        unsigned c1;
        t_size len1 = pfc::utf8_decode_char(p_reserved_chars, c1);
        if (len1 == 0) break;
        m_entries.append_single(replace_entry(c1, p_replacement_char));
        p_reserved_chars += len1;
    }
}

void titleformat_text_filter_impl_reserved_chars_ex::write(const GUID & p_inputtype, pfc::string_receiver & p_out, const char * p_data, t_size p_data_length)
{
    if (p_inputtype == titleformat_inputtypes::meta)
        replace_forbidden_chars(p_out, p_data, p_data_length);
    else
        p_out.add_string(p_data, p_data_length);
}

bool titleformat_text_filter_impl_reserved_chars_ex::test_for_replacement_char(t_size c1, t_size * c2) const
{
    for (t_size n = 0; n < m_entries.get_size(); n++)
    {
        if (m_entries[n].m_from == c1)
        {
            *c2 = m_entries[n].m_to;
            return true;
        }
    }
    return false;
}

void titleformat_text_filter_impl_reserved_chars_ex::replace_forbidden_chars(pfc::string_receiver & p_out, const char * p_source, t_size p_source_len) const
{
    if (m_entries.get_size() == 0)
    {
        p_out.add_string(p_source, p_source_len);
    }
    else
    {
        p_source_len = pfc::strlen_max(p_source, p_source_len);
        t_size index = 0;
        t_size good_byte_count = 0;
        while(index < p_source_len)
        {
            unsigned c1, c2;
            t_size delta = pfc::utf8_decode_char(p_source + index, c1, p_source_len - index);
            if (delta == 0) break;
            if (test_for_replacement_char(c1, &c2))
            {
                if (good_byte_count > 0)
                {
                    p_out.add_string(p_source + index - good_byte_count, good_byte_count);
                    good_byte_count = 0;
                }
                if (c2 != 0) p_out.add_char(c2);
            }
            else
            {
                good_byte_count += delta;
            }
            index += delta;
        }
        if (good_byte_count > 0)
        {
            p_out.add_string(p_source + index - good_byte_count, good_byte_count);
            good_byte_count=0;
        }
    }
}
