#include <winui3style/winui3icons.h>

#include <QFont>
#include <QFontDatabase>
#include <QCache>
#include <QGuiApplication>
#include <QIconEngine>
#include <QPainter>
#include <QPalette>
#include <QPointer>
#include <QThread>

#include <array>

namespace WinUI3 {
namespace {

constexpr auto fluentIconNamePrefix = "winui3-fluent-icon:";
constexpr int fluentIconCount = static_cast<int>(Icon::Warning) + 1;
constexpr int MaxPixmapCacheCost = 4096;

QString colorKey(const QColor &color)
{
    return QString::number(quint64(color.rgba64()), 16);
}

QChar codePoint(Icon icon)
{
    switch (icon) {
    case Icon::Add: return QChar(0xE710);
    case Icon::Back: return QChar(0xE72B);
    case Icon::Check: return QChar(0xE73E);
    case Icon::ChevronDown: return QChar(0xE70D);
    case Icon::ChevronLeft: return QChar(0xE76B);
    case Icon::ChevronRight: return QChar(0xE76C);
    case Icon::ChevronUp: return QChar(0xE70E);
    case Icon::Clear: return QChar(0xE894);
    case Icon::Close: return QChar(0xE711);
    case Icon::Delete: return QChar(0xE74D);
    case Icon::Edit: return QChar(0xE70F);
    case Icon::Error: return QChar(0xE783);
    case Icon::Folder: return QChar(0xE8B7);
    case Icon::Help: return QChar(0xE897);
    case Icon::Home: return QChar(0xE80F);
    case Icon::Info: return QChar(0xE946);
    case Icon::More: return QChar(0xE712);
    case Icon::Pause: return QChar(0xE769);
    case Icon::Play: return QChar(0xE768);
    case Icon::Refresh: return QChar(0xE72C);
    case Icon::Save: return QChar(0xE74E);
    case Icon::Search: return QChar(0xE721);
    case Icon::Settings: return QChar(0xE713);
    case Icon::Stop: return QChar(0xE71A);
    case Icon::Warning: return QChar(0xE7BA);
    }
    return {};
}

struct ParsedFluentIcon {
    Icon glyph = Icon::Add;
    QString variant;
};

bool parseFluentIconName(const QString &name, ParsedFluentIcon *parsed)
{
    const QString prefix = QString::fromLatin1(fluentIconNamePrefix);
    if (!name.startsWith(prefix))
        return false;

    const int variantSeparator = name.indexOf(QLatin1Char(':'), prefix.size());
    const int numberLength = variantSeparator >= 0
        ? variantSeparator - prefix.size() : name.size() - prefix.size();
    bool ok = false;
    const int value = name.mid(prefix.size(), numberLength).toInt(&ok);
    if (!ok || value < 0 || value >= fluentIconCount)
        return false;

    if (parsed) {
        parsed->glyph = static_cast<Icon>(value);
        parsed->variant = variantSeparator >= 0
            ? name.mid(variantSeparator + 1) : QString();
    }
    return true;
}

class IconRuntime final
{
public:
    IconRuntime()
    {
        m_pixmaps.setMaxCost(MaxPixmapCacheCost);
    }

    void syncApplication()
    {
        auto *application = qobject_cast<QGuiApplication *>(
            QCoreApplication::instance());
        if (application == m_application)
            return;

        if (m_fontDatabaseConnection)
            QObject::disconnect(m_fontDatabaseConnection);
        m_application = application;
        m_fontDatabaseConnection = {};
        invalidate();
        if (application) {
            m_fontDatabaseConnection = QObject::connect(
                application, &QGuiApplication::fontDatabaseChanged,
                application, [this] { invalidate(); });
        }
    }

