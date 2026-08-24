add_executable(winui3style_render_benchmark render_benchmark.cpp)
target_link_libraries(winui3style_render_benchmark PRIVATE
    WinUI3::Widgets Qt6::Widgets)
