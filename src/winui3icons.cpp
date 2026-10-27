// SPDX-License-Identifier: LGPL-2.1-or-later
#include <winui3style/winui3icons.h>

#include "winui3icons_p.h"
#include "winui3qtcompat_p.h"

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
constexpr int MaxFontCacheCost = 64;
// Coloured QIcons are normally requested from paint paths. Keep this cache
// deliberately bounded because callers can provide arbitrary accent colors,
// while still avoiding a QIconEngine allocation for every paint event.
constexpr int MaxColoredIconCacheCost = 256;

QString colorKey(const QColor &color)
{
    return QString::number(quint64(color.rgba64()), 16);
}

QString codePointString(Icon icon, bool hasSegoeFont)
{
    if (hasSegoeFont) {
        char32_t ucs4 = 0;
        switch (icon) {
        case Icon::Add:
            ucs4 = 0xE710; break;
        case Icon::Back:
            ucs4 = 0xE72B; break;
        case Icon::Check:
            ucs4 = 0xE73E; break;
        case Icon::ChevronDown:
            ucs4 = 0xE70D; break;
        case Icon::ChevronLeft:
            ucs4 = 0xE76B; break;
        case Icon::ChevronRight:
            ucs4 = 0xE76C; break;
        case Icon::ChevronUp:
            ucs4 = 0xE70E; break;
        case Icon::Clear:
            ucs4 = 0xE894; break;
        case Icon::Close:
            ucs4 = 0xE711; break;
        case Icon::Delete:
            ucs4 = 0xE74D; break;
        case Icon::Edit:
            ucs4 = 0xE70F; break;
        case Icon::Error:
            ucs4 = 0xE783; break;
        case Icon::Folder:
            ucs4 = 0xE8B7; break;
        case Icon::Help:
            ucs4 = 0xE897; break;
        case Icon::Home:
            ucs4 = 0xE80F; break;
        case Icon::Info:
            ucs4 = 0xE946; break;
        case Icon::More:
            ucs4 = 0xE712; break;
        case Icon::Pause:
            ucs4 = 0xE769; break;
        case Icon::Play:
            ucs4 = 0xE768; break;
        case Icon::Refresh:
            ucs4 = 0xE72C; break;
        case Icon::Save:
            ucs4 = 0xE74E; break;
        case Icon::Search:
            ucs4 = 0xE721; break;
        case Icon::Settings:
            ucs4 = 0xE713; break;
        case Icon::Stop:
            ucs4 = 0xE71A; break;
        case Icon::Document:
            ucs4 = 0xE7C3; break;
        case Icon::List:
            ucs4 = 0xE8FD; break;
        case Icon::Warning:
            ucs4 = 0xE7BA; break;
        }
        return QString::fromUcs4(&ucs4, 1);
    }

    // System font standard Unicode fallback glyphs (BMP and non-BMP UCS4)
    char32_t ucs4 = 0;
    switch (icon) {
    case Icon::Add:
        ucs4 = 0x002B; break; // +
    case Icon::Back:
        ucs4 = 0x2190; break; // ←
    case Icon::Check:
        ucs4 = 0x2713; break; // ✓
    case Icon::ChevronDown:
        ucs4 = 0x25BC; break; // ▼
    case Icon::ChevronLeft:
        ucs4 = 0x25C0; break; // ◀
    case Icon::ChevronRight:
        ucs4 = 0x25B6; break; // ▶
    case Icon::ChevronUp:
        ucs4 = 0x25B2; break; // ▲
    case Icon::Clear:
        ucs4 = 0x2715; break; // ✕
    case Icon::Close:
        ucs4 = 0x2715; break; // ✕
    case Icon::Delete:
        ucs4 = 0x1F5D1; break; // 🗑
    case Icon::Edit:
        ucs4 = 0x270F; break; // ✏
    case Icon::Error:
        ucs4 = 0x2716; break; // ✖
    case Icon::Folder:
        ucs4 = 0x1F4C1; break; // 📁
    case Icon::Help:
        ucs4 = 0x003F; break; // ?
    case Icon::Home:
        ucs4 = 0x2302; break; // 🏠 / ⌂
    case Icon::Info:
        ucs4 = 0x2139; break; // ℹ
    case Icon::More:
        ucs4 = 0x22EE; break; // ⋮
    case Icon::Pause:
        ucs4 = 0x23F8; break; // ⏸
    case Icon::Play:
        ucs4 = 0x25B6; break; // ▶
    case Icon::Refresh:
        ucs4 = 0x21BB; break; // ↻
    case Icon::Save:
        ucs4 = 0x1F4BE; break; // 💾
    case Icon::Search:
        ucs4 = 0x1F50D; break; // 🔍
    case Icon::Settings:
        ucs4 = 0x2699; break; // ⚙
    case Icon::Stop:
        ucs4 = 0x23F9; break; // ⏹
    case Icon::Document:
        ucs4 = 0x1F4C4; break; // 📄
    case Icon::List:
        ucs4 = 0x2630; break; // ☰
    case Icon::Warning:
        ucs4 = 0x26A0; break; // ⚠️
    }
    return QString::fromUcs4(&ucs4, 1);
}