    QString fluentFontFamily()
    {
        syncApplication();
        if (!m_fontFamilyResolved) {
            const QString fluent = QStringLiteral("Segoe Fluent Icons");
            const QString mdl2 = QStringLiteral("Segoe MDL2 Assets");
            const QStringList families = QFontDatabase::families();
            // Keep the old fallback semantics: MDL2 is selected whenever the
            // preferred family is unavailable, including when both are
            // absent and Qt has to perform its normal font fallback.
            m_fontFamily = families.contains(fluent) ? fluent : mdl2;
            m_fontFamilyResolved = true;
        }
        return m_fontFamily;
    }

    QPixmap *findPixmap(const QString &key)
    {
        return m_pixmaps.object(key);
    }

    void insertPixmap(const QString &key, const QPixmap &pixmap)
    {
        const qint64 bytes = qint64(pixmap.width()) * pixmap.height() * 4;
        if (bytes <= 0 || bytes > qint64(MaxPixmapCacheCost) * 1024)
            return;
        const int cost = qMax(1, int((bytes + 1023) / 1024));
        m_pixmaps.insert(key, new QPixmap(pixmap), cost);
    }

private:
    void invalidate()
    {
        m_fontFamily.clear();
        m_fontFamilyResolved = false;
        m_pixmaps.clear();
    }

    QPointer<QGuiApplication> m_application;
    QMetaObject::Connection m_fontDatabaseConnection;
    QString m_fontFamily;
    bool m_fontFamilyResolved = false;
    QCache<QString, QPixmap> m_pixmaps;
};

IconRuntime &iconRuntime()
{
    static IconRuntime runtime;
    return runtime;
}

bool canUseGuiCache()
{
    const auto *application = qobject_cast<const QGuiApplication *>(
        QCoreApplication::instance());
    return application && QThread::currentThread() == application->thread();
}

void paintGlyph(QPainter *painter, Icon glyph, const QRect &rect,
                const QColor &color)
{
    painter->save();
    painter->setRenderHint(QPainter::TextAntialiasing);
    QFont font(iconRuntime().fluentFontFamily());
    font.setPixelSize(qMax(8, qMin(rect.width(), rect.height())));
    painter->setFont(font);
    painter->setPen(color);
    painter->drawText(rect, Qt::AlignCenter, QString(codePoint(glyph)));
    painter->restore();
}

QString pixmapKey(const ParsedFluentIcon &parsed, const QSize &logicalSize,
                  const QSize &physicalSize, qreal devicePixelRatio,
                  const QColor &foreground, QIcon::Mode mode,
                  QIcon::State state)
{
    return QStringLiteral("winui3-fluent-pixmap:%1:%2:%3:%4x%5:%6x%7:%8:%9:%10:%11")
        .arg(static_cast<int>(parsed.glyph))
        .arg(parsed.variant)
        .arg(QString::number(devicePixelRatio, 'g', 17))
        .arg(logicalSize.width()).arg(logicalSize.height())
        .arg(physicalSize.width()).arg(physicalSize.height())
        .arg(colorKey(foreground))
        .arg(static_cast<int>(mode)).arg(static_cast<int>(state));
}

class FluentIconEngine final : public QIconEngine
{
public:
    explicit FluentIconEngine(Icon icon, const QColor &color = {})
        : m_icon(icon), m_color(color) {}

    QIconEngine *clone() const override
    {
        return new FluentIconEngine(m_icon, m_color);
    }

    QString key() const override
    {
        return QStringLiteral("WinUI3FluentIconEngine");
    }

    QString iconName() override
    {
        return QString::fromLatin1(fluentIconNamePrefix)
            + QString::number(static_cast<int>(m_icon));
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, result.rect(), mode, state);
        return result;
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode,
               QIcon::State) override
    {
        // QIconEngine is not given the palette of the control asking for a
        // pixmap. Keep the uncoloured overload as a neutral alpha mask; the
        // style resolves the actual foreground through iconPixmap() using the
        // QStyleOption palette. This deliberately avoids QApplication's
        // global palette, which is wrong for accent, selected and local-palette
        // controls.
        QColor color = m_color;
        if (!color.isValid()) {
            const QPalette palette = qApp ? qApp->palette() : QPalette();
            color = palette.color(mode == QIcon::Disabled
                                      ? QPalette::Disabled : QPalette::Active,
                                  QPalette::WindowText);
        }
        paintGlyph(painter, m_icon, rect, color);
    }

