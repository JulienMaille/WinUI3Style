#include <winui3style/settingscard.h>
#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include "winui3tokens_p.h"

#include <QGridLayout>
#include <QEvent>
#include <QHideEvent>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QVariantAnimation>
#include <QVBoxLayout>

namespace WinUI3 {

SettingsCard::SettingsCard(QWidget *parent)
    : QFrame(parent)
    , m_headerLayout(new QGridLayout)
    , m_rootLayout(new QVBoxLayout(this))
    , m_headerHost(new QWidget(this))
    , m_iconLabel(new QLabel(this))
    , m_titleLabel(new QLabel(this))
    , m_descriptionLabel(new QLabel(this))
    , m_chevronLabel(new QLabel(this))
    , m_expandableHost(new QWidget(this))
    , m_expansionAnimation(new QVariantAnimation(this))
{
    Style::setSettingsCard(this);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    setAttribute(Qt::WA_Hover);

    m_rootLayout->setContentsMargins(16, 0, 16, 0);
    m_rootLayout->setSpacing(0);
    m_headerHost->setObjectName(QStringLiteral("_winui_settings_card_headerHost"));
    m_expandableHost->setObjectName(
        QStringLiteral("_winui_settings_card_expandableHost"));
    m_headerHost->setMouseTracking(true);
    m_headerHost->installEventFilter(this);
    m_headerHost->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_headerLayout->setContentsMargins(0, 12, 0, 12);
    m_headerLayout->setHorizontalSpacing(12);
    m_headerLayout->setVerticalSpacing(2);
    m_iconLabel->setFixedWidth(20);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_descriptionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_descriptionLabel->setWordWrap(true);
    m_titleLabel->setObjectName(QStringLiteral("_winui_settings_card_title"));
    m_descriptionLabel->setObjectName(QStringLiteral("_winui_settings_card_description"));
    m_chevronLabel->setObjectName(QStringLiteral("_winui_settings_card_chevron"));
    m_chevronLabel->setFixedWidth(20);
    m_chevronLabel->setAlignment(Qt::AlignCenter);
    m_chevronLabel->setVisible(false);
    // Decorative header children let the header host receive the gesture so
    // it can activate the card. The trailing widget deliberately remains a
    // normal mouse target (a transparent ancestor would disable its entire
    // subtree, including toggles and combo boxes).
    m_iconLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_descriptionLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_chevronLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    QFont titleFont = m_titleLabel->font();
    titleFont.setWeight(QFont::DemiBold);
    m_titleLabel->setFont(titleFont);

    m_headerLayout->addWidget(m_iconLabel, 0, 0, 2, 1);
    m_headerLayout->addWidget(m_titleLabel, 0, 1);
    m_headerLayout->addWidget(m_descriptionLabel, 1, 1);
    m_headerLayout->addWidget(m_chevronLabel, 0, 2, 2, 1,
                              Qt::AlignVCenter);
    m_headerLayout->setColumnMinimumWidth(2, 20);
    m_headerLayout->setColumnStretch(1, 1);
    m_headerHost->setLayout(m_headerLayout);
    m_rootLayout->addWidget(m_headerHost);
    m_rootLayout->addWidget(m_expandableHost);
    // Keep the header anchored to the top of the card. Without a trailing
    // stretch, QBoxLayout can redistribute a card's spare height between the
    // fixed header and the expanding host, moving the title as expansion
    // starts even though the header itself is fixed-height.
    m_rootLayout->addStretch(1);
    m_expandableHost->installEventFilter(this);
    m_expandableHost->setMaximumHeight(0);
    m_expandableHost->setVisible(false);

    connect(m_expansionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) { setExpansionProgress(value.toReal()); });
    m_expansionAnimation->setObjectName(
        QStringLiteral("_winui_settings_card_expansion_animation"));
    refreshChevronPixmap();
    refreshHeaderGeometry();
}

