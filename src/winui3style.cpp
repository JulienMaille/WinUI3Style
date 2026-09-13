// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3style.h>
#include "winui3qtcompat_p.h"

#include <winui3style/winui3backdrop.h>
#include <winui3style/winui3icons.h>

#include "winui3geometry_p.h"
#include "winui3density_p.h"
#include "winui3animations_p.h"
#include "winui3backdrop_p.h"
#include "winui3buttons_p.h"
#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3menus_p.h"
#include "winui3style_contracts_p.h"
#include "winui3complex_p.h"
#include "winui3viewrenderers_p.h"
#include "navigationview_p.h"
#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3interactions_p.h"
#include "winui3surfaces_p.h"
#include "winui3tableeditors_p.h"
#include "winui3theme_p.h"
#include "winui3tokens_p.h"
#include "winui3appearancewatcher_p.h"

#include <QAbstractButton>
#include <QAbstractItemDelegate>
#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QApplication>
#include <QCalendarWidget>
#include <QComboBox>
#include <QCompleter>
#include <QCheckBox>
#include <QCommonStyle>
#include <QDateTime>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFrame>
#include <QFontDatabase>
#include <QGroupBox>
#include <QGuiApplication>
#include <QLinearGradient>
#include <QListView>
#include <QLabel>
#include <QLineEdit>
#include <QLayout>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QStatusBar>
#include <QStyleOptionSizeGrip>
#include <QScrollBar>
#include <QSlider>
#include <QStyleOption>
#include <QStyleOptionProgressBar>
#include <QToolTip>
#include <QStyleHints>
#include <QStyledItemDelegate>
#include <QTabBar>
#include <QTableView>
#include <QTextEdit>
#include <QThread>
#include <QToolBar>
#include <QWizard>
#include <QToolButton>
#include <QTimer>
#include <QVector>
#include <QWidget>

namespace WinUI3 {
using namespace PaintPrivate;
using namespace Private;
namespace {
Backdrop backdropFromProperty(const QVariant &value)
{
    const QString name = value.toString().trimmed().toLower();
    if (name == QLatin1String("mica"))
        return Backdrop::Mica;
    if (name == QLatin1String("micaalt") || name == QLatin1String("mica-alt"))
        return Backdrop::MicaAlt;
    if (name == QLatin1String("acrylic"))
        return Backdrop::Acrylic;
    if (name == QLatin1String("none"))
        return Backdrop::None;
    bool ok = false;
    const int numeric = value.toInt(&ok);
    if (ok && numeric >= static_cast<int>(Backdrop::None)
        && numeric <= static_cast<int>(Backdrop::Acrylic))
        return static_cast<Backdrop>(numeric);
    return Backdrop::None;
}

constexpr auto wizardFooterName = "_winui_wizard_footer_surface";

class WizardFooterSurface final : public QWidget
{
public:
    explicit WizardFooterSurface(QWizard *wizard) : QWidget(wizard), m_wizard(wizard)
    {
        setObjectName(QString::fromLatin1(wizardFooterName));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        wizard->installEventFilter(this);
        queueGeometrySync();
    }

    void setColors(const QColor &fill, const QColor &stroke)
    {
        if (m_fill == fill && m_stroke == stroke) {
            // A theme switch can re-resolve to the same stored colors while
            // the backing store still holds the previous theme's pixels
            // (live defect 2026-09-09: white footer in Dark). Force repaint.
            update();
            return;
        }
        m_fill = fill;
        m_stroke = stroke;
        update();
    }

    void queueGeometrySync()
    {
        if (m_syncQueued)
            return;
        m_syncQueued = true;
        QTimer::singleShot(0, this, [this] {
            m_syncQueued = false;
            syncGeometry();
        });
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_wizard
            && (event->type() == QEvent::Resize || event->type() == QEvent::Show
                || event->type() == QEvent::LayoutRequest || event->type() == QEvent::ChildAdded)) {
            queueGeometrySync();
        }
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), m_fill);
        painter.setPen(QPen(m_stroke, 1));
        painter.drawLine(rect().topLeft(), rect().topRight());
    }

private:
    void syncGeometry()
    {
        if (!m_wizard)
            return;

        int buttonTop = m_wizard->height();
        bool hasButton = false;
        const QWizard::WizardButton buttons[] = { QWizard::BackButton,    QWizard::NextButton,
                                                  QWizard::CommitButton,  QWizard::FinishButton,
                                                  QWizard::CancelButton,  QWizard::HelpButton,
                                                  QWizard::CustomButton1, QWizard::CustomButton2,
                                                  QWizard::CustomButton3 };
        for (QWizard::WizardButton role : buttons) {
            if (QAbstractButton *button = m_wizard->button(role); button && button->isVisible()) {
                buttonTop = qMin(buttonTop, button->mapTo(m_wizard, QPoint()).y());
                hasButton = true;
            }
        }
        if (!hasButton) {
            hide();
            return;
        }

        // QWizard's native button row has a 12 px breathing room above and
        // below the controls. Paint the command surface behind that complete
        // row, including the native separator's former backing pixels.
        const int top = qBound(0, buttonTop - 12, m_wizard->height());
        setGeometry(0, top, m_wizard->width(), m_wizard->height() - top);
        lower();
        setVisible(m_wizard->isVisible());
    }

    QPointer<QWizard> m_wizard;
    QColor m_fill;
    QColor m_stroke;
    bool m_syncQueued = false;
};

WizardFooterSurface *wizardFooterSurface(QWizard *wizard, bool create)
{
    if (!wizard)
        return nullptr;
    auto *surface = static_cast<WizardFooterSurface *>(wizard->findChild<QWidget *>(
            QString::fromLatin1(wizardFooterName), Qt::FindDirectChildrenOnly));
    if (!surface && create)
        surface = new WizardFooterSurface(wizard);
    return surface;
}

// QWizard creates its page stack and banner widgets with explicit palettes
// chosen by the platform style. Those palettes are not updated when the
// application palette changes, and on Windows they can default to a light
// surface even while the rest of the application is dark. Keep every
// framework-owned content child on the raised content layer and every button
// on the command palette. The footer widget supplies the full-width command
// surface behind the native button row.
void refreshWizardSurface(QWizard *wizard, const QPalette &applicationPalette)
{
    if (!wizard)
        return;

    QPalette contentPalette = applicationPalette;
    contentPalette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
    QPalette commandPalette = applicationPalette;
    const auto isWizardButton = [wizard](QWidget *candidate) {
        for (QWizard::WizardButton role :
             { QWizard::BackButton, QWizard::NextButton, QWizard::CommitButton,
               QWizard::FinishButton, QWizard::CancelButton, QWizard::HelpButton,
               QWizard::CustomButton1, QWizard::CustomButton2, QWizard::CustomButton3 }) {
            if (wizard->button(role) == candidate)
                return true;
        }
        return false;
    };
    const auto descendants = wizard->findChildren<QWidget *>();
    for (QWidget *child : descendants) {
        if (!child)
            continue;
        const bool wizardButton = isWizardButton(child);
        const bool internalButton = qobject_cast<QAbstractButton *>(child)
                && (!child->property(originalPaletteExplicitProperty).toBool() || wizardButton);
        const bool hasUserPalette = child->property(originalPaletteExplicitProperty).toBool()
                && !child->property(ownedPaletteProperty).toBool() && !wizardButton;
        if (hasUserPalette)
            continue;

        child->setPalette(internalButton ? commandPalette : contentPalette);
        if (auto *label = qobject_cast<QLabel *>(child)) {
            // QWizard's internal title/description labels carry the platform
            // Link-role ink, unreadable on a dark page. Force content ink.
            QPalette labelPalette = contentPalette;
            labelPalette.setColor(QPalette::WindowText, contentPalette.color(QPalette::WindowText));
            labelPalette.setColor(QPalette::Text, contentPalette.color(QPalette::Text));
            labelPalette.setColor(QPalette::Link, contentPalette.color(QPalette::WindowText));
            labelPalette.setColor(QPalette::LinkVisited,
                                  contentPalette.color(QPalette::WindowText));
            label->setPalette(labelPalette);
        }
        if (!internalButton
            && (qobject_cast<QWizardPage *>(child) || qobject_cast<QFrame *>(child)
                || child->parentWidget() == wizard))
            child->setAutoFillBackground(true);
    }

    // The top-level wizard is the content backing layer. The footer is a
    // separate child so the command region remains full-width and opaque.
    wizard->setPalette(contentPalette);
    wizard->setAutoFillBackground(true);
    if (WizardFooterSurface *footer = wizardFooterSurface(wizard, true)) {
        const QColor commandFill = applicationPalette.color(QPalette::Window);
        footer->setColors(commandFill, tokens(commandPalette).stroke);
        footer->queueGeometrySync();
    }
}

const QAbstractItemView *itemView(const QWidget *widget)
{
    if (const auto *view = qobject_cast<const QAbstractItemView *>(widget))
        return view;
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (const auto *view = qobject_cast<const QAbstractItemView *>(candidate))
            return view;
    }
    return nullptr;
}

bool insideCalendarWidget(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QCalendarWidget *>(candidate))
            return true;
    }
    return false;
}

// CalendarViewBackground is InputActive over its enclosing content/card,
// not the translucent item-view Base and not the popup acrylic recipe.
QColor inlineCalendarSurface(const QWidget *widget, const QPalette &applicationPalette)
{
    const QWidget *calendar = widget;
    while (calendar && !qobject_cast<const QCalendarWidget *>(calendar))
        calendar = calendar->parentWidget();
    const Private::Tokens t = Private::tokens(applicationPalette);
    const bool composited = widget
            && Private::backdropEffectiveSurface(widget->window())
                    == Private::BackdropSurface::Composited;
    QImage pixel(1, 1, QImage::Format_ARGB32_Premultiplied);
    // Granted Mica has no opaque Window under the card: start transparent so
    // the card veil and InputActive compose their alphas instead of baking
    // onto an opaque seed. Refused/offscreen fallbacks keep the opaque
    // surface seed and resolve fully opaque below.
    if (composited)
        pixel.fill(Qt::transparent);
    else
        pixel.fill(t.surface);
    QPainter painter(&pixel);
    QList<const QWidget *> ancestors;
    for (const QWidget *parent = calendar ? calendar->parentWidget() : nullptr; parent;
         parent = parent->parentWidget())
        ancestors.prepend(parent);
    for (const QWidget *parent : ancestors) {
        if (parent->property(Style::SurfaceProperty).toString() == QLatin1String("layer"))
            painter.fillRect(pixel.rect(), Private::popupSurfaceColor(applicationPalette));
        else if (qobject_cast<const QGroupBox *>(parent))
            painter.fillRect(
                    pixel.rect(),
                    Private::veiledCard(t.layer,
                                        composited && Private::paintsDirectlyOnBackdrop(parent)));
    }
    painter.fillRect(pixel.rect(), t.editorFocusedFill);
    painter.end();
    return pixel.pixelColor(0, 0);
}

// The navigation-bar lane (qt_calendar_navigationbar) paints its empty
// stretches with its own Window fill while the month/year/prev/next
// QToolButtons paint over it. The lane reads as one surface only when every
// button child shares the bar's opaque Window surface instead of the default
// Button role/palette.
bool insideCalendarNavigationBar(const QWidget *widget)
{
    const QWidget *parent = widget ? widget->parentWidget() : nullptr;
    return parent && parent->objectName() == QStringLiteral("qt_calendar_navigationbar")
            && insideCalendarWidget(widget);
}

const QWidget *richTextEditor(const QWidget *widget)
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        if (qobject_cast<const QTextEdit *>(candidate)
            || qobject_cast<const QPlainTextEdit *>(candidate)) {
            return candidate;
        }
    }
    return nullptr;
}

const QEasingCurve &fluentCurve()
{
    static const QEasingCurve curve = [] {
        QEasingCurve result(QEasingCurve::BezierSpline);
        result.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.0, 1.0), QPointF(1.0, 1.0));
        return result;
    }();
    return curve;
}

bool animationsAllowed()
{
    return Style::animationsAllowed();
}

bool densityModeFromProperty(const QVariant &value, WinUI3::DensityMode *mode)
{
    return Private::parseDensity(value, mode);
}

constexpr auto completerLastPopupProperty = "_winui_completer_last_popup";
constexpr auto completerOriginalDensityProperty = "_winui_completer_original_density";
constexpr auto completerOriginalDensityValidProperty = "_winui_completer_original_density_valid";

