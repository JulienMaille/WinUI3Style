// SPDX-License-Identifier: LGPL-2.1-or-later
// Dialog surfaces: wizard/content-dialog footer painting, dialog state
// arm/restore and the smoke scrim live here. The shared widget-state snapshot
// helpers (remember, rememberPalette, restoreRememberedPalette) stay in
// winui3surfaces_p.cpp.
#include "winui3dialogs_p.h"

#include "winui3surfaces_p.h"

#include "winui3paint_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFrame>
#include <QGuiApplication>
#include <QLabel>
#include <QLayout>
#include <QMessageBox>
#include <QParallelAnimationGroup>
#include <QPaintEvent>
#include <QPainter>
#include <QPointer>
#include <QTimer>
#include <QWidget>
#include <QWizard>

namespace WinUI3::Private {

namespace {

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

} // namespace

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
    contentPalette.setColor(QPalette::Window, popupSurfaceColor(applicationPalette));
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

void deleteWizardFooterSurface(QWizard *wizard)
{
    delete wizardFooterSurface(wizard, false);
}

namespace {

constexpr auto contentDialogFooterName = "_winui_content_dialog_footer_surface";

class ContentDialogFooterSurface final : public QWidget
{
public:
    explicit ContentDialogFooterSurface(QDialog *dialog) : QWidget(dialog), m_dialog(dialog)
    {
        setObjectName(QString::fromLatin1(contentDialogFooterName));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        dialog->installEventFilter(this);
        queueGeometrySync();
    }

    void setColors(const QColor &fill, const QColor &stroke)
    {
        if (m_fill == fill && m_stroke == stroke)
            return;
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
        if (watched == m_dialog
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
        if (!m_dialog)
            return;
        auto *messageBox = qobject_cast<QMessageBox *>(m_dialog.data());
        if (messageBox) {
            // QMessageBox recalculates and fixes its size in showEvent after
            // the style's polish pass. Reassert the ContentDialog minimum on
            // the queued, post-show layout pass so the body is not crushed
            // into a single text line above an oversized footer.
            // Each mutator below re-triggers Resize/LayoutRequest, which
            // re-queues this sync via the event filter. Guard every write
            // by value so a converged dialog settles instead of ping-ponging
            // (a persistent <184px height would otherwise loop forever).
            // The explicit resize runs only on a pass that actually raised a
            // constraint: once the minimum is asserted the layout converges
            // on its own, and re-requesting the same height while the dialog
            // reports <184px would re-queue this sync forever.
            bool progressed = false;
            if (QLayout *layout = m_dialog->layout()) {
                if (layout->sizeConstraint() != QLayout::SetMinimumSize) {
                    layout->setSizeConstraint(QLayout::SetMinimumSize);
                    progressed = true;
                }
            }
            const QSize maxWanted(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            if (m_dialog->maximumSize() != maxWanted) {
                m_dialog->setMaximumSize(maxWanted.width(), maxWanted.height());
                progressed = true;
            }
            const QSize minWanted(qMax(320, m_dialog->minimumWidth()),
                                  qMax(184, m_dialog->minimumHeight()));
            if (m_dialog->minimumSize() != minWanted) {
                m_dialog->setMinimumSize(minWanted.width(), minWanted.height());
                progressed = true;
            }
            if (progressed && m_dialog->height() < 184)
                m_dialog->resize(m_dialog->width(), 184);
        }
        QDialogButtonBox *buttons = m_dialog->findChild<QDialogButtonBox *>();
        if (!buttons) {
            hide();
            return;
        }
        const int commandInset =
                m_dialog->layout() ? qMax(0, m_dialog->layout()->contentsMargins().bottom()) : 24;
        const int buttonTop = buttons->mapTo(m_dialog, QPoint(0, 0)).y();
        const int top = qBound(0, buttonTop - commandInset, m_dialog->height());
        setGeometry(0, top, m_dialog->width(), m_dialog->height() - top);
        lower();
        setVisible(m_dialog->isVisible());

        if (messageBox) {
            auto *label = messageBox->findChild<QLabel *>(QStringLiteral("qt_msgbox_label"),
                                                          Qt::FindDirectChildrenOnly);
            auto *icon = messageBox->findChild<QLabel *>(QStringLiteral("qt_msgboxex_icon_label"),
                                                         Qt::FindDirectChildrenOnly);
            if (label && label->isVisible()) {
                // QMessageBox keeps the message widgets top-aligned when its
                // height is enlarged to the ContentDialog minimum. Centre the
                // actual content in the region above the command footer, then
                // align the icon with the message instead of the layout row.
                const int contentCenter = QRect(0, 0, messageBox->width(), top).center().y();
                label->move(label->x(),
                            label->y() + contentCenter - label->geometry().center().y());
                if (icon && icon->isVisible()) {
                    icon->move(icon->x(), contentCenter - icon->rect().center().y());
                }
            }
        }
    }

    QPointer<QDialog> m_dialog;
    QColor m_fill;
    QColor m_stroke;
    bool m_syncQueued = false;
};

ContentDialogFooterSurface *contentDialogFooter(QDialog *dialog, bool create)
{
    if (!dialog)
        return nullptr;
    auto *surface = static_cast<ContentDialogFooterSurface *>(dialog->findChild<QWidget *>(
            QString::fromLatin1(contentDialogFooterName), Qt::FindDirectChildrenOnly));
    if (!surface && create)
        surface = new ContentDialogFooterSurface(dialog);
    return surface;
}

} // namespace

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
    delete contentDialogFooter(dialog, false);
    restoreRememberedPalette(dialog);
    if (dialog->property(originalAutoFillProperty).isValid())
        dialog->setAutoFillBackground(dialog->property(originalAutoFillProperty).toBool());
    if (dialog->property(originalMinimumSizeProperty).isValid())
        dialog->setMinimumSize(dialog->property(originalMinimumSizeProperty).value<QSize>());
    if (dialog->property(originalMaximumSizeProperty).isValid())
        dialog->setMaximumSize(dialog->property(originalMaximumSizeProperty).value<QSize>());
    if (QLayout *layout = dialog->layout()) {
        if (dialog->property(originalMarginsProperty).isValid())
            layout->setContentsMargins(dialog->property(originalMarginsProperty).value<QMargins>());
        if (dialog->property(originalSpacingProperty).isValid())
            layout->setSpacing(dialog->property(originalSpacingProperty).toInt());
        if (dialog->property(originalLayoutConstraintProperty).isValid())
            layout->setSizeConstraint(static_cast<QLayout::SizeConstraint>(
                    dialog->property(originalLayoutConstraintProperty).toInt()));
    }
    dialog->setProperty(ownedPaletteProperty, {});
    if (clearSavedState) {
        dialog->setProperty(originalPaletteProperty, {});
        dialog->setProperty(originalPaletteExplicitProperty, {});
        dialog->setProperty(originalAutoFillProperty, {});
        dialog->setProperty(originalMinimumSizeProperty, {});
        dialog->setProperty(originalMaximumSizeProperty, {});
        dialog->setProperty(originalLayoutConstraintProperty, {});
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
    remember(dialog, originalMinimumSizeProperty, QVariant::fromValue(dialog->minimumSize()));
    remember(dialog, originalMaximumSizeProperty, QVariant::fromValue(dialog->maximumSize()));
    if (QLayout *layout = dialog->layout()) {
        remember(dialog, originalMarginsProperty, QVariant::fromValue(layout->contentsMargins()));
        remember(dialog, originalSpacingProperty, layout->spacing());
        remember(dialog, originalLayoutConstraintProperty, int(layout->sizeConstraint()));
        layout->setContentsMargins(24, 24, 24, 24);
        layout->setSpacing(12);
    }
    dialog->setProperty(ownedPaletteProperty, true);
    QPalette palette = dialog->palette();
    const QColor commandFill = dialogCommandFillColor(dark);
    // ContentDialog uses LayerFillColorAlt for its content region and
    // SolidBackgroundFillColorBase for the full-width command footer.
    // Always derive the layer from the fixed base token. prepare() is called
    // again on runtime theme changes; lifting the already lifted color would
    // drift #2C2C2C to #383838 on the second pass.
    QPalette basePalette = palette;
    basePalette.setColor(QPalette::Window, commandFill);
    palette.setColor(QPalette::Window, popupSurfaceColor(basePalette));
    dialog->setPalette(palette);
    dialog->setAutoFillBackground(true);
    dialog->setMinimumSize(320, 184);
    dialog->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    ContentDialogFooterSurface *footer = contentDialogFooter(dialog, true);
    footer->setColors(commandFill, tokens(palette).stroke);
    footer->queueGeometrySync();
}

namespace {

constexpr auto contentDialogScrimProperty = "_winui_content_dialog_scrim";

// Simulates the WinUI ContentDialog smoke layer: while a content dialog over a
// parent window is open, a translucent child paints SmokeFillColorDefault
// (#4D000000, theme-independent) over the whole parent client area. The
// dialog itself stays an opaque native window above the dimmed surface.
class ContentDialogScrim final : public QWidget
{
public:
    explicit ContentDialogScrim(QWidget *parentWindow) : QWidget(parentWindow)
    {
        setObjectName(QStringLiteral("_winui_content_dialog_scrim"));
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAutoFillBackground(false);
        setGeometry(parentWindow->rect());
        parentWindow->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == parentWidget() && event->type() == QEvent::Resize)
            setGeometry(parentWidget()->rect());
        return QWidget::eventFilter(watched, event);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), dialogScrimColor());
    }
};

} // namespace

