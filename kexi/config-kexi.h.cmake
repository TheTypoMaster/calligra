#ifndef KEXI_CONFIG_H
#define KEXI_CONFIG_H

/* config-kexi.h. Generated by cmake from config-kexi.h.cmake */

/*! @file config-kexi.h
    Global Kexi configuration (build time)
*/

#include <config-kdb.h>

//! @def KEXI_MOBILE
//! @brief If defined, a mobile version of Kexi if compiled
#cmakedefine KEXI_MOBILE

/* define if you have libreadline available */
/* TODO: detect #define HAVE_READLINE 1 */

//! @def HAVE_UNAME
//! @brief If defined, uname(2) is available
#cmakedefine HAVE_UNAME 1

/*! For KexiUtils::encoding() */
#cmakedefine01 HAVE_LANGINFO_H

//! @def KEXI_DEBUG_GUI
//! @brief If defined, a debugging GUI for Kexi is enabled
#cmakedefine KEXI_DEBUG_GUI

#if defined KEXI_DEBUG_GUI && !defined KDB_DEBUG_GUI
# error KEXI_DEBUG_GUI requires a KDB_DEBUG_GUI cmake option to be set too in KDb.
#endif

/* -- Experimental -- */

//! @def KEXI_SCRIPTS_SUPPORT
//! @brief If defined, scripting GUI plugin is enabled in Kexi
#cmakedefine KEXI_SCRIPTS_SUPPORT

//! @def KEXI_SHOW_UNFINISHED
//! @brief If defined unfinished features are enabled and presented in Kexi.
//! This is useful for testing but may confuse end-users.
#cmakedefine KEXI_SHOW_UNFINISHED

//! @def KEXI_PROJECT_TEMPLATES
//! @brief If defined, support for project templates is enabled in Kexi
#cmakedefine KEXI_PROJECT_TEMPLATES

//! @def KEXI_AUTORISE_TABBED_TOOLBAR
//! @brief If defined, tabs in the main tabbed toolbar autorise in Kexi
#cmakedefine KEXI_AUTORISE_TABBED_TOOLBAR

#endif
