#include <winui3style/winui3style.h>

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3icons.h>

#include "winui3tokens_p.h"

#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QCheckBox>
#include <QCommonStyle>
#include <QDateTime>
#include <QDialog>
#include <QDockWidget>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QGroupBox>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLinearGradient>
#include <QListView>
#include <QLineF>
#include <QLineEdit>
#include <QItemSelectionModel>
#include <QKeyEvent>
#include <QLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QParallelAnimationGroup>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollBar>
#include <QScreen>
#include <QSettings>
#include <QSlider>
#include <QStyleOption>
#include <QStyleOptionButton>
#include <QStyleOptionDockWidget>
#include <QStyleOptionFocusRect>
#include <QStyleOptionGroupBox>
#include <QStyleOptionProgressBar>
#include <QStyleOptionSlider>
#include <QStyleOptionTab>
#include <QStyleOptionToolButton>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QToolBar>
#include <QToolButton>
#include <QTimer>
#include <QTreeView>
#include <QVariantAnimation>
#include <QWidget>

#include <cmath>

#ifdef Q_OS_WIN
#  define NOMINMAX
#  include <windows.h>
#  include <dwmapi.h>
#endif

namespace WinUI3 {
namespace {

constexpr auto roleProperty = "_winui_control_role";
constexpr auto hoverProperty = "_winui_hover_progress";
constexpr auto pressProperty = "_winui_press_progress";
constexpr auto buttonPressGenerationProperty = "_winui_button_press_generation";
constexpr auto buttonPressReleasePendingProperty = "_winui_button_press_release_pending";
constexpr auto focusProperty = "_winui_focus_progress";
constexpr auto focusVisibleProperty = "_winui_focus_visible";
constexpr auto checkProperty = "_winui_check_progress";
constexpr auto togglePositionProperty = "_winui_toggle_position";
constexpr auto navigationIndicatorProperty = "_winui_navigation_indicator_y";
constexpr auto navigationDelegateProperty = "_winui_navigation_delegate";
constexpr auto navigationOriginalDelegateProperty = "_winui_navigation_original_delegate";
constexpr auto navigationStateProperty = "_winui_navigation_state";
constexpr auto scrollBarInsideProperty = "_winui_scrollbar_inside";
constexpr auto scrollBarGenerationProperty = "_winui_scrollbar_generation";
constexpr auto sliderToolTipVisibleProperty = "_winui_slider_tooltip_visible";
constexpr auto sliderToolTipValueProperty = "_winui_slider_tooltip_value";
constexpr auto progressPhaseProperty = "_winui_progress_phase";
constexpr auto comboChevronProperty = "_winui_combo_chevron_progress";
constexpr auto originalPaletteProperty = "_winui_original_palette";
constexpr auto originalPaletteExplicitProperty = "_winui_original_palette_explicit";
constexpr auto originalAutoFillProperty = "_winui_original_auto_fill";
constexpr auto originalHoverAttributeProperty = "_winui_original_hover_attribute";
constexpr auto originalMinimumSizeProperty = "_winui_original_minimum_size";
constexpr auto originalFrameShapeProperty = "_winui_original_frame_shape";
constexpr auto originalMarginsProperty = "_winui_original_layout_margins";
constexpr auto originalSpacingProperty = "_winui_original_layout_spacing";
constexpr auto originalRoleProperty = "_winui_original_control_role";
constexpr auto originalRoleWasValidProperty = "_winui_original_control_role_valid";
constexpr auto originalMouseTrackingProperty = "_winui_original_mouse_tracking";
constexpr auto originalOpaquePaintProperty = "_winui_original_opaque_paint";
constexpr auto originalListSpacingProperty = "_winui_original_list_spacing";
constexpr auto ownedPaletteProperty = "_winui_theme_owned_palette";

void paintThemedIcon(QPainter *painter, const QIcon &source, const QRectF &rect,
                     Qt::Alignment alignment, const QColor &foreground,
                     QIcon::Mode mode = QIcon::Normal,
                     QIcon::State state = QIcon::Off);
void remember(QWidget *widget, const char *property, const QVariant &value)
{
    if (widget && !widget->property(property).isValid())
        widget->setProperty(property, value);
}

void rememberPalette(QWidget *widget)
{
    if (!widget)
        return;
    remember(widget, originalPaletteExplicitProperty,
             widget->testAttribute(Qt::WA_SetPalette));
    remember(widget, originalPaletteProperty,
             QVariant::fromValue(widget->palette()));
}

void restoreRememberedPalette(QWidget *widget)
{
    if (!widget || !widget->property(originalPaletteProperty).isValid())
        return;
    if (widget->property(originalPaletteExplicitProperty).toBool())
        widget->setPalette(widget->property(originalPaletteProperty).value<QPalette>());
    else
        widget->setPalette(QPalette());
}

QPalette effectivePopupPalette(QWidget *widget, const QPalette &fallback)
{
    if (!widget || !widget->property(originalPaletteExplicitProperty).toBool())
        return fallback;
    return widget->property(originalPaletteProperty).value<QPalette>().resolve(fallback);
}

void stopDialogAnimations(QDialog *dialog)
{
    if (!dialog)
        return;
    const auto groups = dialog->findChildren<QParallelAnimationGroup *>(
        QStringLiteral("_winui_dialog_animation"), Qt::FindDirectChildrenOnly);
    for (QParallelAnimationGroup *group : groups) {
        group->stop();
        delete group;
    }
    dialog->setWindowOpacity(1.0);
    dialog->setProperty("_winui_dialog_animating", false);
}

void restoreContentDialogState(QDialog *dialog, bool clearSavedState)
{
    if (!dialog)
        return;
    stopDialogAnimations(dialog);
    restoreRememberedPalette(dialog);
    if (dialog->property(originalAutoFillProperty).isValid())
        dialog->setAutoFillBackground(
            dialog->property(originalAutoFillProperty).toBool());
    if (dialog->property(originalMinimumSizeProperty).isValid())
        dialog->setMinimumSize(
            dialog->property(originalMinimumSizeProperty).value<QSize>());
    if (QLayout *layout = dialog->layout()) {
        if (dialog->property(originalMarginsProperty).isValid())
            layout->setContentsMargins(
                dialog->property(originalMarginsProperty).value<QMargins>());
        if (dialog->property(originalSpacingProperty).isValid())
            layout->setSpacing(dialog->property(originalSpacingProperty).toInt());
    }
    dialog->setProperty(ownedPaletteProperty, {});
    if (clearSavedState) {
        dialog->setProperty(originalPaletteProperty, {});
        dialog->setProperty(originalPaletteExplicitProperty, {});
        dialog->setProperty(originalAutoFillProperty, {});
        dialog->setProperty(originalMinimumSizeProperty, {});
        dialog->setProperty(originalMarginsProperty, {});
        dialog->setProperty(originalSpacingProperty, {});
    }
}

void prepareContentDialogState(QDialog *dialog, bool dark)
{
    if (!dialog)
        return;
    rememberPalette(dialog);
    remember(dialog, originalAutoFillProperty, dialog->autoFillBackground());
    remember(dialog, originalMinimumSizeProperty,
             QVariant::fromValue(dialog->minimumSize()));
    if (QLayout *layout = dialog->layout()) {
        remember(dialog, originalMarginsProperty,
                 QVariant::fromValue(layout->contentsMargins()));
        remember(dialog, originalSpacingProperty, layout->spacing());
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);
    }
    dialog->setProperty(ownedPaletteProperty, true);
    QPalette palette = dialog->palette();
    palette.setColor(QPalette::Window,
                     dark ? QColor(32, 32, 32) : QColor(255, 255, 255));
    dialog->setPalette(palette);
    dialog->setAutoFillBackground(true);
    dialog->setMinimumSize(320, 184);
}

bool toggleSwitch(const QWidget *widget)
{
    return qobject_cast<const QCheckBox *>(widget)
        && widget->property(Style::ToggleSwitchProperty).toBool();
}

bool verticalSpinButtons(const QWidget *widget)
{
    return qobject_cast<const QAbstractSpinBox *>(widget)
        && widget->property(Style::VerticalSpinButtonsProperty).toBool();
}

bool spinBoxEditor(const QWidget *widget)
{
    return qobject_cast<const QLineEdit *>(widget)
        && qobject_cast<const QAbstractSpinBox *>(widget->parentWidget());
}

bool textBoxHelperButton(const QWidget *widget)
{
    return qobject_cast<const QAbstractButton *>(widget)
        && qobject_cast<const QLineEdit *>(widget->parentWidget());
}

bool buttonPressPulse(const QWidget *widget)
{
    return !textBoxHelperButton(widget)
        && (qobject_cast<const QPushButton *>(widget)
            || qobject_cast<const QToolButton *>(widget));
}

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

constexpr int itemSelectionGutter = 12;
constexpr int itemSelectionMarkerWidth = 3;
constexpr int itemSelectionMarkerInset = 2;

const QAbstractItemView *selectionMarkerView(const QWidget *widget)
{
    const QAbstractItemView *view = itemView(widget);
    if (!view || (view->window() && view->window()->windowType() == Qt::Popup))
        return nullptr;
    if (qobject_cast<const QTableView *>(view))
        return nullptr;
    if (!qobject_cast<const QListView *>(view)
        && !qobject_cast<const QTreeView *>(view))
        return nullptr;
    return view;
}

int treeItemIndent(const QStyleOptionViewItem &option,
                   const QAbstractItemView *view)
{
    const auto *tree = qobject_cast<const QTreeView *>(view);
    if (!tree || !option.index.isValid() || option.index.column() != 0)
        return 0;

    int depth = 0;
    for (QModelIndex parent = option.index.parent(); parent.isValid();
         parent = parent.parent()) {
        ++depth;
    }
    // QTreeView reserves one indentation level for top-level items while
    // rootIsDecorated is enabled. The branch area is not part of the item
    // delegate's option.rect, so content must explicitly start after it.
    if (tree->rootIsDecorated())
        ++depth;
    return depth * tree->indentation();
}

QRect itemSelectionGutterRect(const QStyleOptionViewItem &option,
                              const QAbstractItemView *view)
{
    const int indent = treeItemIndent(option, view);
    if (option.direction == Qt::RightToLeft) {
        const int right = qMin(option.rect.right(), option.rect.right() - indent);
        const int left = qMax(option.rect.left(), right - itemSelectionGutter + 1);
        return QRect(left, option.rect.top(), qMax(0, right - left + 1),
                     option.rect.height());
    }
    const int left = qMin(option.rect.right() + 1,
                          option.rect.left() + indent);
    const int right = qMin(option.rect.right(), left + itemSelectionGutter - 1);
    return QRect(left, option.rect.top(), qMax(0, right - left + 1),
                 option.rect.height());
}

QRect selectionMarkerRect(const QStyleOptionViewItem &option,
                          const QAbstractItemView *view)
{
    const QRect gutter = itemSelectionGutterRect(option, view);
    if (gutter.isEmpty())
        return {};
    const int y = option.rect.center().y() - 8;
    if (option.direction == Qt::RightToLeft) {
        return QRect(gutter.right() - itemSelectionMarkerInset
                         - itemSelectionMarkerWidth + 1,
                     y, itemSelectionMarkerWidth, 16);
    }
    return QRect(gutter.left() + itemSelectionMarkerInset, y,
                 itemSelectionMarkerWidth, 16);
}

QRect headerSortIndicatorRect(const QStyleOptionHeader &header)
{
    // Align the complete glyph slot once. In particular, do not derive the
    // y coordinate from a half-pixel QRect center: odd header heights and
    // fractional DPRs otherwise move the glyph by one physical pixel.
    const QRect slot = header.rect.adjusted(10, 0, -10, 0);
    return QStyle::alignedRect(header.direction,
                               Qt::AlignRight | Qt::AlignVCenter,
                               QSize(16, 16), slot);
}

QRectF snappedSplitterGrip(const QRectF &grip, bool horizontal,
                           const QPainter *painter)
{
    if (!painter || !painter->device())
        return grip;

    const QTransform device = painter->deviceTransform();
    const qreal scale = horizontal ? qAbs(device.m11()) : qAbs(device.m22());
    if (scale <= 0.0 || !std::isfinite(scale))
        return grip;

    const qreal origin = horizontal ? device.dx() : device.dy();
    const qreal center = horizontal ? grip.center().x() : grip.center().y();
    const qreal half = (horizontal ? grip.width() : grip.height()) * 0.5;
    const qreal physicalStart = origin + (center - half) * scale;
    const qreal physicalEnd = origin + (center + half) * scale;
    const qreal snappedStart = qRound(physicalStart);
    const qreal snappedEnd = qRound(physicalEnd);
    const qreal snappedCenter = (snappedStart + snappedEnd) * 0.5;
    const qreal physicalOffset = snappedCenter - (origin + center * scale);

    // Only translate the painted grip when its physical edges prove that the
    // rasterizer would otherwise split the grip asymmetrically. The splitter
    // handle geometry, pane sizes and hit testing remain untouched.
    if (qAbs(physicalOffset) < 0.01)
        return grip;
    const qreal logicalOffset = physicalOffset / scale;
    QRectF result = grip;
    if (horizontal)
        result.translate(logicalOffset, 0.0);
    else
        result.translate(0.0, logicalOffset);
    return result;
}

const QWidget *richTextEditor(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate;
         candidate = candidate->parentWidget()) {
        if (qobject_cast<const QTextEdit *>(candidate)
            || qobject_cast<const QPlainTextEdit *>(candidate)) {
            return candidate;
        }
    }
    return nullptr;
}

enum class InteractionMotion {
    Hover,
    Press,
    Focus
};

int interactionDuration(const QWidget *widget, InteractionMotion motion,
                        bool active)
{
    // TextBox and NumberBox switch their common visual states through setters;
    // their templates define no timed transition. The TextBox helper button is
    // discrete as well. Other button-like surfaces use WinUI's faster brush
    // transition, while RadioButton's dot uses the normal duration.
    if (qobject_cast<const QLineEdit *>(widget)
        || qobject_cast<const QAbstractSpinBox *>(widget)
        || qobject_cast<const QTabBar *>(widget)
        || textBoxHelperButton(widget)) {
        return 0;
    }
    if (qobject_cast<const QRadioButton *>(widget))
        return Private::NormalDuration;
    if (qobject_cast<const QSlider *>(widget)) {
        if (motion == InteractionMotion::Focus)
            return 0;
        return active ? Private::NormalDuration : Private::FastDuration;
    }
    if (qobject_cast<const QScrollBar *>(widget))
        return Private::FastDuration;
    return Private::FasterDuration;
}

qreal progress(const QWidget *widget, const char *name, qreal fallback = 0.0);
const QEasingCurve &fluentCurve();
void roundedRect(QPainter *painter, const QRectF &rect, const QColor &fill,
                 const QColor &stroke, qreal radius, qreal strokeWidth = 1.0);
bool keyboardFocusVisible(const QWidget *widget);
bool animationsAllowed();

class SliderValueTip final : public QWidget
{
public:
    explicit SliderValueTip(QSlider *slider)
        : QWidget(slider, Qt::Tool | Qt::FramelessWindowHint
                            | Qt::WindowDoesNotAcceptFocus
                            | Qt::WindowTransparentForInput)
    {
        setObjectName(QStringLiteral("_winui_slider_value_tip"));
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
    }

    void showValue(const QString &text, const QPoint &anchor, bool horizontal)
    {
        m_text = text;
        const QFontMetrics metrics(font());
        resize(qMax(32, metrics.horizontalAdvance(text) + 16), 32);
        QPoint position = horizontal
            ? QPoint(anchor.x() - width() / 2, anchor.y() - height())
            : QPoint(anchor.x(), anchor.y() - height() / 2);
        if (QScreen *screen = QGuiApplication::screenAt(anchor)) {
            const QRect available = screen->availableGeometry();
            position.setX(qBound(available.left(), position.x(),
                                 available.right() - width() + 1));
            position.setY(qBound(available.top(), position.y(),
                                 available.bottom() - height() + 1));
        }
        move(position);
        WinUI3::applyBackdrop(this, WinUI3::Backdrop::Acrylic);
        show();
        raise();
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const Private::Tokens t = Private::tokens(palette());
        QColor fill = palette().color(QPalette::ToolTipBase);
        fill.setAlpha(242);
        QPainter painter(this);
        roundedRect(&painter, QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5),
                    fill, t.strokeSecondary, Private::ControlRadius);
        painter.setPen(palette().color(QPalette::ToolTipText));
        painter.drawText(rect().adjusted(8, 0, -8, 0),
                         Qt::AlignCenter, m_text);
    }

private:
    QString m_text;
};

bool isLineEditClearButton(const QLineEdit *lineEdit,
                           const QAbstractButton *button)
{
    if (!lineEdit || !button)
        return false;
    for (QAction *action : lineEdit->actions())
        if (action->associatedObjects().contains(button))
            return false;
    // QLineEdit's private clear affordance is deliberately not a QAction;
    // custom leading/trailing actions are associated above.
    return true;
}

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit)
{
    if (!lineEdit)
        return;
    const QPointer<QLineEdit> guarded(lineEdit);
    QTimer::singleShot(0, lineEdit, [guarded] {
        if (!guarded)
            return;
        for (QAbstractButton *button : guarded->findChildren<QAbstractButton *>())
            if (isLineEditClearButton(guarded, button))
                button->setVisible(!guarded->isReadOnly()
                                   && guarded->isEnabled()
                                   && guarded->isClearButtonEnabled()
                                   && !guarded->text().isEmpty());
    });
}

void showSliderValueToolTip(QSlider *slider)
{
    if (!slider || !slider->isEnabled())
        return;
    QStyleOptionSlider option;
    option.initFrom(slider);
    option.orientation = slider->orientation();
    option.minimum = slider->minimum();
    option.maximum = slider->maximum();
    option.sliderPosition = slider->sliderPosition();
    option.sliderValue = slider->value();
    option.singleStep = slider->singleStep();
    option.pageStep = slider->pageStep();
    option.upsideDown = slider->orientation() == Qt::Horizontal
        ? (slider->invertedAppearance()
           != (slider->layoutDirection() == Qt::RightToLeft))
        : !slider->invertedAppearance();
    const QRect handle = slider->style()->subControlRect(
        QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, slider);
    const QPoint anchor = slider->orientation() == Qt::Horizontal
        ? QPoint(handle.center().x(), handle.top() - 8)
        : QPoint(handle.right() + 8, handle.center().y());
    const QString valueText = QString::number(slider->value());
    slider->setProperty(sliderToolTipVisibleProperty, true);
    slider->setProperty(sliderToolTipValueProperty, valueText);
    // The headless platform cannot host a non-activating tool window and
    // synthesizes a FocusOut when one is shown. State/value remain testable;
    // the popup itself is covered by the native Windows test path.
    if (QGuiApplication::platformName() == QStringLiteral("offscreen"))
        return;
    auto *tip = static_cast<SliderValueTip *>(slider->findChild<QWidget *>(
        QStringLiteral("_winui_slider_value_tip"), Qt::FindDirectChildrenOnly));
    if (!tip)
        tip = new SliderValueTip(slider);
    tip->showValue(valueText, slider->mapToGlobal(anchor),
                   slider->orientation() == Qt::Horizontal);
}

void hideSliderValueToolTip(QSlider *slider)
{
    if (slider) {
        slider->setProperty(sliderToolTipVisibleProperty, false);
        slider->setProperty(sliderToolTipValueProperty, {});
        if (auto *tip = slider->findChild<QWidget *>(
                QStringLiteral("_winui_slider_value_tip"),
                Qt::FindDirectChildrenOnly)) {
            tip->hide();
        }
    }
}

class NavigationItemDelegate final : public QStyledItemDelegate
{
public:
    explicit NavigationItemDelegate(QAbstractItemView *view,
                                    QAbstractItemDelegate *original)
        : QStyledItemDelegate(view)
        , m_view(view)
        , m_original(original)
        , m_indicatorAnimation(this)
    {
        QObject::connect(&m_indicatorAnimation, &QVariantAnimation::valueChanged,
                         this, [this](const QVariant &value) {
            m_indicatorY = value.toReal();
            if (m_view) {
                m_view->viewport()->setProperty(navigationIndicatorProperty,
                                                m_indicatorY);
                m_view->viewport()->update();
            }
        });
        attachSelectionModel();
        m_verticalConnection = QObject::connect(
            view->verticalScrollBar(), &QScrollBar::valueChanged,
            this, [this] { syncIndicatorToViewport(); });
        m_horizontalConnection = QObject::connect(
            view->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, [this] { syncIndicatorToViewport(); });
    }

    ~NavigationItemDelegate() override { shutdown(false); }

    QAbstractItemDelegate *originalDelegate() const { return m_original.data(); }

    void shutdown(bool clearViewport = true)
    {
        if (m_shutdown)
            return;
        m_shutdown = true;
        m_indicatorAnimation.stop();
        if (m_selectionConnection)
            QObject::disconnect(m_selectionConnection);
        if (m_verticalConnection)
            QObject::disconnect(m_verticalConnection);
        if (m_horizontalConnection)
            QObject::disconnect(m_horizontalConnection);
        m_selectionConnection = {};
        m_verticalConnection = {};
        m_horizontalConnection = {};
        if (clearViewport && m_view && m_view->viewport()) {
            m_view->viewport()->setProperty(navigationIndicatorProperty, {});
            m_view->viewport()->update();
        }
        m_selectionModel.clear();
    }

    QSize sizeHint(const QStyleOptionViewItem &, const QModelIndex &) const override
    {
        return {220, 40};
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override
    {
        if (m_shutdown)
            return;
        const_cast<NavigationItemDelegate *>(this)->attachSelectionModel();
        const auto t = Private::tokens(option.palette);
        const bool selected = option.state & QStyle::State_Selected;
        const bool hovered = option.state & QStyle::State_MouseOver;
        const qreal press = progress(option.widget, pressProperty,
                                     option.state & QStyle::State_Sunken ? 1.0 : 0.0);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QColor fill = selected ? t.subtlePressed : Qt::transparent;
        if (hovered)
            fill = Private::mix(fill, t.subtleHover, 1.0 - press);
        fill = Private::mix(fill, t.subtlePressed, press);
        if (fill.alpha() > 0)
            roundedRect(painter, QRectF(option.rect).adjusted(2, 2, -2, -2),
                        fill, Qt::transparent, Private::ControlRadius);

        const QIcon itemIcon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        if (!itemIcon.isNull()) {
            const QRect logicalIcon(option.rect.left() + 14,
                                    option.rect.center().y() - 8, 16, 16);
            paintThemedIcon(painter, itemIcon,
                QStyle::visualRect(option.direction, option.rect, logicalIcon),
                Qt::AlignCenter, option.state & QStyle::State_Enabled
                    ? t.textPrimary : t.textDisabled,
                option.state & QStyle::State_Enabled
                    ? QIcon::Normal : QIcon::Disabled);
        }
        painter->setPen(option.state & QStyle::State_Enabled
                            ? t.textPrimary : t.textDisabled);
        const QRect textRect = QStyle::visualRect(option.direction, option.rect,
                                          option.rect.adjusted(42, 0, -12, 0));
        painter->drawText(textRect,
                          QStyle::visualAlignment(option.direction,
                                          Qt::AlignLeft | Qt::AlignVCenter),
                          option.fontMetrics.elidedText(index.data().toString(),
                                                        Qt::ElideRight,
                                                        textRect.width()));

        if (m_view && m_indicatorY < 0.0 && m_view->currentIndex().isValid())
            m_indicatorY = m_view->visualRect(m_view->currentIndex()).top();
        if (m_indicatorY >= 0.0) {
            const qreal indicatorX = option.direction == Qt::RightToLeft
                ? option.rect.right() - 5.0 : option.rect.left() + 2.0;
            const QRectF indicator(indicatorX, m_indicatorY + 12.0, 3.0, 16.0);
            if (indicator.intersects(option.rect))
            roundedRect(painter, indicator, t.selectionAccent, Qt::transparent, 1.5);
        }
        if ((option.state & QStyle::State_HasFocus)
            && keyboardFocusVisible(m_view)) {
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2));
            painter->drawRoundedRect(QRectF(option.rect).adjusted(3, 3, -3, -3),
                                     4, 4);
            painter->setPen(QPen(t.focusInner, 1));
            painter->drawRoundedRect(QRectF(option.rect).adjusted(5, 5, -5, -5),
                                     3, 3);
        }
        painter->restore();
    }

