#include <winui3style/settingscard.h>
#include <winui3style/winui3icons.h>
#include <winui3style/winui3style.h>

#include "winui3tokens_p.h"

#include <QGridLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QMouseEvent>
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
    m_headerHost->setAttribute(Qt::WA_TransparentForMouseEvents);
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
    m_expandableHost->setMaximumHeight(0);
    m_expandableHost->setVisible(false);

    connect(m_expansionAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant &value) { setExpansionProgress(value.toReal()); });
    refreshChevronPixmap();
}

SettingsCard::~SettingsCard() = default;

QString SettingsCard::title() const { return m_titleLabel->text(); }
void SettingsCard::setTitle(const QString &title)
{
    if (this->title() == title) return;
    m_titleLabel->setText(title);
    emit titleChanged(title);
}

QString SettingsCard::description() const { return m_descriptionLabel->text(); }
void SettingsCard::setDescription(const QString &description)
{
    if (this->description() == description) return;
    m_descriptionLabel->setText(description);
    m_descriptionLabel->setVisible(!description.isEmpty());
    emit descriptionChanged(description);
}

QIcon SettingsCard::icon() const { return m_icon; }
void SettingsCard::setIcon(const QIcon &icon)
{
    if (m_icon.cacheKey() == icon.cacheKey()) return;
    m_icon = icon;
    refreshIconPixmap();
    m_iconLabel->setVisible(!icon.isNull());
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
    refreshChevronPixmap();
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
}

QWidget *SettingsCard::expandableWidget() const { return m_expandableWidget; }
void SettingsCard::setExpandableWidget(QWidget *widget)
{
    if (widget == m_expandableWidget) return;
    if (m_expandableWidget) {
        delete m_expandableHost->layout();
        m_expandableWidget->setParent(nullptr);
    }
    m_expandableWidget = widget;
    if (widget) {
        auto *layout = new QVBoxLayout(m_expandableHost);
        const bool rtl = layoutDirection() == Qt::RightToLeft;
        layout->setContentsMargins(rtl ? 0 : 32, 0, rtl ? 32 : 0, 16);
        layout->addWidget(widget);
        widget->show();
    } else {
        setExpanded(false);
    }
    refreshChevronPixmap();
    updateGeometry();
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
    const int contentHeight = m_expandableWidget
        ? m_expandableWidget->sizeHint().height() + 16 : 0;
    m_expandableHost->setMaximumHeight(qRound(contentHeight * m_expansionProgress));
    if (qFuzzyIsNull(m_expansionProgress) && !m_expanded)
        m_expandableHost->setVisible(false);
    // The header lives in a fixed-size host. The parent may still relayout to
    // accommodate the animated content, but title/description geometry is
    // independent of that changing height.
    update();
}

bool SettingsCard::headerContains(const QPoint &position) const
{
    return m_headerHost && m_headerHost->geometry().contains(position);
}

void SettingsCard::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::PaletteChange
        || event->type() == QEvent::StyleChange
        || event->type() == QEvent::DevicePixelRatioChange
        || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::LayoutDirectionChange) {
        refreshIconPixmap();
        if (event->type() == QEvent::LayoutDirectionChange
            && m_expandableHost->layout()) {
            const bool rtl = layoutDirection() == Qt::RightToLeft;
            m_expandableHost->layout()->setContentsMargins(
                rtl ? 0 : 32, 0, rtl ? 32 : 0, 16);
        }
    }
    QFrame::changeEvent(event);
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