    QSize actualSize(const QSize &size, QIcon::Mode, QIcon::State) override
    {
        return size;
    }

private:
    Icon m_icon;
    QColor m_color;
};

const QIcon &cachedIcon(Icon glyph)
{
    static const std::array<QIcon, fluentIconCount> icons = [] {
        std::array<QIcon, fluentIconCount> result;
        for (int index = 0; index < fluentIconCount; ++index)
            result[index] = QIcon(new FluentIconEngine(static_cast<Icon>(index)));
        return result;
    }();
    return icons[static_cast<int>(glyph)];
}

} // namespace

QIcon icon(Icon glyph)
{
    return cachedIcon(glyph);
}

QIcon icon(Icon glyph, const QColor &color)
{
    if (!color.isValid())
        return icon(glyph);
    return QIcon(new FluentIconEngine(glyph, color));
}

bool isFluentIcon(const QIcon &icon)
{
    return parseFluentIconName(icon.name(), nullptr);
}

QPixmap iconPixmap(const QIcon &source, const QSize &size,
                   qreal devicePixelRatio, const QColor &foreground,
                   QIcon::Mode mode, QIcon::State state)
{
    if (source.isNull() || size.isEmpty())
        return {};
    ParsedFluentIcon parsed;
    if (!parseFluentIconName(source.name(), &parsed)
        || !foreground.isValid()) {
        return source.pixmap(size, devicePixelRatio, mode, state);
    }

    const bool neutralSource = source.cacheKey()
        == cachedIcon(parsed.glyph).cacheKey();
    // Keep the public icon name stable. A coloured QIcon has a distinct
    // cache identity, so include that identity only in the private pixmap
    // key and render its source directly to preserve its alpha. The neutral
    // source also carries the application WindowText alpha; include the
    // application palette identity so a palette change cannot reuse it.
    if (!neutralSource) {
        parsed.variant = QString::number(source.cacheKey(), 16);
    } else if (auto *application = qobject_cast<QGuiApplication *>(
                   QCoreApplication::instance())) {
        parsed.variant = QString::number(application->palette().cacheKey(), 16);
    }

    const QSize physicalSize(
        qMax(1, qRound(size.width() * devicePixelRatio)),
        qMax(1, qRound(size.height() * devicePixelRatio)));
    const QString key = pixmapKey(parsed, size, physicalSize, devicePixelRatio,
                                  foreground, mode, state);
    if (!canUseGuiCache())
        return source.pixmap(size, devicePixelRatio, mode, state);

    IconRuntime &runtime = iconRuntime();
    runtime.syncApplication();
    if (QPixmap *cached = runtime.findPixmap(key))
        return *cached;

    // Render neutral Fluent glyphs as a white alpha mask. This is
    // pixel-equivalent for the chevron hot paths; glyphs that differ after
    // recolouring keep their exact coloured-engine paint path in the style.
    QPixmap pixmap;
    if (neutralSource) {
        pixmap = QPixmap(physicalSize);
        pixmap.fill(Qt::transparent);
        QPainter maskPainter(&pixmap);
        paintGlyph(&maskPainter, parsed.glyph, pixmap.rect(), Qt::white);
        maskPainter.end();
        pixmap.setDevicePixelRatio(devicePixelRatio);
    } else {
        // Keep the source alpha exactly as QIcon::pixmap() produces it. The
        // foreground and application-palette identities are part of the key,
        // so local-palette recolouring cannot reuse a stale mask.
        pixmap = source.pixmap(size, devicePixelRatio, mode, state);
    }
    if (pixmap.isNull())
        return pixmap;

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), foreground);
    painter.end();
    runtime.insertPixmap(key, pixmap);
    return pixmap;
}

} // namespace WinUI3
