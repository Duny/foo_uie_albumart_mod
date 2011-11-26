#ifndef __FOO_UIE_ALBUMART__PLAYLIST_TREE__H__
#define __FOO_UIE_ALBUMART__PLAYLIST_TREE__H__

// Original code courtesy of cwbowron @ Hydrogenaudio.org

class callback_node
{
public:
virtual bool get_entries( pfc::list_base_t<metadb_handle_ptr> & list ) const = 0;
virtual bool is_leaf() const = 0;
virtual bool is_folder() const = 0;
virtual bool is_query() const = 0;
virtual int get_entry_count() const = 0;
virtual void get_name( pfc::string_base & out ) const = 0;
};

class NOVTABLE node_select_callback : public service_base
{
public:
virtual void on_node_select( const callback_node * node ) = 0;

FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(node_select_callback);
};

DECLARE_CLASS_GUID(node_select_callback,
    0x9c2ae3c3, 0xdc04, 0x4042, 0xad, 0xf3, 0x88, 0x84, 0x11, 0x7b, 0x49, 0x55);




// Code from this point forward written by Cheran Shunmugavel

class albumart_ns_callback
{
public:
    virtual void on_node_select(const callback_node * p_node) = 0;
};

class NOVTABLE albumart_ns_manager : public service_base
{
public:
    virtual void run_callbacks(const callback_node * p_node) = 0;
    virtual void register_callback(albumart_ns_callback * p_callback) = 0;
    virtual void unregister_callback(albumart_ns_callback * p_callback) = 0;

    FB2K_MAKE_SERVICE_INTERFACE_ENTRYPOINT(albumart_ns_manager);
};

class albumart_ns_manager_impl : public albumart_ns_manager
{
public:
    void run_callbacks(const callback_node * p_node)
    {
        t_size idx;
        for (idx=0; idx<m_callbacks.get_count(); idx++)
        {
            m_callbacks[idx]->on_node_select(p_node);
        }
    }

    void register_callback(albumart_ns_callback * p_callback)
    {
        if (p_callback)
            m_callbacks.add_item(p_callback);
    }

    void unregister_callback(albumart_ns_callback * p_callback)
    {
        m_callbacks.remove_item(p_callback);
    }

protected:
    pfc::list_t<albumart_ns_callback*> m_callbacks;
};

class albumart_node_select_callback_impl : public node_select_callback
{
public:
    void on_node_select(const callback_node * node)
    {
        static_api_ptr_t<albumart_ns_manager>()->run_callbacks(node);
    }
};

// {71F7AAB6-8735-4505-8309-289096A751B2}
DECLARE_CLASS_GUID(albumart_ns_manager, 
0x71f7aab6, 0x8735, 0x4505, 0x83, 0x9, 0x28, 0x90, 0x96, 0xa7, 0x51, 0xb2);

#endif //__FOO_UIE_ALBUMART__PLAYLIST_TREE__H__