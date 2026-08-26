set(winui3style_public_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3style.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3backdrop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/winui3icons.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/toggleswitch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/settingscard.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/animatedstack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../include/winui3style/navigationview.h
)

add_library(winui3style SHARED
    ${winui3style_public_headers}
    winui3style.cpp
    winui3paint_p.h
    winui3paint_p.cpp
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

target_compile_definitions(winui3style PRIVATE WINUI3STYLE_LIBRARY)
target_include_directories(winui3style
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(winui3style PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets)

if(WIN32)
    target_link_libraries(winui3style PRIVATE dwmapi user32)
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