private:
    void attachSelectionModel()
    {
        if (m_shutdown || !m_view
            || m_selectionModel == m_view->selectionModel())
            return;
        if (m_selectionConnection)
            QObject::disconnect(m_selectionConnection);
        m_selectionModel = m_view->selectionModel();
        if (!m_selectionModel)
            return;
        m_selectionConnection = QObject::connect(
            m_selectionModel, &QItemSelectionModel::currentChanged, this,
            [this](const QModelIndex &current) { setIndicatorTarget(current, true); });
        setIndicatorTarget(m_selectionModel->currentIndex(), false);
    }

    void setIndicatorTarget(const QModelIndex &current, bool animate)
    {
        if (m_shutdown || !m_view)
            return;
        if (!current.isValid()) {
            m_indicatorAnimation.stop();
            m_indicatorY = -1.0;
            if (m_view->viewport()) {
                m_view->viewport()->setProperty(navigationIndicatorProperty, {});
                m_view->viewport()->update();
            }
            return;
        }
        const qreal target = m_view->visualRect(current).top();
        if (m_indicatorY < 0.0 || !animate || !animationsAllowed()) {
            m_indicatorAnimation.stop();
            m_indicatorY = target;
            if (m_view->viewport())
                m_view->viewport()->setProperty(navigationIndicatorProperty,
                                                m_indicatorY);
            if (m_view->viewport())
                m_view->viewport()->update();
            return;
        }
        m_indicatorAnimation.stop();
        m_indicatorAnimation.setStartValue(m_indicatorY);
        m_indicatorAnimation.setEndValue(target);
        m_indicatorAnimation.setDuration(Private::NormalDuration);
        m_indicatorAnimation.setEasingCurve(fluentCurve());
        m_indicatorAnimation.start();
    }

    void syncIndicatorToViewport()
    {
        attachSelectionModel();
        if (m_selectionModel)
            setIndicatorTarget(m_selectionModel->currentIndex(), false);
    }

    QPointer<QAbstractItemView> m_view;
    QPointer<QItemSelectionModel> m_selectionModel;
    QPointer<QAbstractItemDelegate> m_original;
    QMetaObject::Connection m_selectionConnection;
    QMetaObject::Connection m_verticalConnection;
    QMetaObject::Connection m_horizontalConnection;
    mutable qreal m_indicatorY = -1.0;
    QVariantAnimation m_indicatorAnimation;
    bool m_shutdown = false;
};

class NavigationViewState final : public QObject
{
public:
    explicit NavigationViewState(QAbstractItemView *view)
        : QObject(view)
    {
        setObjectName(QString::fromLatin1(navigationStateProperty));
    }

    QPointer<NavigationItemDelegate> delegate;
    QPointer<QAbstractItemDelegate> original;
};

NavigationViewState *navigationState(QAbstractItemView *view, bool create)
{
    if (!view)
        return nullptr;
    QObject *object = view->findChild<QObject *>(
        QString::fromLatin1(navigationStateProperty), Qt::FindDirectChildrenOnly);
    auto *state = dynamic_cast<NavigationViewState *>(object);
    if (!state && create)
        state = new NavigationViewState(view);
    return state;
}

void clearNavigationProperties(QAbstractItemView *view)
{
    if (!view)
        return;
    view->setProperty(navigationDelegateProperty, {});
    view->setProperty(navigationOriginalDelegateProperty, {});
    if (view->viewport())
        view->viewport()->setProperty(navigationIndicatorProperty, {});
}

void retireNavigationDelegate(QAbstractItemView *view, NavigationViewState *state)
{
    if (!view || !state)
        return;

    NavigationItemDelegate *delegate = state->delegate.data();
    const bool installed = delegate && view->itemDelegate() == delegate;
    QAbstractItemDelegate *original = state->original.data();

    // Stop and disconnect while the delegate is still alive. A deferred
    // deletion must never leave a selection/model or scrollbar callback
    // connected during a rapid enable/disable cycle.
    if (delegate)
        delegate->shutdown();
    state->delegate.clear();
    state->original.clear();
    clearNavigationProperties(view);

    if (installed) {
        if (original)
            view->setItemDelegate(original);
        else
            view->setItemDelegate(new QStyledItemDelegate(view));
    }
    if (delegate)
        delegate->deleteLater();
}

void prepareNavigationView(QAbstractItemView *view)
{
    if (!view || !view->property(Style::NavigationViewProperty).toBool())
        return;

    NavigationViewState *state = navigationState(view, true);
    if (state->delegate && view->itemDelegate() == state->delegate)
        return;
    if (state->delegate)
        retireNavigationDelegate(view, state);

    QAbstractItemDelegate *original = view->itemDelegate();
    auto *delegate = new NavigationItemDelegate(view, original);
    state->original = original;
    state->delegate = delegate;
    view->setProperty(navigationOriginalDelegateProperty,
                      QVariant::fromValue<QObject *>(original));
    view->setProperty(navigationDelegateProperty,
                      QVariant::fromValue<QObject *>(delegate));

    const QPointer<QAbstractItemView> guardedView(view);
    const QPointer<NavigationViewState> guardedState(state);
    QObject::connect(delegate, &QObject::destroyed, state,
                     [guardedView, guardedState, delegate] {
        if (!guardedView || !guardedState)
            return;
        if (guardedState->delegate.data() != delegate)
            return;
        guardedState->delegate.clear();
        if (guardedView->itemDelegate() == delegate)
            guardedView->setItemDelegate(new QStyledItemDelegate(guardedView));
        guardedView->setProperty(navigationDelegateProperty, {});
    });
    if (original) {
        QObject::connect(original, &QObject::destroyed, state,
                         [guardedView, guardedState] {
            if (!guardedView || !guardedState)
                return;
            guardedState->original.clear();
            guardedView->setProperty(navigationOriginalDelegateProperty, {});
        });
    }
    view->setItemDelegate(delegate);
    view->viewport()->setProperty(Style::NavigationViewProperty, true);
    remember(view->viewport(), originalMouseTrackingProperty,
             view->viewport()->hasMouseTracking());
    view->viewport()->setMouseTracking(true);
}

void restoreNavigationView(QAbstractItemView *view)
{
    if (!view)
        return;
    NavigationViewState *state = navigationState(view, false);
    if (state)
        retireNavigationDelegate(view, state);
    else
        clearNavigationProperties(view);
    if (view->viewport()->property(originalMouseTrackingProperty).isValid())
        view->viewport()->setMouseTracking(
            view->viewport()->property(originalMouseTrackingProperty).toBool());
    view->viewport()->setProperty(originalMouseTrackingProperty, {});
    view->viewport()->setProperty(Style::NavigationViewProperty, {});
    view->viewport()->setProperty(navigationIndicatorProperty, {});
}

qreal progress(const QWidget *widget, const char *name, qreal fallback)
{
    if (!widget)
        return fallback;
    const QVariant value = widget->property(name);
    return value.isValid() ? value.toReal() : fallback;
}

const QEasingCurve &fluentCurve()
{
    static const QEasingCurve curve = [] {
        QEasingCurve result(QEasingCurve::BezierSpline);
        result.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.0, 1.0),
                                     QPointF(1.0, 1.0));
        return result;
    }();
    return curve;
}

bool systemUsesDarkTheme()
{
#ifdef Q_OS_WIN
    QSettings settings(QStringLiteral(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),
        QSettings::NativeFormat);
    return settings.value(QStringLiteral("AppsUseLightTheme"), 1).toInt() == 0;
#else
    return qGray(QApplication::palette().color(QPalette::Window).rgb()) < 128;
#endif
}

struct SystemAccentRamp
{
    QColor accent;
    QColor light2;
    QColor dark1;
};

SystemAccentRamp systemAccentRamp()
{
    SystemAccentRamp ramp;
#ifdef Q_OS_WIN
    // Explorer stores the Windows accent ramp as BGRA entries ordered
    // Light3, Light2, Light1, Accent, Dark1, Dark2, Dark3, complement.
    // These are the same SystemAccentColor* roles consumed by WinUI's
    // Common_themeresources_any.xaml.
    QSettings settings(QStringLiteral(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Accent"),
        QSettings::NativeFormat);
    const QByteArray bytes = settings.value(QStringLiteral("AccentPalette")).toByteArray();
    const auto entry = [&bytes](int index) {
        const int offset = index * 4;
        if (bytes.size() < offset + 3)
            return QColor{};
        return QColor(quint8(bytes.at(offset + 2)), quint8(bytes.at(offset + 1)),
                      quint8(bytes.at(offset)));
    };
    ramp.light2 = entry(1);
    ramp.accent = entry(3);
    ramp.dark1 = entry(4);

    DWORD color = 0;
    BOOL opaque = FALSE;
    if (SUCCEEDED(DwmGetColorizationColor(&color, &opaque))) {
        QColor result = QColor::fromRgba(color);
        result.setAlpha(255);
        if (result.isValid())
            ramp.accent = result;
    }
#endif
    if (!ramp.accent.isValid())
        ramp.accent = QColor(0, 120, 212);
    if (!ramp.light2.isValid())
        ramp.light2 = Private::mix(ramp.accent, QColor(Qt::white), 0.32);
    if (!ramp.dark1.isValid())
        ramp.dark1 = Private::mix(ramp.accent, QColor(Qt::black), 0.18);
    return ramp;
}

QColor systemAccentColor()
{
    return systemAccentRamp().accent;
}

bool animationsAllowed()
{
    return Style::animationsAllowed();
}

void roundedRect(QPainter *painter, const QRectF &rect, const QColor &fill,
                 const QColor &stroke, qreal radius, qreal strokeWidth)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(fill);
    painter->setPen(stroke.alpha() > 0 ? QPen(stroke, strokeWidth) : Qt::NoPen);
    const qreal half = stroke.alpha() > 0 ? strokeWidth / 2.0 : 0.0;
    painter->drawRoundedRect(rect.adjusted(half, half, -half, -half), radius, radius);
    painter->restore();
}

void paintThemedIcon(QPainter *painter, const QIcon &source, const QRectF &rect,
                     Qt::Alignment alignment, const QColor &foreground,
                     QIcon::Mode mode, QIcon::State state)
{
    if (source.isNull() || rect.isEmpty())
        return;
    const qreal dpr = painter->device() ? painter->device()->devicePixelRatioF() : 1.0;
    const QSize requestedSize(qMax(1, qRound(rect.width())),
                              qMax(1, qRound(rect.height())));
    const QPixmap pixmap = iconPixmap(source, requestedSize, dpr, foreground,
                                     mode, state);
    if (pixmap.isNull())
        return;
    const QSizeF logicalSize = pixmap.deviceIndependentSize();
    QPointF topLeft = rect.topLeft();
    if (alignment & Qt::AlignRight)
        topLeft.setX(rect.right() - logicalSize.width());
    else if (alignment & Qt::AlignHCenter)
        topLeft.setX(rect.center().x() - logicalSize.width() / 2.0);
    if (alignment & Qt::AlignBottom)
        topLeft.setY(rect.bottom() - logicalSize.height());
    else if (alignment & Qt::AlignVCenter)
        topLeft.setY(rect.center().y() - logicalSize.height() / 2.0);
    painter->drawPixmap(topLeft, pixmap);
}

QRectF visualRectF(Qt::LayoutDirection direction, const QRectF &bounds,
                   const QRectF &logical)
{
    if (direction == Qt::LeftToRight)
        return logical;
    return QRectF(bounds.left() + bounds.right() - logical.right(),
                  logical.top(), logical.width(), logical.height());
}

void controlSurface(QPainter *painter, const QRectF &rect, const QColor &fill,
                    const QColor &strokeTop, const QColor &strokeBottom,
                    qreal radius, qreal strokeWidth = 1.0)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(fill);
    QLinearGradient border(rect.topLeft(), rect.bottomLeft());
    border.setColorAt(0.0, strokeTop);
    border.setColorAt(1.0, strokeBottom);
    painter->setPen(QPen(QBrush(border), strokeWidth));
    const qreal half = strokeWidth / 2.0;
    painter->drawRoundedRect(rect.adjusted(half, half, -half, -half), radius, radius);
    painter->restore();
}

bool keyboardFocusVisible(const QWidget *widget)
{
    return widget && widget->property(focusVisibleProperty).toBool();
}

bool revealsKeyboardFocus(int key)
{
    switch (key) {
    case Qt::Key_Tab:
    case Qt::Key_Backtab:
    case Qt::Key_Left:
    case Qt::Key_Right:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Home:
    case Qt::Key_End:
    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        return true;
    default:
        return false;
    }
}

Icon arrowIcon(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_IndicatorArrowDown: return Icon::ChevronDown;
    case QStyle::PE_IndicatorArrowLeft: return Icon::ChevronLeft;
    case QStyle::PE_IndicatorArrowRight: return Icon::ChevronRight;
    case QStyle::PE_IndicatorArrowUp: return Icon::ChevronUp;
    default: return Icon::ChevronRight;
    }
}

bool coveredPrimitive(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_PanelMenuBar:
    case QStyle::PE_FrameTabBarBase:
    case QStyle::PE_FrameTabWidget:
    case QStyle::PE_PanelButtonCommand:
    case QStyle::PE_PanelButtonTool:
    case QStyle::PE_IndicatorCheckBox:
    case QStyle::PE_IndicatorRadioButton:
    case QStyle::PE_PanelLineEdit:
    case QStyle::PE_FrameLineEdit:
    case QStyle::PE_FrameFocusRect:
    case QStyle::PE_IndicatorArrowDown:
    case QStyle::PE_IndicatorArrowLeft:
    case QStyle::PE_IndicatorArrowRight:
    case QStyle::PE_IndicatorArrowUp:
    case QStyle::PE_PanelMenu:
    case QStyle::PE_PanelItemViewItem:
    case QStyle::PE_IndicatorBranch:
    case QStyle::PE_IndicatorHeaderArrow:
    case QStyle::PE_IndicatorToolBarSeparator:
    case QStyle::PE_FrameDockWidget:
    case QStyle::PE_IndicatorDockWidgetResizeHandle:
        return true;
    default:
        return false;
    }
}

bool coveredControl(QStyle::ControlElement element)
{
    switch (element) {
    case QStyle::CE_PushButton:
    case QStyle::CE_PushButtonLabel:
    case QStyle::CE_CheckBox:
    case QStyle::CE_RadioButton:
    case QStyle::CE_MenuBarItem:
    case QStyle::CE_ItemViewItem:
    case QStyle::CE_ComboBoxLabel:
    case QStyle::CE_ToolButtonLabel:
    case QStyle::CE_ProgressBar:
    case QStyle::CE_ProgressBarGroove:
    case QStyle::CE_ProgressBarContents:
    case QStyle::CE_ProgressBarLabel:
    case QStyle::CE_ToolBar:
    case QStyle::CE_Splitter:
    case QStyle::CE_DockWidgetTitle:
    case QStyle::CE_MenuBarEmptyArea:
    case QStyle::CE_TabBarTabShape:
    case QStyle::CE_TabBarTabLabel:
    case QStyle::CE_TabBarTab:
    case QStyle::CE_HeaderSection:
    case QStyle::CE_Header:
    case QStyle::CE_HeaderLabel:
    case QStyle::CE_MenuItem:
        return true;
    default:
        return false;
    }
}

bool coveredComplex(QStyle::ComplexControl control)
{
    switch (control) {
    case QStyle::CC_ToolButton:
    case QStyle::CC_GroupBox:
    case QStyle::CC_ComboBox:
    case QStyle::CC_SpinBox:
    case QStyle::CC_Slider:
    case QStyle::CC_ScrollBar:
        return true;
    default:
        return false;
    }
}

void preparePopupSurface(QWidget *widget)
{
    if (!widget || !widget->window() || widget->window()->windowType() != Qt::Popup)
        return;
    QWidget *popup = widget->window();
    rememberPalette(popup);
    remember(popup, originalAutoFillProperty, popup->autoFillBackground());
    // Creating a native handle from inside QWidget::polish() recursively
    // polishes an unopened QMenu. Apply the DWM material from the Show path;
    // palette and layout preparation can safely happen before visibility.
    if (popup->isVisible())
        applyBackdrop(popup, Backdrop::Acrylic);
    // Popup widgets keep an explicit palette after their first polish. Rebase
    // every show on the current application palette so runtime theme changes
    // cannot leave dark text roles on a light Acrylic surface (or vice versa).
    QPalette popupPalette = effectivePopupPalette(popup, QApplication::palette());
    QColor transparentWindow = popupPalette.color(QPalette::Window);
    transparentWindow.setAlpha(0);
    popupPalette.setColor(QPalette::Window, transparentWindow);
    popupPalette.setColor(QPalette::Base, Qt::transparent);
    popup->setPalette(popupPalette);
    popup->setAutoFillBackground(false);
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(popup, originalMarginsProperty,
                 QVariant::fromValue(popup->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
    }
    QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget);
    if (!view)
        view = popup->findChild<QAbstractItemView *>();
    if (view) {
        rememberPalette(view);
        const QPalette viewPalette = effectivePopupPalette(view, popupPalette);
        view->setPalette(viewPalette);
        rememberPalette(view->viewport());
        remember(view->viewport(), originalAutoFillProperty,
                 view->viewport()->autoFillBackground());
        remember(view->viewport(), originalOpaquePaintProperty,
                 view->viewport()->testAttribute(Qt::WA_OpaquePaintEvent));
        view->viewport()->setPalette(
            effectivePopupPalette(view->viewport(), viewPalette));
        view->viewport()->setAutoFillBackground(false);
        view->viewport()->setAttribute(Qt::WA_OpaquePaintEvent, false);
        if (auto *list = qobject_cast<QListView *>(view)) {
            remember(list, originalListSpacingProperty, list->spacing());
            list->setSpacing(0);
        }
    }
}

void prepareComboPopupFirstFrame(QComboBox *combo)
{
    if (!combo || combo->count() <= 0)
        return;

    QAbstractItemView *view = combo->view();
    if (!view)
        return;

    preparePopupSurface(view);
    const QModelIndex current = combo->model()->index(
        combo->currentIndex(), combo->modelColumn(), combo->rootModelIndex());
    if (!current.isValid())
        return;

    view->setCurrentIndex(current);
    view->doItemsLayout();
    view->scrollTo(current, QAbstractItemView::PositionAtCenter);
}

QComboBox *comboForPopupWidget(QWidget *widget)
{
    if (!widget)
        return nullptr;
    QWidget *popup = widget->window();
    if (!popup || popup->windowType() != Qt::Popup)
        return nullptr;
    return qobject_cast<QComboBox *>(popup->parentWidget());
}

} // namespace

class StylePrivate
{
public:
    struct ToggleDragState {
        QPoint pressPosition;
        bool candidate = false;
        bool dragging = false;
    };

    explicit StylePrivate(Style *owner, ThemeMode initialMode)
        : q(owner), mode(initialMode)
    {
    }

    bool needsSystemAppearancePolling() const
    {
        return mode == ThemeMode::System || !accent.isValid();
    }

    void restartSystemAppearancePolling()
    {
        if (!systemAppearanceTimer)
            return;
        if (!needsSystemAppearancePolling()) {
            systemAppearanceTimer->stop();
            return;
        }
        if (mode == ThemeMode::System)
            lastSystemDark = systemUsesDarkTheme();
        if (!accent.isValid())
            lastSystemAccent = systemAccentColor();
        systemAppearanceTimer->start();
    }

    bool progressBarNeedsAnimation(const QProgressBar *progressBar) const
    {
        return progressBar && progressBar->minimum() == progressBar->maximum()
            && progressBar->isVisible() && Style::animationsAllowed();
    }

    void refreshProgressTimer()
    {
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            if (it->isNull())
                it = progressBars.erase(it);
            else
                ++it;
        }

