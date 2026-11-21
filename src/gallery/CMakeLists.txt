if(WIN32)
    # rc.exe resolves ICON paths from the build directory, so generate the
    # resource script with an absolute path to the icon in the source tree.
    set(WINUI3_GALLERY_ICON "${CMAKE_CURRENT_SOURCE_DIR}/gallery.ico")
    configure_file(winui3style_gallery.rc.in winui3style_gallery.rc @ONLY)
    set(_winui3_gallery_rc "${CMAKE_CURRENT_BINARY_DIR}/winui3style_gallery.rc")
else()
    set(_winui3_gallery_rc "")
endif()

add_executable(winui3style_gallery
    main.cpp
    gallerywindow.cpp
    gallerywindow.h
    gallerywindow.ui
    ${_winui3_gallery_rc}
)

target_link_libraries(winui3style_gallery PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(winui3style_gallery PRIVATE winui3style_warnings)
set_target_properties(winui3style_gallery PROPERTIES WIN32_EXECUTABLE TRUE)

if(TARGET winui3styleplugin)
    # Stage the self-contained plugin on every Gallery build. The executable
    # does not relink when its dynamically loaded plugin changes.
    add_custom_target(winui3style_gallery_stage_plugin
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:winui3style_gallery>/styles"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:winui3styleplugin>
            "$<TARGET_FILE_DIR:winui3style_gallery>/styles"
        DEPENDS winui3styleplugin
        VERBATIM
    )
    add_dependencies(winui3style_gallery winui3style_gallery_stage_plugin)
endif()

# The snapshot oracle deliberately uses Qt's offscreen platform. A previously
# deployed gallery directory can otherwise contain Qt runtime DLLs plus only
# qwindows.dll, causing Windows to show a modal plugin error before main().
if(WIN32)
    get_filename_component(_winui3style_qt_root
        "${${WINUI3STYLE_QT}_DIR}/../../.." ABSOLUTE)
    add_custom_command(TARGET winui3style_gallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:winui3style_gallery>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_winui3style_qt_root}/plugins/platforms/qoffscreen.dll"
            "$<TARGET_FILE_DIR:winui3style_gallery>/platforms/qoffscreen.dll"
    )
    if(EXISTS "${_winui3style_qt_root}/plugins/platforms/qoffscreend.dll")
        add_custom_command(TARGET winui3style_gallery POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_winui3style_qt_root}/plugins/platforms/qoffscreend.dll"
                "$<TARGET_FILE_DIR:winui3style_gallery>/platforms/qoffscreend.dll"
        )
    endif()
endif()

# Linux release bundles use Qt's supported CMake deployment API. Install the
# Gallery alongside the library/plugin targets above and have the generated
# install script collect Qt runtime dependencies and platform plugins. The
# additional-module path is essential: QStyleFactory loads our style plugin
# dynamically, so it is not visible from the Gallery executable's link graph.
if(UNIX AND NOT APPLE)
    if(NOT WINUI3STYLE_QT STREQUAL "Qt6")
        message(FATAL_ERROR "The Linux Gallery deployment requires Qt 6")
    endif()
    if(NOT TARGET winui3styleplugin)
        message(FATAL_ERROR "The Linux Gallery deployment requires winui3styleplugin")
    endif()
    include(GNUInstallDirs)
    set_target_properties(winui3style_gallery PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    set_target_properties(winui3style PROPERTIES INSTALL_RPATH "$ORIGIN")
    set_target_properties(winui3styleplugin PROPERTIES
        INSTALL_RPATH "$ORIGIN/../../${CMAKE_INSTALL_LIBDIR}")

    # Install the target (rather than copying its file as a PROGRAM) so CMake
    # rewrites its build-tree RPATH to the package-relative INSTALL_RPATH.
    install(TARGETS winui3style_gallery
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT Runtime)
    # Qt 6.10 added plugin-type filtering. Earlier versions need explicit
    # plugin modules and must scan the build binaries before RPATH is replaced.
    set(_winui3style_deploy_executable "\"\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:winui3style_gallery>\"")
    set(_winui3style_deploy_modules "\"\${QT_DEPLOY_PLUGINS_DIR}/styles/$<TARGET_FILE_NAME:winui3styleplugin>\"")
    set(_winui3style_deploy_plugin_types "INCLUDE_PLUGIN_TYPES platforms imageformats")
    if(WINUI3STYLE_QT_VERSION VERSION_LESS "6.10")
        set(_winui3style_deploy_executable "\"$<TARGET_FILE:winui3style_gallery>\"")
        set(_winui3style_deploy_modules "\"$<TARGET_FILE:winui3styleplugin>\"")
        set(_winui3style_deploy_plugin_types "")
        foreach(_plugin IN ITEMS QXcbIntegrationPlugin QOffscreenIntegrationPlugin
                QWaylandIntegrationPlugin QWaylandEglPlatformIntegrationPlugin
                QGifPlugin QICOPlugin QJpegPlugin)
            if(TARGET Qt6::${_plugin})
                get_target_property(_type Qt6::${_plugin} QT_PLUGIN_TYPE)
                install(FILES "$<TARGET_FILE:Qt6::${_plugin}>" DESTINATION "plugins/${_type}"
                    COMPONENT Runtime)
                string(APPEND _winui3style_deploy_modules " \"$<TARGET_FILE:Qt6::${_plugin}>\"")
            endif()
        endforeach()
        if(TARGET Qt6::QWaylandIntegrationPlugin)
            install(FILES "$<TARGET_FILE_DIR:Qt6::QWaylandIntegrationPlugin>/../wayland-shell-integration/libxdg-shell.so"
                DESTINATION plugins/wayland-shell-integration COMPONENT Runtime OPTIONAL)
        endif()
    endif()
    # Use the versioned command so QT_DEPLOY_* references remain unevaluated
    # until install time even when QTP0008 is not enabled by the parent project.
    qt6_generate_deploy_script(
        TARGET winui3style_gallery
        OUTPUT_SCRIPT winui3style_gallery_deploy_script
        CONTENT "
qt_deploy_runtime_dependencies(
    EXECUTABLE ${_winui3style_deploy_executable}
    ADDITIONAL_MODULES ${_winui3style_deploy_modules}
    BIN_DIR bin
    LIB_DIR lib
    PLUGINS_DIR plugins
    GENERATE_QT_CONF
    ${_winui3style_deploy_plugin_types}
    NO_TRANSLATIONS
    VERBOSE
)")
    install(SCRIPT ${winui3style_gallery_deploy_script} COMPONENT Runtime)
endif()