void syncCompleterPopupDensity(QLineEdit *editor)
{
    if (!editor || !editor->completer() || !editor->completer()->popup())
        return;
    QWidget *popup = editor->completer()->popup();
    const quintptr owner = reinterpret_cast<quintptr>(editor->style());
    auto restorePopup = [owner](QWidget *candidate) {
        if (!candidate || candidate->property(completerOwnerProperty).value<quintptr>() != owner)
            return;
        if (candidate->property(completerOriginalDensityValidProperty).toBool())
            candidate->setProperty(Style::DensityProperty,
                                   candidate->property(completerOriginalDensityProperty));
        else
            candidate->setProperty(Style::DensityProperty, {});
        for (const char *property : { completerOwnerProperty, completerOriginalDensityProperty,
                                      completerOriginalDensityValidProperty })
            candidate->setProperty(property, {});
    };
    auto *previous = qobject_cast<QWidget *>(
            editor->property(completerLastPopupProperty).value<QObject *>());
    if (previous && previous != popup)
        restorePopup(previous);
    if (previous != popup) {
        editor->setProperty(completerLastPopupProperty, QVariant::fromValue<QObject *>(popup));
        QObject::connect(popup, &QObject::destroyed, editor, [editor](QObject *destroyed) {
            if (editor->property(completerLastPopupProperty).value<QObject *>() == destroyed)
                editor->setProperty(completerLastPopupProperty, {});
        });
    }
    if (!popup->property(completerOwnerProperty).isValid()) {
        popup->setProperty(completerOwnerProperty, QVariant::fromValue(owner));
        popup->setProperty(completerOriginalDensityValidProperty,
                           popup->property(Style::DensityProperty).isValid());
        popup->setProperty(completerOriginalDensityProperty,
                           popup->property(Style::DensityProperty));
    }
    if (popup->property(completerOwnerProperty).value<quintptr>() != owner)
        return;
    const QVariant density = QVariant::fromValue(Style::densityMode(editor));
    if (popup->property(Style::DensityProperty) != density)
        popup->setProperty(Style::DensityProperty, density);
    // QCompleter's private delegate caches its size hint. FontChange is the
    // least invasive public invalidation event that clears that cache; a
    // geometry update alone leaves the old 12 px native row in place.
    QEvent fontChange(QEvent::FontChange);
    QCoreApplication::sendEvent(popup, &fontChange);
    popup->updateGeometry();
    if (auto *view = qobject_cast<QAbstractItemView *>(popup)) {
        view->doItemsLayout();
        if (view->viewport())
            view->viewport()->update();
    }
}

void restoreCompleterPopup(QLineEdit *editor, Style *style)
{
    if (!editor)
        return;
    auto *popup = qobject_cast<QWidget *>(
            editor->property(completerLastPopupProperty).value<QObject *>());
    if (!popup) {
        editor->setProperty(completerLastPopupProperty, {});
        return;
    }
    if (popup->property(completerOwnerProperty).value<quintptr>()
        != reinterpret_cast<quintptr>(style))
        return;
    if (popup->property(completerOriginalDensityValidProperty).toBool())
        popup->setProperty(Style::DensityProperty,
                           popup->property(completerOriginalDensityProperty));
    else
        popup->setProperty(Style::DensityProperty, {});
    for (const char *property : { completerOwnerProperty, completerOriginalDensityProperty,
                                  completerOriginalDensityValidProperty })
        popup->setProperty(property, {});
    editor->setProperty(completerLastPopupProperty, {});
}

void invalidateDensityTree(QWidget *root)
{
    if (!root)
        return;
    // Bottom-up : les enfants d'abord, le parent ensuite. Un layout parent
    // repositionne ses enfants ; si le parent repeint avant, ses lignes
    // backing-store gardent les enfants a l'ancienne geometrie (doublons
    // "Palette lab", combos superposes). Le layout doit etre actif avant le
    // repaint, sinon updateGeometry() ne fait que marquer dirty.
    const auto invalidateWidget = [](QWidget *widget) {
        if (widget->layout())
            widget->layout()->activate();
        widget->updateGeometry();
        // Density changes resize rows/controls in place: every backing store
        // keeps stale rows exactly like a scroll blit (the left-pane smear
        // when switching Standard/Compact). update() only repaints the
        // exposed strip, so force the synchronous full repaint here, like
        // the island scroll guard does after a valueChanged tick.
        widget->repaint();
        if (auto *editor = qobject_cast<QLineEdit *>(widget))
            syncCompleterPopupDensity(editor);
        if (auto *completerView = qobject_cast<QAbstractItemView *>(widget)) {
            // Hidden completer popups are top-level native popups: no density
            // ancestor reaches them from the editor tree, and the delegate
            // caches its sizeHint. Re-sync any completer view (claimed or
            // fresh from QCompleter::popup()) through the same editor path
            // so a hidden switch lands before the next show.
            const bool completerViewClaimed =
                    completerView->property(completerOwnerProperty).isValid()
                    || qobject_cast<QCompleter *>(completerView->parent());
            if (completerViewClaimed) {
                if (auto *completer = qobject_cast<QCompleter *>(completerView->parent())) {
                    if (auto *reanchor = qobject_cast<QLineEdit *>(completer->widget()))
                        syncCompleterPopupDensity(reanchor);
                }
            }
        }
        if (qobject_cast<QMenuBar *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(widget, &styleChange);
        } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(combo, &styleChange);
            combo->updateGeometry();
            // Open popup: rows and geometry follow density. Rows are already
            // re-laid out (doItemsLayout below), but Qt never re-lays out a
            // visible popup on its own: its height keeps the Standard rows
            // (fail-first openComboPopupFollowsDensitySwitch: 220x120 instead
            // of 220x96 for 3 rows 40 -> 32). Re-prepare the frame, then
            // resize the popup to the new rows.
            if (combo->view() && combo->view()->isVisible()) {
                combo->view()->doItemsLayout();
                Private::prepareComboPopupFirstFrameImpl(combo);
                if (QWidget *popup = combo->view()->window()) {
                    // Measure through the delegate (sizeHint exposes the
                    // comboPopupItemHeight metric), not through visualRect:
                    // the viewport layout has not picked up the new rows
                    // yet at switch time.
                    const QModelIndex first =
                            combo->model()->index(0, combo->modelColumn(), combo->rootModelIndex());
                    QStyleOptionViewItem itemOption;
                    itemOption.initFrom(combo->view()->viewport());
                    itemOption.index = first;
                    const QSize rowSize = combo->style()->sizeFromContents(
                            QStyle::CT_ItemViewItem, &itemOption, QSize(), combo->view());
                    const int margins =
                            popup->contentsMargins().top() + popup->contentsMargins().bottom();
                    popup->resize(popup->width(), combo->count() * rowSize.height() + margins);
                }
            }
        } else if (auto *spinBox = qobject_cast<QAbstractSpinBox *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(spinBox, &styleChange);
            spinBox->updateGeometry();
        } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(button, &styleChange);
            button->updateGeometry();
        } else if (auto *menu = qobject_cast<QMenu *>(widget)) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(menu, &styleChange);
            // Visible popup: rows follow the profile like the open combo
            // popup above. QMenu caches its layout while shown, so force a
            // resize to the new row profile.
            if (menu->isVisible() && !menu->actions().isEmpty()) {
                const int compact = qobject_cast<const Style *>(menu->style())
                        ? densityMetricsFor(menu, qobject_cast<const Style *>(menu->style()))
                                  .menuBarItemHeight
                        : densityMetricsFor(menu).menuBarItemHeight;
                const int row = compact == 24 ? 32 : 36;
                const int width = menu->width();
                menu->resize(width, menu->actions().size() * row);
            }
            menu->updateGeometry();
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->doItemsLayout();
            if (view->viewport())
                view->viewport()->update();
        }
    };
    // Bottom-up: leaves first. findChildren returns parents before children,
    // so walk in reverse. Each level is repositioned and repainted before
    // its parent freezes its own rows.
    const auto descendants = root->findChildren<QWidget *>(QString(), Qt::FindChildrenRecursively);
    for (auto it = descendants.crbegin(); it != descendants.crend(); ++it)
        invalidateWidget(*it);
    invalidateWidget(root);
}

Icon arrowIcon(QStyle::PrimitiveElement element)
{
    switch (element) {
    case QStyle::PE_IndicatorArrowDown:
        return Icon::ChevronDown;
    case QStyle::PE_IndicatorArrowLeft:
        return Icon::ChevronLeft;
    case QStyle::PE_IndicatorArrowRight:
        return Icon::ChevronRight;
    case QStyle::PE_IndicatorArrowUp:
        return Icon::ChevronUp;
    default:
        return Icon::ChevronRight;
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
    case QStyle::PE_FrameMenu:
    case QStyle::PE_Widget:
    case QStyle::PE_PanelItemViewItem:
    case QStyle::PE_IndicatorBranch:
    case QStyle::PE_IndicatorHeaderArrow:
    case QStyle::PE_IndicatorTabClose:
    case QStyle::PE_PanelTipLabel:
    case QStyle::PE_IndicatorToolBarSeparator:
    case QStyle::PE_FrameDockWidget:
    case QStyle::PE_IndicatorDockWidgetResizeHandle:
    case QStyle::PE_PanelStatusBar:
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
    case QStyle::CE_SizeGrip:
        return true;
    default:
        return false;
    }
}

} // namespace

class StylePrivate
{
public:
    using ToggleDragState = Private::ToggleDragState;