        bool active = false;
        for (const QPointer<QProgressBar> &guarded : progressBars) {
            if (progressBarNeedsAnimation(guarded)) {
                active = true;
                break;
            }
        }
        if (active)
            progressTimer->start();
        else
            progressTimer->stop();
    }

    void advanceProgressBars()
    {
        const bool allowed = Style::animationsAllowed();
        const qreal phase = allowed
            ? qreal(QDateTime::currentMSecsSinceEpoch() % 1500) / 1500.0
            : 0.35;
        bool active = false;
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            const QPointer<QProgressBar> guarded = *it;
            if (!guarded) {
                it = progressBars.erase(it);
                continue;
            }
            if (progressBarNeedsAnimation(guarded)) {
                active = true;
                guarded->setProperty(progressPhaseProperty, phase);
                guarded->update();
            }
            ++it;
        }
        if (!active)
            progressTimer->stop();
    }

    void registerProgressBar(QProgressBar *progressBar)
    {
        if (!progressBar)
            return;
        if (progressBarStateConnections.contains(progressBar)) {
            refreshProgressTimer();
            return;
        }
        progressBars.append(QPointer<QProgressBar>(progressBar));
        // QProgressBar has no rangeChanged signal. valueChanged covers the
        // normal range-reset path, while UpdateRequest below closes the case
        // where a range changes without changing the current value.
        progressBarStateConnections.insert(progressBar,
            QObject::connect(progressBar, &QProgressBar::valueChanged, q,
                             [this](int) {
                refreshProgressTimer();
            }));
        QObject::connect(progressBar, &QObject::destroyed, q,
                         [this, progressBar] {
            unregisterProgressBar(progressBar);
        });
        refreshProgressTimer();
    }

    void unregisterProgressBar(QProgressBar *progressBar)
    {
        if (!progressBar)
            return;
        if (const auto connection = progressBarStateConnections.take(progressBar))
            QObject::disconnect(connection);
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            if (it->isNull() || it->data() == progressBar)
                it = progressBars.erase(it);
            else
                ++it;
        }
        refreshProgressTimer();
    }

    QTimer *ensureScrollBarTimer(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return nullptr;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(scrollBar);
        timer->setObjectName(QStringLiteral("_winui_scrollbar_timer"));
        timer->setSingleShot(true);
        const QPointer<QScrollBar> guarded(scrollBar);
        QObject::connect(timer, &QTimer::timeout, q, [this, guarded] {
            if (!guarded || !guarded->isVisible() || !guarded->isEnabled()
                || !guarded->property(scrollBarInsideProperty).isValid()) {
                return;
            }
            if (guarded->property(scrollBarInsideProperty).toBool()) {
                animate(guarded, hoverProperty, 1.0, Private::FastDuration);
            } else {
                animate(guarded, hoverProperty, 0.0, Private::FastDuration);
            }
        });
        scrollBarTimers.insert(scrollBar, QPointer<QTimer>(timer));
        QObject::connect(scrollBar, &QObject::destroyed, q,
                         [this, scrollBar] {
            unregisterScrollBar(scrollBar);
        });
        return timer;
    }

    void scheduleScrollBar(QScrollBar *scrollBar, int delay)
    {
        if (auto *timer = ensureScrollBarTimer(scrollBar))
            timer->start(delay);
    }

    void cancelScrollBarTimer(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterScrollBar(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        if (auto it = scrollBarTimers.find(scrollBar);
            it != scrollBarTimers.end()) {
            QTimer *timer = it->data();
            if (timer)
                timer->stop();
            scrollBarTimers.erase(it);
            delete timer;
        }
    }

    QTimer *ensureSliderToolTipTimer(QSlider *slider)
    {
        if (!slider)
            return nullptr;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(slider);
        timer->setObjectName(QStringLiteral("_winui_slider_tooltip_timer"));
        timer->setSingleShot(true);
        const QPointer<QSlider> guarded(slider);
        QObject::connect(timer, &QTimer::timeout, q, [guarded] {
            if (guarded && guarded->isEnabled())
                showSliderValueToolTip(guarded);
        });
        sliderToolTipTimers.insert(slider, QPointer<QTimer>(timer));
        QObject::connect(slider, &QObject::destroyed, q,
                         [this, slider] {
            unregisterSlider(slider);
        });
        return timer;
    }

    void scheduleSliderToolTip(QSlider *slider)
    {
        if (!slider || !slider->isEnabled())
            return;
        if (auto *timer = ensureSliderToolTipTimer(slider))
            timer->start(0);
    }

    void cancelSliderToolTip(QSlider *slider)
    {
        if (!slider)
            return;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterSlider(QSlider *slider)
    {
        if (!slider)
            return;
        if (auto it = sliderToolTipTimers.find(slider);
            it != sliderToolTipTimers.end()) {
            QTimer *timer = it->data();
            if (timer)
                timer->stop();
            sliderToolTipTimers.erase(it);
            delete timer;
        }
    }

    QVariantAnimation *findAnimation(QWidget *widget, const char *property)
    {
        if (!widget)
            return nullptr;
        const auto widgetIt = animations.find(widget);
        if (widgetIt == animations.end())
            return nullptr;
        const auto propertyIt = widgetIt->find(QByteArray(property));
        if (propertyIt == widgetIt->end())
            return nullptr;
        return propertyIt->data();
    }

    void forgetAnimation(QWidget *widget, const QByteArray &property,
                         QVariantAnimation *expected)
    {
        if (!widget || !expected)
            return;
        auto widgetIt = animations.find(widget);
        if (widgetIt == animations.end())
            return;
        const auto propertyIt = widgetIt->find(property);
        if (propertyIt == widgetIt->end() || propertyIt->data() != expected)
            return;
        const QPointer<QVariantAnimation> animation = propertyIt->data();
        widgetIt->erase(propertyIt);
        if (widgetIt->isEmpty()) {
            animations.erase(widgetIt);
            if (const auto connection = animationCleanupConnections.take(widget))
                QObject::disconnect(connection);
        }
        if (animation) {
            animation->stop();
            delete animation;
        }
    }

    QVariantAnimation *ensureAnimation(QWidget *widget, const char *property)
    {
        if (!widget)
            return nullptr;
        auto widgetIt = animations.find(widget);
        if (widgetIt == animations.end())
            widgetIt = animations.insert(widget, {});
        const QByteArray propertyName(property);
        auto propertyIt = widgetIt->find(propertyName);
        if (propertyIt != widgetIt->end() && propertyIt->data())
            return propertyIt->data();

        auto *animation = new QVariantAnimation(q);
        widgetIt->insert(propertyName, QPointer<QVariantAnimation>(animation));
        if (!animationCleanupConnections.contains(widget)) {
            animationCleanupConnections.insert(widget,
                QObject::connect(widget, &QObject::destroyed, q,
                                 [this, widget] { stopAnimations(widget); }));
        }
        const QPointer<QWidget> guardedWidget(widget);
        QObject::connect(animation, &QVariantAnimation::valueChanged, q,
                         [guardedWidget, propertyName](const QVariant &value) {
            if (!guardedWidget)
                return;
            guardedWidget->setProperty(propertyName.constData(), value);
            guardedWidget->update();
        });
        QObject::connect(animation, &QVariantAnimation::finished, q,
                         [this, widget, propertyName, animation] {
            auto widgetIt = animations.find(widget);
            if (widgetIt == animations.end())
                return;
            const auto propertyIt = widgetIt->find(propertyName);
            if (propertyIt == widgetIt->end() || propertyIt->data() != animation)
                return;
            widgetIt->erase(propertyIt);
            if (widgetIt->isEmpty()) {
                animations.erase(widgetIt);
                if (const auto connection = animationCleanupConnections.take(widget))
                    QObject::disconnect(connection);
            }
            delete animation;
        });
        return animation;
    }

    void animate(QWidget *widget, const char *property, qreal target, int duration)
    {
        if (!widget)
            return;

        const qreal start = progress(widget, property, 1.0 - target);
        QPointer<QVariantAnimation> previous = findAnimation(widget, property);
        if (previous)
            previous->stop();
        if (duration <= 0 || !animationsAllowed() || qFuzzyCompare(start, target)) {
            forgetAnimation(widget, QByteArray(property), previous);
            widget->setProperty(property, target);
            widget->update();
            return;
        }

        auto *animation = ensureAnimation(widget, property);
        if (!animation)
            return;
        animation->setKeyValues({});
        animation->setStartValue(start);
        animation->setEndValue(target);
        animation->setDuration(duration);
        animation->setEasingCurve(fluentCurve());
        animation->start();
    }

    void stopAnimations(QWidget *widget)
    {
        if (!widget)
            return;
        if (auto it = animations.find(widget); it != animations.end()) {
            const auto propertyAnimations = std::move(it.value());
            animations.erase(it);
            for (const QPointer<QVariantAnimation> &animation : propertyAnimations) {
                if (animation) {
                    animation->stop();
                    delete animation;
                }
            }
        }
        if (const auto connection = animationCleanupConnections.take(widget))
            QObject::disconnect(connection);
    }

    void beginButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = widget->property(
            buttonPressGenerationProperty).toULongLong() + 1;
        widget->setProperty(buttonPressGenerationProperty,
                            QVariant::fromValue(generation));
        widget->setProperty(buttonPressReleasePendingProperty, false);
        // A synchronous pressed frame is intentional. It makes a very fast
        // click observable and also cancels a release animation already in
        // flight before the next press starts.
        animate(widget, pressProperty, 1.0, 0);
    }

    void releaseButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = widget->property(
            buttonPressGenerationProperty).toULongLong() + 1;
        widget->setProperty(buttonPressGenerationProperty,
                            QVariant::fromValue(generation));
        widget->setProperty(buttonPressReleasePendingProperty, true);
        const QPointer<QWidget> guardedWidget(widget);
        QTimer::singleShot(16, q, [this, guardedWidget, generation] {
            if (!guardedWidget
                || guardedWidget->property(buttonPressGenerationProperty)
                       .toULongLong() != generation
                || !guardedWidget->property(buttonPressReleasePendingProperty)
                       .toBool()) {
                return;
            }
            guardedWidget->setProperty(buttonPressReleasePendingProperty, false);
            animate(guardedWidget, pressProperty, 0.0, Private::FasterDuration);
        });
    }

    void cancelButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation = widget->property(
            buttonPressGenerationProperty).toULongLong() + 1;
        widget->setProperty(buttonPressGenerationProperty,
                            QVariant::fromValue(generation));
        widget->setProperty(buttonPressReleasePendingProperty, false);
        animate(widget, pressProperty, 0.0, Private::FasterDuration);
    }

    void clearPointerInteraction(QWidget *widget)
    {
        if (!widget)
            return;
        if (buttonPressPulse(widget))
            cancelButtonPress(widget);
        stopAnimations(widget);
        widget->setProperty(hoverProperty, 0.0);
        widget->setProperty(pressProperty, 0.0);
    }

    void releaseComboChevron(QWidget *widget)
    {
        if (!widget)
            return;
        const qreal start = progress(widget, comboChevronProperty, 0.0);
        QPointer<QVariantAnimation> previous = findAnimation(widget, comboChevronProperty);
        if (previous)
            previous->stop();
        if (!animationsAllowed() || qFuzzyIsNull(start)) {
            forgetAnimation(widget, QByteArray(comboChevronProperty), previous);
            widget->setProperty(comboChevronProperty, 0.0);
            widget->update();
            return;
        }
        // AnimatedChevronDownSmallVisualSource: PressedToNormal moves from
        // y=31.5 to y=21 then y=24 on a 48 px canvas. At the 12 px ComboBox
        // glyph this is +1.875 px, -0.75 px, then rest over about 300 ms.
        auto *animation = ensureAnimation(widget, comboChevronProperty);
        if (!animation)
            return;
        animation->setStartValue(start);
        animation->setKeyValues({{0.28, QVariant(-0.4)}});
        animation->setEndValue(0.0);
        animation->setDuration(300);
        animation->setEasingCurve(fluentCurve());
        animation->start();
    }

    bool dark() const
    {
        return mode == ThemeMode::Dark || (mode == ThemeMode::System && systemUsesDarkTheme());
    }

    Style *q = nullptr;
    ThemeMode mode = ThemeMode::System;
    QColor accent;
    QHash<QWidget *, QHash<QByteArray, QPointer<QVariantAnimation>>> animations;
    QHash<QWidget *, QMetaObject::Connection> animationCleanupConnections;
    QVector<QPointer<QProgressBar>> progressBars;
    QHash<QProgressBar *, QMetaObject::Connection> progressBarStateConnections;
    QHash<QScrollBar *, QPointer<QTimer>> scrollBarTimers;
    QHash<QSlider *, QPointer<QTimer>> sliderToolTipTimers;
    QHash<QWidget *, QMetaObject::Connection> toggleConnections;
    QHash<QRadioButton *, QMetaObject::Connection> radioConnections;
    QHash<QWidget *, QMetaObject::Connection> tableConnections;
    QHash<QCheckBox *, ToggleDragState> toggleDragStates;
    bool keyboardInput = false;
    bool applicationStateSaved = false;
    bool lastSystemDark = false;
    QColor lastSystemAccent;
    QTimer *progressTimer = nullptr;
    QTimer *systemAppearanceTimer = nullptr;
    QFont originalApplicationFont;
    QPalette originalApplicationPalette;
};

Style::Style(ThemeMode mode)
    : QProxyStyle(new QCommonStyle), d(std::make_unique<StylePrivate>(this, mode))
{
    setObjectName(QStringLiteral("winui3"));
    d->progressTimer = new QTimer(this);
    d->progressTimer->setObjectName(QStringLiteral("_winui_progress_timer"));
    d->progressTimer->setInterval(16);
    connect(d->progressTimer, &QTimer::timeout, this,
            [this] { d->advanceProgressBars(); });
    d->systemAppearanceTimer = new QTimer(this);
    d->systemAppearanceTimer->setObjectName(
        QStringLiteral("_winui_system_appearance_timer"));
    d->systemAppearanceTimer->setInterval(750);
    connect(d->systemAppearanceTimer, &QTimer::timeout,
            this, &Style::checkSystemAppearance);
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
                [this](Qt::ColorScheme) { checkSystemAppearance(); });
    }
    d->restartSystemAppearancePolling();
}

Style::~Style() = default;

ThemeMode Style::themeMode() const
{
    return d->mode;
}

void Style::setThemeMode(ThemeMode mode)
{
    if (d->mode == mode)
        return;
    d->mode = mode;
    refreshApplicationAppearance();
    d->restartSystemAppearancePolling();
    emit themeChanged(mode);
}

QColor Style::accentColor() const
{
    return d->accent.isValid() ? d->accent : systemAccentColor();
}

bool Style::animationsAllowed()
{
    return !qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
}

void Style::setAccentColor(const QColor &color)
{
    if (d->accent == color)
        return;
    d->accent = color;
    refreshApplicationAppearance();
    d->restartSystemAppearancePolling();
    emit accentColorChanged(accentColor());
}

void Style::refreshApplicationAppearance()
{
    if (!qApp)
        return;
    qApp->setPalette(standardPalette());
    for (QWidget *widget : qApp->allWidgets()) {
        if (widget->property(ownedPaletteProperty).toBool()) {
            QPalette palette = standardPalette();
            if (qobject_cast<QTableView *>(widget)) {
                const Private::Tokens tableTokens =
                    Private::tokens(standardPalette());
                palette.setColor(QPalette::Highlight, tableTokens.subtleHover);
                palette.setColor(QPalette::HighlightedText, tableTokens.textPrimary);
            } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
                       editor && itemView(editor)) {
                const Private::Tokens editorTokens =
                    Private::tokens(standardPalette());
                palette.setColor(QPalette::Highlight, accentColor());
                palette.setColor(QPalette::HighlightedText,
                                 editorTokens.textOnAccentPrimary);
            } else if (qobject_cast<QDialog *>(widget)) {
                palette.setColor(QPalette::Window,
                    d->dark() ? QColor(32, 32, 32) : QColor(255, 255, 255));
            }
            widget->setPalette(palette);
        }
        widget->update();
    }
    for (QWidget *window : qApp->topLevelWidgets()) {
        const QVariant backdrop = window->property("_winui_backdrop");
        if (backdrop.isValid()) {
            QTimer::singleShot(0, window, [window, backdrop] {
                applyBackdrop(window, static_cast<Backdrop>(backdrop.toInt()));
            });
        }
        if (window->windowType() == Qt::Popup)
            preparePopupSurface(window);
    }
}

void Style::checkSystemAppearance()
{
    if (!d->needsSystemAppearancePolling()) {
        d->systemAppearanceTimer->stop();
        return;
    }
    bool themeChangedAtRuntime = false;
    bool accentChangedAtRuntime = false;
    QColor systemAccent;
    if (d->mode == ThemeMode::System) {
        const bool systemDark = systemUsesDarkTheme();
        themeChangedAtRuntime = d->lastSystemDark != systemDark;
        d->lastSystemDark = systemDark;
    }
    if (!d->accent.isValid()) {
        systemAccent = systemAccentColor();
        accentChangedAtRuntime = d->lastSystemAccent != systemAccent;
        d->lastSystemAccent = systemAccent;
    }
    if (!themeChangedAtRuntime && !accentChangedAtRuntime)
        return;
    refreshApplicationAppearance();
    if (themeChangedAtRuntime)
        emit themeChanged(ThemeMode::System);
    if (accentChangedAtRuntime)
        emit accentColorChanged(systemAccent);
}

void Style::setControlRole(QWidget *widget, ControlRole role)
{
    if (!widget)
        return;
    if (!widget->property(originalRoleWasValidProperty).isValid()) {
        widget->setProperty(originalRoleWasValidProperty,
                            widget->property(roleProperty).isValid());
        widget->setProperty(originalRoleProperty, widget->property(roleProperty));
    }
    widget->setProperty(roleProperty, static_cast<int>(role));
    widget->update();
}

ControlRole Style::controlRole(const QWidget *widget)
{
    if (!widget)
        return ControlRole::Standard;
    if (!widget->property(roleProperty).isValid()) {
        if (const auto *button = qobject_cast<const QPushButton *>(widget);
            button && button->isDefault()) {
            return ControlRole::Accent;
        }
    }
    return static_cast<ControlRole>(widget->property(roleProperty).toInt());
}

void Style::setToggleSwitch(QCheckBox *checkBox, bool enabled)
{
    if (!checkBox)
        return;
    checkBox->setProperty(ToggleSwitchProperty, enabled);
    checkBox->setTristate(false);
    checkBox->updateGeometry();
    checkBox->update();
}

bool Style::isToggleSwitch(const QCheckBox *checkBox)
{
    return checkBox && checkBox->property(ToggleSwitchProperty).toBool();
}

void Style::setToggleSwitchText(QCheckBox *checkBox, const QString &onText,
                                const QString &offText)
{
    if (!checkBox)
        return;
    checkBox->setProperty(ToggleSwitchOnTextProperty, onText);
    checkBox->setProperty(ToggleSwitchOffTextProperty, offText);
    checkBox->updateGeometry();
    checkBox->update();
}

void Style::setSettingsCard(QFrame *frame, bool enabled)
{
    if (!frame)
        return;
    if (enabled) {
        remember(frame, originalFrameShapeProperty, int(frame->frameShape()));
        frame->setProperty(SettingsCardProperty, true);
        frame->setFrameShape(QFrame::StyledPanel);
    } else {
        frame->setProperty(SettingsCardProperty, false);
        if (frame->property(originalFrameShapeProperty).isValid()) {
            frame->setFrameShape(static_cast<QFrame::Shape>(
                frame->property(originalFrameShapeProperty).toInt()));
            frame->setProperty(originalFrameShapeProperty, {});
        }
    }
    frame->updateGeometry();
    frame->update();
}

void Style::setNavigationView(QAbstractItemView *view, bool enabled)
{
    if (!view)
        return;
    view->setProperty(NavigationViewProperty, enabled);
    if (view->viewport())
        view->viewport()->setProperty(NavigationViewProperty, enabled);
    view->updateGeometry();
    view->viewport()->update();
}

void Style::setVerticalSpinButtons(QAbstractSpinBox *spinBox, bool enabled)
{
    if (!spinBox)
        return;
    spinBox->setProperty(VerticalSpinButtonsProperty, enabled);
    spinBox->updateGeometry();
    spinBox->update();
}

bool Style::hasVerticalSpinButtons(const QAbstractSpinBox *spinBox)
{
    return spinBox && spinBox->property(VerticalSpinButtonsProperty).toBool();
}

void Style::setContentDialog(QDialog *dialog, bool enabled)
{
    if (!dialog)
        return;
    dialog->setProperty(ContentDialogProperty, enabled);
    if (!enabled && !qobject_cast<QMessageBox *>(dialog))
        restoreContentDialogState(dialog, true);
    dialog->updateGeometry();
    dialog->update();
}

QPalette Style::standardPalette() const
{
    const bool darkTheme = d->dark();
    const QColor accent = accentColor();
    const SystemAccentRamp systemRamp = systemAccentRamp();
    const QColor accentFill = d->accent.isValid()
        ? Private::mix(accent, darkTheme ? QColor(Qt::white) : QColor(Qt::black),
                       darkTheme ? 0.32 : 0.18)
        : (darkTheme ? systemRamp.light2 : systemRamp.dark1);
    QPalette palette;

    if (darkTheme) {
        palette.setColor(QPalette::Window, QColor(32, 32, 32));
        palette.setColor(QPalette::WindowText, QColor(255, 255, 255));
        palette.setColor(QPalette::Base, QColor(58, 58, 58, 76));
        palette.setColor(QPalette::AlternateBase, QColor(255, 255, 255, 13));
        palette.setColor(QPalette::Button, QColor(255, 255, 255, 15));
        palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
        palette.setColor(QPalette::Text, QColor(255, 255, 255));
        palette.setColor(QPalette::PlaceholderText, QColor(255, 255, 255, 197));
        palette.setColor(QPalette::Mid, QColor(255, 255, 255, 18));
        palette.setColor(QPalette::Midlight, QColor(255, 255, 255, 24));
        palette.setColor(QPalette::Dark, QColor(0, 0, 0, 80));
        palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 160));
        palette.setColor(QPalette::ToolTipBase, QColor(44, 44, 44));
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                         QColor(255, 255, 255, 93));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(255, 255, 255, 92));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(255, 255, 255, 11));
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(255, 255, 255, 8));
    } else {
        palette.setColor(QPalette::Window, QColor(243, 243, 243));
        palette.setColor(QPalette::WindowText, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::Base, QColor(255, 255, 255, 128));
        palette.setColor(QPalette::AlternateBase, QColor(246, 246, 246, 128));
        palette.setColor(QPalette::Button, QColor(255, 255, 255, 179));
        palette.setColor(QPalette::ButtonText, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::Text, QColor(0, 0, 0, 228));
        palette.setColor(QPalette::PlaceholderText, QColor(0, 0, 0, 158));
        palette.setColor(QPalette::Mid, QColor(0, 0, 0, 15));
        palette.setColor(QPalette::Midlight, QColor(0, 0, 0, 41));
        palette.setColor(QPalette::Dark, QColor(0, 0, 0, 41));
        palette.setColor(QPalette::Shadow, QColor(0, 0, 0, 90));
        palette.setColor(QPalette::ToolTipBase, QColor(249, 249, 249));
        palette.setColor(QPalette::ToolTipText, QColor(26, 26, 26));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::PlaceholderText,
                         QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0, 0, 0, 92));
        palette.setColor(QPalette::Disabled, QPalette::Button, QColor(249, 249, 249, 77));
        palette.setColor(QPalette::Disabled, QPalette::Base, QColor(246, 246, 246, 128));
    }

    const QColor textOnAccent = Private::contrastText(accent);
    palette.setColor(QPalette::Highlight, accent); // system selection accent
    palette.setColor(QPalette::HighlightedText, textOnAccent);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::LinkVisited, accent.darker(112));
    palette.setColor(QPalette::BrightText, textOnAccent);
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // WinUI uses SystemAccentColor for selection, but Light2 in dark mode and
    // Dark1 in light mode for AccentFillColorDefault.
    palette.setColor(QPalette::Accent, accentFill);
