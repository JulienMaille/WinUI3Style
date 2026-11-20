cmake_minimum_required(VERSION 3.21)

# Issue #70: a normal Qt plugin deployment must need only qwinui3style.dll.
# Guard against a separate implementation DLL/SO being copied beside the
# plugin or into the gallery's styles directory. The implementation is embedded
# in the MODULE via the shared object library, so no companion may appear.

if(NOT DEFINED PLUGIN_FILE OR PLUGIN_FILE STREQUAL "")
    message(FATAL_ERROR "PLUGIN_FILE is required")
endif()

function(_winui3style_check_styles_dir _label _file)
    if(NOT EXISTS "${_file}" OR IS_DIRECTORY "${_file}")
        message(FATAL_ERROR "${_label} plugin is missing: ${_file}")
    endif()
    get_filename_component(_dir "${_file}" DIRECTORY)
    file(GLOB _entries "${_dir}/*")
    foreach(_entry IN LISTS _entries)
        if(IS_DIRECTORY "${_entry}")
            continue()
        endif()
        get_filename_component(_name "${_entry}" NAME)
        if(_name MATCHES "winui3style" AND NOT _name MATCHES "^qwinui3style")
            message(FATAL_ERROR
                "${_label} contains a separate implementation artifact: ${_name}. "
                "The style must be embedded in qwinui3style.dll (issue #70).")
        endif()
    endforeach()
endfunction()

_winui3style_check_styles_dir("plugin output" "${PLUGIN_FILE}")
if(DEFINED GALLERY_PLUGIN_FILE)
    _winui3style_check_styles_dir("gallery" "${GALLERY_PLUGIN_FILE}")
    file(SHA256 "${PLUGIN_FILE}" _plugin_hash)
    file(SHA256 "${GALLERY_PLUGIN_FILE}" _gallery_hash)
    if(NOT _plugin_hash STREQUAL _gallery_hash)
        message(FATAL_ERROR "Gallery plugin differs from the build output: ${GALLERY_PLUGIN_FILE}")
    endif()
endif()
