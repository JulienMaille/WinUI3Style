// SPDX-License-Identifier: LGPL-2.1-or-later
#pragma once

#include "winui3tokens_p.h"

#include <QEasingCurve>

namespace WinUI3::Private {

// SettingsExpander.xaml pins expand to (0,0,0,1) and collapse to (1,1,0,1).
// Keep the compiled card and the stock QStyle path on this one definition.
inline int settingsExpanderDuration(bool expanding)
{
    return expanding ? SettingsExpanderExpandDuration : SettingsExpanderCollapseDuration;
}

inline QEasingCurve settingsExpanderCurve(bool expanding)
{
    QEasingCurve curve(QEasingCurve::BezierSpline);
    if (expanding)
        curve.addCubicBezierSegment(QPointF(0.0, 0.0), QPointF(0.0, 1.0), QPointF(1.0, 1.0));
    else
        curve.addCubicBezierSegment(QPointF(1.0, 1.0), QPointF(0.0, 1.0), QPointF(1.0, 1.0));
    return curve;
}

} // namespace WinUI3::Private