#endif
    return palette;
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option,
                          QPainter *painter, const QWidget *widget) const
{
    using namespace Private;
    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & State_Enabled;
    const bool hovered = enabled && (option->state & State_MouseOver);
    const bool pressed = enabled && (option->state & State_Sunken);
    const qreal hover = enabled
        ? progress(widget, hoverProperty, hovered ? 1.0 : 0.0) : 0.0;
    const qreal press = enabled
        ? progress(widget, pressProperty, pressed ? 1.0 : 0.0) : 0.0;

    if (element == PE_PanelMenuBar) {
        painter->fillRect(option->rect, t.surface);
        return;
    }

    if (element == PE_FrameTabBarBase) {
        painter->setPen(t.strokeSecondary);
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        return;
    }

    if (element == PE_FrameTabWidget) {
        roundedRect(painter, QRectF(option->rect).adjusted(0, 0, -1, -1),
                    t.control, t.stroke, ControlRadius);
        return;
    }

    if (element == PE_PanelButtonCommand || element == PE_PanelButtonTool) {
        const ControlRole role = controlRole(widget);
        const bool textHelper = textBoxHelperButton(widget);
        QColor fill = t.control;
        QColor stroke = t.stroke;

        if (role == ControlRole::Accent) {
            fill = enabled ? t.accentFill : t.accentFillDisabled;
            fill = mix(fill, t.accentFillHover, hover);
            fill = mix(fill, t.accentFillPressed, press);
            stroke = fill.darker(t.dark ? 90 : 112);
        } else if (role == ControlRole::Destructive) {
            fill = enabled ? t.danger : mix(t.surface, t.danger, 0.35);
            fill = mix(fill, fill.lighter(112), hover);
            fill = mix(fill, fill.darker(112), press);
            stroke = fill.darker(112);
        } else if (role == ControlRole::Subtle || role == ControlRole::Navigation
                   || (element == PE_PanelButtonTool && widget
                       && (qobject_cast<const QToolBar *>(widget->parentWidget())
                           || textHelper))) {
            fill = Qt::transparent;
            fill = mix(fill, t.subtleHover, hover);
            fill = mix(fill, t.subtlePressed, press);
            if (option->state & State_On)
                fill = mix(t.subtleHover, t.accentFill, 0.14);
            stroke = Qt::transparent;
        } else {
            if (option->state & State_On) {
                fill = enabled ? t.accentFill : t.accentFillDisabled;
                fill = mix(fill, t.accentFillHover, hover);
                fill = mix(fill, t.accentFillPressed, press);
                stroke = fill;
            } else {
                fill = enabled ? fill : t.controlDisabled;
                fill = mix(fill, t.controlHover, hover);
                fill = mix(fill, t.controlPressed, press);
            }
        }

        QRectF surfaceRect = option->rect;
        if (textHelper) {
            const QRect logical = option->rect.adjusted(0, 4, -4, -4);
            surfaceRect = visualRect(option->direction, option->rect, logical);
        }
        if (stroke.alpha() == 0) {
            roundedRect(painter, surfaceRect, fill, Qt::transparent, ControlRadius);
        } else if (role == ControlRole::Accent || role == ControlRole::Destructive
                   || ((option->state & State_On) && role == ControlRole::Standard)) {
            controlSurface(painter, surfaceRect.toRect(), fill,
                           t.accentStroke, t.accentStrokeSecondary, ControlRadius);
        } else {
            controlSurface(painter, surfaceRect.toRect(), fill,
                           t.stroke, t.strokeSecondary, ControlRadius);
        }
        return;
    }

    if (element == PE_IndicatorCheckBox || element == PE_IndicatorRadioButton) {
        const bool checked = option->state & (State_On | State_NoChange);
        const qreal checkAmount = progress(widget, checkProperty,
                                           checked ? 1.0 : 0.0);
        // Keep the 20 px logical silhouette intact. The half-pixel inset
        // keeps the one-pixel outline inside that silhouette when antialiasing
        // is enabled, including at fractional device-pixel ratios.
        const QRectF indicator = QRectF(option->rect).adjusted(0.5, 0.5,
                                                                -0.5, -0.5);
        const qreal hover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
        const qreal press = progress(widget, pressProperty,
                                     option->state & State_Sunken ? 1.0 : 0.0);
        QColor fill = mix(t.layer, t.accentFill, checkAmount);
        QColor stroke = mix(t.strokeStrong, t.accentFill, checkAmount);
        if (!enabled) {
            fill = mix(t.controlDisabled, t.accentFillDisabled, checkAmount);
            stroke = mix(t.textDisabled, t.accentFillDisabled, checkAmount);
        } else {
            const QColor hoverFill = mix(mix(t.layer, t.textPrimary, 0.05),
                                         t.accentFillHover, checkAmount);
            const QColor pressedFill = mix(t.controlPressed,
                                           t.accentFillPressed, checkAmount);
            fill = mix(fill, hoverFill, hover);
            fill = mix(fill, pressedFill, press);
            if (checkAmount < 0.001)
                stroke = mix(stroke, t.textDisabled, press);
        }
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(fill);
        painter->setPen(QPen(stroke, 1));
        if (element == PE_IndicatorRadioButton)
            painter->drawEllipse(indicator);
        else
            painter->drawRoundedRect(indicator, 3, 3);

        if (checkAmount > 0.001) {
            const QColor onAccent = enabled ? t.textOnAccentPrimary
                                             : t.textOnAccentDisabled;
            painter->setPen(QPen(onAccent, 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            if (element == PE_IndicatorRadioButton) {
                painter->setBrush(onAccent);
                painter->setPen(Qt::NoPen);
                // WinUI: 12 px at rest, 14 px on pointer-over, 10 px pressed.
                const qreal diameter = ((12.0 + 2.0 * hover) * (1.0 - press)
                    + 10.0 * press) * checkAmount;
                painter->drawEllipse(indicator.center(), diameter / 2.0, diameter / 2.0);
            } else if (option->state & State_NoChange) {
                const qreal halfWidth = 5.0 * checkAmount;
                painter->drawLine(QPointF(indicator.center().x() - halfWidth,
                                          indicator.center().y()),
                                  QPointF(indicator.center().x() + halfWidth,
                                          indicator.center().y()));
            } else {
                // Approximate WinUI's AnimatedAcceptVisualSource by revealing
                // the two check segments at a constant path velocity.
                const QPointF a(indicator.left() + 4, indicator.center().y());
                const QPointF b(indicator.center().x() - 1, indicator.bottom() - 4);
                const QPointF c(indicator.right() - 4, indicator.top() + 4);
                const QLineF first(a, b);
                const QLineF second(b, c);
                const qreal total = first.length() + second.length();
                const qreal visible = total * checkAmount;
                if (visible <= first.length()) {
                    painter->drawLine(QLineF::fromPolar(visible, first.angle())
                                          .translated(a));
                } else {
                    painter->drawLine(first);
                    painter->drawLine(QLineF::fromPolar(visible - first.length(),
                                                        second.angle()).translated(b));
                }
            }
        }
        painter->restore();
        return;
    }

    if (element == PE_PanelLineEdit || element == PE_FrameLineEdit) {
        // QAbstractSpinBox owns the complete NumberBox surface. Its private
        // QLineEdit must paint only text, cursor and selection; painting a
        // second TextBox panel here creates a nested rectangular "cell" on
        // hover and focus.
        if (spinBoxEditor(widget))
            return;
        const bool focused = option->state & State_HasFocus;
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & State_MouseOver ? 1.0 : 0.0);
        QColor fill = !enabled ? t.controlDisabled
            : focused ? (t.dark ? QColor(30, 30, 30, 179) : QColor(255, 255, 255))
                      : t.control;
        if (enabled && !focused)
            fill = mix(fill, t.controlHover, hover);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       ControlRadius);
        if (focused) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                ControlRadius, ControlRadius);
            painter->setClipPath(clip);
            painter->setPen(QPen(t.accentFill, 2.0, Qt::SolidLine, Qt::FlatCap));
            painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                              option->rect.right(), option->rect.bottom() - 1);
            painter->restore();
        }
        return;
    }

    if (element == PE_FrameFocusRect) {
        if (!keyboardFocusVisible(widget))
            return;
        const qreal focus = progress(widget, focusProperty,
                                     option->state & State_HasFocus ? 1.0 : 0.0);
        if (focus <= 0.01)
            return;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QColor outer = t.focusOuter;
        QColor inner = t.focusInner;
        outer.setAlphaF(outer.alphaF() * focus);
        inner.setAlphaF(inner.alphaF() * focus);
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(outer, 2));
        painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1),
                                 ControlRadius + 2, ControlRadius + 2);
        painter->setPen(QPen(inner, 1));
        painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                 ControlRadius, ControlRadius);
        painter->restore();
        return;
    }

    if (element == PE_IndicatorArrowDown || element == PE_IndicatorArrowLeft
        || element == PE_IndicatorArrowRight || element == PE_IndicatorArrowUp) {
        WinUI3::icon(arrowIcon(element), enabled ? t.textPrimary : t.textDisabled)
            .paint(painter, option->rect, Qt::AlignCenter,
                   enabled ? QIcon::Normal : QIcon::Disabled);
        return;
    }

    if (element == PE_PanelMenu) {
        QColor fill = t.layer;
        fill.setAlpha(238);
        roundedRect(painter, option->rect, fill, t.stroke, OverlayRadius);
        return;
    }

    if (element == PE_Frame && widget && widget->window()
        && widget->window()->windowType() == Qt::Popup) {
        const QColor acrylic = t.dark ? QColor(44, 44, 44, 230)
                                      : QColor(252, 252, 252, 230);
        controlSurface(painter, option->rect, acrylic,
                       t.dark ? QColor(0, 0, 0, 51) : QColor(0, 0, 0, 15),
                       t.dark ? QColor(0, 0, 0, 51) : QColor(0, 0, 0, 15),
                       OverlayRadius);
        return;
    }

    if (element == PE_Frame && qobject_cast<const QAbstractItemView *>(widget)) {
        roundedRect(painter, QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                    Qt::transparent, t.stroke, ControlRadius);
        return;
    }

    if (element == PE_Frame && richTextEditor(widget)) {
        const QWidget *editor = richTextEditor(widget);
        const bool focused = editor->hasFocus();
        const bool editorEnabled = option->state & State_Enabled;
        const QColor fill = !editorEnabled ? t.controlDisabled
            : focused ? (t.dark ? QColor(30, 30, 30, 179)
                              : QColor(255, 255, 255))
                      : (option->state & State_MouseOver ? t.controlHover
                                                        : t.control);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       ControlRadius);
        if (focused) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                ControlRadius, ControlRadius);
            painter->setClipPath(clip);
            painter->setPen(QPen(t.accentFill, 2.0, Qt::SolidLine, Qt::FlatCap));
            painter->drawLine(option->rect.left(), option->rect.bottom() - 1,
                              option->rect.right(), option->rect.bottom() - 1);
            painter->restore();
        }
        return;
    }

    if (element == PE_PanelItemViewItem) {
        const auto *viewOption = qstyleoption_cast<const QStyleOptionViewItem *>(option);
        const QAbstractItemView *view = itemView(widget);
        const bool popup = widget && widget->window()
            && widget->window()->windowType() == Qt::Popup;
        const bool comboPopup = popup && widget->window()->parentWidget()
            && qobject_cast<const QComboBox *>(widget->window()->parentWidget());
        const Tokens itemTokens = tokens(option->palette);
        const bool tree = qobject_cast<const QTreeView *>(view);
        const bool table = qobject_cast<const QTableView *>(view);
        const bool selected = option->state & State_Selected;
        const bool hovered = option->state & State_MouseOver;
        const bool pressedItem = hovered && (option->state & State_Sunken);
        QColor fill = Qt::transparent;
        if (pressedItem)
            fill = itemTokens.subtlePressed;
        else if (selected && hovered)
            fill = itemTokens.subtlePressed;
        else if (selected || hovered)
            fill = itemTokens.subtleHover;
        QRectF itemRect = popup
            ? QRectF(option->rect).adjusted(5, 2, -5, -2)
            : tree ? QRectF(option->rect).adjusted(4, 2, -4, -2)
                   : table ? QRectF(option->rect)
                           : QRectF(option->rect).adjusted(2, 1, -2, -1);
        if (fill.alpha() > 0)
            roundedRect(painter, itemRect, fill, Qt::transparent,
                        popup ? 3.0 : table ? 0.0 : ControlRadius);
        const bool firstColumn = !viewOption || !viewOption->index.isValid()
            || viewOption->index.column() == 0;
        if (selected && comboPopup && firstColumn) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(enabled ? itemTokens.selectionAccent
                                       : itemTokens.accentFillDisabled);
            const qreal indicatorX = option->direction == Qt::RightToLeft
                ? itemRect.right() - 3.0 : itemRect.left();
            painter->drawRoundedRect(
                QRectF(indicatorX, option->rect.center().y() - 8.0,
                       3.0, 16.0), 1.5, 1.5);
            painter->restore();
        } else if (selected && popup && firstColumn) {
            const QRect checkRect = visualRect(option->direction, option->rect,
                QRect(option->rect.left() + 12,
                      option->rect.center().y() - 8, 16, 16));
            icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled).paint(painter,
                checkRect,
                Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        } else if (selected && viewOption && selectionMarkerView(widget)
                   && firstColumn) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(enabled ? itemTokens.selectionAccent
                                       : itemTokens.accentFillDisabled);
            const QRectF indicator = selectionMarkerRect(*viewOption, view);
            painter->drawRoundedRect(indicator, 1.5, 1.5);
            painter->restore();
        }
        return;
    }

    if (element == PE_IndicatorBranch) {
        if (!(option->state & State_Children))
            return;
        const bool open = option->state & State_Open;
        const Icon glyph = open ? Icon::ChevronDown
            : option->direction == Qt::RightToLeft ? Icon::ChevronLeft
                                                   : Icon::ChevronRight;
        const int extent = 12;
        icon(glyph, enabled ? t.textPrimary : t.textDisabled).paint(painter,
            QRect(option->rect.center().x() - extent / 2,
                  option->rect.center().y() - extent / 2, extent, extent),
            Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
        return;
    }

    if (element == PE_IndicatorHeaderArrow) {
        const Icon glyph = option->state & State_UpArrow
            ? Icon::ChevronUp : Icon::ChevronDown;
        icon(glyph, enabled ? t.textSecondary : t.textDisabled).paint(painter,
            QRect(option->rect.center().x() - 6, option->rect.center().y() - 6,
                  12, 12), Qt::AlignCenter,
            enabled ? QIcon::Normal : QIcon::Disabled);
        return;
    }

    if (element == PE_IndicatorToolBarSeparator) {
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        if (option->state & State_Horizontal) {
            const int x = option->rect.center().x();
            painter->drawLine(x, option->rect.top() + 8,
                              x, option->rect.bottom() - 8);
        } else {
            const int y = option->rect.center().y();
            painter->drawLine(option->rect.left() + 8, y,
                              option->rect.right() - 8, y);
        }
        painter->restore();
        return;
    }

    if (element == PE_FrameDockWidget) {
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(t.stroke, 1));
        painter->drawRect(option->rect.adjusted(0, 0, -1, -1));
        painter->restore();
        return;
    }

    if (element == PE_IndicatorDockWidgetResizeHandle) {
        QStyleOption splitter = *option;
        // Qt describes dock separators in the opposite axis to CE_Splitter.
        splitter.state.setFlag(State_Horizontal,
                               !(option->state & State_Horizontal));
        drawControl(CE_Splitter, &splitter, painter, widget);
        return;
    }

    Q_ASSERT_X(!coveredPrimitive(element), "WinUI3::Style::drawPrimitive",
               "a covered primitive reached QCommonStyle");
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawControl(ControlElement element, const QStyleOption *option,
                        QPainter *painter, const QWidget *widget) const
{
    using namespace Private;
    const Tokens t = tokens(option->palette);

    if (element == CE_ProgressBar) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            drawControl(CE_ProgressBarGroove, bar, painter, widget);
            drawControl(CE_ProgressBarContents, bar, painter, widget);
            if (bar->textVisible)
                drawControl(CE_ProgressBarLabel, bar, painter, widget);
            return;
        }
    }

    if (element == CE_TabBarTab) {
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            drawControl(CE_TabBarTabShape, tab, painter, widget);
            drawControl(CE_TabBarTabLabel, tab, painter, widget);
            return;
        }
    }

    if (element == CE_PushButton) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            drawPrimitive(PE_PanelButtonCommand, button, painter, widget);
            drawControl(CE_PushButtonLabel, button, painter, widget);
            if (button->features & QStyleOptionButton::HasMenu) {
                const QRect logical(button->rect.right() - 22,
                                    button->rect.center().y() - 7, 14, 14);
                icon(Icon::ChevronDown,
                     button->state & State_Enabled ? t.textPrimary : t.textDisabled)
                    .paint(painter, visualRect(button->direction, button->rect, logical),
                           Qt::AlignCenter,
                           button->state & State_Enabled
                               ? QIcon::Normal : QIcon::Disabled);
            }
            return;
        }
    }

    if ((element == CE_CheckBox && !toggleSwitch(widget))
        || element == CE_RadioButton) {
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool radio = element == CE_RadioButton;
            QStyleOptionButton indicator = *button;
            indicator.rect = subElementRect(
                radio ? SE_RadioButtonIndicator : SE_CheckBoxIndicator,
                button, widget);
            drawPrimitive(radio ? PE_IndicatorRadioButton : PE_IndicatorCheckBox,
                          &indicator, painter, widget);

            QRect contents = subElementRect(
                radio ? SE_RadioButtonContents : SE_CheckBoxContents,
                button, widget);
            const bool enabled = button->state & State_Enabled;
            if (!button->icon.isNull()) {
                const QSize iconSize = button->iconSize.isValid()
                    ? button->iconSize : QSize(16, 16);
                const QRect logical(contents.left(),
                    contents.center().y() - iconSize.height() / 2,
                    iconSize.width(), iconSize.height());
                const QRect iconRect = visualRect(button->direction, contents, logical);
                paintThemedIcon(painter, button->icon, iconRect, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    button->state & State_On ? QIcon::On : QIcon::Off);
                if (button->direction == Qt::RightToLeft)
                    contents.setRight(iconRect.left() - 6);
                else
                    contents.setLeft(iconRect.right() + 6);
            }
            painter->save();
            painter->setFont(widget ? widget->font() : QApplication::font());
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(contents,
                visualAlignment(button->direction, Qt::AlignLeft | Qt::AlignVCenter)
                    | Qt::TextShowMnemonic,
                button->text);
            if ((button->state & State_HasFocus) && keyboardFocusVisible(widget)) {
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(button->rect).adjusted(1, 1, -1, -1),
                                         5, 5);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(button->rect).adjusted(3, 3, -3, -3),
                                         3, 3);
            }
            painter->restore();
            return;
        }
    }

    if (element == CE_MenuBarItem) {
        if (const auto *item = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            // QMenuBar owns the animation properties, while this option is
            // painted once per QAction. Never let the shared bar progress
            // leak into a sibling item that is not active.
            const bool enabled = item->state & State_Enabled;
            const bool selected = enabled && (item->state & State_Selected);
            const bool sunken = enabled && (item->state & State_Sunken);
            const qreal hover = selected
                ? progress(widget, hoverProperty, 1.0) : 0.0;
            const qreal press = sunken
                ? progress(widget, pressProperty, 1.0) : 0.0;
            QColor fill = mix(Qt::transparent, t.subtleHover, hover);
            fill = mix(fill, t.subtlePressed, press);
            if (fill.alpha() > 0)
                roundedRect(painter, QRectF(item->rect).adjusted(2, 2, -2, -2),
                            fill, Qt::transparent, ControlRadius);
            painter->setFont(item->font);
            painter->setPen(item->state & State_Enabled
                                ? t.textPrimary : t.textDisabled);
            painter->drawText(item->rect.adjusted(10, 0, -10, 0),
                              Qt::AlignCenter | Qt::TextShowMnemonic
                                  | Qt::TextSingleLine,
                              item->text);
            return;
        }
    }

    if (element == CE_CheckBox && toggleSwitch(widget)) {
        if (const auto *check = qstyleoption_cast<const QStyleOptionButton *>(option)) {
            const bool enabled = check->state & State_Enabled;
            const bool checked = check->state & (State_On | State_NoChange);
            const qreal hover = progress(widget, hoverProperty,
                                         check->state & State_MouseOver ? 1.0 : 0.0);
            const qreal press = progress(widget, pressProperty,
                                         check->state & State_Sunken ? 1.0 : 0.0);
            const qreal position = progress(widget, togglePositionProperty,
                                            checked ? 1.0 : 0.0);
            const bool dragging = widget->property("_winui_toggle_dragging").toBool();

            const qreal trackLeft = check->direction == Qt::RightToLeft
                ? check->rect.right() - 39.5 : check->rect.left() + 0.5;
            const QRectF track(trackLeft,
                               check->rect.center().y() - 9.5, 39.0, 19.0);
            QColor trackFill;
            QColor trackStroke;
            QColor knob;
            if (!enabled) {
                trackFill = checked ? t.accentFillDisabled : Qt::transparent;
                trackStroke = withAlpha(t.strokeStrong, 40);
                knob = withAlpha(checked ? t.textOnAccentDisabled : t.textDisabled, 150);
            } else if (dragging) {
                const QColor off = t.dark ? QColor(0, 0, 0, 25)
                                          : QColor(0, 0, 0, 6);
                trackFill = mix(off, t.accentFillPressed, position);
                trackStroke = mix(t.strokeStrong, t.accentFillPressed, position);
                knob = mix(t.textSecondary, t.textOnAccentPrimary, position);
            } else if (checked) {
                trackFill = mix(t.accentFill, t.accentFillHover, hover * (1.0 - press));
                trackFill = mix(trackFill, t.accentFillPressed, press);
                trackStroke = trackFill;
                knob = t.textOnAccentPrimary;
            } else {
                const QColor off = t.dark ? QColor(0, 0, 0, 25)
                                          : QColor(0, 0, 0, 6);
                const QColor offHover = t.dark ? QColor(255, 255, 255, 11)
                                               : QColor(0, 0, 0, 15);
                const QColor offPressed = t.dark ? QColor(255, 255, 255, 18)
                                                 : QColor(0, 0, 0, 24);
                trackFill = mix(off, offHover, hover * (1.0 - press));
                trackFill = mix(trackFill, offPressed, press);
                trackStroke = t.strokeStrong;
                knob = t.textSecondary;
            }
            roundedRect(painter, track, trackFill, trackStroke, 10.0);

            const qreal knobWidth = 12.0 + 2.0 * hover + 3.0 * press;
            const qreal knobHeight = 12.0 + 2.0 * hover;
            qreal visualPosition = position;
            if (check->direction == Qt::RightToLeft)
                visualPosition = 1.0 - visualPosition;
            const qreal knobCenter = track.left() + 9.5 + 20.0 * visualPosition;
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(knob);
            painter->drawEllipse(QRectF(knobCenter - knobWidth / 2.0,
                                        track.center().y() - knobHeight / 2.0,
                                        knobWidth, knobHeight));
            painter->restore();

            const QVariant stateText = widget->property(
                checked ? ToggleSwitchOnTextProperty : ToggleSwitchOffTextProperty);
            const QString label = stateText.isValid() ? stateText.toString() : check->text;
            if (!label.isEmpty()) {
                painter->setFont(widget->font());
                painter->setPen(enabled ? t.textPrimary : t.textDisabled);
                const QRect labelRect = check->direction == Qt::RightToLeft
                    ? check->rect.adjusted(0, 0, -50, 0)
                    : check->rect.adjusted(50, 0, 0, 0);
                const Qt::Alignment horizontal = check->direction == Qt::RightToLeft
                    ? Qt::AlignRight : Qt::AlignLeft;
                painter->drawText(labelRect,
                                  horizontal | Qt::AlignVCenter
                                      | Qt::TextShowMnemonic,
                                  label);
            }

            if (keyboardFocusVisible(widget)) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2.0));
                painter->drawRoundedRect(track.adjusted(-3, -3, 3, 3), 12, 12);
                painter->setPen(QPen(t.focusInner, 1.0));
                painter->drawRoundedRect(track.adjusted(-1, -1, 1, 1), 11, 11);
                painter->restore();
            }
            return;
        }
    }

    if (element == CE_ShapedFrame
        && widget && widget->property(SettingsCardProperty).toBool()) {
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & State_MouseOver ? 1.0 : 0.0);
        const qreal press = progress(widget, pressProperty,
                                     option->state & State_Sunken ? 1.0 : 0.0);
        QColor fill = mix(t.control, t.controlHover, hover * (1.0 - press));
        fill = mix(fill, t.controlPressed, press);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary,
                       OverlayRadius);
        if (keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(2, 2, -3, -3),
                                     6, 6);
            painter->setPen(QPen(t.focusInner, 1.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(4, 4, -5, -5),
                                     5, 5);
            painter->restore();
        }
        return;
    }

    if (element == CE_ItemViewItem) {
        if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
            if (source->backgroundBrush.style() != Qt::NoBrush) {
                painter->fillRect(source->rect, source->backgroundBrush);
            } else if (source->features & QStyleOptionViewItem::Alternate) {
                painter->fillRect(source->rect,
                                  source->palette.brush(QPalette::AlternateBase));
            }
            drawPrimitive(PE_PanelItemViewItem, source, painter, widget);
            const QAbstractItemView *view = itemView(widget);
            const bool table = qobject_cast<const QTableView *>(view);
            const bool popup = widget && widget->window()
                && widget->window()->windowType() == Qt::Popup;
            const bool comboPopup = popup && widget->window()->parentWidget()
                && qobject_cast<const QComboBox *>(widget->window()->parentWidget());
            const Tokens itemTokens = tokens(option->palette);
            const bool enabled = source->state & State_Enabled;
            const bool checkedPopupSelection = popup && !comboPopup
                && (source->state & State_Selected);

            if (source->features & QStyleOptionViewItem::HasCheckIndicator) {
                QStyleOptionButton check;
                check.rect = subElementRect(SE_ItemViewItemCheckIndicator,
                                            source, widget);
                check.palette = source->palette;
                check.state = source->state;
                check.state.setFlag(State_Selected, false);
                check.state.setFlag(State_On,
                    source->checkState == Qt::Checked);
                check.state.setFlag(State_NoChange,
                    source->checkState == Qt::PartiallyChecked);
                check.state.setFlag(State_Off,
                    source->checkState == Qt::Unchecked);
                drawPrimitive(PE_IndicatorCheckBox, &check, painter, widget);
            }

            if ((source->features & QStyleOptionViewItem::HasDecoration)
                && !source->icon.isNull() && !checkedPopupSelection) {
                const QRect decoration = subElementRect(
                    SE_ItemViewItemDecoration, source, widget);
                paintThemedIcon(painter, source->icon, decoration,
                    source->decorationAlignment,
                    enabled ? itemTokens.textPrimary : itemTokens.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    source->state & State_Selected ? QIcon::On : QIcon::Off);
            }

            const bool tableEditing = table && (source->state & State_Editing);
            if ((source->features & QStyleOptionViewItem::HasDisplay)
                && !tableEditing) {
                const bool hasLeadingContent =
                    (source->features & QStyleOptionViewItem::HasDecoration)
                    || (source->features & QStyleOptionViewItem::HasCheckIndicator);
                QRect textRect = popup
                    ? source->rect.adjusted(
                        comboPopup && !hasLeadingContent ? 16 : 42, 0, -12, 0)
                    : subElementRect(SE_ItemViewItemText, source, widget);
                painter->save();
                painter->setFont(source->font);
                painter->setPen(enabled ? itemTokens.textPrimary
                                        : itemTokens.textDisabled);
                Qt::Alignment alignment = Qt::Alignment(source->displayAlignment)
                    | Qt::AlignVCenter;
                alignment = visualAlignment(source->direction, alignment);
                const bool wraps = source->features & QStyleOptionViewItem::WrapText;
                const int textFlags = int(alignment)
                    | (wraps ? int(Qt::TextWordWrap) : int(Qt::TextSingleLine));
                const QString text = wraps ? source->text
                    : source->fontMetrics.elidedText(source->text,
                        source->textElideMode, textRect.width());
                painter->drawText(textRect, textFlags, text);
                painter->restore();
            }

            if ((source->state & State_HasFocus) && keyboardFocusVisible(view)) {
                const bool tree = qobject_cast<const QTreeView *>(view);
                const bool table = qobject_cast<const QTableView *>(view);
                const QRectF focusRect = tree
                    ? QRectF(source->rect).adjusted(4, 2, -4, -2)
                    : table ? QRectF(source->rect).adjusted(1, 1, -1, -1)
                            : QRectF(source->rect).adjusted(2, 1, -2, -1);
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(itemTokens.focusOuter, 2.0));
                painter->drawRoundedRect(focusRect.adjusted(1, 1, -1, -1),
                                         table ? 0.0 : 5.0,
                                         table ? 0.0 : 5.0);
                painter->setPen(QPen(itemTokens.focusInner, 1.0));
                painter->drawRoundedRect(focusRect.adjusted(3, 3, -3, -3),
                                         table ? 0.0 : 3.0,
                                         table ? 0.0 : 3.0);
                painter->restore();
            }
            return;
        }
    }

    if (element == CE_ComboBoxLabel) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (combo->editable)
                return;
            QRect content = subControlRect(CC_ComboBox, combo,
                                            SC_ComboBoxEditField, widget);
            if (!combo->currentIcon.isNull()) {
                const QSize iconSize = combo->iconSize.isValid() ? combo->iconSize : QSize(16, 16);
                const QRect logicalIcon(content.left(),
                                        content.center().y() - iconSize.height() / 2,
                                        iconSize.width(), iconSize.height());
                const QRect iconRect = visualRect(option->direction, content,
                                                  logicalIcon);
                paintThemedIcon(painter, combo->currentIcon, iconRect,
                    Qt::AlignCenter,
                    option->state & State_Enabled ? t.textPrimary : t.textDisabled,
                    option->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
                if (option->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            painter->setPen(option->state & State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(content,
                              visualAlignment(option->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              option->fontMetrics.elidedText(combo->currentText,
                                                              Qt::ElideRight, content.width()));
            return;
        }
    }

    if (element == CE_PushButtonLabel || element == CE_ToolButtonLabel) {
        if (element == CE_PushButtonLabel) {
            if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option)) {
                const bool enabled = button->state & State_Enabled;
                const ControlRole role = controlRole(widget);
                const bool accent = role == ControlRole::Accent
                    || role == ControlRole::Destructive
                    || ((button->state & State_On) && role == ControlRole::Standard);
                const QColor textColor = !enabled ? t.textDisabled
                    : accent ? t.textOnAccentPrimary : t.textPrimary;
                QRect content = subElementRect(SE_PushButtonContents, button, widget);
                if (button->features & QStyleOptionButton::HasMenu) {
                    const QRect logical = content.adjusted(0, 0, -22, 0);
                    content = visualRect(button->direction, button->rect, logical);
                }
                const QFontMetrics metrics(button->fontMetrics);
                const int textWidth = button->text.isEmpty()
                    ? 0 : metrics.horizontalAdvance(button->text);
                const QSize iconSize = button->iconSize.isValid()
                    ? button->iconSize : QSize(16, 16);
                const bool hasIcon = !button->icon.isNull();
                const int gap = hasIcon && textWidth > 0 ? 8 : 0;
                const int totalWidth = (hasIcon ? iconSize.width() : 0)
                    + gap + textWidth;
                const int logicalStart = content.left()
                    + qMax(0, (content.width() - totalWidth) / 2);
                if (hasIcon) {
                    const QRect iconRect = visualRect(button->direction, content,
                        QRect(logicalStart,
                              content.center().y() - iconSize.height() / 2,
                              iconSize.width(), iconSize.height()));
                    paintThemedIcon(painter, button->icon, iconRect,
                        Qt::AlignCenter, textColor,
                        enabled ? QIcon::Normal : QIcon::Disabled,
                        button->state & State_On ? QIcon::On : QIcon::Off);
                }
                if (textWidth > 0) {
                    const QRect textRect = visualRect(button->direction, content,
                        QRect(logicalStart
                                  + (hasIcon ? iconSize.width() + gap : 0),
                              content.top(), textWidth, content.height()));
                    painter->save();
                    painter->setFont(widget ? widget->font() : QApplication::font());
                    painter->setPen(textColor);
                    painter->drawText(textRect,
                                      Qt::AlignCenter | Qt::TextShowMnemonic,
                                      button->text);
                    painter->restore();
                }
                return;
            }
        } else if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const bool enabled = tool->state & State_Enabled;
            const ControlRole role = controlRole(widget);
            const bool accent = role == ControlRole::Accent
                || role == ControlRole::Destructive
                || ((tool->state & State_On) && role == ControlRole::Standard);
            const QColor textColor = !enabled ? t.textDisabled
                : accent ? t.textOnAccentPrimary : t.textPrimary;
            const QRectF content = QRectF(subControlRect(CC_ToolButton, tool,
                                                         SC_ToolButton, widget))
                                       .adjusted(4.0, 2.0, -4.0, -2.0);
            Qt::ToolButtonStyle buttonStyle = Qt::ToolButtonIconOnly;
            if (const auto *toolButton = qobject_cast<const QToolButton *>(widget))
                buttonStyle = toolButton->toolButtonStyle();
            if (buttonStyle == Qt::ToolButtonFollowStyle)
                buttonStyle = Qt::ToolButtonIconOnly;
            const bool hasIcon = !tool->icon.isNull();
            const QSize iconSize = tool->iconSize.isValid() ? tool->iconSize : QSize(16, 16);
            const QFontMetrics metrics(tool->fontMetrics);
            const int textWidth = tool->text.isEmpty()
                ? 0 : metrics.horizontalAdvance(tool->text);
            if (buttonStyle == Qt::ToolButtonTextOnly)
                painter->setPen(textColor);
            if (buttonStyle == Qt::ToolButtonTextOnly || !hasIcon) {
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(content, Qt::AlignCenter | Qt::TextShowMnemonic,
                                  tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextBesideIcon && textWidth > 0) {
                const qreal total = iconSize.width() + 6.0 + textWidth;
                const qreal logicalStart = content.left()
                    + qMax<qreal>(0.0, (content.width() - total) / 2.0);
                const QRectF iconRect = visualRectF(tool->direction, content,
                    QRectF(logicalStart,
                           content.center().y() - iconSize.height() / 2.0,
                           iconSize.width(), iconSize.height()));
                paintThemedIcon(painter, tool->icon,
                    iconRect, Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & State_On ? QIcon::On : QIcon::Off);
                const QRectF textRect = visualRectF(tool->direction, content,
                    QRectF(logicalStart + iconSize.width() + 6.0, content.top(),
                           textWidth, content.height()));
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(textRect,
                                  Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (buttonStyle == Qt::ToolButtonTextUnderIcon && textWidth > 0) {
                paintThemedIcon(painter, tool->icon,
                    QRectF(content.center().x() - iconSize.width() / 2.0,
                           content.top(), iconSize.width(), iconSize.height()),
                    Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & State_On ? QIcon::On : QIcon::Off);
                painter->save();
                painter->setFont(tool->font);
                painter->setPen(textColor);
                painter->drawText(content.adjusted(0, iconSize.height(), 0, 0),
                                  Qt::AlignCenter | Qt::TextShowMnemonic, tool->text);
                painter->restore();
            } else if (hasIcon) {
                paintThemedIcon(painter, tool->icon,
                    QRectF(content.center().x() - iconSize.width() / 2.0,
                           content.center().y() - iconSize.height() / 2.0,
                           iconSize.width(), iconSize.height()), Qt::AlignCenter, textColor,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    tool->state & State_On ? QIcon::On : QIcon::Off);
            }
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                const QRect menuRect = subControlRect(CC_ToolButton, tool,
                                                       SC_ToolButtonMenu, widget);
                icon(Icon::ChevronDown, textColor).paint(painter, menuRect, Qt::AlignCenter,
                                             enabled ? QIcon::Normal : QIcon::Disabled);
            }
            return;
        }
    }

    if (element == CE_ProgressBarGroove) {
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
        const bool horizontal = !progressBar
            || progressBar->orientation() == Qt::Horizontal;
        const QRect groove = horizontal
            ? QRect(option->rect.left(), option->rect.center().y() - 2,
                    option->rect.width(), 4)
            : QRect(option->rect.center().x() - 2, option->rect.top(),
                    4, option->rect.height());
        roundedRect(painter, groove, t.stroke, Qt::transparent, 2);
        return;
    }

    if (element == CE_ProgressBarContents) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
            const bool horizontal = !progressBar
                || progressBar->orientation() == Qt::Horizontal;
            const bool inverted = progressBar ? progressBar->invertedAppearance()
                                               : bar->invertedAppearance;
            const QRect track = horizontal
                ? QRect(option->rect.left(), option->rect.center().y() - 2,
                        option->rect.width(), 4)
                : QRect(option->rect.center().x() - 2, option->rect.top(),
                        4, option->rect.height());
            const QColor indicatorColor = bar->state & State_Enabled
                ? t.accentFill : t.accentFillDisabled;
            if (bar->minimum == 0 && bar->maximum == 0) {
                // ProgressRing-style indeterminate progress uses two
                // independently moving indicators.  The phase is advanced
                // by a widget-owned timer, so every repaint is intentional
                // and capture mode can freeze it at a stable value.
                const qreal phase = animationsAllowed()
                    ? qBound<qreal>(0.0, progress(widget, progressPhaseProperty, 0.0), 1.0)
                    : 0.35;
                const int axis = horizontal ? track.width() : track.height();
                const int firstLength = qMax(12, axis / 4);
                const int secondLength = qMax(10, axis / 6);
                const bool reverse = horizontal
                    ? (inverted
                       != (bar->direction == Qt::RightToLeft))
                    : !bar->invertedAppearance;
                const auto drawIndicator = [&](int length, qreal offset) {
                    const qreal travel = axis + length;
                    const int distance = qRound(travel * std::fmod(phase + offset, 1.0));
                    QRect indicator = track;
                    if (horizontal) {
                        const int left = reverse ? axis - distance : distance - length;
                        indicator.setLeft(track.left() + left);
                        indicator.setWidth(length);
                    } else {
                        const int top = reverse ? axis - distance : distance - length;
                        indicator.setTop(track.top() + top);
                        indicator.setHeight(length);
                    }
                    indicator = indicator.intersected(track);
                    if (!indicator.isEmpty())
                        roundedRect(painter, indicator, indicatorColor,
                                    Qt::transparent, 2);
                };
                drawIndicator(firstLength, 0.0);
                drawIndicator(secondLength, 0.5);
                return;
            }
            const qint64 range = qint64(bar->maximum) - qint64(bar->minimum);
            const qint64 value = qint64(bar->progress) - qint64(bar->minimum);
            const qreal ratio = range > 0
                ? qBound<qreal>(0, qreal(value) / qreal(range), 1)
                : 0;
            QRect fill = track;
            if (horizontal) {
                const int length = qRound(track.width() * ratio);
                if (inverted
                    != (bar->direction == Qt::RightToLeft))
                    fill.setLeft(track.right() - length + 1);
                else
                    fill.setWidth(length);
            } else {
                const int length = qRound(track.height() * ratio);
                if (inverted)
                    fill.setTop(track.bottom() - length + 1);
                else
                    fill.setHeight(length);
            }
            if (!fill.isEmpty())
                roundedRect(painter, fill, indicatorColor, Qt::transparent, 2);
            return;
        }
    }

    if (element == CE_ProgressBarLabel)
        return;

    if (element == CE_ToolBar) {
        return;
    }

    if (element == CE_Splitter) {
        const qreal hover = progress(widget, hoverProperty,
                                     option->state & State_MouseOver ? 1.0 : 0.0);
        const qreal press = progress(widget, pressProperty,
                                     option->state & State_Sunken ? 1.0 : 0.0);
        QColor color = mix(t.stroke, t.strokeStrong, hover);
        color = mix(color, t.accentFill, press);
        const qreal thickness = 1.0 + hover + press;
        const bool horizontal = option->state & State_Horizontal;
        QRectF handle;
        if (horizontal) {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.height() - 12));
            handle = QRectF(option->rect.center().x() - thickness / 2.0,
                            option->rect.center().y() - length / 2.0,
                            thickness, length);
        } else {
            const qreal length = qMin<qreal>(100.0, qMax(0, option->rect.width() - 12));
            handle = QRectF(option->rect.center().x() - length / 2.0,
                            option->rect.center().y() - thickness / 2.0,
                            length, thickness);
        }
        handle = snappedSplitterGrip(handle, horizontal, painter);
        roundedRect(painter, handle, color, Qt::transparent, thickness / 2.0);
        return;
    }

    if (element == CE_DockWidgetTitle) {
        if (const auto *dock = qstyleoption_cast<const QStyleOptionDockWidget *>(option)) {
            painter->save();
            painter->fillRect(dock->rect, t.layer);
            painter->setPen(t.stroke);
            if (dock->verticalTitleBar)
                painter->drawLine(dock->rect.topRight(), dock->rect.bottomRight());
            else
                painter->drawLine(dock->rect.bottomLeft(), dock->rect.bottomRight());

            QRect titleRect = subElementRect(SE_DockWidgetTitleBarText, dock, widget);
            if (dock->verticalTitleBar) {
                const QRect transposed = dock->rect.transposed();
                titleRect = QRect(transposed.left() + dock->rect.bottom() - titleRect.bottom(),
                                  transposed.top() + titleRect.left() - dock->rect.left(),
                                  titleRect.height(), titleRect.width());
                painter->translate(transposed.left(), transposed.top() + transposed.width());
                painter->rotate(-90);
                painter->translate(-transposed.left(), -transposed.top());
            }
            QFont font = widget ? widget->font() : QApplication::font();
            font.setWeight(QFont::DemiBold);
            painter->setFont(font);
            painter->setPen(dock->state & State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(titleRect,
                              visualAlignment(dock->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              painter->fontMetrics().elidedText(
                                  dock->title, Qt::ElideRight, titleRect.width()));
            painter->restore();
            return;
        }
    }

    if (element == CE_MenuBarEmptyArea) {
        return;
    }

    if (element == CE_TabBarTabShape) {
        const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option);
        const bool selected = option->state & State_Selected;
        const bool north = !tab || tab->shape == QTabBar::RoundedNorth
            || tab->shape == QTabBar::TriangularNorth;
        if (selected && north) {
            const QColor selectedFill = t.dark ? QColor(44, 44, 44)
                                                : QColor(251, 251, 251);
            const QRectF rect = QRectF(option->rect).adjusted(1, 1, -1, 0);
            const qreal radius = 8.0;
            QPainterPath surface;
            surface.moveTo(rect.left(), rect.bottom());
            surface.lineTo(rect.left(), rect.top() + radius);
            surface.quadTo(rect.left(), rect.top(), rect.left() + radius,
                           rect.top());
            surface.lineTo(rect.right() - radius, rect.top());
            surface.quadTo(rect.right(), rect.top(), rect.right(),
                           rect.top() + radius);
            surface.lineTo(rect.right(), rect.bottom());
            surface.lineTo(rect.left(), rect.bottom());
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->fillPath(surface, selectedFill);
            QPainterPath border;
            border.moveTo(rect.left(), rect.bottom());
            border.lineTo(rect.left(), rect.top() + radius);
            border.quadTo(rect.left(), rect.top(), rect.left() + radius,
                          rect.top());
            border.lineTo(rect.right() - radius, rect.top());
            border.quadTo(rect.right(), rect.top(), rect.right(),
                          rect.top() + radius);
            border.lineTo(rect.right(), rect.bottom());
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.stroke, 1));
            painter->drawPath(border);
            painter->restore();
        } else {
            QColor fill = Qt::transparent;
            if (option->state & State_Sunken)
                fill = t.controlPressed;
            else if (option->state & State_MouseOver)
                fill = t.layer;
            roundedRect(painter, QRectF(option->rect).adjusted(2, 2, -2, -2),
                        fill, Qt::transparent, ControlRadius);
            if (!selected && !(option->state & State_MouseOver)) {
                painter->setPen(t.stroke);
                const int separatorX = option->direction == Qt::RightToLeft
                    ? option->rect.left() : option->rect.right();
                painter->drawLine(separatorX, option->rect.top() + 8,
                                  separatorX, option->rect.bottom() - 8);
            }
        }
        if ((option->state & State_HasFocus) && keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                     ControlRadius, ControlRadius);
            painter->setPen(QPen(t.focusInner, 1));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(5, 5, -5, -5),
                                     ControlRadius - 1, ControlRadius - 1);
            painter->restore();
        }
        return;
    }

    if (element == CE_TabBarTabLabel) {
        if (const auto *tab = qstyleoption_cast<const QStyleOptionTab *>(option)) {
            const bool enabled = tab->state & State_Enabled;
            const bool selected = tab->state & State_Selected;
            QRect textRect = tab->rect.adjusted(8, 0, -8, 0);
            if (tab->leftButtonSize.isValid())
                textRect.adjust(tab->leftButtonSize.width() + 4, 0, 0, 0);
            if (tab->rightButtonSize.isValid())
                textRect.adjust(0, 0, -tab->rightButtonSize.width() - 4, 0);
            painter->save();
            QFont font = widget ? widget->font() : QApplication::font();
            font.setPixelSize(12);
            font.setWeight(selected ? QFont::DemiBold : QFont::Normal);
            painter->setFont(font);
            painter->setPen(enabled
                ? (selected ? t.textPrimary : t.textSecondary)
                : t.textDisabled);
            if (!tab->icon.isNull()) {
                const QRect logicalIcon(textRect.left(), textRect.center().y() - 8,
                                        16, 16);
                const QRect iconRect = visualRect(tab->direction, tab->rect,
                                                  logicalIcon);
                paintThemedIcon(painter, tab->icon, iconRect, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled,
                    selected ? QIcon::On : QIcon::Off);
                if (tab->direction == Qt::RightToLeft)
                    textRect.setRight(iconRect.left() - 10);
                else
                    textRect.setLeft(iconRect.right() + 10);
            }
            painter->drawText(textRect,
                              visualAlignment(tab->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              tab->fontMetrics.elidedText(tab->text, Qt::ElideRight,
                                                          textRect.width()));
            painter->restore();
            return;
        }
    }

    if (element == CE_HeaderSection) {
        QColor fill = t.layer;
        if (option->state & State_Sunken)
            fill = t.subtlePressed;
        else if (option->state & State_MouseOver)
            fill = t.subtleHover;
        painter->fillRect(option->rect, fill);
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        painter->drawLine(option->rect.bottomLeft(), option->rect.bottomRight());
        const int separatorX = option->direction == Qt::RightToLeft
            ? option->rect.left() : option->rect.right();
        painter->drawLine(separatorX, option->rect.top(),
                          separatorX, option->rect.bottom());
        painter->restore();
        return;
    }

    if (element == CE_Header) {
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            drawControl(CE_HeaderSection, header, painter, widget);
            QStyleOptionHeader label = *header;
            label.sortIndicator = QStyleOptionHeader::None;
            if (header->sortIndicator != QStyleOptionHeader::None) {
                const QRect arrowRect = headerSortIndicatorRect(*header);
                if (header->direction == Qt::RightToLeft)
                    label.rect.setLeft(qMin(label.rect.right(), arrowRect.right() + 6));
                else
                    label.rect.setRight(qMax(label.rect.left(), arrowRect.left() - 6));
            }
            drawControl(CE_HeaderLabel, &label, painter, widget);
            if (header->sortIndicator != QStyleOptionHeader::None) {
                QStyleOption arrow;
                arrow.rect = headerSortIndicatorRect(*header);
                arrow.palette = header->palette;
                arrow.state = header->state;
                arrow.state.setFlag(State_UpArrow,
                    header->sortIndicator == QStyleOptionHeader::SortUp);
                drawPrimitive(PE_IndicatorHeaderArrow, &arrow, painter, widget);
            }
            return;
        }
    }

    if (element == CE_HeaderLabel) {
        if (const auto *header = qstyleoption_cast<const QStyleOptionHeader *>(option)) {
            QRect content = header->rect.adjusted(12, 0, -10, 0);
            painter->save();
            QFont font = widget ? widget->font() : QApplication::font();
            font.setPixelSize(12);
            font.setWeight(QFont::DemiBold);
            painter->setFont(font);
            painter->setPen(header->state & State_Enabled
                                ? t.textSecondary : t.textDisabled);
            if (!header->icon.isNull()) {
                const QRect logicalIcon(content.left(), content.center().y() - 8,
                                        16, 16);
                const QRect iconRect = visualRect(header->direction, header->rect,
                                                  logicalIcon);
                paintThemedIcon(painter, header->icon, iconRect,
                    Qt::AlignCenter, header->state & State_Enabled
                        ? t.textSecondary : t.textDisabled,
                    header->state & State_Enabled
                        ? QIcon::Normal : QIcon::Disabled);
                if (header->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            const Qt::Alignment horizontal = header->textAlignment
                & (Qt::AlignLeft | Qt::AlignRight | Qt::AlignHCenter);
            painter->drawText(content,
                visualAlignment(header->direction, horizontal | Qt::AlignVCenter)
                    | Qt::TextSingleLine,
                header->fontMetrics.elidedText(header->text, Qt::ElideRight,
                                                content.width()));
            painter->restore();
            return;
        }
        return;
    }

    if (element == CE_MenuItem) {
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            const bool comboItem = qobject_cast<const QComboBox *>(widget);
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                painter->setPen(t.stroke);
                painter->drawLine(menu->rect.left() + 12, menu->rect.center().y(),
                                  menu->rect.right() - 12, menu->rect.center().y());
                return;
            }
            if (menu->checked || menu->state & (State_Selected | State_Sunken))
                roundedRect(painter,
                            QRectF(menu->rect).adjusted(comboItem ? 5 : 4, 2,
                                                       comboItem ? -5 : -4, -2),
                            menu->state & State_Sunken ? t.subtlePressed
                                                      : t.subtleHover,
                            Qt::transparent, ControlRadius);

            const bool enabled = menu->state & State_Enabled;
            const QRect leading = visualRect(menu->direction, menu->rect,
                                             QRect(menu->rect.left() + 12,
                                                   menu->rect.center().y() - 8,
                                                   16, 16));
            if (comboItem && menu->checked) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(Qt::NoPen);
                painter->setBrush(enabled ? t.selectionAccent : t.accentFillDisabled);
                const qreal x = leading.left()
                    + (menu->direction == Qt::RightToLeft ? 13.0 : -6.0);
                painter->drawRoundedRect(
                    QRectF(x, menu->rect.center().y() - 8.0, 3.0, 16.0),
                    1.5, 1.5);
                painter->restore();
            } else if (menu->checked) {
                icon(Icon::Check, enabled ? t.textPrimary : t.textDisabled).paint(painter,
                    leading,
                    Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            } else if (!menu->icon.isNull()) {
                paintThemedIcon(painter, menu->icon, leading, Qt::AlignCenter,
                    enabled ? t.textPrimary : t.textDisabled,
                    enabled ? QIcon::Normal : QIcon::Disabled);
            }
            const QStringList parts = menu->text.split(QLatin1Char('\t'));
            painter->setFont(menu->font);
            painter->setPen(enabled ? t.textPrimary : t.textDisabled);
            const QFontMetrics metrics(menu->font);
            const int submenuWidth = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? 24 : 0;
            const int shortcutWidth = parts.size() > 1
                ? metrics.horizontalAdvance(parts.value(1)) : 0;
            const int shortcutRight = menu->rect.right() - 16 - submenuWidth;
            const int shortcutLeft = shortcutRight - shortcutWidth;
            const int textRight = shortcutWidth > 0
                ? shortcutLeft - 20 : shortcutRight;
            const int textLeft = comboItem && menu->icon.isNull() ? 16 : 42;
            const QRect textRect = visualRect(menu->direction, menu->rect,
                QRect(menu->rect.left() + textLeft, menu->rect.top(),
                      qMax(0, textRight - menu->rect.left() - textLeft + 1),
                      menu->rect.height()));
            painter->drawText(textRect,
                              visualAlignment(menu->direction,
                                              Qt::AlignLeft | Qt::AlignVCenter),
                              metrics.elidedText(parts.value(0), Qt::ElideRight,
                                                 textRect.width()));
            if (parts.size() > 1) {
                painter->setPen(enabled ? t.textSecondary : t.textDisabled);
                const QRect shortcutRect = visualRect(menu->direction, menu->rect,
                    QRect(shortcutLeft, menu->rect.top(), shortcutWidth,
                          menu->rect.height()));
                painter->drawText(shortcutRect,
                                  visualAlignment(menu->direction,
                                                  Qt::AlignRight | Qt::AlignVCenter),
                                  parts.value(1));
            }
            if (menu->menuItemType == QStyleOptionMenuItem::SubMenu) {
                const QRect submenu = visualRect(menu->direction, menu->rect,
                    QRect(menu->rect.right() - 24, menu->rect.center().y() - 8,
                          16, 16));
                icon(menu->direction == Qt::RightToLeft ? Icon::ChevronLeft
                                                         : Icon::ChevronRight,
                     enabled ? t.textPrimary : t.textDisabled).paint(painter, submenu,
                    Qt::AlignCenter, enabled ? QIcon::Normal : QIcon::Disabled);
            }
            return;
        }
    }

    Q_ASSERT_X(!coveredControl(element), "WinUI3::Style::drawControl",
               "a covered control reached QCommonStyle");
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                               QPainter *painter, const QWidget *widget) const
{
    using namespace Private;
    const Tokens t = tokens(option->palette);

    if (control == CC_ToolButton) {
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            drawPrimitive(PE_PanelButtonTool, tool, painter, widget);
            drawControl(CE_ToolButtonLabel, tool, painter, widget);
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup) {
                const QRect menuRect = subControlRect(CC_ToolButton, tool,
                                                       SC_ToolButtonMenu, widget);
                painter->save();
                painter->setPen(t.stroke);
                const int x = option->direction == Qt::RightToLeft
                    ? menuRect.right() : menuRect.left();
                painter->drawLine(x, menuRect.top() + 5, x, menuRect.bottom() - 5);
                painter->restore();
            }
            return;
        }
    }

    if (control == CC_GroupBox) {
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const bool enabled = group->state & State_Enabled;
            roundedRect(painter, group->rect, t.layer, t.stroke, 6.0);

            if (group->subControls & SC_GroupBoxCheckBox) {
                QStyleOptionButton indicator;
                indicator.rect = subControlRect(CC_GroupBox, group,
                                                SC_GroupBoxCheckBox, widget);
                indicator.state = group->state;
                indicator.palette = group->palette;
                drawPrimitive(PE_IndicatorCheckBox, &indicator, painter, widget);
            }
            if (group->subControls & SC_GroupBoxLabel) {
                const QRect label = subControlRect(CC_GroupBox, group,
                                                   SC_GroupBoxLabel, widget);
                painter->save();
                QFont titleFont = widget ? widget->font() : QApplication::font();
                titleFont.setWeight(QFont::DemiBold);
                painter->setFont(titleFont);
                painter->setPen(enabled ? t.textPrimary : t.textDisabled);
                painter->drawText(label,
                                  visualAlignment(group->direction,
                                                  Qt::AlignLeft | Qt::AlignVCenter),
                                  group->text);
                painter->restore();
            }
            return;
        }
    }

    if (control == CC_ComboBox) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            const bool enabled = combo->state & State_Enabled;
            const bool hovered = combo->state & State_MouseOver;
            const bool pressed = combo->state & (State_Sunken | State_On);
            const qreal hover = progress(widget, hoverProperty, hovered ? 1.0 : 0.0);
            const qreal press = progress(widget, pressProperty, pressed ? 1.0 : 0.0);
            QColor fill = enabled ? t.control : t.controlDisabled;
            fill = mix(fill, t.controlHover, hover);
            fill = mix(fill, t.controlPressed, press);
            if (combo->subControls & SC_ComboBoxFrame)
                controlSurface(painter, combo->rect, fill, t.stroke, t.strokeSecondary,
                               ControlRadius);
            if (combo->subControls & SC_ComboBoxArrow) {
                // Keep the chevron rectangle logical; paintThemedIcon resolves
                // the pixmap's DPR without changing this geometry.
                const QRect logicalArrow(combo->rect.right() - 37,
                                         combo->rect.top(), 38,
                                         combo->rect.height());
                const QRect logicalChevron(
                    logicalArrow.left() + (logicalArrow.width() - 12) / 2,
                    logicalArrow.top() + (logicalArrow.height() - 12) / 2,
                    12, 12);
                const QRect chevronRect = visualRect(combo->direction,
                                                     combo->rect,
                                                     logicalChevron);
                const qreal chevron = progress(widget, comboChevronProperty, 0.0);
                painter->save();
                painter->translate(0.0, 1.875 * chevron);
                paintThemedIcon(painter, icon(Icon::ChevronDown), chevronRect,
                                Qt::AlignCenter,
                                enabled ? t.textPrimary : t.textDisabled,
                                enabled ? QIcon::Normal : QIcon::Disabled);
                painter->restore();
            }
            if (keyboardFocusVisible(widget)) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(combo->rect).adjusted(1, 1, -1, -1), 7, 7);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(combo->rect).adjusted(3, 3, -3, -3), 5, 5);
                painter->restore();
            }
            return;
        }
    }
    if (control == CC_SpinBox) {
        if (const auto *spin = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            const bool enabled = spin->state & State_Enabled;
            const bool focused = spin->state & State_HasFocus;
            const bool verticalButtons = verticalSpinButtons(widget);
            const qreal hover = progress(widget, hoverProperty,
                                         spin->state & State_MouseOver ? 1.0 : 0.0);
            QColor fill = !enabled ? t.controlDisabled
                : focused ? (t.dark ? QColor(30, 30, 30, 179) : QColor(255, 255, 255))
                          : mix(t.control, t.controlHover, hover);
            controlSurface(painter, spin->rect, fill, t.stroke, t.strokeSecondary,
                           ControlRadius);
            if (verticalButtons) {
                const QRect editField = subControlRect(CC_SpinBox, spin,
                                                       SC_SpinBoxEditField,
                                                       widget);
                const int separatorX = spin->direction == Qt::RightToLeft
                    ? editField.left() : editField.right();
                painter->save();
                painter->setPen(QPen(t.stroke, 1, Qt::SolidLine, Qt::FlatCap));
                painter->drawLine(separatorX, spin->rect.top() + 1,
                                  separatorX, spin->rect.bottom() - 1);
                painter->restore();
            }
            if (focused) {
                painter->save();
                painter->setRenderHint(QPainter::Antialiasing);
                painter->setPen(QPen(t.accentFill, 2, Qt::SolidLine, Qt::FlatCap));
                const int underlineY = spin->rect.bottom() - 1;
                painter->drawLine(spin->rect.left(), underlineY,
                                  spin->rect.right(), underlineY);
                painter->restore();
            }
            const auto drawStep = [&](SubControl subControl, Icon glyph) {
                if (!(spin->subControls & subControl)) return;
                const QRect rect = subControlRect(CC_SpinBox, spin, subControl, widget);
                const bool stepEnabled = enabled && (subControl == SC_SpinBoxUp
                    ? spin->stepEnabled & QAbstractSpinBox::StepUpEnabled
                    : spin->stepEnabled & QAbstractSpinBox::StepDownEnabled);
                const QRectF visualRect = verticalButtons
                    ? (subControl == SC_SpinBoxUp
                        ? QRectF(rect).adjusted(4, 3, -4, 0)
                        : QRectF(rect).adjusted(4, 0, -4, -3))
                    : (subControl == SC_SpinBoxUp
                        ? QRectF(rect).adjusted(4, 4, 0, -4)
                        : QRectF(rect).adjusted(0, 4, -4, -4));
                if (stepEnabled && (spin->activeSubControls & subControl)
                    && (spin->state & State_MouseOver)) {
                    roundedRect(painter, visualRect,
                                spin->state & State_Sunken ? t.subtlePressed : t.subtleHover,
                                Qt::transparent, ControlRadius);
                }
                const QPoint center = visualRect.center().toPoint();
                icon(glyph, stepEnabled ? t.textPrimary : t.textDisabled).paint(
                    painter, QRect(center.x() - 6, center.y() - 6, 12, 12),
                    Qt::AlignCenter, stepEnabled ? QIcon::Normal : QIcon::Disabled);
            };
            drawStep(SC_SpinBoxUp, Icon::ChevronUp);
            drawStep(SC_SpinBoxDown, Icon::ChevronDown);
            return;
        }
    }

    if (control == CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = slider->orientation == Qt::Horizontal;
            QRect groove = subControlRect(CC_Slider, slider, SC_SliderGroove, widget);
            const QRect handle = subControlRect(CC_Slider, slider, SC_SliderHandle,
                                                widget);
            const qreal hover = progress(widget, hoverProperty,
                option->state & State_MouseOver ? 1.0 : 0.0);
            const qreal pressed = progress(widget, pressProperty,
                option->state & State_Sunken ? 1.0 : 0.0);
            const bool enabled = option->state & State_Enabled;
            QColor track = enabled ? t.strokeStrong : t.textDisabled;
            QColor valueColor = enabled
                ? Private::mix(t.accentFill, t.accentFillHover, hover)
                : t.accentFillDisabled;
            valueColor = Private::mix(valueColor, t.accentFillPressed, pressed);
            roundedRect(painter, groove, track, Qt::transparent, 2);

            QRect value = groove;
            if (horizontal) {
                if (slider->upsideDown)
                    value.setLeft(handle.center().x());
                else
                    value.setRight(handle.center().x());
            } else {
                if (slider->upsideDown)
                    value.setTop(handle.center().y());
                else
                    value.setBottom(handle.center().y());
            }
            roundedRect(painter, value, valueColor, Qt::transparent, 2);

            if (slider->tickPosition != QSlider::NoTicks
                && slider->maximum > slider->minimum) {
                const qint64 minimum = slider->minimum;
                const qint64 maximum = slider->maximum;
                const qint64 range = maximum - minimum;
                const qint64 requested = slider->tickInterval > 0
                    ? qint64(slider->tickInterval)
                    : qint64(qMax(1, slider->pageStep));
                const qint64 interval = qMax<qint64>(1, qMax(requested,
                    (range + 99) / 100));
                painter->save();
                painter->setPen(QPen(enabled ? t.strokeStrong : t.textDisabled, 1));
                for (qint64 tick = minimum;;) {
                    const int span = horizontal ? groove.width() - 1
                                                 : groove.height() - 1;
                    const int offset = QStyle::sliderPositionFromValue(
                        slider->minimum, slider->maximum,
                        int(qBound(minimum, tick, maximum)), span,
                        slider->upsideDown);
                    if (horizontal) {
                        const int x = groove.left() + offset;
                        if (slider->tickPosition & QSlider::TicksAbove)
                            painter->drawLine(x, groove.top() - 8, x,
                                              groove.top() - 5);
                        if (slider->tickPosition & QSlider::TicksBelow)
                            painter->drawLine(x, groove.bottom() + 5, x,
                                              groove.bottom() + 8);
                    } else {
                        const int y = groove.top() + offset;
                        if (slider->tickPosition & QSlider::TicksLeft)
                            painter->drawLine(groove.left() - 8, y,
                                              groove.left() - 5, y);
                        if (slider->tickPosition & QSlider::TicksRight)
                            painter->drawLine(groove.right() + 5, y,
                                              groove.right() + 8, y);
                    }
                    if (tick >= maximum || interval > maximum - tick)
                        break;
                    tick += interval;
                }
                painter->restore();
            }

            qreal innerDiameter = 10.32 + (14.0 - 10.32) * hover;
            innerDiameter += (8.52 - innerDiameter) * pressed;
            if (!enabled)
                innerDiameter = 14.0;
            QColor thumbColor = enabled ? valueColor : t.accentFillDisabled;
            const QColor outerThumb = t.dark ? QColor(69, 69, 69)
                                             : QColor(255, 255, 255);
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(outerThumb);
            painter->setPen(QPen(t.strokeSecondary, 1));
            painter->drawEllipse(QPointF(handle.center()), 10.5, 10.5);
            painter->setBrush(thumbColor);
            painter->setPen(Qt::NoPen);
            painter->drawEllipse(QPointF(handle.center()), innerDiameter / 2.0,
                                 innerDiameter / 2.0);
            if ((option->state & State_HasFocus) && keyboardFocusVisible(widget)) {
                painter->setBrush(Qt::NoBrush);
                painter->setPen(QPen(t.focusOuter, 2));
                painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1),
                                         ControlRadius, ControlRadius);
                painter->setPen(QPen(t.focusInner, 1));
                painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3),
                                         ControlRadius - 1, ControlRadius - 1);
            }
            painter->restore();
            return;
        }
    }

    if (control == CC_ScrollBar) {
        if (const auto *scroll = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            // QAbstractSlider's paint path does not guarantee that a standalone
            // scrollbar's backing store was cleared. WinUI's transparent rest
            // state therefore has to be composited over the widget surface here.
            QColor background = option->palette.color(QPalette::Window);
            for (const QWidget *ancestor = widget ? widget->parentWidget() : nullptr;
                 ancestor; ancestor = ancestor->parentWidget()) {
                if (!qobject_cast<const QGroupBox *>(ancestor))
                    continue;
                QColor opaqueLayer = t.layer;
                const qreal opacity = opaqueLayer.alphaF();
                opaqueLayer.setAlpha(255);
                background = Private::mix(background, opaqueLayer, opacity);
            }
            painter->fillRect(option->rect, background);
            const bool enabled = option->state & State_Enabled;
            // The WinUI ScrollBarThumb template fades the thumb to zero in
            // its Disabled state; arrows and track are suppressed with it.
            if (!enabled)
                return;
            const QRect thumb = subControlRect(CC_ScrollBar, scroll, SC_ScrollBarSlider, widget);
            const qreal expanded = progress(widget, hoverProperty,
                option->state & State_MouseOver ? 1.0 : 0.0);
            const bool horizontal = scroll->orientation == Qt::Horizontal;
            if (expanded > 0.001) {
                QColor track = t.layer;
                track.setAlphaF(track.alphaF() * expanded);
                roundedRect(painter, option->rect, track, Qt::transparent, 3);
            }

            const qreal thickness = 8.0 + 4.0 * expanded;
            QRectF visualThumb(thumb);
            if (horizontal)
                visualThumb.setTop(thumb.bottom() + 1.0 - thickness);
            else
                visualThumb.setLeft(thumb.right() + 1.0 - thickness);
            const QColor thumbColor = t.strokeStrong;
            roundedRect(painter, visualThumb, thumbColor, Qt::transparent,
                        thickness / 2.0);

            if (expanded > 0.001) {
                const QRect decrease = subControlRect(CC_ScrollBar, scroll,
                    SC_ScrollBarSubLine, widget);
                const QRect increase = subControlRect(CC_ScrollBar, scroll,
                    SC_ScrollBarAddLine, widget);
                const bool pressed = option->state & State_Sunken;
                const auto drawArrow = [&](const QRect &rect, SubControl sub,
                                           Icon glyph) {
                    if (!(scroll->subControls & sub))
                        return;
                    const bool active = (scroll->activeSubControls & sub)
                        && (option->state & State_MouseOver);
                    if (active) {
                        const QColor fill = pressed ? t.subtlePressed
                                                    : t.subtleHover;
                        roundedRect(painter, QRectF(rect).adjusted(2, 2, -2, -2),
                                    fill, Qt::transparent, 3);
                    }
                    painter->save();
                    painter->setOpacity(expanded);
                    QRect glyphRect(rect.center().x() - 4, rect.center().y() - 4,
                                    8, 8);
                    if (active && pressed)
                        glyphRect = QRect(rect.center().x() - 3,
                                          rect.center().y() - 3, 7, 7);
                    icon(glyph, t.textPrimary).paint(painter, glyphRect,
                                                     Qt::AlignCenter,
                                                     QIcon::Normal);
                    painter->restore();
                };
                if (horizontal) {
                    const bool rtl = option->direction == Qt::RightToLeft;
                    drawArrow(decrease, SC_ScrollBarSubLine,
                              rtl ? Icon::ChevronRight : Icon::ChevronLeft);
                    drawArrow(increase, SC_ScrollBarAddLine,
                              rtl ? Icon::ChevronLeft : Icon::ChevronRight);
                } else {
                    drawArrow(decrease, SC_ScrollBarSubLine, Icon::ChevronUp);
                    drawArrow(increase, SC_ScrollBarAddLine, Icon::ChevronDown);
                }
            }
            return;
        }
    }

    Q_ASSERT_X(!coveredComplex(control), "WinUI3::Style::drawComplexControl",
               "a covered complex control reached QCommonStyle");
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option,
                       const QWidget *widget) const
{
    if (toggleSwitch(widget)) {
        if (metric == PM_IndicatorWidth)
            return 40;
        if (metric == PM_IndicatorHeight)
            return 20;
    }
    switch (metric) {
    case PM_ButtonMargin: return 8;
    case PM_ButtonDefaultIndicator: return 0;
    case PM_DefaultFrameWidth: return 1;
    case PM_ComboBoxFrameWidth: return 1;
    case PM_SpinBoxFrameWidth: return 1;
    case PM_IndicatorWidth:
    case PM_IndicatorHeight:
    case PM_ExclusiveIndicatorWidth:
    case PM_ExclusiveIndicatorHeight: return 20;
    case PM_ScrollBarExtent: return 12;
    case PM_ScrollBarSliderMin: return 30;
    case PM_SliderThickness:
    case PM_SliderLength: return 20;
    case PM_TabCloseIndicatorWidth: return 32;
    case PM_TabCloseIndicatorHeight: return 24;
    case PM_SmallIconSize: return 16;
    case PM_ButtonIconSize: return 16;
    case PM_ToolBarIconSize: return 20;
    case PM_DockWidgetSeparatorExtent:
    case PM_DockWidgetHandleExtent:
    case PM_SplitterWidth: return 6;
    case PM_DockWidgetFrameWidth: return 1;
    case PM_DockWidgetTitleMargin: return 8;
    case PM_DockWidgetTitleBarButtonMargin: return 4;
    case PM_HeaderMargin: return 12;
    case PM_HeaderMarkSize: return 12;
    case PM_HeaderGripMargin: return 4;
    case PM_HeaderDefaultSectionSizeVertical: return 36;
    case PM_HeaderDefaultSectionSizeHorizontal: return 100;
    default: return QProxyStyle::pixelMetric(metric, option, widget);
    }
}

