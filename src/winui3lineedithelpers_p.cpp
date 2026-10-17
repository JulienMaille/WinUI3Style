// SPDX-License-Identifier: LGPL-2.1-or-later
// QLineEdit helper affordances (S3 surfaces split): the private clear button
// cache, the QAction helper buttons and their coalesced update pass. Moved
// verbatim from winui3surfaces_p.cpp.
#include "winui3lineedithelpers_p.h"

#include "winui3frameproperties_p.h"
#include "winui3helpers_p.h"
#include "winui3qtcompat_p.h"
#include "winui3style_properties_p.h"

#include <winui3style/winui3style.h>

#include <QAbstractButton>
#include <QAction>
#include <QHash>
#include <QLineEdit>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
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

} // namespace WinUI3::Private