    explicit StylePrivate(Style *owner, ThemeMode initialMode, WinUI3::DensityMode initialDensity)
        : q(owner),
          mode(initialMode),
          density(initialDensity),
          animationDriver(owner),
          tableEditorTracker(owner)
    {
        Private::StyleInteractionCallbacks callbacks;
        callbacks.animate = [this](QWidget *widget, const char *property, qreal target,
                                   int duration) { animate(widget, property, target, duration); };
        callbacks.beginButtonPress = [this](QWidget *widget) { beginButtonPress(widget); };
        callbacks.releaseButtonPress = [this](QWidget *widget) { releaseButtonPress(widget); };
        callbacks.cancelButtonPress = [this](QWidget *widget) { cancelButtonPress(widget); };
        callbacks.stopAnimations = [this](QWidget *widget) { stopAnimations(widget); };
        callbacks.clearPointerInteraction = [this](QWidget *widget) {
            clearPointerInteraction(widget);
        };
        callbacks.cancelScrollBarTimer = [this](QScrollBar *scrollBar) {
            cancelScrollBarTimer(scrollBar);
        };
        callbacks.scheduleScrollBar = [this](QScrollBar *scrollBar, int delay) {
            scheduleScrollBar(scrollBar, delay);
        };
        callbacks.scheduleSliderToolTip = [this](QSlider *slider) {
            scheduleSliderToolTip(slider);
        };
        callbacks.cancelSliderToolTip = [this](QSlider *slider) { cancelSliderToolTip(slider); };
        callbacks.refreshProgressTimer = [this] { refreshProgressTimer(); };
        callbacks.progressTimerActive = [this] {
            return progressTimer && progressTimer->isActive();
        };
        callbacks.prepareComboPopupFirstFrame = [this](QComboBox *combo) {
            prepareComboPopupFirstFrame(combo);
        };
        callbacks.releaseComboChevron = [this](QWidget *widget) { releaseComboChevron(widget); };
        callbacks.finishComboPopupCycle = [this](QWidget *popup) { finishComboPopupCycle(popup); };
        callbacks.comboForPopupWidget = [](QWidget *widget) { return comboForPopupWidget(widget); };
        callbacks.updateReadOnlyDeleteAffordance = [](QLineEdit *lineEdit) {
            updateReadOnlyDeleteAffordance(lineEdit);
        };
        callbacks.prepareLineEditHelperButtons = [owner](QLineEdit *lineEdit) {
            prepareLineEditHelperButtons(lineEdit, owner);
        };
        callbacks.prepareContentDialogState = [](QDialog *dialog, bool dark) {
            prepareContentDialogState(dialog, dark);
        };
        callbacks.stopDialogAnimations = [](QDialog *dialog) { stopDialogAnimations(dialog); };
        callbacks.preparePopupSurface = [](QWidget *widget) { preparePopupSurface(widget); };
        callbacks.registerPopupPaletteOwners = [this](QWidget *widget) {
            registerPopupPaletteOwners(widget);
        };
        callbacks.registerPaletteOwner = [this](QDialog *dialog) { registerPaletteOwner(dialog); };
        callbacks.unregisterPaletteOwner = [this](QDialog *dialog) {
            unregisterPaletteOwner(dialog);
        };
        callbacks.restoreContentDialogState = [](QDialog *dialog, bool visible) {
            restoreContentDialogState(dialog, visible);
        };
        callbacks.remember = [](QWidget *widget, const char *property, const QVariant &value) {
            remember(widget, property, value);
        };
        callbacks.prepareNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::prepareNavigationView(view);
        };
        callbacks.restoreNavigationView = [](QAbstractItemView *view) {
            NavigationPrivate::restoreNavigationView(view);
        };
        callbacks.dark = [this] { return dark(); };
        callbacks.keyboardInput = &keyboardInput;
        callbacks.toggleDragStates = &toggleDragStates;
        interactionController =
                std::make_unique<Private::StyleInteractionController>(owner, std::move(callbacks));
    }

    bool needsSystemAppearancePolling() const
    {
        return mode == ThemeMode::System || !accent.isValid();
    }

    void restartSystemAppearanceWatchdog()
    {
        if (!systemAppearanceWatchdog)
            return;
        if (!applicationStyleActive || !needsSystemAppearancePolling()) {
            systemAppearanceWatchdog->stop();
            return;
        }
        Private::invalidateSystemAppearanceCache();
        if (mode == ThemeMode::System)
            lastSystemDark = Private::systemUsesDarkTheme();
        if (!accent.isValid())
            lastSystemAccent = Private::systemAccentColor();
        // Native notifications provide the fast path on Windows. Keep a
        // deliberately slow watchdog for missed broadcasts and portable
        // platforms where the watcher is a no-op.
        systemAppearanceWatchdog->start();
    }

    void prunePaletteOwners()
    {
        for (auto it = paletteOwners.begin(); it != paletteOwners.end();) {
            QWidget *widget = it->data();
            if (!widget || !widget->property(ownedPaletteProperty).toBool()) {
                if (widget) {
                    if (const auto connection = paletteOwnerConnections.take(widget))
                        QObject::disconnect(connection);
                }
                it = paletteOwners.erase(it);
            } else {
                ++it;
            }
        }
    }

    void registerPaletteOwner(QWidget *widget)
    {
        if (!widget || !widget->property(ownedPaletteProperty).toBool())
            return;
        prunePaletteOwners();
        for (const QPointer<QWidget> &owner : paletteOwners) {
            if (owner.data() == widget)
                return;
        }
        paletteOwners.append(QPointer<QWidget>(widget));
        paletteOwnerConnections.insert(
                widget, QObject::connect(widget, &QObject::destroyed, q, [this, widget] {
                    unregisterPaletteOwner(widget);
                }));
    }

    void unregisterPaletteOwner(QWidget *widget)
    {
        if (!widget)
            return;
        if (const auto connection = paletteOwnerConnections.take(widget))
            QObject::disconnect(connection);
        for (auto it = paletteOwners.begin(); it != paletteOwners.end();) {
            if (it->isNull() || it->data() == widget)
                it = paletteOwners.erase(it);
            else
                ++it;
        }
    }

    void clearPaletteOwners()
    {
        for (const auto &connection : paletteOwnerConnections)
            QObject::disconnect(connection);
        paletteOwnerConnections.clear();
        paletteOwners.clear();
    }

    void registerPopupPaletteOwners(QWidget *widget)
    {
        if (!widget)
            return;
        QWidget *popup = widget->window();
        if (!popup || popup->windowType() != Qt::Popup)
            return;

        // preparePopupSurface() has just replaced these palettes with a
        // style-owned, application-palette-based surface. Mark only those
        // surfaces as owned; explicit palettes are still rebased by the
        // effectivePopupPalette() path when they are refreshed.
        popup->setProperty(ownedPaletteProperty, true);
        registerPaletteOwner(popup);
        QAbstractItemView *view = qobject_cast<QAbstractItemView *>(widget);
        if (!view)
            view = popup->findChild<QAbstractItemView *>();
        if (!view)
            return;
        view->setProperty(ownedPaletteProperty, true);
        registerPaletteOwner(view);
        if (QWidget *viewport = view->viewport()) {
            viewport->setProperty(ownedPaletteProperty, true);
            registerPaletteOwner(viewport);
        }
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
        const qreal phase =
                allowed ? qreal(QDateTime::currentMSecsSinceEpoch() % 1500) / 1500.0 : 0.35;
        bool active = false;
        for (auto it = progressBars.begin(); it != progressBars.end();) {
            const QPointer<QProgressBar> guarded = *it;
            if (!guarded) {
                it = progressBars.erase(it);
                continue;
            }
            if (progressBarNeedsAnimation(guarded)) {
                active = true;
                framePropertyRegistry().set(guarded, progressPhaseProperty, phase);
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
        progressBarStateConnections.insert(
                progressBar,
                QObject::connect(progressBar, &QProgressBar::valueChanged, q,
                                 [this](int) { refreshProgressTimer(); }));
        QObject::connect(progressBar, &QObject::destroyed, q,
                         [this, progressBar] { unregisterProgressBar(progressBar); });
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
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end() && it->data()) {
            return it->data();
        }
        auto *timer = new QTimer(scrollBar);
        timer->setObjectName(QStringLiteral("_winui_scrollbar_timer"));
        timer->setSingleShot(true);
        const QPointer<QScrollBar> guarded(scrollBar);
        QObject::connect(timer, &QTimer::timeout, q, [this, guarded] {
            if (!guarded || !guarded->isVisible() || !guarded->isEnabled()
                || !framePropertyRegistry().value(guarded, scrollBarInsideProperty).isValid()) {
                return;
            }
            if (framePropertyRegistry().value(guarded, scrollBarInsideProperty).toBool()) {
                animate(guarded, hoverProperty, 1.0, Private::FastDuration);
            } else {
                animate(guarded, hoverProperty, 0.0, Private::FastDuration);
            }
        });
        scrollBarTimers.insert(scrollBar, QPointer<QTimer>(timer));
        // Pure parent ownership: the timer is parented to the scroll bar, so
        // Qt deletes it with its parent. The destroyed connection is tracked
        // explicitly so unregister (e.g. unpolish) can disconnect it and no
        // connection accumulates across polish/unpolish cycles.
        if (const auto previous = scrollBarDestroyConnections.take(scrollBar))
            QObject::disconnect(previous);
        scrollBarDestroyConnections.insert(
                scrollBar, QObject::connect(scrollBar, &QObject::destroyed, q, [this, scrollBar] {
                    unregisterScrollBar(scrollBar);
                }));
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
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end() && it->data()) {
            it->data()->stop();
        }
    }

    void unregisterScrollBar(QScrollBar *scrollBar)
    {
        if (!scrollBar)
            return;
        // Pure parent ownership: stop and erase only. The timer is a child
        // of the scroll bar, so an explicit delete here would double-delete
        // during parent teardown. Disconnect the tracked destroyed
        // connection so repeated polish/unpolish cannot accumulate it.
        if (const auto connection = scrollBarDestroyConnections.take(scrollBar))
            QObject::disconnect(connection);
        if (auto it = scrollBarTimers.find(scrollBar); it != scrollBarTimers.end()) {
            if (QTimer *timer = it->data())
                timer->stop();
            scrollBarTimers.erase(it);
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
        // Pure parent ownership, mirroring the scroll bar timers above: the
        // timer is parented to the slider and the destroyed connection is
        // tracked so unregister can disconnect it (no accumulation).
        if (const auto previous = sliderToolTipDestroyConnections.take(slider))
            QObject::disconnect(previous);
        sliderToolTipDestroyConnections.insert(
                slider, QObject::connect(slider, &QObject::destroyed, q, [this, slider] {
                    unregisterSlider(slider);
                }));
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
        // Pure parent ownership, mirroring unregisterScrollBar: stop and
        // erase only (no manual delete; the parented timer dies with the
        // slider), and disconnect the tracked destroyed connection.
        if (const auto connection = sliderToolTipDestroyConnections.take(slider))
            QObject::disconnect(connection);
        if (auto it = sliderToolTipTimers.find(slider); it != sliderToolTipTimers.end()) {
            if (QTimer *timer = it->data())
                timer->stop();
            sliderToolTipTimers.erase(it);
        }
    }

    void animate(QWidget *widget, const char *property, qreal target, int duration)
    {
        animationDriver.animate(widget, property, target, duration, animationsAllowed(),
                                fluentCurve());
    }

    void stopAnimations(QWidget *widget) { animationDriver.stop(widget); }

    void beginButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, false);
        // A synchronous pressed frame is intentional. It makes a very fast
        // click observable and also cancels a release animation already in
        // flight before the next press starts.
        animate(widget, pressProperty, 1.0, 0);
    }

    void releaseButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, true);
        const QPointer<QWidget> guardedWidget(widget);
        QTimer::singleShot(16, q, [this, guardedWidget, generation] {
            if (!guardedWidget
                || framePropertyRegistry()
                                .value(guardedWidget, buttonPressGenerationProperty)
                                .toULongLong()
                        != generation
                || !framePropertyRegistry()
                            .value(guardedWidget, buttonPressReleasePendingProperty)
                            .toBool()) {
                return;
            }
            framePropertyRegistry().set(guardedWidget, buttonPressReleasePendingProperty, false);
            animate(guardedWidget, pressProperty, 0.0, Private::FasterDuration);
        });
    }

    void cancelButtonPress(QWidget *widget)
    {
        if (!widget)
            return;
        const qulonglong generation =
                framePropertyRegistry().value(widget, buttonPressGenerationProperty).toULongLong()
                + 1;
        framePropertyRegistry().set(widget, buttonPressGenerationProperty,
                                    QVariant::fromValue(generation));
        framePropertyRegistry().set(widget, buttonPressReleasePendingProperty, false);
        animate(widget, pressProperty, 0.0, Private::FasterDuration);
    }

    void clearPointerInteraction(QWidget *widget)
    {
        if (!widget)
            return;
        if (buttonPressPulse(widget))
            cancelButtonPress(widget);
        stopAnimations(widget);
        framePropertyRegistry().set(widget, hoverProperty, 0.0);
        framePropertyRegistry().set(widget, pressProperty, 0.0);
    }

    void releaseComboChevron(QWidget *widget)
    {
        if (!widget)
            return;
        const qreal start = progress(widget, comboChevronProperty, 0.0);
        if (!animationsAllowed() || qFuzzyIsNull(start)) {
            animationDriver.stop(widget);
            framePropertyRegistry().set(widget, comboChevronProperty, 0.0);
            widget->update();
            return;
        }
        // AnimatedChevronDownSmallVisualSource: PressedToNormal moves from
        // y=31.5 to y=21 then y=24 on a 48 px canvas. At the 12 px ComboBox
        // glyph this is +1.875 px, -0.75 px, then rest over about 300 ms.
        animationDriver.animate(widget, comboChevronProperty, 0.0, 300, true, fluentCurve(),
                                { { 0.28, QVariant(-0.4) } }, start);
    }

    void unregisterComboPopup(QWidget *popup)
    {
        if (!popup)
            return;
        const auto association = comboPopupAssociations.find(popup);
        if (association == comboPopupAssociations.end())
            return;
        QComboBox *combo = association->data();
        comboPopupAssociations.erase(association);
        if (const auto connection = comboPopupPopupConnections.take(popup))
            QObject::disconnect(connection);
        if (combo && comboPopupByCombo.value(combo) == popup) {
            comboPopupByCombo.remove(combo);
            if (const auto connection = comboPopupComboConnections.take(combo))
                QObject::disconnect(connection);
        }
    }

    void unregisterComboPopup(QComboBox *combo)
    {
        if (!combo)
            return;
        QWidget *popup = comboPopupByCombo.value(combo);
        if (popup)
            unregisterComboPopup(popup);
        else
            comboPopupComboConnections.remove(combo);
    }

    void associateComboPopup(QComboBox *combo, QWidget *popup)
    {
        if (!combo || !popup || popup->windowType() != Qt::Popup)
            return;
        if (QWidget *previousPopup = comboPopupByCombo.value(combo);
            previousPopup && previousPopup != popup) {
            unregisterComboPopup(previousPopup);
        }
        if (const auto association = comboPopupAssociations.find(popup);
            association != comboPopupAssociations.end()) {
            if (association->data() == combo) {
                comboPopupByCombo.insert(combo, popup);
                return;
            }
            unregisterComboPopup(popup);
        }
        comboPopupAssociations.insert(popup, QPointer<QComboBox>(combo));
        comboPopupByCombo.insert(combo, popup);
        comboPopupPopupConnections.insert(
                popup, QObject::connect(popup, &QObject::destroyed, q, [this, popup] {
                    unregisterComboPopup(popup);
                }));
        comboPopupComboConnections.insert(
                combo, QObject::connect(combo, &QObject::destroyed, q, [this, combo] {
                    unregisterComboPopup(combo);
                }));
    }

    void prepareComboPopupFirstFrame(QComboBox *combo)
    {
        if (!combo || !combo->view())
            return;
        QWidget *popup = combo->view()->window();
        associateComboPopup(combo, popup);
        if (!popup)
            return;
        // QComboBox can change the popup viewport geometry between the view's
        // Show event and the popup window's Show event. Re-running this
        // idempotent preparation makes the selected-row anchor deterministic
        // on both the first and later openings.
        prepareComboPopupFirstFrameImpl(combo);
    }

    void finishComboPopupCycle(QWidget *popup)
    {
        if (!popup)
            return;
        if (const auto association = comboPopupAssociations.constFind(popup);
            association != comboPopupAssociations.constEnd()) {
            if (QComboBox *combo = association->data()) {
                if (combo->view() && combo->view()->viewport()) {
                    QWidget *viewport = combo->view()->viewport();
                    animationDriver.stop(viewport);
                    framePropertyRegistry().set(viewport, pressProperty, 0.0);
                }
                releaseComboChevron(combo);
            }
        }
    }

    void trackTableEditor(QTableView *table, QWidget *editor)
    {
        tableEditorTracker.track(table, editor);
    }

    void untrackTableEditor(QWidget *editor, bool clearProperty = true)
    {
        tableEditorTracker.untrackEditor(editor, clearProperty);
    }

    void untrackTable(QTableView *table, bool clearProperties = true)
    {
        tableEditorTracker.untrackTable(table, clearProperties);
    }

    bool tableEditorOverlaps(const QTableView *table, const QModelIndex &index,
                             const QRect &itemRect)
    {
        return tableEditorTracker.overlaps(table, index, itemRect);
    }

    bool dark() const
    {
        return mode == ThemeMode::Dark
                || (mode == ThemeMode::System && Private::systemUsesDarkTheme());
    }

    Style *q = nullptr;
    ThemeMode mode = ThemeMode::System;
    WinUI3::DensityMode density = WinUI3::DensityMode::Standard;
    QColor accent;
    FrameAnimationDriver animationDriver;
    QVector<QPointer<QProgressBar>> progressBars;
    QHash<QProgressBar *, QMetaObject::Connection> progressBarStateConnections;
    QHash<QScrollBar *, QPointer<QTimer>> scrollBarTimers;
    QHash<QSlider *, QPointer<QTimer>> sliderToolTipTimers;
    // Tracked destroyed connections for the parent-owned timers above, so
    // repeated polish/unpolish can disconnect instead of accumulating.
    QHash<QScrollBar *, QMetaObject::Connection> scrollBarDestroyConnections;
    QHash<QSlider *, QMetaObject::Connection> sliderToolTipDestroyConnections;
    QHash<QWidget *, QMetaObject::Connection> toggleConnections;
    QHash<QRadioButton *, QMetaObject::Connection> radioConnections;
    QHash<QWidget *, QMetaObject::Connection> tableConnections;
    TableEditorTracker tableEditorTracker;
    QHash<QCheckBox *, ToggleDragState> toggleDragStates;
    QHash<QWidget *, QPointer<QComboBox>> comboPopupAssociations;
    QHash<QComboBox *, QWidget *> comboPopupByCombo;
    QHash<QWidget *, QMetaObject::Connection> comboPopupPopupConnections;
    QHash<QComboBox *, QMetaObject::Connection> comboPopupComboConnections;
    QVector<QPointer<QWidget>> paletteOwners;
    QHash<QWidget *, QMetaObject::Connection> paletteOwnerConnections;
    bool keyboardInput = false;
    bool applicationStateSaved = false;
    bool applicationStyleActive = false;
    bool lastSystemDark = false;
    QColor lastSystemAccent;
    QTimer *progressTimer = nullptr;
    SystemAppearanceWatcher *systemAppearanceWatcher = nullptr;
    QTimer *systemAppearanceWatchdog = nullptr;
    QFont originalApplicationFont;
    QPalette originalApplicationPalette;
    std::unique_ptr<Private::StyleInteractionController> interactionController;
};