QSize Style::sizeFromContents(ContentsType type, const QStyleOption *option,
                              const QSize &contentsSize, const QWidget *widget) const
{
    QSize size = contentsSize;
    switch (type) {
    case CT_PushButton:
        size += QSize(24, 12);
        if (const auto *button = qstyleoption_cast<const QStyleOptionButton *>(option);
            button && (button->features & QStyleOptionButton::HasMenu))
            size.rwidth() += 22;
        size.setWidth(qMax(size.width(), 32));
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_ComboBox:
        size += QSize(50, 12);
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_LineEdit:
        size += QSize(16, 12);
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_SpinBox:
        size += QSize(verticalSpinButtons(widget) ? 44 : 84, 0);
        size.setHeight(qMax(size.height(), 32));
        size.setWidth(qMax(size.width(), 120));
        break;
    case CT_ToolButton:
        if (textBoxHelperButton(widget))
            return QSize(30, 32);
        if (widget && qobject_cast<const QTabBar *>(widget->parentWidget()))
            return QSize(32, 24);
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            const QFontMetrics metrics(tool->fontMetrics);
            const int textWidth = metrics.horizontalAdvance(tool->text);
            const int iconWidth = tool->icon.isNull() ? 0
                : (tool->iconSize.isValid() ? tool->iconSize.width() : 16);
            if (const auto *toolButton = qobject_cast<const QToolButton *>(widget)) {
                if (toolButton->toolButtonStyle() == Qt::ToolButtonTextBesideIcon
                    && textWidth > 0 && iconWidth > 0)
                    size.setWidth(qMax(size.width(), textWidth + iconWidth + 22));
                else if (toolButton->toolButtonStyle() == Qt::ToolButtonTextOnly)
                    size.setWidth(qMax(size.width(), textWidth + 16));
            }
            if (tool->features & QStyleOptionToolButton::MenuButtonPopup)
                size.rwidth() += 24;
        }
        if (size.isEmpty())
            size = QSize(20, 20);
        size += QSize(12, 12);
        size.setHeight(qMax(size.height(), 32));
        size.setWidth(qMax(size.width(), 32));
        break;
    case CT_MenuBarItem:
        size = contentsSize + QSize(24, 12);
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_TabBarTab:
        size += QSize(16, 8);
        size.setHeight(32);
        size.setWidth(qBound(100, size.width(), 240));
        break;
    case CT_MenuItem:
        if (const auto *menu = qstyleoption_cast<const QStyleOptionMenuItem *>(option)) {
            if (menu->menuItemType == QStyleOptionMenuItem::Separator) {
                size.setHeight(7);
                break;
            }
            const QStringList parts = menu->text.split(QLatin1Char('\t'));
            const QFontMetrics metrics(menu->font);
            const int mainWidth = metrics.horizontalAdvance(parts.value(0));
            const int shortcutWidth = parts.size() > 1
                ? metrics.horizontalAdvance(parts.value(1)) : 0;
            const int trailing = menu->menuItemType == QStyleOptionMenuItem::SubMenu
                ? 24 : 0;
            const bool comboItem = qobject_cast<const QComboBox *>(widget);
            const int leading = comboItem && menu->icon.isNull() ? 16 : 42;
            const int requiredWidth = leading + mainWidth + 16 + trailing
                + (shortcutWidth > 0 ? 20 + shortcutWidth : 0);
            size.setWidth(qMax(size.width(), qMax(requiredWidth, 120)));
            // Qt may pass the popup's provisional viewport height here on
            // the first layout. Never preserve that transient value as an
            // item height: it produces a one-frame, full-screen row followed
            // by a visible reposition when the selected row is not zero.
            size.setHeight(comboItem ? 40 : 36);
        }
        break;
    case CT_ItemViewItem:
        if (widget && widget->window()
            && widget->window()->windowType() == Qt::Popup) {
            size.setHeight(40);
        } else if (qobject_cast<const QTreeView *>(itemView(widget))) {
            size.setHeight(28);
        } else if (qobject_cast<const QTableView *>(itemView(widget))) {
            size.setHeight(36);
        } else {
            size.setHeight(40);
        }
        break;
    case CT_HeaderSection:
        size += QSize(24, 8);
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_CheckBox:
        if (toggleSwitch(widget)) {
            const auto *check = qstyleoption_cast<const QStyleOptionButton *>(option);
            QString onText = widget->property(ToggleSwitchOnTextProperty).toString();
            QString offText = widget->property(ToggleSwitchOffTextProperty).toString();
            if (check && onText.isEmpty() && offText.isEmpty())
                onText = offText = check->text;
            const QFontMetrics metrics = check ? check->fontMetrics
                                               : widget->fontMetrics();
            const int labelWidth = qMax(metrics.horizontalAdvance(onText),
                                        metrics.horizontalAdvance(offText));
            size = QSize(labelWidth > 0 ? qMax(154, 50 + labelWidth) : 40, 40);
            break;
        }
        size += QSize(32, 8);
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_RadioButton:
        size += QSize(32, 8);
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_ProgressBar:
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 20));
        break;
    case CT_Slider:
        size.setWidth(qMax(size.width(), 120));
        size.setHeight(qMax(size.height(), 32));
        break;
    case CT_ScrollBar:
        size.setWidth(qMax(size.width(), 12));
        size.setHeight(qMax(size.height(), 12));
        break;
    case CT_GroupBox:
        size.setWidth(qMax(size.width(), contentsSize.width() + 24));
        size.setHeight(qMax(size.height(), contentsSize.height() + 48));
        break;
    default:
        return QProxyStyle::sizeFromContents(type, option, contentsSize, widget);
    }
    return size;
}

