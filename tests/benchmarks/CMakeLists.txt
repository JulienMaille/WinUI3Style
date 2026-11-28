add_executable(winui3style_render_benchmark render_benchmark.cpp)
target_include_directories(winui3style_render_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src/lib)
target_link_libraries(winui3style_render_benchmark PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(winui3style_render_benchmark PRIVATE winui3style_warnings)

# Same rule as the test executables in tests/CMakeLists.txt: on Windows the
# executable directory is searched before PATH, so the perf budget step can
# launch this benchmark without the style DLL if it is not staged beside it
# (the test staging target writes to the tests output directory, not here).
if(WIN32 AND NOT WINUI3STYLE_STATIC AND TARGET winui3style)
    add_custom_command(TARGET winui3style_render_benchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:winui3style>"
            "$<TARGET_FILE_DIR:winui3style_render_benchmark>"
        VERBATIM
    )
endif()