SettingsCard::~SettingsCard()
{
    if (m_expandableDestroyedConnection)
        disconnect(m_expandableDestroyedConnection);
    if (m_expandableWidget)
        m_expandableWidget->removeEventFilter(this);
}

QString SettingsCard::title() const { return m_titleLabel->text(); }
void SettingsCard::setTitle(const QString &title)
{
    if (this->title() == title) return;
    m_titleLabel->setText(title);
    m_headerWidth = -1;
    refreshHeaderGeometry();
    emit titleChanged(title);
}

QString SettingsCard::description() const { return m_descriptionLabel->text(); }
void SettingsCard::setDescription(const QString &description)
{
    if (this->description() == description) return;
    m_descriptionLabel->setText(description);
    m_descriptionLabel->setVisible(!description.isEmpty());
    m_headerWidth = -1;
    refreshHeaderGeometry();
    emit descriptionChanged(description);
}

QIcon SettingsCard::icon() const { return m_icon; }
void SettingsCard::setIcon(const QIcon &icon)
{
    if (m_icon.cacheKey() == icon.cacheKey()) return;
    m_icon = icon;
    refreshIconPixmap();
    refreshChevronPixmap();
    m_iconLabel->setVisible(!icon.isNull());
    m_headerWidth = -1;
    refreshHeaderGeometry();
    emit iconChanged(icon);
}

void SettingsCard::refreshIconPixmap()
{
    if (!m_icon.isNull()) {
        const Private::Tokens t = Private::tokens(palette());
        m_iconLabel->setPixmap(iconPixmap(m_icon, QSize(20, 20),
            devicePixelRatioF(), isEnabled() ? t.textPrimary : t.textDisabled,
            isEnabled() ? QIcon::Normal : QIcon::Disabled));
    }
}

void SettingsCard::refreshChevronPixmap()
{
    if (!m_chevronLabel)
        return;
    const Icon glyph = m_expanded ? Icon::ChevronDown
        : layoutDirection() == Qt::RightToLeft ? Icon::ChevronLeft
                                                : Icon::ChevronRight;
    const Private::Tokens t = Private::tokens(palette());
    m_chevronLabel->setPixmap(iconPixmap(
        WinUI3::icon(glyph), QSize(20, 20), devicePixelRatioF(),
        isEnabled() ? t.textSecondary : t.textDisabled,
        isEnabled() ? QIcon::Normal : QIcon::Disabled));
    m_chevronLabel->setProperty("_winui_settings_card_chevron_glyph",
                               static_cast<int>(glyph));
    m_chevronLabel->setVisible(m_expandableWidget != nullptr);
}

QWidget *SettingsCard::trailingWidget() const { return m_trailingWidget; }
void SettingsCard::setTrailingWidget(QWidget *widget)
{
    if (widget == m_trailingWidget) return;
    if (m_trailingWidget) {
        m_headerLayout->removeWidget(m_trailingWidget);
        m_trailingWidget->setParent(nullptr);
    }
    m_trailingWidget = widget;
    if (widget) {
        widget->setParent(this);
        m_headerLayout->addWidget(widget, 0, 3, 2, 1, Qt::AlignVCenter);
    }
    m_headerWidth = -1;
    refreshHeaderGeometry();
}

QWidget *SettingsCard::expandableWidget() const { return m_expandableWidget; }
void SettingsCard::setExpandableWidget(QWidget *widget)
{
    if (widget == m_expandableWidget) return;

    const bool wasExpanded = m_expanded;
    resetExpansionState(false);
    if (m_expandableDestroyedConnection)
        disconnect(m_expandableDestroyedConnection);
    if (m_expandableWidget) {
        m_expandableWidget->removeEventFilter(this);
        delete m_expandableHost->layout();
        m_expandableWidget->setParent(nullptr);
    }
    m_expandableWidget = widget;
    if (widget) {
        auto *layout = new QVBoxLayout(m_expandableHost);
        const bool rtl = layoutDirection() == Qt::RightToLeft;
        layout->setContentsMargins(rtl ? 0 : 32, 0, rtl ? 32 : 0, 16);
        layout->addWidget(widget);
        widget->installEventFilter(this);
        const QPointer<QWidget> guarded = widget;
        m_expandableDestroyedConnection = connect(widget, &QObject::destroyed,
            this, [this, guarded] {
            if (m_expandableWidget == guarded) {
                m_expandableWidget = nullptr;
                resetExpansionState(true);
                refreshChevronPixmap();
                updateGeometry();
            }
        });
        widget->show();
    }
    invalidateExpandableHeight();
    refreshChevronPixmap();
    m_headerWidth = -1;
    refreshHeaderGeometry();
    updateGeometry();
    if (wasExpanded)
        emit expandedChanged(false);
}