QRect Style::subElementRect(SubElement element, const QStyleOption *option,
                            const QWidget *widget) const
{
    if (element == SE_PushButtonContents)
        return option->rect.adjusted(8, 4, -8, -4);
    if (element == SE_ToolButtonLayoutItem)
        return option->rect.adjusted(4, 2, -4, -2);
    if (element == SE_CheckBoxIndicator || element == SE_RadioButtonIndicator) {
        const QRect logical(option->rect.left() + 4,
                            option->rect.center().y() - 10, 20, 20);
        return visualRect(option->direction, option->rect, logical);
    }
    if (element == SE_CheckBoxContents || element == SE_RadioButtonContents) {
        const QRect logical = option->rect.adjusted(32, 0, -4, 0);
        return visualRect(option->direction, option->rect, logical);
    }
    if (element == SE_CheckBoxClickRect || element == SE_RadioButtonClickRect)
        return option->rect;
    if (element == SE_LineEditContents && !spinBoxEditor(widget)) {
        const QRect logical = option->rect.adjusted(10, 5, -6, -6);
        return visualRect(option->direction, option->rect, logical);
    }
    QRect result = QProxyStyle::subElementRect(element, option, widget);
    if (const auto *source = qstyleoption_cast<const QStyleOptionViewItem *>(option)) {
        const QAbstractItemView *view = selectionMarkerView(widget);
        if (view && (element == SE_ItemViewItemCheckIndicator
                     || element == SE_ItemViewItemDecoration
                     || element == SE_ItemViewItemText)) {
            const int offset = treeItemIndent(*source, view) + itemSelectionGutter;
            const int delta = source->direction == Qt::RightToLeft ? -offset : offset;
            result.translate(delta, 0);

            // Keep the reserved gutter stable even when the base style
            // changes its default item padding. The content slot is the
            // same for selected and unselected rows.
            QRect content = source->rect;
            if (source->direction == Qt::RightToLeft)
                content.setRight(qMax(content.left() - 1,
                                      content.right() - offset));
            else
                content.setLeft(qMin(content.right() + 1,
                                     content.left() + offset));
            result = result.intersected(content);
        }
    }
    const bool popup = widget && widget->window()
        && widget->window()->windowType() == Qt::Popup;
    if (popup && element == SE_ItemViewItemText) {
        result.setLeft(qMax(result.left(), option->rect.left() + 42));
        result.setRight(qMin(result.right(), option->rect.right() - 12));
    }
    return result;
}

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                            SubControl subControl, const QWidget *widget) const
{
    if (control == CC_Slider) {
        if (const auto *slider = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = slider->orientation == Qt::Horizontal;
            const int preMargin = 14;
            QRect groove;
            if (horizontal) {
                groove = QRect(slider->rect.left() + preMargin,
                               slider->rect.center().y() - 2,
                               qMax(1, slider->rect.width() - 2 * preMargin), 4);
            } else {
                groove = QRect(slider->rect.center().x() - 2,
                               slider->rect.top() + preMargin, 4,
                               qMax(1, slider->rect.height() - 2 * preMargin));
            }
            if (subControl == SC_SliderGroove)
                return groove;
            if (subControl == SC_SliderHandle) {
                const int span = qMax(0, (horizontal ? groove.width()
                                                      : groove.height()) - 1);
                const int offset = QStyle::sliderPositionFromValue(
                    slider->minimum, slider->maximum, slider->sliderPosition,
                    span, slider->upsideDown);
                const QPoint center = horizontal
                    ? QPoint(groove.left() + offset, groove.center().y())
                    : QPoint(groove.center().x(), groove.top() + offset);
                return QRect(center.x() - 8, center.y() - 8, 18, 18);
            }
            if (subControl == SC_SliderTickmarks)
                return slider->rect;
        }
    }
    if (control == CC_ScrollBar) {
        if (const auto *scroll = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            const bool horizontal = scroll->orientation == Qt::Horizontal;
            const int buttonLength = 12;
            const int axisLength = horizontal ? scroll->rect.width()
                                              : scroll->rect.height();
            const int grooveLength = qMax(0, axisLength - 2 * buttonLength);
            const QRect logicalSub = horizontal
                ? QRect(scroll->rect.left(), scroll->rect.top(), buttonLength,
                        scroll->rect.height())
                : QRect(scroll->rect.left(), scroll->rect.top(),
                        scroll->rect.width(), buttonLength);
            const QRect logicalAdd = horizontal
                ? QRect(scroll->rect.right() - buttonLength + 1,
                        scroll->rect.top(), buttonLength, scroll->rect.height())
                : QRect(scroll->rect.left(), scroll->rect.bottom() - buttonLength + 1,
                        scroll->rect.width(), buttonLength);
            const QRect groove = horizontal
                ? QRect(scroll->rect.left() + buttonLength, scroll->rect.top(),
                        grooveLength, scroll->rect.height())
                : QRect(scroll->rect.left(), scroll->rect.top() + buttonLength,
                        scroll->rect.width(), grooveLength);
            if (subControl == SC_ScrollBarSubLine)
                return visualRect(scroll->direction, scroll->rect, logicalSub);
            if (subControl == SC_ScrollBarAddLine)
                return visualRect(scroll->direction, scroll->rect, logicalAdd);
            if (subControl == SC_ScrollBarGroove)
                return groove;

            const qint64 range = qMax<qint64>(0,
                qint64(scroll->maximum) - qint64(scroll->minimum));
            int thumbLength = grooveLength;
            if (range > 0) {
                const qint64 denominator = qint64(range) + scroll->pageStep;
                thumbLength = denominator > 0
                    ? int(qint64(grooveLength) * scroll->pageStep / denominator)
                    : 0;
                const int minimumThumb = qMin(30, grooveLength);
                thumbLength = qBound(minimumThumb, thumbLength, grooveLength);
            }
            const int available = qMax(0, grooveLength - thumbLength);
            const int offset = QStyle::sliderPositionFromValue(
                scroll->minimum, scroll->maximum, scroll->sliderPosition,
                available, scroll->upsideDown);
            const QRect thumb = horizontal
                ? QRect(groove.left() + offset, groove.top(), thumbLength,
                        groove.height())
                : QRect(groove.left(), groove.top() + offset, groove.width(),
                        thumbLength);
            if (subControl == SC_ScrollBarSlider)
                return thumb;

            if (subControl == SC_ScrollBarSubPage) {
                if (horizontal) {
                    return scroll->upsideDown
                        ? QRect(thumb.right() + 1, groove.top(),
                                qMax(0, groove.right() - thumb.right()),
                                groove.height())
                        : QRect(groove.left(), groove.top(),
                                qMax(0, thumb.left() - groove.left()),
                                groove.height());
                }
                return scroll->upsideDown
                    ? QRect(groove.left(), thumb.bottom() + 1, groove.width(),
                            qMax(0, groove.bottom() - thumb.bottom()))
                    : QRect(groove.left(), groove.top(), groove.width(),
                            qMax(0, thumb.top() - groove.top()));
            }
            if (subControl == SC_ScrollBarAddPage) {
                if (horizontal) {
                    return scroll->upsideDown
                        ? QRect(groove.left(), groove.top(),
                                qMax(0, thumb.left() - groove.left()),
                                groove.height())
                        : QRect(thumb.right() + 1, groove.top(),
                                qMax(0, groove.right() - thumb.right()),
                                groove.height());
                }
                return scroll->upsideDown
                    ? QRect(groove.left(), groove.top(), groove.width(),
                            qMax(0, thumb.top() - groove.top()))
                    : QRect(groove.left(), thumb.bottom() + 1, groove.width(),
                            qMax(0, groove.bottom() - thumb.bottom()));
            }
        }
    }
    if (control == CC_GroupBox) {
        if (const auto *group = qstyleoption_cast<const QStyleOptionGroupBox *>(option)) {
            const bool checkable = group->subControls & SC_GroupBoxCheckBox;
            if (subControl == SC_GroupBoxFrame)
                return group->rect;
            if (subControl == SC_GroupBoxCheckBox)
                return visualRect(group->direction, group->rect,
                                  QRect(group->rect.left() + 12,
                                        group->rect.top() + 8, 20, 20));
            if (subControl == SC_GroupBoxLabel) {
                const int left = group->rect.left() + (checkable ? 40 : 12);
                return visualRect(group->direction, group->rect,
                                  QRect(left, group->rect.top() + 4,
                                        qMax(0, group->rect.right() - left - 12),
                                        28));
            }
            if (subControl == SC_GroupBoxContents)
                return visualRect(group->direction, group->rect,
                                  group->rect.adjusted(12, 36, -12, -12));
        }
    }
    if (control == CC_ComboBox) {
        if (subControl == SC_ComboBoxArrow) {
            const QRect logical(option->rect.right() - 37, option->rect.top(),
                                38, option->rect.height());
            return visualRect(option->direction, option->rect, logical);
        }
        if (subControl == SC_ComboBoxEditField) {
            const QRect logical = option->rect.adjusted(12, 1, -38, -1);
            return visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == CC_ToolButton) {
        const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
        if (subControl == SC_ToolButton) {
            if (tool && (tool->features & QStyleOptionToolButton::MenuButtonPopup)) {
                const QRect logical = option->rect.adjusted(0, 0, -24, 0);
                return visualRect(option->direction, option->rect, logical);
            }
            return option->rect;
        }
        if (subControl == SC_ToolButtonMenu) {
            const QRect logical(option->rect.right() - 23, option->rect.top(),
                                24, option->rect.height());
            return visualRect(option->direction, option->rect, logical);
        }
    }
    if (control == CC_SpinBox) {
        const bool verticalButtons = verticalSpinButtons(widget);
        const int buttonWidth = verticalButtons ? 32 : 36;
        QRect logical;
        if (verticalButtons && subControl == SC_SpinBoxUp) {
            const int upperHeight = option->rect.height() / 2;
            logical = QRect(option->rect.right() - buttonWidth + 1,
                            option->rect.top(), buttonWidth, upperHeight);
        } else if (verticalButtons && subControl == SC_SpinBoxDown) {
            const int upperHeight = option->rect.height() / 2;
            logical = QRect(option->rect.right() - buttonWidth + 1,
                            option->rect.top() + upperHeight, buttonWidth,
                            option->rect.height() - upperHeight);
        } else if (verticalButtons && subControl == SC_SpinBoxEditField) {
            logical = option->rect.adjusted(12, 1, -buttonWidth, -1);
        } else if (subControl == SC_SpinBoxUp) {
            logical = QRect(option->rect.right() - 2 * buttonWidth + 1,
                            option->rect.top(), buttonWidth,
                            option->rect.height());
        } else if (subControl == SC_SpinBoxDown) {
            logical = QRect(option->rect.right() - buttonWidth + 1,
                            option->rect.top(), buttonWidth,
                            option->rect.height());
        } else if (subControl == SC_SpinBoxEditField) {
            logical = option->rect.adjusted(12, 1, -2 * buttonWidth, -1);
        }
        if (logical.isValid())
            return visualRect(option->direction, option->rect, logical);
        if (subControl == SC_SpinBoxFrame)
            return option->rect;
    }
    return QProxyStyle::subControlRect(control, option, subControl, widget);
}

QStyle::SubControl Style::hitTestComplexControl(
    ComplexControl control, const QStyleOptionComplex *option,
    const QPoint &position, const QWidget *widget) const
{
    if (!option || !option->rect.contains(position))
        return SC_None;

    const auto contains = [&](SubControl subControl) {
        return subControlRect(control, option, subControl, widget)
            .contains(position);
    };
    switch (control) {
    case CC_ToolButton:
        if (const auto *tool = qstyleoption_cast<const QStyleOptionToolButton *>(option);
            tool && (tool->features & QStyleOptionToolButton::MenuButtonPopup)
            && contains(SC_ToolButtonMenu))
            return SC_ToolButtonMenu;
        return contains(SC_ToolButton) ? SC_ToolButton : SC_None;
    case CC_ComboBox:
        if (contains(SC_ComboBoxArrow))
            return SC_ComboBoxArrow;
        if (contains(SC_ComboBoxEditField))
            return SC_ComboBoxEditField;
        return SC_ComboBoxFrame;
    case CC_GroupBox:
        if (contains(SC_GroupBoxCheckBox))
            return SC_GroupBoxCheckBox;
        if (contains(SC_GroupBoxLabel))
            return SC_GroupBoxLabel;
        if (contains(SC_GroupBoxContents))
            return SC_GroupBoxContents;
        return SC_GroupBoxFrame;
    case CC_SpinBox:
        if (contains(SC_SpinBoxUp))
            return SC_SpinBoxUp;
        if (contains(SC_SpinBoxDown))
            return SC_SpinBoxDown;
        if (contains(SC_SpinBoxEditField))
            return SC_SpinBoxEditField;
        return SC_SpinBoxFrame;
    case CC_Slider:
        if (contains(SC_SliderHandle))
            return SC_SliderHandle;
        return contains(SC_SliderGroove) ? SC_SliderGroove : SC_None;
    case CC_ScrollBar:
        for (SubControl sub : {SC_ScrollBarSubLine, SC_ScrollBarAddLine,
                               SC_ScrollBarSlider, SC_ScrollBarSubPage,
                               SC_ScrollBarAddPage, SC_ScrollBarGroove})
            if (contains(sub))
                return sub;
        return SC_None;
    default:
        return QProxyStyle::hitTestComplexControl(control, option, position, widget);
    }
}

int Style::styleHint(StyleHint hint, const QStyleOption *option,
                     const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch (hint) {
    case SH_Widget_Animate: return animationsAllowed() ? 1 : 0;
    case SH_ScrollBar_Transient: return 1;
    case SH_ComboBox_Popup: return 1;
    case SH_ComboBox_PopupFrameStyle: return QFrame::NoFrame;
    case SH_ComboBox_ListMouseTracking:
    case SH_MenuBar_MouseTracking:
    case SH_Menu_MouseTracking: return 1;
    case SH_Menu_SubMenuPopupDelay: return 400;
    case SH_Slider_AbsoluteSetButtons: return Qt::LeftButton;
    case SH_ToolButtonStyle: return Qt::ToolButtonFollowStyle;
    default: return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
}

QIcon Style::standardIcon(StandardPixmap standard, const QStyleOption *option,
                           const QWidget *widget) const
{
    Q_UNUSED(option)
    Q_UNUSED(widget)
    // Keep standard icons semantic rather than baking the current palette
    // into a cached QIcon. The engine follows the application palette for
    // direct Qt use, while our paint paths recolour its alpha mask from the
    // control's effective QStyleOption palette.
    switch (standard) {
    case SP_ArrowBack: return icon(Icon::Back);
    case SP_ArrowDown: return icon(Icon::ChevronDown);
    case SP_ArrowLeft: return icon(Icon::ChevronLeft);
    case SP_ArrowRight: return icon(Icon::ChevronRight);
    case SP_ArrowUp: return icon(Icon::ChevronUp);
    case SP_BrowserReload: return icon(Icon::Refresh);
    case SP_DialogApplyButton: return icon(Icon::Check);
    case SP_DialogCancelButton:
    case SP_DockWidgetCloseButton:
    case SP_TabCloseButton:
    case SP_TitleBarCloseButton: return icon(Icon::Close);
    case SP_LineEditClearButton: return icon(Icon::Clear);
    case SP_DialogSaveButton:
    case SP_DialogYesButton: return icon(Icon::Save);
    case SP_DirIcon:
    case SP_DirOpenIcon: return icon(Icon::Folder);
    case SP_FileDialogNewFolder: return icon(Icon::Add);
    case SP_MediaPause: return icon(Icon::Pause);
    case SP_MediaPlay: return icon(Icon::Play);
    case SP_MediaStop: return icon(Icon::Stop);
    case SP_MessageBoxCritical: return icon(Icon::Error);
    case SP_MessageBoxInformation: return icon(Icon::Info);
    case SP_MessageBoxQuestion: return icon(Icon::Help);
    case SP_MessageBoxWarning: return icon(Icon::Warning);
    case SP_ToolBarHorizontalExtensionButton:
    case SP_ToolBarVerticalExtensionButton: return icon(Icon::More);
    default: return QProxyStyle::standardIcon(standard, option, widget);
    }
}

void Style::polish(QApplication *application)
{
    QProxyStyle::polish(application);
    if (!d->applicationStateSaved) {
        d->originalApplicationFont = application->font();
        d->originalApplicationPalette = application->palette();
        d->applicationStateSaved = true;
    }
    QFont font(QStringLiteral("Segoe UI Variable Text"));
    if (!QFontDatabase::families().contains(font.family()))
        font.setFamily(QStringLiteral("Segoe UI"));
    font.setPixelSize(14);
    application->setFont(font);
    application->setPalette(standardPalette());
    d->restartSystemAppearancePolling();
}

void Style::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (!widget)
        return;
    rememberPalette(widget);
    remember(widget, originalAutoFillProperty, widget->autoFillBackground());
    remember(widget, originalHoverAttributeProperty,
             widget->testAttribute(Qt::WA_Hover));
    remember(widget, originalRoleProperty, widget->property(roleProperty));
    widget->setAttribute(Qt::WA_Hover, true);
    widget->installEventFilter(this);
    widget->setProperty(hoverProperty,
                        widget->isEnabled() && widget->underMouse() ? 1.0 : 0.0);
    widget->setProperty(pressProperty, 0.0);
    widget->setProperty(focusProperty, widget->hasFocus() ? 1.0 : 0.0);
    widget->setProperty(focusVisibleProperty,
                        widget->hasFocus() && d->keyboardInput);
    if (qobject_cast<QScrollBar *>(widget)) {
        widget->setProperty(scrollBarInsideProperty, widget->underMouse());
        widget->setProperty(scrollBarGenerationProperty, 0);
    }
    if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        widget->setProperty(checkProperty,
                            checkBox->checkState() == Qt::Unchecked ? 0.0 : 1.0);
        widget->setProperty(togglePositionProperty, checkBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(widget,
            connect(checkBox, &QCheckBox::stateChanged, this,
                    [this, checkBox](int state) {
                d->animate(checkBox, checkProperty,
                           state == Qt::Unchecked ? 0.0 : 1.0,
                           toggleSwitch(checkBox) ? Private::FastDuration
                                                   : Private::CheckBoxDuration);
                if (toggleSwitch(checkBox))
                    d->animate(checkBox, togglePositionProperty,
                               state == Qt::Unchecked ? 0.0 : 1.0,
                               Private::FasterDuration);
            }));
    } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
        if (const auto previous = d->radioConnections.take(radio))
            disconnect(previous);
        widget->setProperty(checkProperty, radio->isChecked() ? 1.0 : 0.0);
        d->radioConnections.insert(radio,
            connect(radio, &QAbstractButton::toggled, this,
                    [this, radio](bool checked) {
                d->animate(radio, checkProperty, checked ? 1.0 : 0.0,
                           Private::FastDuration);
            }));
    } else if (auto *groupBox = qobject_cast<QGroupBox *>(widget);
               groupBox && groupBox->isCheckable()) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        widget->setProperty(checkProperty, groupBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(widget,
            connect(groupBox, &QGroupBox::toggled, this,
                    [this, groupBox](bool checked) {
                d->animate(groupBox, checkProperty, checked ? 1.0 : 0.0,
                           Private::FastDuration);
                    }));
    }

    if (auto *progressBar = qobject_cast<QProgressBar *>(widget)) {
        d->registerProgressBar(progressBar);
        progressBar->setProperty(progressPhaseProperty,
                                  Style::animationsAllowed() ? 0.0 : 0.35);
        d->refreshProgressTimer();
    }
    if (auto *view = qobject_cast<QAbstractItemView *>(widget))
        prepareNavigationView(view);

    if (auto *table = qobject_cast<QTableView *>(widget)) {
        if (const auto previous = d->tableConnections.take(widget))
            disconnect(previous);
        widget->setProperty(ownedPaletteProperty, true);
        const auto applyTableSelectionPalette = [this, table] {
            QPalette palette = table->palette();
            const Private::Tokens tableTokens = Private::tokens(standardPalette());
            palette.setColor(QPalette::Highlight, tableTokens.subtleHover);
            palette.setColor(QPalette::HighlightedText, tableTokens.textPrimary);
            table->setPalette(palette);
        };
        applyTableSelectionPalette();
        d->tableConnections.insert(table,
            connect(this, &Style::themeChanged, table,
                    [applyTableSelectionPalette](ThemeMode) {
                applyTableSelectionPalette();
            }));
    } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
               editor && qobject_cast<const QTableView *>(itemView(editor))) {
        widget->setProperty(ownedPaletteProperty, true);
        QPalette palette = editor->palette();
        const Private::Tokens editorTokens = Private::tokens(standardPalette());
        palette.setColor(QPalette::Highlight, accentColor());
        palette.setColor(QPalette::HighlightedText, editorTokens.textOnAccentPrimary);
        editor->setPalette(palette);
    }

    if (auto *toolButton = qobject_cast<QAbstractButton *>(widget)) {
        if (qobject_cast<QToolBar *>(toolButton->parentWidget())
            || qobject_cast<QTabBar *>(toolButton->parentWidget()))
            setControlRole(toolButton, ControlRole::Subtle);
    }

    if (qobject_cast<QComboBox *>(widget))
        widget->setProperty(comboChevronProperty, 0.0);

    // QMenu computes its first popup geometry after polish but before Show.
    // Install only the layout inset here; native backdrop and palette work
    // remain in the Show path to avoid creating a handle recursively.
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(menu, originalMarginsProperty,
                 QVariant::fromValue(menu->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
    }

    if (auto *dialog = qobject_cast<QDialog *>(widget);
        dialog && (qobject_cast<QMessageBox *>(dialog)
                   || dialog->property(ContentDialogProperty).toBool())) {
        prepareContentDialogState(dialog, d->dark());
    }

}