struct ParsedFluentIcon
{
    Icon glyph = Icon::Add;
    QString variant;
};

bool parseFluentIconName(const QString &name, ParsedFluentIcon *parsed)
{
    const QString prefix = QString::fromLatin1(fluentIconNamePrefix);
    if (!name.startsWith(prefix))
        return false;

    const int variantSeparator = name.indexOf(QLatin1Char(':'), prefix.size());
    const int numberLength =
            variantSeparator >= 0 ? variantSeparator - prefix.size() : name.size() - prefix.size();
    bool ok = false;
    const int value = name.mid(prefix.size(), numberLength).toInt(&ok);
    if (!ok || value < 0 || value >= fluentIconCount)
        return false;

    if (parsed) {
        parsed->glyph = static_cast<Icon>(value);
        parsed->variant = variantSeparator >= 0 ? name.mid(variantSeparator + 1) : QString();
    }
    return true;
}

class IconRuntime final
{
public:
    IconRuntime()
    {
        m_pixmaps.setMaxCost(MaxPixmapCacheCost);
        m_fonts.setMaxCost(MaxFontCacheCost);
        m_coloredIcons.setMaxCost(MaxColoredIconCacheCost);
    }

    void syncApplication()
    {
        auto *application = qobject_cast<QGuiApplication *>(QCoreApplication::instance());
        if (application == m_application)
            return;

        if (m_fontDatabaseConnection)
            QObject::disconnect(m_fontDatabaseConnection);
        m_application = application;
        m_fontDatabaseConnection = {};
        invalidate();
        if (application) {
            m_fontDatabaseConnection =
                    QObject::connect(application, &QGuiApplication::fontDatabaseChanged,
                                     application, [this] { invalidate(); });
        }
    }

    // GUI-thread-only: the QPointer, QString and QCache members below are
    // mutated here (and cleared by invalidate()). Callers must only reach
    // this runtime where canUseGuiCache() is true; worker-thread paint uses
    // offThreadFluentFont() and never touches the runtime.
    QString fluentFontFamily()
    {
        syncApplication();
        if (!m_fontFamilyResolved) {
            const QString fluent = QStringLiteral("Segoe Fluent Icons");
            const QString mdl2 = QStringLiteral("Segoe MDL2 Assets");
            const QStringList families = Private::fontFamilies();
            if (families.contains(fluent)) {
                m_fontFamily = fluent;
                m_hasSegoeFont = true;
            } else if (families.contains(mdl2)) {
                m_fontFamily = mdl2;
                m_hasSegoeFont = true;
            } else {
                m_fontFamily.clear(); // Empty string defaults to application default/system font
                m_hasSegoeFont = false;
            }
            m_fontFamilyResolved = true;
        }
        return m_fontFamily;
    }

    bool hasSegoeFont()
    {
        syncApplication();
        if (!m_fontFamilyResolved)
            fluentFontFamily();
        return m_hasSegoeFont;
    }

    QFont fluentFont(int pixelSize)
    {
        syncApplication();
        if (QFont *cached = m_fonts.object(pixelSize))
            return *cached;

        QFont font = m_fontFamily.isEmpty() ? QFont() : QFont(fluentFontFamily());
        font.setPixelSize(pixelSize);
        m_fonts.insert(pixelSize, new QFont(font));
        return *m_fonts.object(pixelSize);
    }

    // Test-only seams (see src/winui3icons_p.h): the icons unit test drives
    // these to prove worker-thread isolation and value semantics. Production
    // code must not use them.
    bool fontFamilyResolvedForTest() const { return m_fontFamilyResolved; }
    void invalidateForTest() { invalidate(); }

    QPixmap *findPixmap(const QString &key) { return m_pixmaps.object(key); }

    void insertPixmap(const QString &key, const QPixmap &pixmap)
    {
        const qint64 bytes = qint64(pixmap.width()) * pixmap.height() * 4;
        if (bytes <= 0 || bytes > qint64(MaxPixmapCacheCost) * 1024)
            return;
        const int cost = qMax(1, int((bytes + 1023) / 1024));
        m_pixmaps.insert(key, new QPixmap(pixmap), cost);
    }

    QIcon *findColoredIcon(const QString &key) { return m_coloredIcons.object(key); }

