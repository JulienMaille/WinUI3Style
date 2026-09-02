set(winui3style_public_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3style.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3backdrop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3icons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/toggleswitch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/settingscard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/animatedstack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/navigationview.h
)

option(WINUI3STYLE_STATIC
       "Build the style as a static library (self-contained plugin DLL)" OFF)
if(WINUI3STYLE_STATIC)
    set(WINUI3STYLE_LIBRARY_TYPE STATIC)
else()
    set(WINUI3STYLE_LIBRARY_TYPE SHARED)
endif()

add_library(winui3style ${WINUI3STYLE_LIBRARY_TYPE}
    ${winui3style_public_headers}
    winui3style.cpp
    winui3interactions_p.h
    winui3interactions_p.cpp
    winui3buttons_p.h
    winui3buttons_p.cpp
    winui3animations_p.h
    winui3animations_p.cpp
    winui3menus_p.h
    winui3menus_p.cpp
    winui3paint_p.h
    winui3paint_p.cpp
    winui3geometry_p.h
    winui3geometry_p.cpp
    winui3density_p.h
    winui3style_contracts_p.h
    winui3style_contracts_p.cpp
    winui3theme_p.h
    winui3theme_p.cpp
    winui3appearancewatcher_p.h
    winui3appearancewatcher_p.cpp
    winui3style_properties_p.h
    winui3tableeditors_p.h
    winui3tableeditors_p.cpp
    winui3frameproperties_p.h
    winui3frameproperties_p.cpp
    winui3helpers_p.h
    winui3surfaces_p.h
    winui3surfaces_p.cpp
    winui3complex_p.h
    winui3complex_p.cpp
    winui3viewrenderers_p.h
    winui3viewrenderers_p.cpp
    winui3tokens_p.h
    navigationview_p.h
    navigationview_p.cpp
    winui3backdrop_p.h
    winui3backdrop.cpp
    winui3icons.cpp
    toggleswitch.cpp
    settingscard.cpp
    animatedstack.cpp
    navigationview.cpp
)

add_library(WinUI3::Widgets ALIAS winui3style)

if(WINUI3STYLE_STATIC)
    target_compile_definitions(winui3style PUBLIC WINUI3STYLE_STATIC)
else()
    target_compile_definitions(winui3style PRIVATE WINUI3STYLE_LIBRARY)
endif()
target_include_directories(winui3style
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(winui3style PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)

if(WIN32)
    target_link_libraries(winui3style PRIVATE advapi32 dwmapi user32)
endif()

set_target_properties(winui3style PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    DEBUG_POSTFIX d
    EXPORT_NAME Widgets
)

include(GNUInstallDirs)
install(TARGETS winui3style EXPORT WinUI3StyleTargets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
