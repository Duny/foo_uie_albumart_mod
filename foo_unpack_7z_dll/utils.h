#ifndef _FOO_UNPACK_7Z_DLL_UTILS_H_
#define _FOO_UNPACK_7Z_DLL_UTILS_H_

namespace unpack_7z
{
    struct error_log : public pfc::string_formatter
    {
        ~error_log () { if (!is_empty()) console::formatter () << "Error("COMPONENT_NAME"):" << get_ptr (); }
    };

    struct debug_log : public pfc::string_formatter
    {
        ~debug_log () { if (!is_empty()) console::formatter () << "Debug("COMPONENT_NAME"):" << get_ptr (); }
    };
}
#endif