bool SettingsCard::isExpanded() const { return m_expanded; }
void SettingsCard::setExpanded(bool expanded)
{
    if (!m_expandableWidget) expanded = false;
    if (m_expanded == expanded) return;
    m_expanded = expanded;
    refreshChevronPixmap();
    m_expandableHost->setVisible(true);
    m_expansionAnimation->stop();
    m_expansionAnimation->setStartValue(m_expansionProgress);
    m_expansionAnimation->setEndValue(expanded ? 1.0 : 0.0);
    m_expansionAnimation->setDuration(Private::NormalDuration);
    m_expansionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    if (Style::animationsAllowed())
        m_expansionAnimation->start();
    else
        setExpansionProgress(expanded ? 1.0 : 0.0);
    emit expandedChanged(expanded);
}

qreal SettingsCard::expansionProgress() const { return m_expansionProgress; }
void SettingsCard::setExpansionProgress(qreal progress)
{
    m_expansionProgress = qBound<qreal>(0.0, progress, 1.0);
    const int contentHeight = m_expandableWidget ? expandableContentHeight() : 0;
    const int maximumHeight = qRound(contentHeight * m_expansionProgress);
    if (m_expandableHost->maximumHeight() != maximumHeight)
        m_expandableHost->setMaximumHeight(maximumHeight);
    if (qFuzzyIsNull(m_expansionProgress) && !m_expanded)
        m_expandableHost->setVisible(false);
    // The header lives in a fixed-size host. The parent may still relayout to
    // accommodate the animated content, but title/description geometry is
    // independent of that changing height.
    update();
}

void SettingsCard::refreshHeaderGeometry()
{
    if (!m_headerHost || !m_headerLayout)
        return;

    m_headerLayout->activate();
    const int width = m_headerHost->width();
    if (width <= 0 && m_headerHeight > 0)
        return;

    const int preferredHeight = m_headerLayout->sizeHint().height();
    if (preferredHeight <= 0)
        return;

    if (m_headerWidth == width && m_headerHeight == preferredHeight)
        return;
    m_headerWidth = width;
    m_headerHeight = preferredHeight;
    if (m_headerHost->height() != preferredHeight)
        m_headerHost->setFixedHeight(preferredHeight);
}

void SettingsCard::invalidateExpandableHeight()
{
    m_expandableHeightValid = false;
    m_expandableContentWidth = -1;
}

void SettingsCard::scheduleExpandableHeightRefresh()
{
    if (!m_expanded || m_expandableHeightRefreshPending) {
        return;
    }
    m_expandableHeightRefreshPending = true;
    QMetaObject::invokeMethod(this, [this] {
        m_expandableHeightRefreshPending = false;
        if (m_expandableWidget && m_expanded)
            setExpansionProgress(m_expansionProgress);
    }, Qt::QueuedConnection);
}

int SettingsCard::expandableContentHeight()
{
    if (!m_expandableWidget)
        return 0;

    const int width = m_expandableWidget->width();
    if (!m_expandableHeightValid || m_expandableContentWidth != width) {
        m_expandableContentHeight = qMax(0, m_expandableWidget->sizeHint().height() + 16);
        m_expandableContentWidth = width;
        m_expandableHeightValid = true;
    }
    return m_expandableContentHeight;
}