Style::Style(ThemeMode mode) : Style(mode, WinUI3::DensityMode::Standard) { }

Style::Style(WinUI3::DensityMode density) : Style(ThemeMode::System, density) { }

Style::Style(ThemeMode mode, WinUI3::DensityMode density) : Style(nullptr, mode, density) { }

Style::Style(QStyle *base, ThemeMode mode, WinUI3::DensityMode density)
    : QProxyStyle(base ? base : new QCommonStyle),
      d(std::make_unique<StylePrivate>(this, mode, density))
{
    setObjectName(QStringLiteral("winui3"));
    d->progressTimer = new QTimer(this);
    d->progressTimer->setObjectName(QStringLiteral("_winui_progress_timer"));
    d->progressTimer->setInterval(16);
    connect(d->progressTimer, &QTimer::timeout, this, [this] { d->advanceProgressBars(); });
    d->systemAppearanceWatchdog = new QTimer(this);
    d->systemAppearanceWatchdog->setObjectName(QStringLiteral("_winui_system_appearance_watchdog"));
    d->systemAppearanceWatchdog->setInterval(15000);
    connect(d->systemAppearanceWatchdog, &QTimer::timeout, this, &Style::checkSystemAppearance);
    d->systemAppearanceWatcher =
            new SystemAppearanceWatcher(this, [this] { checkSystemAppearance(); });
    d->systemAppearanceWatcher->setActive(false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    if (QStyleHints *hints = QGuiApplication::styleHints()) {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
                [this](Qt::ColorScheme) { checkSystemAppearance(); });
    }
#endif
}

Style::~Style() = default;

ThemeMode Style::themeMode() const
{
    return d->mode;
}

WinUI3::DensityMode Style::densityMode() const
{
    return d->density;
}

WinUI3::DensityMode Style::effectiveDensityMode(const QWidget *widget) const
{
    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        WinUI3::DensityMode local = WinUI3::DensityMode::Standard;
        if (densityModeFromProperty(candidate->property(DensityProperty), &local))
            return local;
    }
    return d->density;
}

void Style::setThemeMode(ThemeMode mode)
{
    if (d->mode == mode)
        return;
    d->mode = mode;
    refreshApplicationAppearance();
    d->restartSystemAppearanceWatchdog();
    emit themeChanged(mode);
}

void Style::setDensityMode(WinUI3::DensityMode mode)
{
    if (d->density == mode)
        return;
    d->density = mode;
    invalidateDensity();
    emit densityChanged(mode);
}

QColor Style::accentColor() const
{
    return d->accent.isValid() ? d->accent : Private::systemAccentColor();
}

bool Style::animationsAllowed()
{
    return !qEnvironmentVariableIsSet("WINUI3STYLE_DISABLE_ANIMATIONS");
}

namespace {
bool g_altMnemonicsVisible = false;

// GUI-thread-only contract: the flag is written by setAltMnemonicsVisible()
// (Alt key tracking from the style's event filter / application code) and
// read from SH_UnderlineShortcut paint paths. Both run on the GUI thread,
// and the setter below touches top-level widgets, which is itself
// GUI-thread-only. The assert documents and enforces this; a plain bool is
// sufficient because no cross-thread access is permitted.
void assertGuiThread(const char *operation)
{
    const bool onGuiThread = !QCoreApplication::instance()
            || QThread::currentThread() == QCoreApplication::instance()->thread();
    Q_ASSERT_X(onGuiThread, operation, "must be called on the GUI thread");
    Q_UNUSED(operation);
}
} // namespace

bool Style::altMnemonicsVisible()
{
    assertGuiThread("Style::altMnemonicsVisible");
    return g_altMnemonicsVisible;
}

void Style::setAltMnemonicsVisible(bool visible)
{
    assertGuiThread("Style::setAltMnemonicsVisible");
    if (g_altMnemonicsVisible == visible)
        return;
    g_altMnemonicsVisible = visible;
    for (QWidget *widget : QApplication::topLevelWidgets()) {
        if (auto *menuBar = qobject_cast<QMenuBar *>(widget))
            menuBar->update();
        else if (widget->isWindow())
            widget->update();
    }
}

void Style::setAccentColor(const QColor &color)
{
    if (d->accent == color)
        return;
    d->accent = color;
    refreshApplicationAppearance();
    d->restartSystemAppearanceWatchdog();
    emit accentColorChanged(accentColor());
}

void Style::refreshApplicationAppearance()
{
    if (!qApp)
        return;
    const QPalette applicationPalette = standardPalette();
    const Private::Tokens applicationTokens = Private::tokens(applicationPalette);
    const QColor applicationAccent = accentColor();
    const bool darkTheme = d->dark();
    qApp->setPalette(applicationPalette);
    QToolTip::setPalette(applicationPalette);
    // A popup's view and viewport are often created after their combo box was
    // polished. Prepare and register them now that the popup exists, before
    // walking the bounded owner registry below.
    for (QWidget *window : qApp->topLevelWidgets()) {
        const QVariant backdrop = window->property("_winui_backdrop");
        if (backdrop.isValid()) {
            QTimer::singleShot(0, window, [window, backdrop] {
                applyBackdrop(window, static_cast<Backdrop>(backdrop.toInt()));
            });
        } else if (auto *wizard = qobject_cast<QWizard *>(window)) {
            refreshWizardSurface(wizard, applicationPalette);
            const QPointer<QWizard> guardedWizard(wizard);
            const QPointer<Style> guardedStyle(this);
            QTimer::singleShot(0, wizard, [guardedWizard, guardedStyle] {
                if (guardedWizard && guardedStyle)
                    refreshWizardSurface(guardedWizard, guardedStyle->standardPalette());
            });
        } else if (qobject_cast<QDialog *>(window)) {
            applyDialogCaptionTheme(window);
        }
        if (window->windowType() == Qt::Popup) {
            preparePopupSurface(window);
            d->registerPopupPaletteOwners(window);
        }
    }
    d->prunePaletteOwners();
    // Backdrop islands transparentized by syncContentSurfacesForBackdrop carry
    // no palette-owner registration (only polish-registered widgets are
    // visited below), so a Light->Dark switch leaves their stale fill behind
    // as a ghost. Rebase every opted-in island's Window role on the new
    // application palette here, preserving its current alpha: opaque islands
    // follow the theme, transparent live-material islands keep revealing DWM
    // with fresh RGB. Toggle-off still restores the remembered state.
    // A backdrop toggle re-runs this same per-window pass through
    // refreshOwnedPalettes() so the inline calendar surface (computed at
    // polish) converges onto the granted material instead of staying opaque.
    for (QWidget *window : qApp->topLevelWidgets())
        refreshOwnedPalettes(window);
    // The popup may be newly created and not yet have delivered its first
    // Show event. Refresh visible popup children as well as the window itself.
    for (QWidget *topLevel : qApp->topLevelWidgets()) {
        if (topLevel->windowType() == Qt::Popup && topLevel->isVisible()) {
            topLevel->update();
            if (auto *view = topLevel->findChild<QAbstractItemView *>()) {
                view->update();
                if (view->viewport())
                    view->viewport()->update();
            }
        }
    }
}

void Style::refreshOwnedPalettes(QWidget *window)
{
    if (!window)
        return;
    d->prunePaletteOwners();
    const QPalette applicationPalette = standardPalette();
    const Private::Tokens applicationTokens = Private::tokens(applicationPalette);
    const QColor applicationAccent = accentColor();
    const bool darkTheme = d->dark();
    if (window->property("_winui_backdrop").isValid()) {
        const QList<QWidget *> islands = window->findChildren<QWidget *>();
        for (QWidget *island : islands) {
            const QVariant surface = island->property(Style::SurfaceProperty);
            const QString name = surface.toString();
            const bool optedIn = surface.toBool()
                    || name.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
                    || name.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0;
            if (!optedIn)
                continue;
            QPalette rebased = island->palette();
            QColor rebasedWindow = applicationPalette.color(QPalette::Window);
            rebasedWindow.setAlpha(rebased.color(QPalette::Window).alpha());
            if (rebased.color(QPalette::Window) == rebasedWindow)
                continue;
            rebased.setColor(QPalette::Window, rebasedWindow);
            island->setPalette(rebased);
            island->update();
        }
    }
    // Calendar header ordering: the owner loop visits widgets in
    // registration order, and lane children are polished (registered)
    // before the bar. In a single pass the lane is therefore computed
    // from the pre-refresh (stale-theme) bar. Phase 1 rebases every
    // surface except the lane; phase 2 then reads the post-phase-1 bar,
    // so calendar -> bar -> lane converges in one pass. Phase-skip
    // predicates match insideCalendarNavigationBar exactly.
    for (const QPointer<QWidget> &guarded : d->paletteOwners) {
        QWidget *widget = guarded.data();
        if (!widget)
            continue;
        // Per-window pass: refreshOwnedPalettes() is also invoked on a single
        // window when its backdrop surface toggles, so owners belonging to a
        // different real window are left untouched this pass. Owners whose
        // window is not a real top-level window (Designer-assembled or
        // orphan hierarchies) keep the old visit-every-pass behavior.
        if (QWidget *ownerWindow = widget->window();
            ownerWindow != window && ownerWindow->isWindow())
            continue;
        if (insideCalendarNavigationBar(widget))
            continue;
        if (widget->window() && widget->window()->windowType() == Qt::Popup) {
            // Hidden calendar popups skip the owned-palette branches
            // (preparePopupSurface re-asserts the surface on show), but
            // their chrome still needs the surface-roles semantics now:
            // converge bar and lane onto the uniform popup tint (same
            // popupSurfaceColor recipe + alpha preparePopupSurface paints)
            // without manufacturing winId/DWM state (the callee already
            // guards windowHandle/offscreen itself). Bar first, lane copies
            // the bar exactly, preserving the calendar -> bar -> lane order.
            if (auto *calendar = qobject_cast<QCalendarWidget *>(widget);
                calendar && widget->windowType() != Qt::Popup) {
                preparePopupSurface(widget);
                if (QWidget *bar = calendar->findChild<QWidget *>(
                            QStringLiteral("qt_calendar_navigationbar"))) {
                    QPalette barPalette = applicationPalette;
                    const QColor barWindow = widget->window()->palette().color(QPalette::Window);
                    barPalette.setColor(QPalette::Window, barWindow);
                    barPalette.setColor(QPalette::Base, barWindow);
                    bar->setPalette(barPalette);
                    bar->update();
                    calendar->update();
                }
                widget->update();
                if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
                    view->update();
                    if (view->viewport())
                        view->viewport()->update();
                }
                continue;
            }
            preparePopupSurface(widget);
        } else if (auto *wizard = qobject_cast<QWizard *>(widget)) {
            refreshWizardSurface(wizard, applicationPalette);
        } else if (widget->property(originalPaletteExplicitProperty).toBool()
                   && !widget->property(ownedPaletteProperty).toBool()) {
            // The widget carried an explicit palette before the style touched
            // it, and the style never claimed it (no surface, no implicit
            // registration). A style-wide theme refresh must not clobber
            // user-set colors; painters already derive their tokens from the
            // widget's own palette at draw time. Style-claimed widgets
            // (ownedPaletteProperty) always rebase: their "explicit" flag
            // only records the factory palette Qt set before polish.
            continue;
        } else {
            QPalette palette = applicationPalette;
            if (qobject_cast<QStatusBar *>(widget) || qobject_cast<QWizard *>(widget)) {
                palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
            } else if (qobject_cast<QWizardPage *>(widget)) {
                palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
            } else if (auto *calendarGrid = qobject_cast<QTableView *>(widget);
                       calendarGrid && insideCalendarWidget(widget)) {
                // Transient pickers spawn a QCalendarWidget in Qt::Popup (gets
                // the preparePopupSurface palette rebase) while the Dialogs
                // persistentCalendar is inline (keeps the app palette). The
                // shared calendarPopupView day chrome paints its own accent
                // circle, so neutralize the native Highlight role for ALL
                // calendar grids here, not only popups. Keep Base on the
                // content surface (only real popups get the flyout tint) and
                // kill just the native blue rect/strip.
                palette.setColor(QPalette::Highlight, Qt::transparent);
                palette.setColor(QPalette::HighlightedText, applicationTokens.textPrimary);
            } else if (widget->property(SurfaceProperty)
                               .toString()
                               .compare(QLatin1String("layer"), Qt::CaseInsensitive)
                       == 0) {
                const QColor layer = Private::popupSurfaceColor(applicationPalette);
                palette.setColor(QPalette::Window, layer);
                if (qobject_cast<QAbstractItemView *>(widget))
                    palette.setColor(QPalette::Base, layer);
            } else if (qobject_cast<QTableView *>(widget)) {
                palette.setColor(QPalette::Highlight, applicationTokens.subtleHover);
                palette.setColor(QPalette::HighlightedText, applicationTokens.textPrimary);
            } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
                       editor && itemView(editor)) {
                palette.setColor(QPalette::Highlight, applicationAccent);
                palette.setColor(QPalette::HighlightedText, applicationTokens.textOnAccentPrimary);
            } else if (widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
                       && qobject_cast<QCalendarWidget *>(widget->parentWidget())) {
                // Popup bars are handled by preparePopupSurface above. Inline
                // bars follow the same Composited-aware surface as the day
                // grid (mirroring polish): veiled under a granted backdrop so
                // bar, lane, and body read as one surface; opaque Window only
                // on the refused/offscreen fallback.
                QColor navigationWindow;
                if (Private::backdropEffectiveSurface(widget->window())
                    == Private::BackdropSurface::Composited)
                    navigationWindow = inlineCalendarSurface(widget, applicationPalette);
                else {
                    navigationWindow = applicationPalette.color(QPalette::Window);
                    navigationWindow.setAlpha(255);
                }
                palette.setColor(QPalette::Window, navigationWindow);
            } else if (qobject_cast<QDialog *>(widget)) {
                palette.setColor(QPalette::Window, Private::popupSurfaceColor(applicationPalette));
            }
            if (insideCalendarWidget(widget)) {
                const QColor surface = inlineCalendarSurface(widget, applicationPalette);
                palette.setColor(QPalette::Window, surface);
                palette.setColor(QPalette::Base, surface);
            }
            widget->setPalette(palette);
            if (auto *dialog = qobject_cast<QDialog *>(widget))
                prepareContentDialogState(dialog, darkTheme);
        }
        widget->update();
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->update();
            if (view->viewport())
                view->viewport()->update();
        }
    }
    // Phase 2: the lane copies the post-phase-1 bar exactly (RGB+alpha),
    // guaranteed same pass. Lane buttons paint Subtle flat at rest over
    // the now-translucent bar; roles stay untouched here per the two-phase
    // contract (Standard->Subtle happens in polish only).
    for (const QPointer<QWidget> &guarded : d->paletteOwners) {
        QWidget *widget = guarded.data();
        if (!widget || !insideCalendarNavigationBar(widget))
            continue;
        if (QWidget *ownerWindow = widget->window();
            ownerWindow != window && ownerWindow->isWindow())
            continue;
        if (widget->property(originalPaletteExplicitProperty).toBool()
            && !widget->property(ownedPaletteProperty).toBool())
            continue;
        const QWidget *navigationBar = widget->parentWidget();
        const QColor laneWindow = navigationBar ? navigationBar->palette().color(QPalette::Window)
                                                : applicationPalette.color(QPalette::Window);
        QPalette lanePalette = applicationPalette;
        lanePalette.setColor(QPalette::Window, laneWindow);
        // The year editor frame fills from the Button role: repoint it at
        // the lane surface so hover-free pixels match the bar's empty
        // stretches; Button/Window/Base stay one identical tint.
        lanePalette.setColor(QPalette::Button, laneWindow);
        if (qobject_cast<QAbstractSpinBox *>(widget))
            lanePalette.setColor(QPalette::Base, laneWindow);
        widget->setPalette(lanePalette);
        widget->update();
        if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
            view->update();
            if (view->viewport())
                view->viewport()->update();
        }
    }
    // Re-sync transparentized chrome and content islands after the recompute
    // above: the generic owner branches rebase every widget from the opaque
    // application palette, which would clobber the live-material recipe
    // (alpha 0) the backdrop sync established for opted-in islands and
    // chrome. Re-running the sync under a granted backdrop restores alpha 0
    // idempotently (remember-once state); the calendar keeps the veiled
    // surface computed above since it carries no island property and the
    // sync never visits it. Other effective states keep the recomputed
    // opaque recipe untouched.
    if (Private::backdropEffectiveSurface(window) == Private::BackdropSurface::Composited) {
        Private::makeChromeSurfacesTransparent(window);
        Private::syncContentSurfacesForBackdrop(window);
    }
}

