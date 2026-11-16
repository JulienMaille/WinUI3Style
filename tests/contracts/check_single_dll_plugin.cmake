cmake_minimum_required(VERSION 3.21)

# Issue #70: a normal Qt plugin deployment must need only qwinui3style.dll.
# Guard against a separate implementation DLL/SO being copied beside the
# plugin or into the gallery's styles directory. The implementation is embedded
# in the MODULE via the shared object library, so no companion may appear.

if(NOT DEFINED PLUGIN_DIR OR PLUGIN_DIR STREQUAL "")
    message(FATAL_ERROR "PLUGIN_DIR is required")
endif()

function(_winui3style_check_styles_dir _label _dir)
    if(NOT IS_DIRECTORY "${_dir}")
        # Target not built for this configuration; nothing to inspect.
        return()
    endif()
    file(GLOB _entries "${_dir}/*")
    set(_found_plugin FALSE)
    foreach(_entry IN LISTS _entries)
        if(IS_DIRECTORY "${_entry}")
            continue()
        endif()
        get_filename_component(_name "${_entry}" NAME)
        if(_name MATCHES "winui3style")
            if(_name MATCHES "^qwinui3style")
                set(_found_plugin TRUE)
            else()
                message(FATAL_ERROR
                    "${_label} contains a separate implementation artifact: ${_name}. "
                    "The style must be embedded in qwinui3style.dll (issue #70).")
            endif()
        endif()
    endforeach()
    if(NOT _found_plugin)
        message(FATAL_ERROR "${_label} does not contain qwinui3style: ${_dir}")
    endif()
endfunction()

_winui3style_check_styles_dir("plugin styles directory" "${PLUGIN_DIR}")
if(DEFINED GALLERY_STYLES_DIR AND NOT GALLERY_STYLES_DIR STREQUAL "")
    _winui3style_check_styles_dir("gallery styles directory" "${GALLERY_STYLES_DIR}")
endif()
