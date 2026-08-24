add_library(winui3styleplugin MODULE
    winui3styleplugin.cpp
    winui3styleplugin.h
    winui3style.json
)

target_link_libraries(winui3styleplugin PRIVATE WinUI3::Widgets Qt6::Widgets)
set_target_properties(winui3styleplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/styles"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/styles"
    DEBUG_POSTFIX d
)

add_custom_command(TARGET winui3styleplugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "${CMAKE_BINARY_DIR}/plugins/styles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:winui3styleplugin>
        "${CMAKE_BINARY_DIR}/plugins/styles"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:winui3style>
        $<TARGET_FILE_DIR:winui3styleplugin>
)

install(TARGETS winui3styleplugin
    RUNTIME DESTINATION plugins/styles
    LIBRARY DESTINATION plugins/styles
)