void Style::invalidateDensity(QWidget *scope)
{
    if (scope) {
        invalidateDensityTree(scope);
        return;
    }
    if (!qApp)
        return;
    const auto topLevels = qApp->topLevelWidgets();
    if (!topLevels.isEmpty()) {
        for (QWidget *window : topLevels)
            invalidateDensityTree(window);
        return;
    }
    // Widgets can exist before they are assigned a top-level window (for
    // example while a Designer form is being assembled).
    const auto widgets = QApplication::allWidgets();
    for (QWidget *widget : widgets)
        invalidateDensityTree(widget);
}

void Style::checkSystemAppearance()
{
    Private::invalidateSystemAppearanceCache();
    if (!d->needsSystemAppearancePolling()) {
        d->systemAppearanceWatchdog->stop();
        return;
    }
    bool themeChangedAtRuntime = false;
    bool accentChangedAtRuntime = false;
    QColor systemAccent;
    if (d->mode == ThemeMode::System) {
        const bool systemDark = Private::systemUsesDarkTheme();
        themeChangedAtRuntime = d->lastSystemDark != systemDark;
        d->lastSystemDark = systemDark;
    }
    if (!d->accent.isValid()) {
        systemAccent = Private::systemAccentColor();
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
        widget->setProperty(originalRoleWasValidProperty, widget->property(roleProperty).isValid());
        widget->setProperty(originalRoleProperty, widget->property(roleProperty));
    }
    widget->setProperty(roleProperty, static_cast<int>(role));
    widget->update();
}

void Style::setDensityMode(QWidget *widget, WinUI3::DensityMode mode)
{
    if (!widget)
        return;
    widget->setProperty(DensityProperty, QVariant::fromValue(mode));
    invalidateDensityTree(widget);
}

WinUI3::DensityMode Style::densityMode(const QWidget *widget)
{
    if (!widget)
        return qApp && qobject_cast<const Style *>(qApp->style())
                ? qobject_cast<const Style *>(qApp->style())->densityMode()
                : WinUI3::DensityMode::Standard;

    if (const auto *style = qobject_cast<const Style *>(widget->style()))
        return style->effectiveDensityMode(widget);

    for (const QWidget *candidate = widget; candidate; candidate = candidate->parentWidget()) {
        WinUI3::DensityMode local = WinUI3::DensityMode::Standard;
        if (densityModeFromProperty(candidate->property(DensityProperty), &local))
            return local;
    }

    if (qApp) {
        if (const auto *style = qobject_cast<const Style *>(qApp->style()))
            return style->densityMode();
    }
    return WinUI3::DensityMode::Standard;
}

void Style::clearDensityMode(QWidget *widget)
{
    if (!widget)
        return;
    widget->setProperty(DensityProperty, QVariant());
    invalidateDensityTree(widget);
}

ControlRole Style::controlRole(const QWidget *widget)
{
    if (!widget)
        return ControlRole::Standard;
    const QVariant designerRole = widget->property(ControlRoleProperty);
    if (designerRole.isValid()) {
        const QString name = designerRole.toString().trimmed().toLower();
        if (name == QLatin1String("accent"))
            return ControlRole::Accent;
        if (name == QLatin1String("subtle"))
            return ControlRole::Subtle;
        if (name == QLatin1String("navigation"))
            return ControlRole::Navigation;
        if (name == QLatin1String("destructive"))
            return ControlRole::Destructive;
        if (name == QLatin1String("standard"))
            return ControlRole::Standard;
        bool converted = false;
        const int numericRole = designerRole.toInt(&converted);
        if (converted && numericRole >= int(ControlRole::Standard)
            && numericRole <= int(ControlRole::Destructive)) {
            return static_cast<ControlRole>(numericRole);
        }
    }
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

void Style::setToggleSwitchText(QCheckBox *checkBox, const QString &onText, const QString &offText)
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
    return Private::standardPalette(darkTheme, accent, d->accent.isValid());
}

void Style::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter,
                          const QWidget *widget) const
{
    if (Private::drawMenuPrimitive(this, element, option, painter, widget))
        return;
    if (Private::drawButtonPrimitive(this, element, option, painter, widget))
        return;
    using namespace Private;
    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & State_Enabled;

    if (Private::drawViewPrimitive(this, element, option, painter, widget))
        return;

    if (element == PE_Widget && widget
        && widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
        && qobject_cast<QCalendarWidget *>(widget->parentWidget())
        && widget->window()->windowType() == Qt::Popup) {
        const QColor surface = widget->window()->palette().color(QPalette::Window);
        if (!Private::clearForBackdropFill(painter, widget, option->rect, surface))
            painter->fillRect(option->rect, surface);
        return;
    }

    if (element == PE_Widget && widget
        && widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
        && qobject_cast<QCalendarWidget *>(widget->parentWidget())
        && widget->window()->windowType() != Qt::Popup) {
        // Inline lane under a granted backdrop must read the same
        // single-composite ink as the day cells: Source-fill the resolved bar
        // surface instead of layering a second SourceOver fill over the
        // calendar fill beneath (which drifts toward opaque). Refused and
        // offscreen fallbacks keep the plain fill.
        const QColor lane = widget->palette().color(QPalette::Window);
        if (!Private::clearForBackdropFill(painter, widget, option->rect, lane))
            painter->fillRect(option->rect, lane);
        return;
    }

    if (element == PE_Widget && widget
        && Private::eraseForBackdrop(painter, widget, option->rect, Private::ControlRadius)) {
        // Erase to transparent on every paint over a live material: Qt's
        // erase is disabled here (StyledBackground, no autofill) and neither
        // the island nor its descendants repaint fully on scroll/hover/page
        // switches, so shifted frames accumulate as permanent smear until a
        // resize reallocates the buffer. DWM composites the material
        // underneath. The island itself (zero-alpha Window role) clears and
        // returns; descendants clear the same way and fall through so their
        // content paints over the freshly erased rect (the island has no
        // fill to punch through). Unrelated widgets keep Qt's default erase
        // path: the paintsDirectlyOnBackdrop gate already excluded opaque
        // islands and non-Composited fallbacks, so this branch never runs
        // for them.
        if (widget->palette().color(QPalette::Window).alpha() == 0)
            return;
    }

    if (element == PE_FrameFocusRect) {
        if (!keyboardFocusVisible(widget))
            return;
        const qreal focus =
                progress(widget, focusProperty, option->state & State_HasFocus ? 1.0 : 0.0);
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
        painter->drawRoundedRect(QRectF(option->rect).adjusted(1, 1, -1, -1), ControlRadius + 2,
                                 ControlRadius + 2);
        painter->setPen(QPen(inner, 1));
        painter->drawRoundedRect(QRectF(option->rect).adjusted(3, 3, -3, -3), ControlRadius,
                                 ControlRadius);
        painter->restore();
        return;
    }

    if (element == PE_PanelTipLabel) {
        const QColor fill = t.tooltipFill;
        roundedRect(painter, QRectF(option->rect).adjusted(1, 1, -1, -1), fill, t.tooltipStroke, 4);
        return;
    }

    if (element == PE_PanelStatusBar) {
        painter->fillRect(option->rect, option->palette.color(QPalette::Window));
        painter->setPen(QPen(t.stroke, 1));
        painter->drawLine(option->rect.topLeft(), option->rect.topRight());
        return;
    }

    if (element == PE_IndicatorArrowDown || element == PE_IndicatorArrowLeft
        || element == PE_IndicatorArrowRight || element == PE_IndicatorArrowUp) {
        WinUI3::icon(arrowIcon(element), enabled ? t.textPrimary : t.textDisabled)
                .paint(painter, option->rect, Qt::AlignCenter,
                       enabled ? QIcon::Normal : QIcon::Disabled);
        return;
    }

    if (element == PE_Frame && widget && widget->window()
        && widget->window()->windowType() == Qt::Popup) {
        // preparePopupSurface already rebound the popup palette's Window role
        // to the raised translucent-layer stand-in color.
        const QColor popupSurface = option->palette.color(QPalette::Window);
        controlSurface(painter, option->rect, popupSurface, t.flyoutStroke, t.flyoutStroke,
                       OverlayRadius);
        return;
    }

    if (element == PE_Frame && qobject_cast<const QAbstractItemView *>(widget)) {
        roundedRect(painter, QRectF(option->rect).adjusted(0.5, 0.5, -0.5, -0.5), Qt::transparent,
                    t.stroke, ControlRadius);
        return;
    }

    if (element == PE_Frame) {
        if (const QWidget *editor = richTextEditor(widget)) {
            const bool focused = editor->hasFocus();
            const bool editorEnabled = option->state & State_Enabled;
            const QColor fill = !editorEnabled ? t.controlDisabled
                    : focused                  ? t.editorFocusedFill
                              : (option->state & State_MouseOver ? t.controlHover : t.control);
            controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary, ControlRadius);
            if (focused)
                drawEditorFocusUnderline(painter, QRectF(option->rect), t.accentFill,
                                         ControlRadius);
            return;
        }
    }

    if (element == PE_IndicatorToolBarSeparator) {
        painter->save();
        painter->setPen(QPen(t.stroke, 1));
        if (option->state & State_Horizontal) {
            const int x = option->rect.center().x();
            painter->drawLine(x, option->rect.top() + 8, x, option->rect.bottom() - 8);
        } else {
            const int y = option->rect.center().y();
            painter->drawLine(option->rect.left() + 8, y, option->rect.right() - 8, y);
        }
        painter->restore();
        return;
    }

    Q_ASSERT_X(!coveredPrimitive(element), "WinUI3::Style::drawPrimitive",
               "a covered primitive reached QCommonStyle");
    QProxyStyle::drawPrimitive(element, option, painter, widget);
}

