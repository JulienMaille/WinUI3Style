# The implementation is embedded via the shared object library so this MODULE
# is self-contained: a normal Qt plugin deployment needs only qwinui3style.dll
# (plus Qt's own runtime dependencies), not a separate winui3style.dll. Tests
# and internal targets can still link WinUI3::Widgets, which is built from the
# same object files (issue #70).
add_library(winui3styleplugin MODULE
    winui3styleplugin.cpp
    winui3styleplugin.h
    winui3style.json
    $<TARGET_OBJECTS:winui3style_impl>
)
add_dependencies(winui3styleplugin winui3style_impl)

# The plugin source includes the public headers; the embedded implementation
# objects need the same private include directory as the library build. Match
# the library's export macro so calls from the plugin into the embedded
# implementation are not emitted as dllimport references to this same DLL.
target_include_directories(winui3styleplugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)
if(WINUI3STYLE_STATIC)
    target_compile_definitions(winui3styleplugin PRIVATE WINUI3STYLE_STATIC)
else()
    target_compile_definitions(winui3styleplugin PRIVATE WINUI3STYLE_LIBRARY)
endif()

target_link_libraries(winui3styleplugin PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(winui3styleplugin PRIVATE winui3style_warnings)
if(WIN32)
    # Backing the registry/DWM probes used by the embedded implementation.
    target_link_libraries(winui3styleplugin PRIVATE advapi32 dwmapi user32)
endif()

set_target_properties(winui3styleplugin PROPERTIES
    OUTPUT_NAME qwinui3style
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/styles"
    LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/plugins/styles"
    LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/plugins/styles"
    LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/plugins/styles"
    LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/plugins/styles"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/plugins/styles"
    DEBUG_POSTFIX d
)

install(TARGETS winui3styleplugin
    RUNTIME DESTINATION plugins/styles COMPONENT Runtime
    LIBRARY DESTINATION plugins/styles COMPONENT Runtime
)
