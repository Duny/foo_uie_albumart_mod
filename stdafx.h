#ifndef __FOO_UIE_ALBUMART__STDAFX_H__
#define __FOO_UIE_ALBUMART__STDAFX_H__

#define COMPONENT_NAME "foo_uie_albumart_mod"

// foobar includes
#include "foobar2000/ATLHelpers/ATLHelpers.h"
// columns ui sdk
#include "ui_extension.h"

// Boost library
#include "boost/function.hpp"

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
#include "resource.h"

#endif