void Style::drawControl(ControlElement element, const QStyleOption *option, QPainter *painter,
                        const QWidget *widget) const
{
    if (Private::drawMenuControl(this, element, option, painter, widget))
        return;
    if (Private::drawButtonControl(this, element, option, painter, widget))
        return;
    using namespace Private;
    // Same erase as PE_Widget below: any control painting straight onto the
    // live material through a translucent island must rebuild from
    // transparent first. CE paths never reach the PE_Widget branch, so the
    // scroll-shifted pixels they leave behind smear exactly like the
    // unguarded viewport case. The gate excludes opaque islands and
    // non-Composited fallbacks, so unrelated controls are untouched.
    if (element == CE_PushButton || element == CE_PushButtonLabel || element == CE_CheckBox
        || element == CE_CheckBoxLabel || element == CE_RadioButton
        || element == CE_RadioButtonLabel || element == CE_ToolButtonLabel)
        Private::eraseForBackdrop(painter, widget, option->rect);
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

    if (Private::drawViewControl(
                this, element, option, painter, widget,
                [this](const QTableView *table, const QModelIndex &index, const QRect &rect) {
                    return d->tableEditorOverlaps(table, index, rect);
                })) {
        return;
    }

    if (element == CE_ShapedFrame && widget && widget->property(SettingsCardProperty).toBool()) {
        // Card-interactive iff an expandable child is bound: trailing-only
        // cards (Notifications/Updates) keep the resting control fill and
        // never show hover/press. The expandable child lives under the
        // _winui_settings_card_expandableHost, so presence of that host's
        // layout marks interactivity without including settingscard.h here.
        const QWidget *host =
                widget->findChild<QWidget *>(QStringLiteral("_winui_settings_card_expandableHost"));
        const bool interactive = host && host->layout() && host->layout()->count() > 0;
        const qreal hover = interactive
                ? progress(widget, hoverProperty, option->state & State_MouseOver ? 1.0 : 0.0)
                : 0.0;
        const qreal press = interactive
                ? progress(widget, pressProperty, option->state & State_Sunken ? 1.0 : 0.0)
                : 0.0;
        QColor fill = mix(t.control, t.controlHover, hover * (1.0 - press));
        fill = mix(fill, t.controlPressed, press);
        controlSurface(painter, option->rect, fill, t.stroke, t.strokeSecondary, OverlayRadius);
        if (keyboardFocusVisible(widget)) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setBrush(Qt::NoBrush);
            painter->setPen(QPen(t.focusOuter, 2.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(2, 2, -3, -3), 6, 6);
            painter->setPen(QPen(t.focusInner, 1.0));
            painter->drawRoundedRect(QRectF(option->rect).adjusted(4, 4, -5, -5), 5, 5);
            painter->restore();
        }
        return;
    }

    if (element == CE_ComboBoxLabel) {
        if (const auto *combo = qstyleoption_cast<const QStyleOptionComboBox *>(option)) {
            if (combo->editable)
                return;
            QRect content = subControlRect(CC_ComboBox, combo, SC_ComboBoxEditField, widget);
            if (!combo->currentIcon.isNull()) {
                const QSize iconSize = combo->iconSize.isValid() ? combo->iconSize : QSize(16, 16);
                const QRect logicalIcon(content.left(),
                                        content.center().y() - iconSize.height() / 2,
                                        iconSize.width(), iconSize.height());
                const QRect iconRect = visualRect(option->direction, content, logicalIcon);
                paintThemedIcon(painter, combo->currentIcon, iconRect, Qt::AlignCenter,
                                option->state & State_Enabled ? t.textPrimary : t.textDisabled,
                                option->state & State_Enabled ? QIcon::Normal : QIcon::Disabled);
                if (option->direction == Qt::RightToLeft)
                    content.setRight(iconRect.left() - 8);
                else
                    content.setLeft(iconRect.right() + 8);
            }
            painter->setPen(option->state & State_Enabled ? t.textPrimary : t.textDisabled);
            painter->drawText(content,
                              visualAlignment(option->direction, Qt::AlignLeft | Qt::AlignVCenter),
                              option->fontMetrics.elidedText(combo->currentText, Qt::ElideRight,
                                                             content.width()));
            return;
        }
    }

    if (element == CE_ProgressBarGroove) {
        const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option);
        const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
        const bool horizontal = !progressBar || progressBar->orientation() == Qt::Horizontal;
        const bool textAboveLine = horizontal && bar && bar->textVisible && !bar->text.isEmpty();
        const QRect groove = horizontal
                ? (textAboveLine
                           // The label owns the bar's body; the track becomes a thin
                           // underline so the text never overlaps the fill.
                           ? QRect(option->rect.left(), option->rect.bottom() - 2,
                                   option->rect.width(), 3)
                           : QRect(option->rect.left(), option->rect.center().y() - 2,
                                   option->rect.width(), 4))
                : QRect(option->rect.center().x() - 2, option->rect.top(), 4,
                        option->rect.height());
        roundedRect(painter, groove, t.stroke, Qt::transparent, 2);
        return;
    }

    if (element == CE_ProgressBarContents) {
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
            const bool horizontal = !progressBar || progressBar->orientation() == Qt::Horizontal;
            const bool inverted =
                    progressBar ? progressBar->invertedAppearance() : bar->invertedAppearance;
            const bool textAboveLine = horizontal && bar->textVisible && !bar->text.isEmpty();
            const QRect track = horizontal
                    ? (textAboveLine ? QRect(option->rect.left(), option->rect.bottom() - 2,
                                             option->rect.width(), 3)
                                     : QRect(option->rect.left(), option->rect.center().y() - 2,
                                             option->rect.width(), 4))
                    : QRect(option->rect.center().x() - 2, option->rect.top(), 4,
                            option->rect.height());
            const QColor indicatorColor =
                    bar->state & State_Enabled ? t.accentFill : t.accentFillDisabled;
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
                const bool reverse = horizontal ? (inverted != (bar->direction == Qt::RightToLeft))
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
                        roundedRect(painter, indicator, indicatorColor, Qt::transparent, 2);
                };
                drawIndicator(firstLength, 0.0);
                drawIndicator(secondLength, 0.5);
                return;
            }
            const qint64 range = qint64(bar->maximum) - qint64(bar->minimum);
            const qint64 value = qint64(bar->progress) - qint64(bar->minimum);
            const qreal ratio = range > 0 ? qBound<qreal>(0, qreal(value) / qreal(range), 1) : 0;
            QRect fill = track;
            if (horizontal) {
                const int length = qRound(track.width() * ratio);
                if (inverted != (bar->direction == Qt::RightToLeft))
                    fill.setLeft(track.right() - length + 1);
                else
                    fill.setWidth(length);
            } else {
                // Qt fills a vertical, non-inverted bar from the bottom up
                // (QCommonStyle flips "reverse" for vertical orientation), so
                // invertedAppearance must grow from the top instead.
                const int length = qRound(track.height() * ratio);
                if (inverted)
                    fill.setHeight(length);
                else
                    fill.setTop(track.bottom() - length + 1);
            }
            if (!fill.isEmpty())
                roundedRect(painter, fill, indicatorColor, Qt::transparent, 2);
            return;
        }
    }

    if (element == CE_ProgressBarLabel) {
        // WinUI's ProgressBar shows no inline percentage text, but Qt-based
        // applications may render meaningful information there with
        // textVisible=true. Keep their label: paint
        // it centered over the bar using the style's text color.
        if (const auto *bar = qstyleoption_cast<const QStyleOptionProgressBar *>(option)) {
            if (bar->textVisible && !bar->text.isEmpty()) {
                painter->save();
                const QPalette &pal = option->palette;
                const QPalette::ColorGroup group = !(bar->state & State_Enabled)
                        ? QPalette::Disabled
                        : (bar->state & State_Active ? QPalette::Active : QPalette::Inactive);
                const QColor fg = pal.color(group, QPalette::WindowText);
                painter->setPen(QPen(fg));
                const auto *progressBar = qobject_cast<const QProgressBar *>(widget);
                const bool horizontal =
                        !progressBar || progressBar->orientation() == Qt::Horizontal;
                if (horizontal) {
                    // The thin track sits at the bottom of the rect; the label
                    // owns the area above it.  Elide so long transfer strings
                    // (e.g. "2,513,696 (22.0%)") never bleed past the bar.
                    const QRect labelRect = bar->rect.adjusted(0, 0, 0, -6);
                    painter->drawText(labelRect, Qt::AlignCenter,
                                      option->fontMetrics.elidedText(bar->text, Qt::ElideRight,
                                                                     labelRect.width()));
                } else {
                    painter->drawText(bar->rect, Qt::AlignCenter, bar->text);
                }
                painter->restore();
            }
            return;
        }
    }

    if (element == CE_ToolBar) {
        if (widget && widget->property(SurfaceProperty).isValid())
            painter->fillRect(option->rect, widget->palette().color(QPalette::Window));
        return;
    }

    if (element == CE_SizeGrip) {
        const auto *grip = qstyleoption_cast<const QStyleOptionSizeGrip *>(option);
        const Qt::Corner corner = grip ? grip->corner : Qt::BottomRightCorner;
        const bool right = corner == Qt::TopRightCorner || corner == Qt::BottomRightCorner;
        const bool bottom = corner == Qt::BottomLeftCorner || corner == Qt::BottomRightCorner;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(Qt::NoPen);
        painter->setBrush(t.textTertiary);
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column <= row; ++column) {
                const int dx = 3 + column * 4;
                const int dy = 3 + row * 4;
                const qreal x = right ? option->rect.right() - dx : option->rect.left() + dx;
                const qreal y = bottom ? option->rect.bottom() - dy : option->rect.top() + dy;
                painter->drawEllipse(QPointF(x, y), 1.0, 1.0);
            }
        }
        painter->restore();
        return;
    }

    Q_ASSERT_X(!coveredControl(element), "WinUI3::Style::drawControl",
               "a covered control reached QCommonStyle");
    QProxyStyle::drawControl(element, option, painter, widget);
}

void Style::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option,
                               QPainter *painter, const QWidget *widget) const
{
    if (Private::drawComplexControl(this, control, option, painter, widget))
        return;
    // Same erase as PE_Widget: group boxes and other complex frames paint
    // straight onto the live material through a translucent island and never
    // pass the PE/CE branches, so their scroll-shifted pixels smear.
    if (control == CC_GroupBox || control == CC_ToolButton)
        Private::eraseForBackdrop(painter, widget, option->rect);

    Q_ASSERT_X(!Private::coveredComplex(control), "WinUI3::Style::drawComplexControl",
               "a covered complex control reached QCommonStyle");
    QProxyStyle::drawComplexControl(control, option, painter, widget);
}

int Style::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const
{
    return Private::pixelMetric(this, metric, option, widget);
}
QSize Style::sizeFromContents(ContentsType type, const QStyleOption *option,
                              const QSize &contentsSize, const QWidget *widget) const
{
    return Private::sizeFromContents(this, type, option, contentsSize, widget);
}

QRect Style::subElementRect(SubElement element, const QStyleOption *option,
                            const QWidget *widget) const
{
    return Private::subElementRect(this, element, option, widget);
}

QRect Style::subControlRect(ComplexControl control, const QStyleOptionComplex *option,
                            SubControl subControl, const QWidget *widget) const
{
    return Private::subControlRect(this, control, option, subControl, widget);
}
QStyle::SubControl Style::hitTestComplexControl(ComplexControl control,
                                                const QStyleOptionComplex *option,
                                                const QPoint &position, const QWidget *widget) const
{
    if (const auto result = Private::complexControlHitTest(control, option, position, widget))
        return *result;
    return QProxyStyle::hitTestComplexControl(control, option, position, widget);
}
int Style::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget,
                     QStyleHintReturn *returnData) const
{
    return Private::styleHint(this, hint, option, widget, returnData);
}

QIcon Style::standardIcon(StandardPixmap standard, const QStyleOption *option,
                          const QWidget *widget) const
{
    return Private::standardIcon(this, standard, option, widget);
}

void Style::polish(QApplication *application)
{
    QProxyStyle::polish(application);
    d->applicationStyleActive = true;
    if (!d->applicationStateSaved) {
        d->originalApplicationFont = application->font();
        d->originalApplicationPalette = application->palette();
        d->applicationStateSaved = true;
    }
    QByteArray overrideFamily = qgetenv("WINUI3STYLE_APP_FONT");
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6 renders the Windows 11 variable font natively.
    QString preferred = overrideFamily.isEmpty() ? QStringLiteral("Segoe UI Variable Text")
                                                 : QString::fromLocal8Bit(overrideFamily);
    if (!QFontDatabase::families().contains(preferred))
        preferred = QStringLiteral("Segoe UI");
#else
    // Qt 5.x's DirectWrite path can rasterize the variable-font outlines
    // with touching/overlapping glyphs ("o"+"a" looking glued).  Use the
    // static family unless the app opts in via WINUI3STYLE_APP_FONT.
    QString preferred = overrideFamily.isEmpty() ? QStringLiteral("Segoe UI")
                                                 : QString::fromLocal8Bit(overrideFamily);
    // QFontDatabase::families() is static from Qt 6 onward; instantiate on
    // Qt 5.
    const QFontDatabase fontDatabase;
    if (!fontDatabase.families().contains(preferred))
        preferred = QStringLiteral("Segoe UI");
#endif
    QFont font(preferred);
    font.setPixelSize(14);
    application->setFont(font);
    application->setPalette(standardPalette());
    QToolTip::setPalette(standardPalette());
    d->systemAppearanceWatcher->setActive(true);
    d->restartSystemAppearanceWatchdog();
}