    void insertColoredIcon(const QString &key, const QIcon &icon)
    {
        m_coloredIcons.insert(key, new QIcon(icon));
    }

private:
    void invalidate()
    {
        m_fontFamily.clear();
        m_fontFamilyResolved = false;
        m_fonts.clear();
        m_pixmaps.clear();
        m_coloredIcons.clear();
    }

    QPointer<QGuiApplication> m_application;
    QMetaObject::Connection m_fontDatabaseConnection;
    QString m_fontFamily;
    bool m_fontFamilyResolved = false;
    bool m_hasSegoeFont = false;
    QCache<int, QFont> m_fonts;
    QCache<QString, QPixmap> m_pixmaps;
    QCache<QString, QIcon> m_coloredIcons;
};

IconRuntime &iconRuntime()
{
    static IconRuntime runtime;
    return runtime;
}

QString glyphString(Icon glyph, bool hasSegoeFont)
{
    const int index = static_cast<int>(glyph);
    if (index < 0 || index >= fluentIconCount)
        return {};

    if (hasSegoeFont) {
        static const std::array<QString, fluentIconCount> segoeGlyphs = [] {
            std::array<QString, fluentIconCount> result;
            for (int i = 0; i < fluentIconCount; ++i)
                result[i] = codePointString(static_cast<Icon>(i), true);
            return result;
        }();
        return segoeGlyphs[index];
    }

    static const std::array<QString, fluentIconCount> fallbackGlyphs = [] {
        std::array<QString, fluentIconCount> result;
        for (int i = 0; i < fluentIconCount; ++i)
            result[i] = codePointString(static_cast<Icon>(i), false);
        return result;
    }();
    return fallbackGlyphs[index];
}

bool canUseGuiCache()
{
    const auto *application = qobject_cast<const QGuiApplication *>(QCoreApplication::instance());
    return application && QThread::currentThread() == application->thread();
}

QString pixmapKey(const ParsedFluentIcon &parsed, const QSize &logicalSize,
                  const QSize &physicalSize, qreal devicePixelRatio, const QColor &foreground,
                  QIcon::Mode mode, QIcon::State state)
{
    return QStringLiteral("winui3-fluent-pixmap:%1:%2:%3:%4x%5:%6x%7:%8:%9:%10:%11")
            .arg(static_cast<int>(parsed.glyph))
            .arg(parsed.variant)
            .arg(QString::number(devicePixelRatio, 'g', 17))
            .arg(logicalSize.width())
            .arg(logicalSize.height())
            .arg(physicalSize.width())
            .arg(physicalSize.height())
            .arg(colorKey(foreground))
            .arg(static_cast<int>(mode))
            .arg(static_cast<int>(state));
}

QString coloredIconKey(Icon glyph, const QColor &color)
{
    return QStringLiteral("winui3-fluent-colored-icon:%1:%2")
            .arg(static_cast<int>(glyph))
            .arg(colorKey(color));
}

class FluentIconEngine final : public QIconEngine
{
public:
    explicit FluentIconEngine(Icon icon, const QColor &color = {})
        : m_icon(icon), m_color(color) { }

    QIconEngine *clone() const override { return new FluentIconEngine(m_icon, m_color); }

    QString key() const override { return QStringLiteral("WinUI3FluentIconEngine"); }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt 6's QIconEngine::iconName() is not const (Qt 5's is).
    QString iconName() override
    {
        return QString::fromLatin1(fluentIconNamePrefix)
                + QString::number(static_cast<int>(m_icon));
    }
#endif

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        QPixmap result(size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, result.rect(), mode, state);
        return result;
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State) override
    {
        painter->save();
        painter->setRenderHint(QPainter::TextAntialiasing);

        const int pixelSize = qMax(8, qMin(rect.width(), rect.height()));
        bool hasSegoe = false;
        if (canUseGuiCache()) {
            painter->setFont(iconRuntime().fluentFont(pixelSize));
            hasSegoe = iconRuntime().hasSegoeFont();
        } else {
            const QString fluent = QStringLiteral("Segoe Fluent Icons");
            const QString mdl2 = QStringLiteral("Segoe MDL2 Assets");
            const QStringList families = Private::fontFamilies();
            hasSegoe = families.contains(fluent) || families.contains(mdl2);
            QFont font = hasSegoe ? QFont(families.contains(fluent) ? fluent : mdl2) : QFont();
            font.setPixelSize(pixelSize);
            painter->setFont(font);
        }

        QColor color = m_color;
        if (!color.isValid()) {
            const QPalette palette = qApp ? qApp->palette() : QPalette();
            color = palette.color(mode == QIcon::Disabled ? QPalette::Disabled : QPalette::Active,
                                  QPalette::WindowText);
        }
        painter->setPen(color);
        painter->drawText(rect, Qt::AlignCenter, glyphString(m_icon, hasSegoe));
        painter->restore();
    }

    QSize actualSize(const QSize &size, QIcon::Mode, QIcon::State) override { return size; }