void Style::polish(QPalette &palette)
{
    palette = standardPalette();
}

void Style::unpolish(QApplication *application)
{
    d->systemAppearanceTimer->stop();
    if (application && d->applicationStateSaved) {
        application->setFont(d->originalApplicationFont);
        application->setPalette(d->originalApplicationPalette);
        d->applicationStateSaved = false;
    }
    QProxyStyle::unpolish(application);
}

void Style::unpolish(QWidget *widget)
{
    // Let the base style release its state before restoring application-owned
    // values. Some Qt widgets (notably item views) recompute frame margins in
    // QCommonStyle::unpolish(); restoring first would immediately lose the
    // original values again.
    QProxyStyle::unpolish(widget);
    if (widget) {
        d->stopAnimations(widget);
        if (widget->property("_winui_backdrop").isValid())
            applyBackdrop(widget, Backdrop::None);
        restoreRememberedPalette(widget);
        if (widget->property(originalAutoFillProperty).isValid())
            widget->setAutoFillBackground(
                widget->property(originalAutoFillProperty).toBool());
        if (widget->property(originalHoverAttributeProperty).isValid())
            widget->setAttribute(Qt::WA_Hover,
                widget->property(originalHoverAttributeProperty).toBool());
        if (widget->property(originalOpaquePaintProperty).isValid())
            widget->setAttribute(Qt::WA_OpaquePaintEvent,
                widget->property(originalOpaquePaintProperty).toBool());
        if (widget->property(originalMarginsProperty).isValid()
            && !qobject_cast<QDialog *>(widget))
            widget->setContentsMargins(
                widget->property(originalMarginsProperty).value<QMargins>());
        if (auto *list = qobject_cast<QListView *>(widget))
            if (widget->property(originalListSpacingProperty).isValid())
                list->setSpacing(widget->property(originalListSpacingProperty).toInt());
        if (auto *dialog = qobject_cast<QDialog *>(widget)) {
            restoreContentDialogState(dialog, false);
        }
        if (auto *frame = qobject_cast<QFrame *>(widget))
            if (widget->property(originalFrameShapeProperty).isValid())
                frame->setFrameShape(static_cast<QFrame::Shape>(
                    widget->property(originalFrameShapeProperty).toInt()));
        if (widget->property(originalRoleWasValidProperty).isValid()) {
            if (widget->property(originalRoleWasValidProperty).toBool())
                widget->setProperty(roleProperty,
                                    widget->property(originalRoleProperty));
            else
                widget->setProperty(roleProperty, {});
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget))
            restoreNavigationView(view);
        if (const auto connection = d->toggleConnections.take(widget))
            disconnect(connection);
        if (auto *radio = qobject_cast<QRadioButton *>(widget))
            if (const auto connection = d->radioConnections.take(radio))
                disconnect(connection);
        if (const auto connection = d->tableConnections.take(widget))
            disconnect(connection);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget))
            d->toggleDragStates.remove(checkBox);
        if (auto *slider = qobject_cast<QSlider *>(widget))
            d->unregisterSlider(slider);
        if (auto *progressBar = qobject_cast<QProgressBar *>(widget))
            d->unregisterProgressBar(progressBar);
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            d->cancelScrollBarTimer(scrollBar);
            d->unregisterScrollBar(scrollBar);
        }
        if (auto *slider = qobject_cast<QSlider *>(widget))
            hideSliderValueToolTip(slider);
        widget->removeEventFilter(this);
        widget->setProperty(hoverProperty, {});
        widget->setProperty(pressProperty, {});
        widget->setProperty(buttonPressGenerationProperty, {});
        widget->setProperty(buttonPressReleasePendingProperty, {});
        widget->setProperty(focusProperty, {});
        widget->setProperty(focusVisibleProperty, {});
        widget->setProperty(checkProperty, {});
        widget->setProperty(togglePositionProperty, {});
        widget->setProperty("_winui_toggle_dragging", {});
        widget->setProperty(scrollBarInsideProperty, {});
        widget->setProperty(scrollBarGenerationProperty, {});
        widget->setProperty(sliderToolTipVisibleProperty, {});
        widget->setProperty(sliderToolTipValueProperty, {});
        widget->setProperty(progressPhaseProperty, {});
        widget->setProperty(ownedPaletteProperty, {});
        widget->setProperty(originalPaletteProperty, {});
        widget->setProperty(originalPaletteExplicitProperty, {});
        widget->setProperty(originalAutoFillProperty, {});
        widget->setProperty(originalHoverAttributeProperty, {});
        widget->setProperty(originalOpaquePaintProperty, {});
        widget->setProperty(originalMouseTrackingProperty, {});
        widget->setProperty(originalListSpacingProperty, {});
        widget->setProperty(originalMinimumSizeProperty, {});
        widget->setProperty(originalFrameShapeProperty, {});
        widget->setProperty(originalMarginsProperty, {});
        widget->setProperty(originalSpacingProperty, {});
        widget->setProperty(originalRoleProperty, {});
        widget->setProperty(originalRoleWasValidProperty, {});
    }
}

