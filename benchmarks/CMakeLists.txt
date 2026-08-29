add_executable(winui3style_render_benchmark render_benchmark.cpp)
target_include_directories(winui3style_render_benchmark PRIVATE
    ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(winui3style_render_benchmark PRIVATE
    WinUI3::Widgets Qt${QT_VERSION_MAJOR}::Widgets)