void showContentDialogScrim(QDialog *dialog)
{
    if (!dialog)
        return;
    hideContentDialogScrim(dialog);
    QWidget *owner = dialog->parentWidget();
    owner = owner ? owner->window() : nullptr;
    if (!owner)
        return;
    auto *scrim = new ContentDialogScrim(owner);
    dialog->setProperty(contentDialogScrimProperty,
                        QVariant::fromValue(static_cast<QObject *>(scrim)));
    // The scrim is parented to the owner window, not the dialog, so either
    // side can die first. If the owner (hence the scrim) is destroyed, the
    // raw property would dangle and a later hideContentDialogScrim() would
    // deleteLater() freed memory: clear the property synchronously on the
    // scrim's own destroyed signal. If the dialog dies first, the scrim
    // would otherwise orphan on the owner: schedule its teardown. The
    // equality guard keeps a queued destroy of a replaced scrim from
    // clearing a newer scrim's property.
    QObject::connect(scrim, &QObject::destroyed, dialog, [dialog, scrim] {
        if (dialog->property(contentDialogScrimProperty).value<QObject *>() == scrim)
            dialog->setProperty(contentDialogScrimProperty, {});
    });
    QObject::connect(dialog, &QObject::destroyed, scrim, [scrim] { scrim->deleteLater(); });
    scrim->show();
    scrim->raise();
}

void hideContentDialogScrim(QDialog *dialog)
{
    if (!dialog)
        return;
    // The scrim's destroyed signal clears this property synchronously when
    // the owner tears down first, so a dangling raw pointer can never reach
    // the deleteLater() below. The equality-cleared slot above also makes
    // an independently destroyed scrim a no-op here.
    auto *scrim = dialog->property(contentDialogScrimProperty).value<QObject *>();
    if (!scrim)
        return;
    dialog->setProperty(contentDialogScrimProperty, {});
    scrim->deleteLater();
}

} // namespace WinUI3::Private