bool Style::eventFilter(QObject *watched, QEvent *event)
{
    auto *widget = qobject_cast<QWidget *>(watched);
    if (!widget)
        return QProxyStyle::eventFilter(watched, event);

    using namespace Private;
    switch (event->type()) {
    case QEvent::EnabledChange: {
        // Qt normally stops delivering pointer events to a disabled widget,
        // but an in-flight style animation has no such protection. Clear the
        // interaction state at the source so disabling during a press cannot
        // leave a stale hover/pressed surface behind.
        d->clearPointerInteraction(widget);
        const bool enabled = widget->isEnabled();
        widget->setProperty(hoverProperty,
                            enabled && widget->underMouse() ? 1.0 : 0.0);
        widget->setProperty(pressProperty, 0.0);
        widget->setProperty(focusProperty,
                            enabled && widget->hasFocus() ? 1.0 : 0.0);
        widget->setProperty(focusVisibleProperty,
                            enabled && widget->hasFocus() && d->keyboardInput);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
            checkBox->setProperty(checkProperty,
                                  checkBox->checkState() == Qt::Unchecked
                                      ? 0.0 : 1.0);
            checkBox->setProperty(togglePositionProperty,
                                  checkBox->isChecked() ? 1.0 : 0.0);
        } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
            radio->setProperty(checkProperty, radio->isChecked() ? 1.0 : 0.0);
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget))
            d->cancelScrollBarTimer(scrollBar);
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            d->cancelSliderToolTip(slider);
            if (!enabled)
                hideSliderValueToolTip(slider);
        }
        if (qobject_cast<QProgressBar *>(widget))
            d->refreshProgressTimer();
        widget->update();
        break;
    }
    case QEvent::Paint:
        break;
    case QEvent::UpdateRequest:
        // QProgressBar exposes no rangeChanged signal. Refresh only while the
        // shared clock is stopped, so ordinary timer-driven updates remain
        // O(1) in callbacks and do not rescan the registry per paint.
        if (qobject_cast<QProgressBar *>(widget)
            && !d->progressTimer->isActive())
            d->refreshProgressTimer();
        break;
    case QEvent::Enter:
        if (!widget->isEnabled()) {
            d->clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            const int generation = widget->property(scrollBarGenerationProperty).toInt() + 1;
            widget->setProperty(scrollBarInsideProperty, true);
            widget->setProperty(scrollBarGenerationProperty, generation);
            if (!animationsAllowed()) {
                d->cancelScrollBarTimer(scrollBar);
                d->animate(widget, hoverProperty, 1.0, 0);
            } else if (progress(widget, hoverProperty) > 0.001) {
                // Re-entering during contraction reverses from the current
                // thickness immediately. Waiting for a fresh 400 ms reveal
                // would make the thumb disappear under a stationary pointer.
                d->cancelScrollBarTimer(scrollBar);
                d->animate(widget, hoverProperty, 1.0, Private::FastDuration);
            } else {
                d->scheduleScrollBar(scrollBar, 400);
            }
        } else {
            d->animate(widget, hoverProperty, 1.0,
                       interactionDuration(widget, InteractionMotion::Hover,
                                           true));
        }
        break;
    case QEvent::Leave:
        if (!widget->isEnabled()) {
            d->clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget)) {
            const int generation = widget->property(scrollBarGenerationProperty).toInt() + 1;
            widget->setProperty(scrollBarInsideProperty, false);
            widget->setProperty(scrollBarGenerationProperty, generation);
            if (!animationsAllowed()) {
                d->cancelScrollBarTimer(scrollBar);
                d->animate(widget, hoverProperty, 0.0, 0);
            } else {
                d->scheduleScrollBar(scrollBar, 500);
            }
        } else {
            d->animate(widget, hoverProperty, 0.0,
                       interactionDuration(widget, InteractionMotion::Hover,
                                           false));
        }
        if (buttonPressPulse(widget))
            d->cancelButtonPress(widget);
        else
            d->animate(widget, pressProperty, 0.0,
                       interactionDuration(widget, InteractionMotion::Press,
                                           false));
        break;
    case QEvent::MouseButtonPress:
        d->keyboardInput = false;
        widget->setProperty(focusVisibleProperty, false);
        if (!widget->isEnabled()) {
            d->clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (auto *viewport = qobject_cast<QAbstractItemView *>(widget->parentWidget());
            viewport && viewport->viewport() == widget) {
            viewport->setProperty(focusVisibleProperty, false);
            viewport->viewport()->update();
        }
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                d->animate(combo, comboChevronProperty, 1.0, 150);
                prepareComboPopupFirstFrame(combo);
            }
        }
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                QRect track = checkBox->rect();
                if (checkBox->layoutDirection() == Qt::RightToLeft)
                    track.setLeft(track.right() - 39);
                else
                    track.setWidth(40);
                StylePrivate::ToggleDragState state;
                state.pressPosition = mouse->position().toPoint();
                state.candidate = track.contains(state.pressPosition);
                d->toggleDragStates.insert(checkBox, state);
            }
        }
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton && slider->isEnabled()) {
                d->scheduleSliderToolTip(slider);
            }
        }
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton)
            break;
        if (buttonPressPulse(widget))
            d->beginButtonPress(widget);
        else
            d->animate(widget, pressProperty, 1.0,
                       interactionDuration(widget, InteractionMotion::Press,
                                           true));
        break;
    case QEvent::MouseMove:
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if ((mouse->buttons() & Qt::LeftButton) && slider->isEnabled()) {
                d->scheduleSliderToolTip(slider);
            }
        }
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            auto state = d->toggleDragStates.find(checkBox);
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (state != d->toggleDragStates.end() && state->candidate
                && (mouse->buttons() & Qt::LeftButton)) {
                if (!state->dragging
                    && (mouse->position().toPoint() - state->pressPosition).manhattanLength()
                        >= QApplication::startDragDistance()) {
                    state->dragging = true;
                    checkBox->setProperty("_winui_toggle_dragging", true);
                }
                if (state->dragging) {
                    qreal position;
                    if (checkBox->layoutDirection() == Qt::RightToLeft)
                        position = (checkBox->rect().right() - 9.5
                                    - mouse->position().x()) / 20.0;
                    else
                        position = (mouse->position().x()
                                    - checkBox->rect().left() - 9.5) / 20.0;
                    checkBox->setProperty(togglePositionProperty,
                                          qBound<qreal>(0.0, position, 1.0));
                    checkBox->update();
                    return true;
                }
            }
        }
        break;
    case QEvent::MouseButtonRelease:
        if (!widget->isEnabled()) {
            d->clearPointerInteraction(widget);
            widget->update();
            break;
        }
        if (static_cast<const QMouseEvent *>(event)->button() != Qt::LeftButton) {
            if (auto *slider = qobject_cast<QSlider *>(widget)) {
                d->cancelSliderToolTip(slider);
                hideSliderValueToolTip(slider);
            }
            break;
        }
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            d->cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        if (qobject_cast<QComboBox *>(widget))
            d->releaseComboChevron(widget);
        if (auto *checkBox = qobject_cast<QCheckBox *>(widget);
            checkBox && toggleSwitch(checkBox)) {
            const auto state = d->toggleDragStates.take(checkBox);
            if (state.dragging) {
                const bool checked = progress(checkBox, togglePositionProperty) >= 0.5;
                checkBox->setProperty("_winui_toggle_dragging", false);
                checkBox->setDown(false);
                Q_EMIT checkBox->released();
                if (checkBox->isChecked() != checked)
                    checkBox->setChecked(checked);
                else
                    d->animate(checkBox, togglePositionProperty,
                               checked ? 1.0 : 0.0, FasterDuration);
                Q_EMIT checkBox->clicked(checkBox->isChecked());
                d->animate(checkBox, pressProperty, 0.0, FasterDuration);
                return true;
            }
        }
        if (buttonPressPulse(widget))
            d->releaseButtonPress(widget);
        else
            d->animate(widget, pressProperty, 0.0,
                       interactionDuration(widget, InteractionMotion::Press,
                                           false));
        break;
    case QEvent::FocusIn:
        if (const auto *focus = static_cast<QFocusEvent *>(event)) {
            const bool keyboard = focus->reason() == Qt::TabFocusReason
                || focus->reason() == Qt::BacktabFocusReason
                || focus->reason() == Qt::ShortcutFocusReason
                || d->keyboardInput;
            widget->setProperty(focusVisibleProperty, keyboard);
            if (auto *view = qobject_cast<QAbstractItemView *>(widget->parentWidget());
                view && view->viewport() == widget) {
                if (keyboard) {
                    view->setProperty(focusVisibleProperty, true);
                    view->update();
                }
            }
        }
        d->animate(widget, focusProperty, 1.0,
                   interactionDuration(widget, InteractionMotion::Focus,
                                       true));
        break;
    case QEvent::FocusOut:
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            d->cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        widget->setProperty(focusVisibleProperty, false);
        if (auto *view = qobject_cast<QAbstractItemView *>(widget->parentWidget());
            view && view->viewport() == widget) {
            view->setProperty(focusVisibleProperty, false);
            view->update();
        }
        d->animate(widget, focusProperty, 0.0,
                   interactionDuration(widget, InteractionMotion::Focus,
                                       false));
        break;
    case QEvent::ReadOnlyChange:
        // WinUI TextBox does not expose its delete affordance while it is
        // read-only. QLineEdit keeps the private clear button visible, so
        // suppress it after Qt has processed the property change.
        updateReadOnlyDeleteAffordance(qobject_cast<QLineEdit *>(widget));
        break;
    case QEvent::KeyPress:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *key = static_cast<QKeyEvent *>(event);
            const bool activates = key->key() == Qt::Key_Space
                || key->key() == Qt::Key_Enter || key->key() == Qt::Key_Return
                || key->key() == Qt::Key_F4
                || (key->key() == Qt::Key_Down
                    && key->modifiers().testFlag(Qt::AltModifier));
            if (activates) {
                d->animate(combo, comboChevronProperty, 1.0, 150);
                prepareComboPopupFirstFrame(combo);
            }
        }
        if (const auto *key = static_cast<QKeyEvent *>(event);
            revealsKeyboardFocus(key->key())) {
            d->keyboardInput = true;
        }
        if (d->keyboardInput) {
            widget->setProperty(focusVisibleProperty, true);
            widget->update();
        }
        break;
    case QEvent::KeyRelease:
        if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            const auto *key = static_cast<QKeyEvent *>(event);
            if (key->key() == Qt::Key_Space || key->key() == Qt::Key_Enter
                || key->key() == Qt::Key_Return || key->key() == Qt::Key_F4
                || (key->key() == Qt::Key_Down
                    && key->modifiers().testFlag(Qt::AltModifier))) {
                d->releaseComboChevron(combo);
            }
        }
        break;
    case QEvent::Show:
        updateReadOnlyDeleteAffordance(qobject_cast<QLineEdit *>(widget));
        // The view is shown before its popup window. Prepare selection and
        // scroll position here so even a programmatic first showPopup() has a
        // stable first composited frame; waiting for the popup Show event is
        // observably too late when the selected item is not row zero.
        if (qobject_cast<QAbstractItemView *>(widget))
            if (auto *combo = comboForPopupWidget(widget))
                prepareComboPopupFirstFrame(combo);
        if (widget->isWindow() && widget->windowType() == Qt::Popup) {
            if (auto *combo = qobject_cast<QComboBox *>(widget->parentWidget()))
                prepareComboPopupFirstFrame(combo);
        }
        if (auto *dialog = qobject_cast<QDialog *>(widget);
            dialog && (qobject_cast<QMessageBox *>(dialog)
                       || dialog->property(ContentDialogProperty).toBool())) {
            prepareContentDialogState(dialog, d->dark());
            stopDialogAnimations(dialog);
            if (animationsAllowed()) {
                widget->setProperty("_winui_dialog_animating", true);
                auto *group = new QParallelAnimationGroup(dialog);
                group->setObjectName(QStringLiteral("_winui_dialog_animation"));
                auto *opacity = new QPropertyAnimation(dialog, "windowOpacity", group);
                opacity->setStartValue(0.0);
                opacity->setEndValue(1.0);
                opacity->setDuration(Private::FasterDuration);
                connect(group, &QParallelAnimationGroup::finished, dialog,
                        [dialog, group] {
                    dialog->setWindowOpacity(1.0);
                    dialog->setProperty("_winui_dialog_animating", false);
                    group->deleteLater();
                });
                group->start();
            }
        }
        if (qobject_cast<QProgressBar *>(widget))
            d->refreshProgressTimer();
        if (qobject_cast<QMenu *>(widget)
            || (widget->isWindow() && widget->windowType() == Qt::ToolTip))
            applyBackdrop(widget, Backdrop::Acrylic);
        preparePopupSurface(widget);
        break;
    case QEvent::Hide:
        if (qobject_cast<QProgressBar *>(widget))
            d->refreshProgressTimer();
        if (auto *scrollBar = qobject_cast<QScrollBar *>(widget))
            d->cancelScrollBarTimer(scrollBar);
        if (auto *slider = qobject_cast<QSlider *>(widget)) {
            d->cancelSliderToolTip(slider);
            hideSliderValueToolTip(slider);
        }
        if (widget->isWindow() && widget->windowType() == Qt::Popup) {
            for (QWidget *candidate : qApp->allWidgets()) {
                if (auto *combo = qobject_cast<QComboBox *>(candidate);
                    combo && combo->view() && combo->view()->window() == widget) {
                    d->releaseComboChevron(combo);
                    break;
                }
            }
        }
        if (auto *dialog = qobject_cast<QDialog *>(widget);
            dialog && dialog->property("_winui_dialog_animating").toBool()) {
            stopDialogAnimations(dialog);
        }
        break;
    case QEvent::DynamicPropertyChange:
        if (const auto *change = static_cast<QDynamicPropertyChangeEvent *>(event)) {
            const QByteArray name = change->propertyName();
            if (name == ToggleSwitchProperty) {
                if (const auto *checkBox = qobject_cast<QCheckBox *>(widget))
                    widget->setProperty(togglePositionProperty,
                                        checkBox->isChecked() ? 1.0 : 0.0);
                widget->updateGeometry();
                widget->update();
            } else if (name == ToggleSwitchOnTextProperty
                       || name == ToggleSwitchOffTextProperty) {
                widget->updateGeometry();
                widget->update();
            } else if (name == SettingsCardProperty) {
                if (auto *frame = qobject_cast<QFrame *>(widget)) {
                    if (widget->property(SettingsCardProperty).toBool()) {
                        remember(frame, originalFrameShapeProperty,
                                 int(frame->frameShape()));
                        frame->setFrameShape(QFrame::StyledPanel);
                    } else if (widget->property(originalFrameShapeProperty).isValid()) {
                        frame->setFrameShape(static_cast<QFrame::Shape>(
                            widget->property(originalFrameShapeProperty).toInt()));
                        widget->setProperty(originalFrameShapeProperty, {});
                    }
                }
                widget->updateGeometry();
                widget->update();
            } else if (name == ContentDialogProperty) {
                if (auto *dialog = qobject_cast<QDialog *>(widget);
                    dialog && !qobject_cast<QMessageBox *>(dialog)) {
                    if (dialog->property(ContentDialogProperty).toBool())
                        prepareContentDialogState(dialog, d->dark());
                    else
                        restoreContentDialogState(dialog, true);
                }
            } else if (name == VerticalSpinButtonsProperty) {
                widget->updateGeometry();
                widget->update();
            } else if (name == NavigationViewProperty) {
                if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
                    view->viewport()->setProperty(NavigationViewProperty,
                                                  view->property(NavigationViewProperty));
                    if (view->property(NavigationViewProperty).toBool())
                        prepareNavigationView(view);
                    else
                        restoreNavigationView(view);
                    view->viewport()->update();
                }
            }
        }
        break;
    default:
        break;
    }
    return QProxyStyle::eventFilter(watched, event);
}

} // namespace WinUI3
