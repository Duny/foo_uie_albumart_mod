#include "stdafx.h"
#include "bitmap_file.h"

bitmap_file::bitmap_file(const char * p_path)
{
    m_bitmap.release();
    m_bitmap_stream = NULL;

    service_ptr_t<file> input;
    abort_callback_impl file_abort;
    try
    {
        filesystem::g_open_read(input, p_path, file_abort);
        m_file.copy(input);
    }
    catch (exception_io)
    {
        input.release();
        m_file.release();
        return;
    }
}

bitmap_file::~bitmap_file()
{
    m_file.release();
    release_bitmap();
}

void bitmap_file::release_bitmap()
{
    if (m_bitmap.is_valid())
    {
        m_bitmap.release();

        m_bitmap_stream->Release();
        m_bitmap_stream = NULL;
    }
}

bool bitmap_file::set_file(service_ptr_t<file> & p_source)
{
    if (p_source.is_empty())
    {
        return false;
    }
    else
    {
        if (m_file.is_valid())
            m_file.release();

        m_file.copy(p_source);
        return true;
    }
}

bool bitmap_file::is_bitmap()
{
    pfc::rcptr_t<Bitmap> bmp;
    return get_gdiplus_bitmap(bmp);
}

bool bitmap_file::get_gdi_bitmap(HBITMAP & p_bmp_out)
{
    pfc::rcptr_t<Bitmap> bmp;
    bool success = false;
    HBITMAP hBmp = NULL;

    if (get_gdiplus_bitmap(bmp))
    {
        Status ret_val = bmp->GetHBITMAP(Color(0,0,0), &hBmp);
        success = (ret_val == Ok);
    }
    else
    {
        success = false;
    }

    p_bmp_out = hBmp;
    return success;
}

bool bitmap_file::get_gdiplus_bitmap(pfc::rcptr_t<Bitmap> & p_bmp_out)
{
    if (m_bitmap.is_valid())
    {
        p_bmp_out = m_bitmap;
        return true;
    }

    if (m_file.is_empty())
    {
        return false;
    }

    abort_callback_impl file_abort;
    t_size file_size = m_file->get_size(file_abort);
    if (file_size == filesize_invalid)
        return false;

    HGLOBAL hGlobalMem = GlobalAlloc(GHND, file_size);
    void * buffer = GlobalLock(hGlobalMem);
    m_file->read(buffer, file_size, file_abort);
    GlobalUnlock(hGlobalMem);

    // hGlobalMem will automatically be freed when m_bitmap_stream is released
    HRESULT hr = CreateStreamOnHGlobal(hGlobalMem, TRUE, &m_bitmap_stream);
    if (!SUCCEEDED(hr))
    {
        m_bitmap_stream->Release();
        GlobalFree(hGlobalMem);
        return false;
    }

    m_bitmap.new_t(m_bitmap_stream);

    if (m_bitmap->GetHeight() > 0)
    {
        p_bmp_out = m_bitmap;
        return true;
    }
    else
    {
        m_bitmap.release();
        m_bitmap_stream->Release();
        return false;
    }
}

bool bitmap_file::get_gdiplus_bitmap_from_album_art_data (album_art_data_ptr & pic, pfc::rcptr_t<Bitmap> & p_bmp_out)
{
    if (m_bitmap.is_valid())
    {
        p_bmp_out = m_bitmap;
        return true;
    }

    if (!pic.is_valid () || pic.is_empty())
    {
        return false;
    }

    abort_callback_impl file_abort;
    t_size file_size = pic->get_size ();
    if (file_size == filesize_invalid)
        return false;

    HGLOBAL hGlobalMem = GlobalAlloc(GHND, file_size);
    void * buffer = GlobalLock(hGlobalMem);
    memcpy (buffer, pic->get_ptr (), file_size);
    GlobalUnlock(hGlobalMem);

    // hGlobalMem will automatically be freed when m_bitmap_stream is released
    HRESULT hr = CreateStreamOnHGlobal(hGlobalMem, TRUE, &m_bitmap_stream);
    if (!SUCCEEDED(hr))
    {
        m_bitmap_stream->Release();
        GlobalFree(hGlobalMem);
        return false;
    }

    m_bitmap.new_t(m_bitmap_stream);

    if (m_bitmap->GetHeight() > 0)
    {
        p_bmp_out = m_bitmap;
        return true;
    }
    else
    {
        m_bitmap.release();
        m_bitmap_stream->Release();
        return false;
    }
}