private:
    Icon m_icon;
    QColor m_color;
};

const QIcon &cachedIcon(Icon glyph)
{
    static const QIcon empty;
    static const std::array<QIcon, fluentIconCount> icons = [] {
        std::array<QIcon, fluentIconCount> result;
        for (int index = 0; index < fluentIconCount; ++index)
            result[index] = QIcon(new FluentIconEngine(static_cast<Icon>(index)));
        return result;
    }();
    const int index = static_cast<int>(glyph);
    return index >= 0 && index < fluentIconCount ? icons[index] : empty;
}

} // namespace

QIcon icon(Icon glyph)
{
    return cachedIcon(glyph);
}

namespace Private {

QFont fluentFontForTest(int pixelSize)
{
    // Fixed: by-value return (QFont is implicitly shared, cheap) so callers
    // never alias the QCache-owned slot that invalidate()/clear() or LRU
    // eviction can delete.
    return iconRuntime().fluentFont(pixelSize);
}

QFont offThreadFluentFontForTest(int pixelSize)
{
    // Fixed: mirrors the corrected worker-thread paint path — constructs
    // the font without touching the shared IconRuntime, so a worker-thread
    // resolution leaves iconFontCacheResolvedForTest() false.
    if (canUseGuiCache())
        return iconRuntime().fluentFont(pixelSize);
    QFont font(QStringLiteral("Segoe Fluent Icons"));
    font.setPixelSize(pixelSize);
    return font;
}

bool iconFontCacheResolvedForTest()
{
    return iconRuntime().fontFamilyResolvedForTest();
}

void invalidateIconCachesForTest()
{
    iconRuntime().invalidateForTest();
}

} // namespace Private

QIcon icon(Icon glyph, const QColor &color)
{
    if (!color.isValid())
        return icon(glyph);

    // QStyle paint paths are GUI-thread-only. Keep the fallback for callers
    // that use this public helper before a GUI application exists or from a
    // worker thread; those calls retain the exact historical engine path.
    if (!canUseGuiCache())
        return QIcon(new FluentIconEngine(glyph, color));

    IconRuntime &runtime = iconRuntime();
    runtime.syncApplication();
    const QString key = coloredIconKey(glyph, color);
    if (QIcon *cached = runtime.findColoredIcon(key))
        return *cached;

    // The cached value owns the same FluentIconEngine implementation that
    // the old per-call path created. Only the lifetime changes: rasterization
    // and QIconEngine::pixmap() behavior remain byte-for-byte identical.
    const QIcon colored(new FluentIconEngine(glyph, color));
    runtime.insertColoredIcon(key, colored);
    return colored;
}

bool isFluentIcon(const QIcon &icon)
{
    return parseFluentIconName(icon.name(), nullptr);
}

QPixmap iconPixmap(const QIcon &source, const QSize &size, qreal devicePixelRatio,
                   const QColor &foreground, QIcon::Mode mode, QIcon::State state)
{
    if (source.isNull() || size.isEmpty())
        return {};
    ParsedFluentIcon parsed;
    if (!parseFluentIconName(source.name(), &parsed) || !foreground.isValid()) {
        return Private::iconPixmap(source, size, devicePixelRatio, mode, state);
    }

    const bool neutralSource = source.cacheKey() == cachedIcon(parsed.glyph).cacheKey();
    // Keep the public icon name stable. A coloured QIcon has a distinct
    // cache identity, so include that identity only in the private pixmap
    // key and render its source directly to preserve its alpha. The neutral
    // source also carries the application WindowText alpha; include the
    // application palette identity so a palette change cannot reuse it.
    if (!neutralSource) {
        parsed.variant = QString::number(source.cacheKey(), 16);
    } else if (auto *application = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        parsed.variant = QString::number(application->palette().cacheKey(), 16);
    }

    const QSize physicalSize(qMax(1, qRound(size.width() * devicePixelRatio)),
                             qMax(1, qRound(size.height() * devicePixelRatio)));
    const QString key =
            pixmapKey(parsed, size, physicalSize, devicePixelRatio, foreground, mode, state);
    if (!canUseGuiCache())
        return Private::iconPixmap(source, size, devicePixelRatio, mode, state);

    IconRuntime &runtime = iconRuntime();
    runtime.syncApplication();
    if (QPixmap *cached = runtime.findPixmap(key))
        return *cached;

    // Keep the source alpha exactly as QIcon::pixmap() produces it. The
    // foreground and application-palette identities are part of the key, so
    // local-palette recolouring cannot reuse a stale mask.
    QPixmap pixmap = Private::iconPixmap(source, size, devicePixelRatio, mode, state);
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
