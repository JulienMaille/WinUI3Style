// SPDX-License-Identifier: LGPL-2.1-or-later
// QLineEdit helper affordances (S3 surfaces split): the private clear button
// cache, the QAction helper buttons and their coalesced update pass. Moved
// verbatim from winui3surfaces_p.cpp.
#include "winui3lineedithelpers_p.h"

#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3paint_p.h"
#include "winui3qtcompat_p.h"
#include "winui3style_properties_p.h"
#include "winui3tokens_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QAction>
#include <QFontMetrics>
#include <QHash>
#include <QLineEdit>
#include <QMetaObject>
#include <QObject>
#include <QPainter>
#include <QPointer>
#include <QStyleOption>
#include <QTimer>
#include <QWidget>

namespace WinUI3::Private {

namespace {

struct LineEditClearButtonCache
{
    QHash<QLineEdit *, QPointer<QAbstractButton>> buttons;
    QHash<QLineEdit *, QMetaObject::Connection> cleanupConnections;
};

LineEditClearButtonCache &lineEditClearButtonCache()
{
    // The cache lives until process exit, like the frame registry, so a
    // widget cannot race its cleanup with static destruction.
    static auto *cache = new LineEditClearButtonCache;
    return *cache;
}

} // namespace

namespace {

bool isLineEditClearButton(const QLineEdit *lineEdit, const QAbstractButton *button)
{
    if (!lineEdit || !button)
        return false;
    for (QAction *action : lineEdit->actions())
        if (actionAssociatedWith(action, button))
            return false;
    // QLineEdit's private clear affordance is deliberately not a QAction;
    // custom leading/trailing actions are associated above.
    return true;
}

} // namespace

void cacheLineEditClearButton(QLineEdit *lineEdit, QAbstractButton *button)
{
    if (!lineEdit)
        return;

    auto &cache = lineEditClearButtonCache();
    if (!button) {
        cache.buttons.remove(lineEdit);
        if (const auto connection = cache.cleanupConnections.take(lineEdit))
            QObject::disconnect(connection);
        return;
    }

    cache.buttons.insert(lineEdit, QPointer<QAbstractButton>(button));
    if (!cache.cleanupConnections.contains(lineEdit)) {
        cache.cleanupConnections.insert(lineEdit,
                                        QObject::connect(lineEdit, &QObject::destroyed, [lineEdit] {
                                            auto &cache = lineEditClearButtonCache();
                                            cache.buttons.remove(lineEdit);
                                            cache.cleanupConnections.remove(lineEdit);
                                        }));
    }
}

const QAbstractButton *lineEditClearButton(const QLineEdit *lineEdit)
{
    if (!lineEdit)
        return nullptr;
    const auto &cache = lineEditClearButtonCache();
    const auto button = cache.buttons.constFind(const_cast<QLineEdit *>(lineEdit));
    return button == cache.buttons.cend() ? nullptr : button->data();
}

namespace {

constexpr auto lineEditHelperUpdatePendingProperty = "_winui_line_edit_helper_update_pending";

qulonglong nextLineEditHelperUpdateToken()
{
    // QWidget lifecycle and style events are confined to the GUI thread.
    static qulonglong token = 0;
    return ++token;
}

void scheduleLineEditHelperUpdate(QLineEdit *lineEdit, Style *style)
{
    if (!lineEdit
        || framePropertyRegistry().value(lineEdit, lineEditHelperUpdatePendingProperty).isValid())
        return;

    const qulonglong token = nextLineEditHelperUpdateToken();
    framePropertyRegistry().set(lineEdit, lineEditHelperUpdatePendingProperty,
                                QVariant::fromValue(token));
    const QPointer<QLineEdit> guardedLineEdit(lineEdit);
    const QPointer<Style> guardedStyle(style);
    QTimer::singleShot(0, lineEdit, [guardedLineEdit, guardedStyle, token] {
        if (!guardedLineEdit
            || framePropertyRegistry()
                            .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                            .toULongLong()
                    != token)
            return;

        // updateReadOnlyDeleteAffordance() has no style parameter for callers
        // that only need the visibility contract. Resolve it at execution time
        // so a preceding update call can still be coalesced with preparation.
        Style *helperStyle = guardedStyle.data();
        if (!helperStyle)
            helperStyle = qobject_cast<Style *>(guardedLineEdit->style());

        QAbstractButton *clearButton = nullptr;
        for (QAbstractButton *button : guardedLineEdit->findChildren<QAbstractButton *>()) {
            if (helperStyle) {
                // QLineEdit creates its private clear affordance lazily. Depending
                // on that timing, it can miss the parent's polish pass and never
                // deliver hover events to the style. Prepare both the private
                // affordance and QAction helper buttons as soon as they exist.
                button->setAttribute(Qt::WA_Hover, true);
                button->installEventFilter(helperStyle);
                if (!framePropertyRegistry().value(button, hoverProperty).isValid())
                    framePropertyRegistry().set(button, hoverProperty,
                                                button->isEnabled() && button->underMouse() ? 1.0
                                                                                            : 0.0);
                if (!framePropertyRegistry().value(button, pressProperty).isValid())
                    framePropertyRegistry().set(button, pressProperty, 0.0);
            }

            if (isLineEditClearButton(guardedLineEdit, button)) {
                clearButton = button;
                button->setVisible(!guardedLineEdit->isReadOnly() && guardedLineEdit->isEnabled()
                                   && guardedLineEdit->hasFocus()
                                   && guardedLineEdit->isClearButtonEnabled()
                                   && !guardedLineEdit->text().isEmpty());
            }
        }
        cacheLineEditClearButton(guardedLineEdit, clearButton);
        if (framePropertyRegistry()
                    .value(guardedLineEdit, lineEditHelperUpdatePendingProperty)
                    .toULongLong()
            == token) {
            framePropertyRegistry().clear(guardedLineEdit, lineEditHelperUpdatePendingProperty);
        }
    });
}

} // namespace

void updateReadOnlyDeleteAffordance(QLineEdit *lineEdit)
{
    scheduleLineEditHelperUpdate(lineEdit, nullptr);
}

void prepareLineEditHelperButtons(QLineEdit *lineEdit, Style *style)
{
    scheduleLineEditHelperUpdate(lineEdit, style);
}

void cancelLineEditHelperUpdate(QLineEdit *lineEdit)
{
    if (lineEdit)
        framePropertyRegistry().clear(lineEdit, lineEditHelperUpdatePendingProperty);
}

namespace {

struct ShortcutText
{
    QStringList labels;
    QList<QChar> separators;
};

// "Ctrl+K, Ctrl+G" -> labels with the separator that joined each pair: '+' is
// a chord, ',' is a chord boundary. Qt's live text ends with a ", ..." marker
// while recording, which never reaches a cap.
ShortcutText parseShortcutText(QString text)
{
    text.remove(QStringLiteral(", ..."));
    ShortcutText parsed;
    QString token;
    for (const QChar ch : text) {
        if (ch == QLatin1Char('+') || ch == QLatin1Char(',')) {
            if (!token.trimmed().isEmpty()) {
                parsed.labels.append(token.trimmed());
                parsed.separators.append(ch);
            }
            token.clear();
        } else {
            token.append(ch);
        }
    }
    if (!token.trimmed().isEmpty())
        parsed.labels.append(token.trimmed());
    return parsed;
}

QString shortcutCapLabel(const QString &token)
{
    // Spelled-out names stay legible at 12 px; icons made the caps read as
    // badges rather than keys.
    const QString lower = token.toLower();
    if (lower == QStringLiteral("win") || lower == QStringLiteral("windows")
        || lower == QStringLiteral("meta"))
        return QStringLiteral("Win");
    if (lower == QStringLiteral("shift"))
        return QStringLiteral("Shift");
    return token;
}

} // namespace

void paintShortcutKeycaps(QPainter *painter, const QStyleOption *option, const QLineEdit *editor)
{
    if (!painter || !option || !editor)
        return;
    const Tokens t = tokens(option->palette);
    const bool enabled = option->state & QStyle::State_Enabled;
    // QKeySequenceEdit keeps the editor focused for as long as the recorder
    // owns the keys, so focus is the recording state.
    const bool recording = option->state & QStyle::State_HasFocus;
    const ShortcutText parsed = parseShortcutText(editor->text());
    const QRectF bounds =
            QRectF(option->rect).adjusted(ShortcutFieldTextInset, 0, -ShortcutFieldTextInset, 0);
    const QFont savedFont = painter->font();
    const QColor fill = !enabled ? t.accentFillDisabled
            : recording          ? t.accentFillPressed
                                 : t.accentFill;
    // Contrast-resolved ink: a custom/system accent can be light, so the fixed
    // white-on-accent used by WinUI's check templates is not safe.
    const QColor ink = enabled ? t.textOnAccentPrimary : t.textOnAccentDisabled;
    QFont font = savedFont;
    font.setPixelSize(ShortcutKeycapFontSize);
    painter->setFont(font);
    const QFontMetrics fm(font);
    QVector<QRectF> capRects;
    qreal x = bounds.x();
    const qreal capY = bounds.y() + (bounds.height() - ShortcutKeycapHeight) / 2.0;
    for (int i = 0; i < parsed.labels.size(); ++i) {
        const QString label = shortcutCapLabel(parsed.labels.at(i));
        const qreal width = fm.horizontalAdvance(label) + 2 * ShortcutKeycapPadding;
        const QRectF cap(x, capY, width, ShortcutKeycapHeight);
        PaintPrivate::roundedRect(painter, cap, fill, t.accentStrokeSecondary, ControlRadius);
        painter->setPen(ink);
        painter->drawText(cap, Qt::AlignCenter, label);
        capRects.append(cap);
        if (i == parsed.labels.size() - 1)
            break;
        x = cap.right()
                + (parsed.separators.value(i) == QLatin1Char(',') ? ShortcutChordBreakGap
                                                                  : ShortcutKeycapGap);
        if (x >= bounds.right())
            break;
    }
    // Overlay separators after all caps exist: the dot sits on the seam between
    // two caps, so they read as one group without a wide gap.
    for (int i = 0; i + 1 < capRects.size(); ++i) {
        const QRectF &left = capRects.at(i);
        const QRectF &right = capRects.at(i + 1);
        const QPointF seam((left.right() + right.left()) / 2.0, left.center().y());
        if (parsed.separators.value(i) == QLatin1Char(',')) {
            const qreal markWidth =
                    fm.horizontalAdvance(QStringLiteral(",")) + 2 * ShortcutSeparatorPadding;
            painter->setPen(t.textPrimary);
            painter->drawText(
                    QRectF(seam.x() - markWidth / 2, capY, markWidth, ShortcutKeycapHeight),
                    Qt::AlignCenter, QStringLiteral(","));
        } else {
            QRectF dot(0, 0, 2 * ShortcutChordDotRadius, 2 * ShortcutChordDotRadius);
            dot.moveCenter(seam);
            const bool antialiased = painter->renderHints().testFlag(QPainter::Antialiasing);
            painter->setRenderHint(QPainter::Antialiasing, true);
            // Outline follows the cap ink: white in Light, dark in Dark, with
            // the cap's accent fill inside.
            painter->setPen(ink);
            painter->setBrush(fill);
            painter->drawEllipse(dot);
            painter->setBrush(Qt::NoBrush);
            painter->setRenderHint(QPainter::Antialiasing, antialiased);
        }
    }
    painter->setFont(savedFont);
    if (parsed.labels.isEmpty() && recording) {
        QFont hintFont = savedFont;
        hintFont.setPixelSize(ShortcutKeycapFontSize);
        painter->setFont(hintFont);
        painter->setPen(t.textSecondary);
        painter->drawText(bounds, Qt::AlignLeft | Qt::AlignVCenter,
                          QStringLiteral("Press shortcut"));
        painter->setFont(savedFont);
    }
}

} // namespace WinUI3::Private
