add_executable(winui3style_gallery
    main.cpp
    gallerywindow.cpp
    gallerywindow.h
    gallerywindow.ui
)

target_link_libraries(winui3style_gallery PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Widgets)
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
