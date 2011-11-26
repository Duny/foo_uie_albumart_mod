#ifndef __FOO_UIE_ALBUMART__BITMAP_FILE_H__
#define __FOO_UIE_ALBUMART__BITMAP_FILE_H__

/*   bitmap_file.h
 *
 * Cheran Shunmugavel - September 2006
 *
 * Contains a class that represents an image file.  Has
 * helper functions for reading the bitmap into memory.
 *
 * TODO: Take a stream_reader instead of a file
 *
 * Requires: foobar2000 0.9 SDK, GDI+
*/

class bitmap_file
{
public:
    bitmap_file() {m_bitmap.release(); m_bitmap_stream = NULL; m_file.release();album_art_embedded.release();}
    bitmap_file(const char * p_path);
    ~bitmap_file();

    bool set_file(service_ptr_t<file> & p_source);
    void reset_file();

    bool is_bitmap();
    bool get_gdi_bitmap(HBITMAP & p_bmp_out);
    bool get_gdiplus_bitmap(pfc::rcptr_t<Bitmap> & p_bmp_out);
    bool get_gdiplus_bitmap_from_album_art_data (album_art_data_ptr & pic, pfc::rcptr_t<Bitmap> & p_bmp_out);
    void release_bitmap();

protected:
    service_ptr_t<file> m_file;
    album_art_data_ptr album_art_embedded;
    pfc::rcptr_t<Bitmap> m_bitmap;
    IStream * m_bitmap_stream;
};

#endif