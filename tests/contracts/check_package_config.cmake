# A Qt5 package must not require Qt6 from its consumers.

if(NOT DEFINED SOURCE_DIR)
    message(FATAL_ERROR "SOURCE_DIR must be passed on the command line")
endif()

file(READ "${SOURCE_DIR}/src/lib/WinUI3StyleConfig.cmake.in" _template)

string(FIND "${_template}" "find_dependency(Qt6" _hardcoded6)
string(FIND "${_template}" "find_dependency(Qt5" _hardcoded5)
if(NOT _hardcoded6 EQUAL -1 OR NOT _hardcoded5 EQUAL -1)
    message(FATAL_ERROR
        "Package config: find_dependency hardcodes a Qt major; use "
        "@WINUI3STYLE_QT@ @WINUI3STYLE_QT_MIN_VERSION@ so the shipped dependency "
        "tracks the Qt the library was built against")
endif()

# string(CONFIGURE @ONLY) is the same @VAR@ substitution
# configure_package_config_file applies, exercised in-memory for each Qt line.
foreach(_qt Qt5 Qt6)
    if(_qt STREQUAL "Qt5")
        set(WINUI3STYLE_QT Qt5)
        set(WINUI3STYLE_QT_MIN_VERSION 5.12)
    else()
        set(WINUI3STYLE_QT Qt6)
        set(WINUI3STYLE_QT_MIN_VERSION 6.5)
    endif()
    string(CONFIGURE "${_template}" _generated @ONLY)
    string(FIND "${_generated}"
        "find_dependency(${_qt} ${WINUI3STYLE_QT_MIN_VERSION} COMPONENTS Core Gui Widgets)"
        _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "Package config: ${_qt} build did not yield "
            "find_dependency(${_qt} ${WINUI3STYLE_QT_MIN_VERSION} COMPONENTS Core Gui Widgets)")
    endif()
endforeach()