void Style::polish(QWidget *widget)
{
    QProxyStyle::polish(widget);
    if (!widget)
        return;
    rememberPalette(widget);
    remember(widget, originalAutoFillProperty, widget->autoFillBackground());
    remember(widget, originalHoverAttributeProperty, widget->testAttribute(Qt::WA_Hover));
    remember(widget, originalRoleProperty, widget->property(roleProperty));
    widget->setAttribute(Qt::WA_Hover, true);
    // Idempotent install: polish can run again on an already-polished widget
    // (repolish paths), so remove first to avoid a duplicate filter entry.
    // unpolish(QWidget*) removes it symmetrically.
    widget->removeEventFilter(this);
    widget->installEventFilter(this);
    // Any widget without a user-set palette belongs to the style's palette
    // contract: a later theme/accent change must rebase it on the new
    // standardPalette, exactly like the surface islands below. Without this
    // only explicitly registered owners (islands, popups, chrome) follow the
    // theme, and a plain QLineEdit keeps its light factory palette inside a
    // dark window (white "Search settings" field). The explicit-palette
    // guard in the refresh loop still protects user-set colors, and unpolish
    // still restores the remembered original, so this only widens the set of
    // widgets the refresh visits, never what it writes to custom palettes.
    if (!widget->testAttribute(Qt::WA_SetPalette)) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
    }
    const QVariant surface = widget->property(SurfaceProperty);
    const QString surfaceName = surface.toString();
    if (surface.toBool() || surfaceName.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
        || surfaceName.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
        // A native backdrop makes the top-level Window role transparent.
        // Standard stacked/page widgets otherwise retain stale backing-store
        // pixels while scrolling or switching pages. An explicit content
        // layer is the Qt equivalent of WinUI's opaque content surface.
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        remember(widget, originalOpaquePaintProperty,
                 widget->testAttribute(Qt::WA_OpaquePaintEvent));
        QPalette palette = standardPalette();
        if (surfaceName.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
            const QColor layer = Private::popupSurfaceColor(palette);
            palette.setColor(QPalette::Window, layer);
            if (qobject_cast<QAbstractItemView *>(widget))
                palette.setColor(QPalette::Base, layer);
        }
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
        widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
    }
    // QDialogButtonBox is a layout container, not a command-surface panel.
    // Filling its own inset geometry creates a rectangular footer inside a
    // ContentDialog. It must inherit the dialog surface instead. QWizard has
    // a separate full-width footer surface installed by refreshWizardSurface.
    const bool automaticCommandSurface = qobject_cast<QStatusBar *>(widget);
    const bool automaticDialogContent = qobject_cast<QWizardPage *>(widget);
    if (!surface.isValid() && (automaticCommandSurface || automaticDialogContent)) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette palette = standardPalette();
        if (automaticCommandSurface)
            palette.setColor(QPalette::Window, Private::popupSurfaceColor(palette));
        else if (automaticDialogContent)
            palette.setColor(QPalette::Window, Private::popupSurfaceColor(palette));
        widget->setPalette(palette);
        widget->setAutoFillBackground(true);
    }
    if (auto *wizard = qobject_cast<QWizard *>(widget)) {
        // Keep the wizard in the style-owned palette registry so a later
        // theme change refreshes its internal page and command surfaces too.
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        for (QWizard::WizardButton button : { QWizard::NextButton, QWizard::FinishButton }) {
            if (QAbstractButton *primary = wizard->button(button))
                primary->setProperty(ControlRoleProperty, QStringLiteral("accent"));
        }
        refreshWizardSurface(wizard, standardPalette());
    }
    if (widget->isWindow()) {
        const QVariant backdrop = widget->property(BackdropProperty);
        if (backdrop.isValid()) {
            QPointer<QWidget> guarded(widget);
            QTimer::singleShot(0, widget, [guarded, backdrop] {
                if (guarded)
                    applyBackdrop(guarded, backdropFromProperty(backdrop));
            });
        }
    }
    framePropertyRegistry().set(widget, hoverProperty,
                                widget->isEnabled() && widget->underMouse() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, pressProperty, 0.0);
    framePropertyRegistry().set(widget, focusProperty, widget->hasFocus() ? 1.0 : 0.0);
    framePropertyRegistry().set(widget, focusVisibleProperty,
                                widget->hasFocus() && d->keyboardInput);
    if (widget->property(DensityProperty).isValid())
        invalidateDensityTree(widget);
    if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
        prepareLineEditHelperButtons(lineEdit, this);
        syncCompleterPopupDensity(lineEdit);
    } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        if (effectiveDensityMode(combo) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(combo, &styleChange);
            combo->updateGeometry();
        }
    } else if (auto *spinBox = qobject_cast<QAbstractSpinBox *>(widget)) {
        if (effectiveDensityMode(spinBox) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(spinBox, &styleChange);
            spinBox->updateGeometry();
        }
    } else if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
        if (effectiveDensityMode(button) == DensityMode::Compact) {
            QEvent styleChange(QEvent::StyleChange);
            QCoreApplication::sendEvent(button, &styleChange);
            button->updateGeometry();
        }
    }
    if (auto *view = qobject_cast<QAbstractItemView *>(widget)) {
        if (auto *completer = qobject_cast<QCompleter *>(view->parent())) {
            if (auto *editor = qobject_cast<QLineEdit *>(completer->widget()))
                syncCompleterPopupDensity(editor);
        }
    }
    if (qobject_cast<QScrollBar *>(widget)) {
        framePropertyRegistry().set(widget, scrollBarInsideProperty, widget->underMouse());
        framePropertyRegistry().set(widget, scrollBarGenerationProperty, 0);
    }
    if (auto *checkBox = qobject_cast<QCheckBox *>(widget)) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty,
                                    checkBox->checkState() == Qt::Unchecked ? 0.0 : 1.0);
        framePropertyRegistry().set(widget, togglePositionProperty,
                                    checkBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(
                widget,
                connect(checkBox,
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
                        &QCheckBox::checkStateChanged,
#else
                        &QCheckBox::stateChanged,
#endif
                        this, [this, checkBox](int state) {
                            const bool on = state != Qt::Unchecked;
                            // AnimatedAcceptVisualSource's NormalOnToNormalOff segment
                            // removes the stroke immediately. Only the acceptance path
                            // is animated; an on-transition remains interruptible by
                            // starting from its current progress.
                            d->animate(checkBox, checkProperty, on ? 1.0 : 0.0,
                                       on ? (toggleSwitch(checkBox) ? Private::FastDuration
                                                                    : Private::CheckBoxDuration)
                                          : 0);
                            if (toggleSwitch(checkBox))
                                d->animate(checkBox, togglePositionProperty, on ? 1.0 : 0.0,
                                           Private::FasterDuration);
                        }));
    } else if (auto *radio = qobject_cast<QRadioButton *>(widget)) {
        if (const auto previous = d->radioConnections.take(radio))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty, radio->isChecked() ? 1.0 : 0.0);
        d->radioConnections.insert(
                radio, connect(radio, &QAbstractButton::toggled, this, [this, radio](bool checked) {
                    // RadioButton checked-state switch is discrete: only the dot
                    // hover/press sizes animate (Normal 250ms via pressProperty).
                    // Duration 0 lands instantly from current progress, so rapid
                    // reversals track state exactly like checkbox uncheck.
                    d->animate(radio, checkProperty, checked ? 1.0 : 0.0, 0);
                }));
    } else if (auto *groupBox = qobject_cast<QGroupBox *>(widget);
               groupBox && groupBox->isCheckable()) {
        if (const auto previous = d->toggleConnections.take(widget))
            disconnect(previous);
        framePropertyRegistry().set(widget, checkProperty, groupBox->isChecked() ? 1.0 : 0.0);
        d->toggleConnections.insert(
                widget,
                connect(groupBox, &QGroupBox::toggled, this, [this, groupBox](bool checked) {
                    d->animate(groupBox, checkProperty, checked ? 1.0 : 0.0,
                               checked ? Private::CheckBoxDuration : 0);
                }));
    }

    if (auto *progressBar = qobject_cast<QProgressBar *>(widget)) {
        d->registerProgressBar(progressBar);
        framePropertyRegistry().set(progressBar, progressPhaseProperty,
                                    Style::animationsAllowed() ? 0.0 : 0.35);
        d->refreshProgressTimer();
    }
    if (auto *view = qobject_cast<QAbstractItemView *>(widget))
        NavigationPrivate::prepareNavigationView(view);

    if (auto *table = qobject_cast<QTableView *>(widget)) {
        if (const auto previous = d->tableConnections.take(widget))
            disconnect(previous);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        const auto applyTableSelectionPalette = [this, table] {
            QPalette palette = table->palette();
            const Private::Tokens tableTokens = Private::tokens(standardPalette());
            palette.setColor(QPalette::Highlight, tableTokens.subtleHover);
            palette.setColor(QPalette::HighlightedText, tableTokens.textPrimary);
            table->setPalette(palette);
        };
        applyTableSelectionPalette();
        d->tableConnections.insert(
                table,
                connect(this, &Style::themeChanged, table,
                        [applyTableSelectionPalette](ThemeMode) { applyTableSelectionPalette(); }));
    } else if (auto *editor = qobject_cast<QLineEdit *>(widget);
               editor && qobject_cast<const QTableView *>(itemView(editor))) {
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette palette = editor->palette();
        const Private::Tokens editorTokens = Private::tokens(standardPalette());
        palette.setColor(QPalette::Highlight, accentColor());
        palette.setColor(QPalette::HighlightedText, editorTokens.textOnAccentPrimary);
        editor->setPalette(palette);
    }

    if (auto *view = qobject_cast<QTableView *>(const_cast<QAbstractItemView *>(itemView(widget)));
        view && widget->parentWidget() == view->viewport()) {
        // Editors are children of the viewport and are polished after the
        // delegate creates them. Track that lifecycle in the style so item
        // painting can suppress display text even when Qt omits
        // State_Editing from the real delegate option.
        d->trackTableEditor(view, widget);
    }

    if (auto *toolButton = qobject_cast<QAbstractButton *>(widget)) {
        // The calendar navigation-bar lane paints Subtle flat at rest over
        // the now-translucent tinted bar (see the lane-children block
        // below): no erase can punch a hole because the bar beneath is the
        // same tint, not an opaque slab.
        if (qobject_cast<QToolBar *>(toolButton->parentWidget())
            || qobject_cast<QTabBar *>(toolButton->parentWidget()))
            setControlRole(toolButton, ControlRole::Subtle);
    }

    // Qt hard-codes QCalendarWidget's navigation bar to the Highlight role,
    // producing an accent-blue strip unrelated to WinUI's CalendarView. Keep
    // the native calendar implementation but place its navigation controls on
    // the same neutral popup surface as the day grid.
    if (widget->objectName() == QStringLiteral("qt_calendar_navigationbar")
        && qobject_cast<QCalendarWidget *>(widget->parentWidget())) {
        widget->setBackgroundRole(QPalette::Window);
        // Uniform acrylic: popup calendars carry the composited popup tint
        // (same popupSurfaceColor RGB + 178/242 alpha preparePopupSurface
        // paints); inline calendars keep the opaque content surface. Claim
        // the palette so the appearance refresh keeps the same contract.
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        const QPalette navigationSource = standardPalette();
        const bool popupHeader = widget->window() && widget->window()->windowType() == Qt::Popup;
        QColor navigationWindow;
        if (popupHeader) {
            navigationWindow = Private::popupSurfaceColor(navigationSource);
            if (Private::backdropEffectiveSurface(widget->window())
                == Private::BackdropSurface::Composited)
                navigationWindow = widget->window()->palette().color(QPalette::Window);
        } else {
            navigationWindow = inlineCalendarSurface(widget, navigationSource);
        }
        QPalette navigationPalette = navigationSource;
        navigationPalette.setColor(QPalette::Window, navigationWindow);
        widget->setPalette(navigationPalette);
        if (!popupHeader) {
            // Route the inline bar background through the PE_Widget branch
            // above so the granted-backdrop empty stretch Source-composites
            // exactly once; no Qt auto-fill pre-blend. Popups keep their
            // existing attributes.
            widget->setAutoFillBackground(false);
            widget->setAttribute(Qt::WA_StyledBackground, true);
        }
        widget->update();
    }

    // The lane children (month/year/prev/next buttons, year editor) default
    // to the Button background role with the application palette, while the
    // bar paints its empty stretches from its own header fill. The lane
    // reads as two backgrounds unless every child shares the bar's fill
    // exactly, so claim them the same way and keep them there across theme
    // refreshes. Lane buttons are Subtle flat at rest on popup calendars
    // (same tint beneath, no erase hole); inline lane buttons stay Standard
    // with the opaque bar (a Subtle rest over an opaque slab would read as
    // a missing fill, and the erase gate is closed there anyway).
    if (insideCalendarNavigationBar(widget)) {
        const bool popupCalendar = widget->window() && widget->window()->windowType() == Qt::Popup;
        if (auto *laneButton = qobject_cast<QAbstractButton *>(widget))
            setControlRole(laneButton, ControlRole::Subtle);
        widget->setBackgroundRole(QPalette::Window);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        QPalette lanePalette = standardPalette();
        // The bar's contract is the header fill derived from the application
        // palette: popup tint on popup calendars, opaque Window surface
        // inline. Never the factory Highlight a freshly-created bar may
        // still carry when a lane child is polished first.
        QColor laneWindow;
        if (popupCalendar) {
            laneWindow = Private::popupSurfaceColor(lanePalette);
            if (Private::backdropEffectiveSurface(widget->window())
                == Private::BackdropSurface::Composited)
                laneWindow = widget->window()->palette().color(QPalette::Window);
        } else {
            laneWindow = inlineCalendarSurface(widget, lanePalette);
        }
        lanePalette.setColor(QPalette::Window, laneWindow);
        lanePalette.setColor(QPalette::Button, laneWindow);
        if (qobject_cast<QAbstractSpinBox *>(widget))
            lanePalette.setColor(QPalette::Base, laneWindow);
        widget->setPalette(lanePalette);
        widget->update();
    }

    // The day grid qt_calendar_calendarview is itself a QTableView, so the
    // generic table branch above claimed it with a subtleHover Highlight.
    // That leaves a native full-cell rect behind the CalendarView accent
    // circle, and an un-rebased inline grid keeps the app Highlight blue.
    // The day chrome itself (circle-select, text/disabled/outside-month
    // colors) is already shared for popup and inline grids by the calendar
    // branches in winui3viewrenderers_p.cpp; only the palette is
    // neutralized here. Keep Base on the content surface (no flyout tint
    // inline) and kill just the native rect/strip. Single tableConnections
    // slot per widget: taking the generic entry first keeps the symmetry
    // gate, and generic unpolish (remembered-palette restore +
    // tableConnections.take) covers both palette and connection.
    if (auto *calendarGrid = qobject_cast<QTableView *>(widget);
        calendarGrid && insideCalendarWidget(widget)) {
        if (const auto previous = d->tableConnections.take(widget))
            disconnect(previous);
        const auto applyCalendarSelectionPalette = [this, calendarGrid] {
            QPalette palette = calendarGrid->palette();
            const Private::Tokens calendarTokens = Private::tokens(standardPalette());
            palette.setColor(QPalette::Highlight, Qt::transparent);
            palette.setColor(QPalette::HighlightedText, calendarTokens.textPrimary);
            calendarGrid->setPalette(palette);
        };
        applyCalendarSelectionPalette();
        d->tableConnections.insert(widget,
                                   connect(this, &Style::themeChanged, widget,
                                           [applyCalendarSelectionPalette](ThemeMode) {
                                               applyCalendarSelectionPalette();
                                           }));
    }

    // The native selection rect reads the viewport option palette, so the
    // grid's viewport neutralizes itself in its own polish. Each widget's
    // rememberPalette runs in its own polish, so generic unpolish restores
    // the exact remembered state; no poke-across from the grid's polish.
    if (auto *calendarTable = qobject_cast<QTableView *>(widget->parentWidget()); calendarTable
        && widget == calendarTable->viewport() && insideCalendarWidget(calendarTable)) {
        QPalette viewportPalette = widget->palette();
        const Private::Tokens viewportTokens = Private::tokens(standardPalette());
        viewportPalette.setColor(QPalette::Highlight, Qt::transparent);
        viewportPalette.setColor(QPalette::HighlightedText, viewportTokens.textPrimary);
        widget->setPalette(viewportPalette);
    }

    if (insideCalendarWidget(widget) && widget->window()->windowType() != Qt::Popup) {
        const QColor calendarSurface = inlineCalendarSurface(widget, standardPalette());
        QPalette palette = widget->palette();
        palette.setColor(QPalette::Window, calendarSurface);
        palette.setColor(QPalette::Base, calendarSurface);
        widget->setProperty(ownedPaletteProperty, true);
        d->registerPaletteOwner(widget);
        widget->setPalette(palette);
    }

    if (qobject_cast<QComboBox *>(widget))
        framePropertyRegistry().set(widget, comboChevronProperty, 0.0);

    // QMenu computes its first popup geometry after polish but before Show.
    // Install the layout inset here; the opaque palette is refreshed on Show.
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        remember(menu, originalMarginsProperty, QVariant::fromValue(menu->contentsMargins()));
        menu->setContentsMargins(0, 2, 0, 2);
    }

    if (auto *dialog = qobject_cast<QDialog *>(widget); dialog
        && (qobject_cast<QMessageBox *>(dialog)
            || dialog->property(ContentDialogProperty).toBool())) {
        prepareContentDialogState(dialog, d->dark());
        d->registerPaletteOwner(dialog);
    }
}

