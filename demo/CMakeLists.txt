add_executable(winui3style_gallery
    main.cpp
    gallerywindow.cpp
    gallerywindow.h
    gallerywindow.ui
)

target_link_libraries(winui3style_gallery PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(winui3style_gallery PRIVATE winui3style_warnings)
set_target_properties(winui3style_gallery PROPERTIES WIN32_EXECUTABLE TRUE)

add_custom_command(TARGET winui3style_gallery POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:winui3style>
        $<TARGET_FILE_DIR:winui3style_gallery>
)

if(TARGET winui3styleplugin)
    add_dependencies(winui3style_gallery winui3styleplugin)
    add_custom_command(TARGET winui3style_gallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:winui3style_gallery>/styles"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:winui3styleplugin>
            "$<TARGET_FILE_DIR:winui3style_gallery>/styles"
    )
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
    # Use the versioned command so QT_DEPLOY_* references remain unevaluated
    # until install time even when QTP0008 is not enabled by the parent project.
    qt6_generate_deploy_script(
        TARGET winui3style_gallery
        OUTPUT_SCRIPT winui3style_gallery_deploy_script
        CONTENT "
qt_deploy_runtime_dependencies(
    EXECUTABLE \"\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:winui3style_gallery>\"
    ADDITIONAL_MODULES \"\${QT_DEPLOY_PLUGINS_DIR}/styles/$<TARGET_FILE_NAME:winui3styleplugin>\"
    BIN_DIR bin
    LIB_DIR lib
    PLUGINS_DIR plugins
    GENERATE_QT_CONF
    INCLUDE_PLUGIN_TYPES platforms imageformats
    NO_TRANSLATIONS
    VERBOSE
)")
    install(SCRIPT ${winui3style_gallery_deploy_script} COMPONENT Runtime)
endif()
