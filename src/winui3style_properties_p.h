#pragma once

namespace WinUI3::Private {

inline constexpr auto roleProperty = "_winui_control_role";
inline constexpr auto hoverProperty = "_winui_hover_progress";
inline constexpr auto pressProperty = "_winui_press_progress";
inline constexpr auto buttonPressGenerationProperty =
    "_winui_button_press_generation";
inline constexpr auto buttonPressReleasePendingProperty =
    "_winui_button_press_release_pending";
inline constexpr auto focusProperty = "_winui_focus_progress";
inline constexpr auto focusVisibleProperty = "_winui_focus_visible";
inline constexpr auto checkProperty = "_winui_check_progress";
inline constexpr auto tableEditorProperty = "_winui_table_editor";
inline constexpr auto togglePositionProperty = "_winui_toggle_position";
inline constexpr auto toggleDraggingProperty = "_winui_toggle_dragging";
inline constexpr auto scrollBarInsideProperty = "_winui_scrollbar_inside";
inline constexpr auto scrollBarGenerationProperty = "_winui_scrollbar_generation";
inline constexpr auto sliderToolTipVisibleProperty =
    "_winui_slider_tooltip_visible";
inline constexpr auto sliderToolTipValueProperty = "_winui_slider_tooltip_value";
inline constexpr auto progressPhaseProperty = "_winui_progress_phase";
inline constexpr auto comboChevronProperty = "_winui_combo_chevron_progress";
inline constexpr auto originalPaletteProperty = "_winui_original_palette";
inline constexpr auto originalPaletteExplicitProperty =
    "_winui_original_palette_explicit";
inline constexpr auto originalAutoFillProperty = "_winui_original_auto_fill";
inline constexpr auto originalHoverAttributeProperty =
    "_winui_original_hover_attribute";
inline constexpr auto originalMinimumSizeProperty =
    "_winui_original_minimum_size";
inline constexpr auto originalFrameShapeProperty = "_winui_original_frame_shape";
inline constexpr auto originalMarginsProperty = "_winui_original_layout_margins";
inline constexpr auto originalSpacingProperty = "_winui_original_layout_spacing";
inline constexpr auto originalRoleProperty = "_winui_original_control_role";
inline constexpr auto originalRoleWasValidProperty =
    "_winui_original_control_role_valid";
inline constexpr auto originalOpaquePaintProperty =
    "_winui_original_opaque_paint";
inline constexpr auto originalTranslucentBackgroundProperty =
    "_winui_original_translucent_background";
inline constexpr auto originalNoSystemBackgroundProperty =
    "_winui_original_no_system_background";
inline constexpr auto originalListSpacingProperty =
    "_winui_original_list_spacing";
inline constexpr auto ownedPaletteProperty = "_winui_theme_owned_palette";

} // namespace WinUI3::Private