void Style::polish(QPalette &palette)
{
    palette = standardPalette();
}

void Style::unpolish(QApplication *application)
{
    d->applicationStyleActive = false;
    d->systemAppearanceWatchdog->stop();
    d->systemAppearanceWatcher->setActive(false);
    d->clearPaletteOwners();
    if (application && d->applicationStateSaved) {
        application->setFont(d->originalApplicationFont);
        application->setPalette(d->originalApplicationPalette);
        d->applicationStateSaved = false;
    }
    QProxyStyle::unpolish(application);
}

void Style::unpolish(QWidget *widget)
{
    if (widget) {
        d->unregisterPaletteOwner(widget);
        widget->removeEventFilter(this);
        if (auto *lineEdit = qobject_cast<QLineEdit *>(widget)) {
            cancelLineEditHelperUpdate(lineEdit);
            cacheLineEditClearButton(lineEdit, nullptr);
            restoreCompleterPopup(lineEdit, this);
        }
    }
    if (auto *combo = qobject_cast<QComboBox *>(widget))
        d->unregisterComboPopup(combo);
    if (widget && widget->isWindow() && widget->windowType() == Qt::Popup)
        d->unregisterComboPopup(widget);
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
            widget->setAutoFillBackground(widget->property(originalAutoFillProperty).toBool());
        if (widget->property(originalHoverAttributeProperty).isValid())
            widget->setAttribute(Qt::WA_Hover,
                                 widget->property(originalHoverAttributeProperty).toBool());
        if (widget->property(originalOpaquePaintProperty).isValid())
            widget->setAttribute(Qt::WA_OpaquePaintEvent,
                                 widget->property(originalOpaquePaintProperty).toBool());
        if (widget->property(originalTranslucentBackgroundProperty).isValid())
            widget->setAttribute(Qt::WA_TranslucentBackground,
                                 widget->property(originalTranslucentBackgroundProperty).toBool());
        if (widget->property(originalNoSystemBackgroundProperty).isValid())
            widget->setAttribute(Qt::WA_NoSystemBackground,
                                 widget->property(originalNoSystemBackgroundProperty).toBool());
        if (widget->property(originalMarginsProperty).isValid() && !qobject_cast<QDialog *>(widget))
            widget->setContentsMargins(widget->property(originalMarginsProperty).value<QMargins>());
        if (auto *list = qobject_cast<QListView *>(widget))
            if (widget->property(originalListSpacingProperty).isValid())
                list->setSpacing(widget->property(originalListSpacingProperty).toInt());
        if (auto *wizard = qobject_cast<QWizard *>(widget))
            delete wizardFooterSurface(wizard, false);
        if (auto *dialog = qobject_cast<QDialog *>(widget)) {
            restoreContentDialogState(dialog, false);
        }
        if (auto *frame = qobject_cast<QFrame *>(widget))
            if (widget->property(originalFrameShapeProperty).isValid())
                frame->setFrameShape(static_cast<QFrame::Shape>(
                        widget->property(originalFrameShapeProperty).toInt()));
        if (widget->property(originalRoleWasValidProperty).isValid()) {
            if (widget->property(originalRoleWasValidProperty).toBool())
                widget->setProperty(roleProperty, widget->property(originalRoleProperty));
            else
                widget->setProperty(roleProperty, {});
        }
        if (auto *view = qobject_cast<QAbstractItemView *>(widget))
            NavigationPrivate::restoreNavigationView(view);
        if (auto *table = qobject_cast<QTableView *>(widget))
            d->untrackTable(table);
        else if (qobject_cast<const QTableView *>(itemView(widget)))
            d->untrackTableEditor(widget);
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
        framePropertyRegistry().clearObject(widget);
        widget->setProperty(ownedPaletteProperty, {});
        widget->setProperty(originalPaletteProperty, {});
        widget->setProperty(originalPaletteExplicitProperty, {});
        widget->setProperty(originalAutoFillProperty, {});
        widget->setProperty(originalHoverAttributeProperty, {});
        widget->setProperty(originalOpaquePaintProperty, {});
        widget->setProperty(originalTranslucentBackgroundProperty, {});
        widget->setProperty(originalNoSystemBackgroundProperty, {});
        widget->setProperty("_winui_original_mouse_tracking", {});
        widget->setProperty(originalListSpacingProperty, {});
        widget->setProperty(originalMinimumSizeProperty, {});
        widget->setProperty(originalMaximumSizeProperty, {});
        widget->setProperty(originalLayoutConstraintProperty, {});
        widget->setProperty(originalFrameShapeProperty, {});
        widget->setProperty(originalMarginsProperty, {});
        widget->setProperty(originalSpacingProperty, {});
        widget->setProperty(originalRoleProperty, {});
        widget->setProperty(originalRoleWasValidProperty, {});
    }
}

bool Style::eventFilter(QObject *watched, QEvent *event)
{
    if (auto *wizard = qobject_cast<QWizard *>(watched);
        wizard && (event->type() == QEvent::Show || event->type() == QEvent::ChildAdded)) {
        const QPointer<QWizard> guardedWizard(wizard);
        const QPointer<Style> guardedStyle(this);
        QTimer::singleShot(0, wizard, [guardedWizard, guardedStyle] {
            if (guardedWizard && guardedStyle)
                refreshWizardSurface(guardedWizard, guardedStyle->standardPalette());
        });
    }
    if (auto *editor = qobject_cast<QLineEdit *>(watched); editor
        && (event->type() == QEvent::FocusIn || event->type() == QEvent::KeyPress
            || event->type() == QEvent::MouseButtonPress)) {
        // setCompleter() has no change signal. User interaction is the point
        // at which a replacement popup can first become visible.
        syncCompleterPopupDensity(editor);
    }
    if (event->type() == QEvent::DynamicPropertyChange) {
        auto *change = static_cast<QDynamicPropertyChangeEvent *>(event);
        if (auto *widget = qobject_cast<QWidget *>(watched)) {
            if (change->propertyName() == DensityProperty) {
                // Density is inherited, so a change on a container affects
                // every descendant's geometry as well as its paint state.
                invalidateDensityTree(widget);
            } else if (change->propertyName() == ControlRoleProperty) {
                widget->update();
            } else if (change->propertyName() == SurfaceProperty) {
                const QVariant surface = widget->property(SurfaceProperty);
                const QString surfaceName = surface.toString();
                const bool enabled = surface.toBool()
                        || surfaceName.compare(QLatin1String("content"), Qt::CaseInsensitive) == 0
                        || surfaceName.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0;
                if (enabled) {
                    widget->setProperty(ownedPaletteProperty, true);
                    d->registerPaletteOwner(widget);
                    remember(widget, originalOpaquePaintProperty,
                             widget->testAttribute(Qt::WA_OpaquePaintEvent));
                    QPalette palette = standardPalette();
                    if (surfaceName.compare(QLatin1String("layer"), Qt::CaseInsensitive) == 0) {
                        const QColor layer = Private::popupSurfaceColor(palette);
                        palette.setColor(QPalette::Window, layer);
                        if (qobject_cast<QAbstractItemView *>(widget))
                            palette.setColor(QPalette::Base, layer);
                    }
                    widget->setPalette(palette);
                    widget->setAutoFillBackground(true);
                    widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
                }
                widget->update();
            } else if (change->propertyName() == BackdropProperty && widget->isWindow()) {
                applyBackdrop(widget, backdropFromProperty(widget->property(BackdropProperty)));
                // Navigation surfaces are transparent only while a real
                // backdrop is active. Refresh just the opted-in views when a
                // window changes backdrop so offscreen/opaque windows never
                // retain transparent backing-store rows. The view frame and
                // its viewport repaint synchronously: delegate rows otherwise
                // keep the previous surface's pixels until the next hover
                // (the mica panel-vs-list skew), and the frame area outside
                // the viewport keeps them past a re-enable (stale rows the
                // viewport-only repaint never rebuilds).
                const auto refreshNavigationView = [](QAbstractItemView *candidate) {
                    if (candidate && candidate->property(NavigationViewProperty).toBool()) {
                        NavigationPrivate::prepareNavigationView(candidate);
                        candidate->repaint();
                        candidate->viewport()->repaint();
                    }
                };
                refreshNavigationView(qobject_cast<QAbstractItemView *>(widget));
                for (QAbstractItemView *view : widget->findChildren<QAbstractItemView *>())
                    refreshNavigationView(view);
            } else if (change->propertyName() == Private::effectiveBackdropProperty
                       && widget->isWindow() && widget->windowType() != Qt::Popup) {
                // applyBackdrop just published a new effective surface for this
                // main window (granted, refused, or restored). Re-run the
                // owned-palette pass so surfaces computed at polish time —
                // notably the inline calendar surface — converge onto the
                // granted material instead of keeping the stale opaque fill.
                // Popups are excluded: their surface is re-asserted on Show by
                // preparePopupSurface, and refreshing them here would re-enter
                // that path during Show dispatch.
                refreshOwnedPalettes(widget);
            }
        }
    }
    if (d->interactionController->eventFilter(watched, event))
        return true;
    return QProxyStyle::eventFilter(watched, event);
}
} // namespace WinUI3
