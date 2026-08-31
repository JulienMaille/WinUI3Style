set(_demo_dir "${SOURCE_DIR}/demo")
if(NOT EXISTS "${_demo_dir}/gallerywindow.ui")
    message(FATAL_ERROR "The gallery must be defined by a Qt Designer .ui file")
endif()

file(READ "${_demo_dir}/gallerywindow.cpp" _gallery_cpp)
file(READ "${_demo_dir}/main.cpp" _main_cpp)
file(READ "${_demo_dir}/gallerywindow.ui" _gallery_ui)
foreach(_forbidden "winui3style/" "WinUI3::")
    string(FIND "${_gallery_cpp}${_main_cpp}" "${_forbidden}" _found)
    if(NOT _found EQUAL -1)
        message(FATAL_ERROR
            "The gallery is a plain Qt client; forbidden dependency: ${_forbidden}")
    endif()
endforeach()

string(REGEX MATCHALL "setMinimumWidth\\(120\\)" _dialog_button_widths
       "${_gallery_cpp}")
list(LENGTH _dialog_button_widths _dialog_button_width_count)
if(_dialog_button_width_count LESS 2)
    message(FATAL_ERROR
        "Designer gallery ContentDialog must retain 120 px primary/close buttons")
endif()

# Settings cards are real promoted widgets, not QFrame/QGroupBox lookalikes.
# Their internal header/chevron/expansion hit testing is part of the widget
# contract and cannot be reproduced by a dynamic winuiSettingsCard property.
foreach(_required
        "<class>WinUI3::SettingsCard</class>"
        "<widget class=\"WinUI3::SettingsCard\" name=\"notificationsCard\">"
        "<widget class=\"WinUI3::SettingsCard\" name=\"updatesCard\">"
        "<widget class=\"WinUI3::SettingsCard\" name=\"advancedCard\">"
        "<property name=\"trailingWidgetName\">"
        "<property name=\"expandableWidgetName\">")
    string(FIND "${_gallery_ui}" "${_required}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR "Designer gallery is missing promoted SettingsCard contract: ${_required}")
    endif()
endforeach()
foreach(_forbidden_card
        "<widget class=\"QFrame\" name=\"notificationsCard\">"
        "<widget class=\"QFrame\" name=\"updatesCard\">"
        "<widget class=\"QGroupBox\" name=\"advancedCard\">")
    string(FIND "${_gallery_ui}" "${_forbidden_card}" _found)
    if(NOT _found EQUAL -1)
        message(FATAL_ERROR "Designer gallery regressed to a placeholder settings card: ${_forbidden_card}")
    endif()
endforeach()

# Keep the menu hierarchy in the form so submenu glyph/spacing regressions are
# exercised by the gallery without imperative construction code.
foreach(_required_menu
        "<widget class=\"QMenu\" name=\"exportMenu\">"
        "<addaction name=\"exportMenu\"/>"
        "<action name=\"exportDocumentAction\">"
        "<action name=\"exportImageAction\">")
    string(FIND "${_gallery_ui}" "${_required_menu}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR "Designer gallery is missing submenu coverage: ${_required_menu}")
    endif()
endforeach()

foreach(_required_dock
        "<widget class=\"QMainWindow\" name=\"dockHost\">"
        "<widget class=\"QDockWidget\" name=\"embeddedInspector\">"
        "<attribute name=\"dockWidgetArea\"><number>2</number></attribute>"
        "<widget class=\"QWidget\" name=\"inspectorBody\">")
    string(FIND "${_gallery_ui}" "${_required_dock}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR "Designer gallery is missing dock coverage: ${_required_dock}")
    endif()
endforeach()

file(READ "${_demo_dir}/CMakeLists.txt" _demo_cmake)
string(FIND "${_demo_cmake}" "WinUI3::Widgets" _linked)
if(_linked EQUAL -1)
    message(FATAL_ERROR
        "The gallery must link WinUI3::Widgets because the .ui promotes SettingsCard")
endif()