void SettingsCard::resetExpansionState(bool notify)
{
    m_expansionAnimation->stop();
    m_expansionProgress = 0.0;
    m_expandableHeightRefreshPending = false;
    if (m_expandableHost->maximumHeight() != 0)
        m_expandableHost->setMaximumHeight(0);
    m_expandableHost->setVisible(false);
    const bool wasExpanded = m_expanded;
    m_expanded = false;
    refreshChevronPixmap();
    if (notify && wasExpanded)
        emit expandedChanged(false);
}

bool SettingsCard::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_headerHost) {
        if (event->type() == QEvent::MouseButtonPress
            || event->type() == QEvent::MouseButtonRelease) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            const QPointF cardPosition = m_headerHost->mapTo(
                this, mouse->position().toPoint());
            QMouseEvent forwarded(event->type(), cardPosition,
                                  mouse->globalPosition(), mouse->button(),
                                  mouse->buttons(), mouse->modifiers(),
                                  mouse->pointingDevice());
            QCoreApplication::sendEvent(this, &forwarded);
            event->accept();
            return true;
        }
        if (event->type() == QEvent::Leave) {
            m_pressed = false;
            update();
        }
    }
    if ((watched == m_expandableWidget || watched == m_expandableHost)
        && (event->type() == QEvent::LayoutRequest
            || event->type() == QEvent::Resize)) {
        invalidateExpandableHeight();
        scheduleExpandableHeightRefresh();
    }
    return QFrame::eventFilter(watched, event);
}

bool SettingsCard::headerContains(const QPoint &position) const
{
    return m_headerHost && m_headerHost->geometry().contains(position);
}

void SettingsCard::changeEvent(QEvent *event)
{
    const bool refreshHeader = event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::StyleChange
        || event->type() == QEvent::DevicePixelRatioChange
        || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::LayoutDirectionChange;
    if (refreshHeader) {
        refreshIconPixmap();
        if (event->type() == QEvent::LayoutDirectionChange
            && m_expandableHost->layout()) {
            const bool rtl = layoutDirection() == Qt::RightToLeft;
            m_expandableHost->layout()->setContentsMargins(
                rtl ? 0 : 32, 0, rtl ? 32 : 0, 16);
        }
    }
    QFrame::changeEvent(event);
    if (refreshHeader) {
        m_headerWidth = -1;
        refreshHeaderGeometry();
        refreshChevronPixmap();
    }
}

void SettingsCard::hideEvent(QHideEvent *event)
{
    if (m_expansionAnimation->state() != QAbstractAnimation::Stopped) {
        m_expansionAnimation->stop();
        setExpansionProgress(m_expanded ? 1.0 : 0.0);
    }
    refreshChevronPixmap();
    QFrame::hideEvent(event);
}

void SettingsCard::resizeEvent(QResizeEvent *event)
{
    QFrame::resizeEvent(event);
    m_headerWidth = -1;
    refreshHeaderGeometry();
}

void SettingsCard::leaveEvent(QEvent *event)
{
    m_pressed = false;
    QFrame::leaveEvent(event);
}

void SettingsCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && headerContains(event->position().toPoint())) {
        m_pressed = true;
        update();
        event->accept();
        return;
    }
    QFrame::mousePressEvent(event);
}

void SettingsCard::mouseReleaseEvent(QMouseEvent *event)
{
    const bool activate = m_pressed && event->button() == Qt::LeftButton
                          && headerContains(event->position().toPoint());
    m_pressed = false;
    update();
    if (activate) {
        emit activated();
        if (m_expandableWidget) setExpanded(!m_expanded);
        event->accept();
        return;
    }
    QFrame::mouseReleaseEvent(event);
}

void SettingsCard::keyPressEvent(QKeyEvent *event)
{
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Space)
        && !event->isAutoRepeat()) {
        emit activated();
        if (m_expandableWidget) setExpanded(!m_expanded);
        event->accept();
        return;
    }
    QFrame::keyPressEvent(event);
}

} // namespace WinUI3
