set(_demo_dir "${SOURCE_DIR}/demo")
if(NOT EXISTS "${_demo_dir}/gallerywindow.ui")
    message(FATAL_ERROR "The gallery must be defined by a Qt Designer .ui file")
endif()

file(READ "${_demo_dir}/gallerywindow.cpp" _gallery_cpp)
file(READ "${_demo_dir}/main.cpp" _main_cpp)
file(READ "${_demo_dir}/gallerywindow.ui" _gallery_ui)

# Capture-only Palette Lab fixture: use an isolated factory-created dark style
# to sample the swatches; never change the app's active style/theme/palette.
string(REGEX MATCH
    "QPalette capturePalette = ui->palettePreview->palette\\(\\);[ \t\r\n]*if \\(captureMode\\) \\{[^}]*QStyleFactory::create\\(QStringLiteral\\(\"winui3\"\\)\\)[^}]*captureStyle->setProperty\\(\"themeMode\", 2\\);[^}]*captureStyle->setProperty\\(\"accentColor\", QColor\\(0, 120, 212\\)\\);[^}]*capturePalette = captureStyle->standardPalette\\(\\);[^}]*\\}"
    _capture_palette_fixture "${_gallery_cpp}")
string(FIND "${_main_cpp}" "GalleryWindow window(nullptr, captureMode);" _capture_flag)
string(FIND "${_main_cpp}" "application.style()->setProperty" _global_style_mutation)
if(_capture_palette_fixture STREQUAL "" OR _capture_flag EQUAL -1
    OR NOT _global_style_mutation EQUAL -1)
    message(FATAL_ERROR
        "Capture mode must seed Palette Lab using an isolated dark style and pass captureMode to GalleryWindow")
endif()

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

# Settings cards are stock QFrame + winuiSettingsCard compositions: the style
# owns the surface, the chevron and the expansion, so the gallery stays a plain
# Qt client that only deploys the style plugin.
foreach(_required_card
        "<widget class=\"QFrame\" name=\"notificationsCard\">"
        "<widget class=\"QFrame\" name=\"updatesCard\">"
        "<widget class=\"QFrame\" name=\"advancedCard\">")
    string(FIND "${_gallery_ui}" "${_required_card}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR "Designer gallery is missing stock settings card: ${_required_card}")
    endif()
endforeach()
string(REGEX MATCHALL "winuiSettingsCard\" stdset=\"0\"" _card_properties "${_gallery_ui}")
list(LENGTH _card_properties _card_property_count)
if(NOT _card_property_count EQUAL 3)
    message(FATAL_ERROR
        "Every settings card must declare winuiSettingsCard (found ${_card_property_count}/3)")
endif()
string(FIND "${_gallery_ui}" "<widget class=\"QWidget\" name=\"advancedDetailsHost\">" _expansion_host)
string(FIND "${_gallery_ui}" "winuiSettingsCardExpansion\" stdset=\"0\"" _expansion_property)
if(_expansion_host EQUAL -1 OR _expansion_property EQUAL -1)
    message(FATAL_ERROR
        "The Advanced card must mark its collapsible child with winuiSettingsCardExpansion")
endif()
foreach(_forbidden_card
        "WinUI3::"
        "winui3style/"
        "<customwidgets>")
    string(FIND "${_gallery_ui}" "${_forbidden_card}" _found)
    if(NOT _found EQUAL -1)
        message(FATAL_ERROR
            "The gallery must not depend on a compiled WinUI3 widget: ${_forbidden_card}")
    endif()
endforeach()

# The density comparison must exercise selection indicators in both inherited
# modes, not only editor heights.
foreach(_required_density_control
        "name=\"standardDensityCheck\""
        "name=\"standardDensityRadio\""
        "name=\"compactDensityCheck\""
        "name=\"compactDensityRadio\"")
    string(FIND "${_gallery_ui}" "${_required_density_control}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "Designer gallery is missing density selection coverage: ${_required_density_control}")
    endif()
endforeach()

foreach(_required_completer_setting
        "setCaseSensitivity(Qt::CaseInsensitive)"
        "setFilterMode(Qt::MatchContains)"
        "setCompletionMode(QCompleter::PopupCompletion)")
    string(FIND "${_gallery_cpp}" "${_required_completer_setting}" _found)
    if(_found EQUAL -1)
        message(FATAL_ERROR
            "Gallery AutoSuggestBox is missing: ${_required_completer_setting}")
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
if(NOT _linked EQUAL -1)
    message(FATAL_ERROR
        "The gallery must not link WinUI3::Widgets: the settings cards are stock Qt widgets")
endif()

# Zero-QSS claim loop: Designer-set stylesheets bypass the setStyleSheet
# source grep, so forbid stylesheet properties in the .ui itself.
string(FIND "${_gallery_ui}" "styleSheet" _ui_stylesheet)
if(NOT _ui_stylesheet EQUAL -1)
    message(FATAL_ERROR "Designer gallery must not set styleSheet in gallerywindow.ui")
endif()
