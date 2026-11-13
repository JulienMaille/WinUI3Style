# Source-contract gate: same file(READ) + FATAL_ERROR idiom as check_designer_gallery.cmake.
#
# (a) QSS ban: fail on setStyleSheet anywhere under src/ (docs/METHODOLOGY.md
#     section 4 forbids QSS; .ui stylesheets are covered by the extended
#     check_designer_gallery.cmake).
# (b) Token centralization: fail on QColor( numeric literals in src/lib/*.cpp
#     outside the approved token homes winui3tokens_p.h, winui3paint_p.cpp,
#     winui3geometry_p.cpp, and winui3theme_p.cpp (palette/accent-ramp home).
# (c) License headers: every src/lib, src/include, src/plugin and
#     tests/benchmarks file must reference LGPL-2.1-or-later; the package includes LICENSE.
# (d) Test size ratchet: fail if any tests/tst_*.cpp exceeds 60 KB, so the
#     split test binaries cannot regrow into one giant file.
# (e) Qt 5.12 namespace metadata: Q_NAMESPACE_EXPORT only exists from Qt 5.14;
#     the public header must retain a guarded Q_NAMESPACE fallback.

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR must be passed on the command line")
endif()

# (a) QSS ban.
file(GLOB_RECURSE _qss_sources
    "${SOURCE_DIR}/src/*.cpp" "${SOURCE_DIR}/src/*.h" "${SOURCE_DIR}/src/*.ui")
foreach(_file IN LISTS _qss_sources)
    file(READ "${_file}" _contents)
    string(FIND "${_contents}" "setStyleSheet" _found)
    if(NOT _found EQUAL -1)
        message(FATAL_ERROR "QSS ban: setStyleSheet found in ${_file}")
    endif()
endforeach()

# (b) Token centralization: no QColor( numeric literals outside the approved
# token homes winui3tokens_p.h, winui3paint_p.cpp, winui3geometry_p.cpp,
# winui3theme_p.cpp. Former per-file grandfather list is fully retired.
file(GLOB _token_checked "${SOURCE_DIR}/src/lib/*.cpp")
foreach(_file IN LISTS _token_checked)
    get_filename_component(_name "${_file}" NAME)
    if(_name STREQUAL "winui3paint_p.cpp"
        OR _name STREQUAL "winui3geometry_p.cpp"
        OR _name STREQUAL "winui3theme_p.cpp"
        OR _name STREQUAL "winui3tokens_p.h")
        continue()
    endif()
    file(READ "${_file}" _contents)
    string(REGEX MATCH "QColor\\([0-9]" _hit "${_contents}")
    if(NOT _hit STREQUAL "")
        message(FATAL_ERROR
            "Token centralization: NEW QColor( numeric literal in ${_file}; "
            "move the value to winui3tokens_p.h")
    endif()
endforeach()

# (c) License headers.
file(GLOB _licensed
    "${SOURCE_DIR}/src/lib/*"
    "${SOURCE_DIR}/src/plugin/*"
    "${SOURCE_DIR}/tests/benchmarks/*")
file(GLOB_RECURSE _licensed_headers "${SOURCE_DIR}/src/include/*")
list(APPEND _licensed ${_licensed_headers})
foreach(_file IN LISTS _licensed)
    if(IS_DIRECTORY "${_file}")
        continue()
    endif()
    get_filename_component(_ext "${_file}" EXT)
    if(NOT _ext STREQUAL ".cpp" AND NOT _ext STREQUAL ".h")
        continue()
    endif()
    file(READ "${_file}" _contents)
    string(FIND "${_contents}" "LGPL-2.1-or-later" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR "License header: LGPL-2.1-or-later missing in ${_file}")
    endif()
endforeach()

# (d) Test size ratchet (60 KB per split binary).
file(GLOB _test_sources "${SOURCE_DIR}/tests/tst_*.cpp")
foreach(_file IN LISTS _test_sources)
    file(SIZE "${_file}" _size)
    if(_size GREATER 61440)
        message(FATAL_ERROR
            "Test size ratchet: ${_file} is ${_size} bytes (> 60 KB); "
            "split by domain instead of growing one file")
    endif()
endforeach()

# (e) Keep the Qt 5.12 AUTOMOC path parseable while exporting the namespace
# metadata where Qt provides Q_NAMESPACE_EXPORT.
file(READ "${SOURCE_DIR}/src/include/winui3style/winui3style.h" _style_header)
string(REGEX MATCH
    "#if QT_VERSION >= QT_VERSION_CHECK\\(5, 14, 0\\)[ \t\r\n]+Q_NAMESPACE_EXPORT\\(WINUI3STYLE_EXPORT\\)[ \t\r\n]+#else[ \t\r\n]+#[ \t]+ifdef Q_MOC_RUN[ \t\r\n]+Q_NAMESPACE[ \t\r\n]+#[ \t]+else[ \t\r\n]+extern WINUI3STYLE_EXPORT const QMetaObject staticMetaObject;[ \t\r\n]+#[ \t]+endif[ \t\r\n]+#endif"
    _qt512_namespace_guard "${_style_header}")
if(_qt512_namespace_guard STREQUAL "")
    message(FATAL_ERROR
        "Qt 5.12 compatibility: moc needs a guarded Q_NAMESPACE while C++ "
        "consumers need the exported WinUI3::staticMetaObject declaration")
endif()
