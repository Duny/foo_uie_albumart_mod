#ifndef __FOO_UIE_ALBUMART__TITLEFORMAT_HELPER_H__
#define __FOO_UIE_ALBUMART__TITLEFORMAT_HELPER_H__

//! Replaces or removes reserved characters.
class titleformat_text_filter_impl_reserved_chars_ex : public titleformat_text_filter
{
public:
    //! The n-th character from p_reserved_chars will be replaced with the n-th character
    //! from p_replacement_chars. If p_reserved_chars contains more characters than
    //! p_replacement_chars, any characters from p_reserved_chars that have no corresponding
    //! character in p_replacement_chars will be removed.
    //! @param p_reserved_chars list of UTF-8 characters that will be filtered
    //! @param p_replacement_chars list of UTF-8 characters
    titleformat_text_filter_impl_reserved_chars_ex(const char * p_reserved_chars, const char * p_replacement_chars);

    //! All characters from p_reserved_chars will be replaced with p_replacement_char.
    //! If p_replacement_char is 0, they will be removed.
    //! @param p_reserved_chars list of UTF-8 characters that will be filtered
    //! @param p_replacement_char UTF-8 character
    titleformat_text_filter_impl_reserved_chars_ex(const char * p_reserved_chars, unsigned p_replacement_char);

    virtual void write(const GUID & p_inputtype, pfc::string_receiver & p_out, const char * p_data, t_size p_data_length);
private:
    struct replace_entry
    {
        unsigned m_from, m_to;

        replace_entry() {}
        replace_entry(const replace_entry & p_source) : m_from(p_source.m_from), m_to(p_source.m_to) {}
        replace_entry(unsigned p_from, unsigned p_to) : m_from(p_from), m_to(p_to) {}
    };
    pfc::array_hybrid_t<replace_entry, 8> m_entries;

    bool test_for_replacement_char(unsigned c1, unsigned * c2) const;
    void replace_forbidden_chars(pfc::string_receiver & p_out, const char * p_source, t_size p_source_len) const;
};

#endif
