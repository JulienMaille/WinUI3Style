add_executable(winui3style_gallery
    main.cpp
    gallerywindow.cpp
    gallerywindow.h
)

target_link_libraries(winui3style_gallery PRIVATE WinUI3::Widgets Qt6::Widgets)
set_target_properties(winui3style_gallery PROPERTIES WIN32_EXECUTABLE TRUE)

add_custom_command(TARGET winui3style_gallery POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:winui3style>
        $<TARGET_FILE_DIR:winui3style_gallery>
)

# The snapshot oracle deliberately uses Qt's offscreen platform. A previously
# deployed gallery directory can otherwise contain Qt runtime DLLs plus only
# qwindows.dll, causing Windows to show a modal plugin error before main().
if(WIN32)
    get_filename_component(_winui3style_qt_root
        "${Qt6_DIR}/../../.." ABSOLUTE)
    add_custom_command(TARGET winui3style_gallery POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:winui3style_gallery>/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_winui3style_qt_root}/plugins/platforms/qoffscreen.dll"
            "$<TARGET_FILE_DIR:winui3style_gallery>/platforms/qoffscreen.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${_winui3style_qt_root}/plugins/platforms/qoffscreend.dll"
            "$<TARGET_FILE_DIR:winui3style_gallery>/platforms/qoffscreend.dll"
    )
endif()
