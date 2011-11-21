#ifndef __FOO_UIE_ALBUMART__STDAFX_H__
#define __FOO_UIE_ALBUMART__STDAFX_H__

#include "foobar2000\SDK\foobar2000.h"
#include "foobar2000\helpers\helpers.h"
#include "ui_extension.h"

using namespace pfc;

// 7-zip includes
#include "Common\MyInitGuid.h"
#include "Common\MyCom.h"
#include "7zip\Archive\IArchive.h"

#define COMPONENT_NAME "foo_uie_albumart_mod"

#include "afxres.H"

#include <GdiPlus.h>
#pragma comment(lib, "gdiplus.lib")
using namespace Gdiplus;

#include "playlist_tree_callback.h"
#include "albumart_vars.h"
#include "version_control.h"
#include "albumart_sources.h"
#include "albumart_helpers.h"
#include "titleformat_helper.h"
#include "bitmap_file.h"
#include "uie_albumart.h"
#include "albumart_config.h"
#include "multiline_string.h"
//#include "albumart_embedded.h"
#include "resource.h"

#endif