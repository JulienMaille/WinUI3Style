#include <winui3style/winui3icons.h>

#include <QFont>
#include <QFontDatabase>
#include <QIconEngine>
#include <QApplication>
#include <QPainter>
#include <QPalette>

namespace WinUI3 {
namespace {

constexpr auto fluentIconNamePrefix = "winui3-fluent-icon:";

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
        painter->save();
        painter->setRenderHint(QPainter::TextAntialiasing);

        QFont font(QStringLiteral("Segoe Fluent Icons"));
        if (!QFontDatabase::families().contains(font.family()))
            font.setFamily(QStringLiteral("Segoe MDL2 Assets"));
        font.setPixelSize(qMax(8, qMin(rect.width(), rect.height())));
        painter->setFont(font);

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
        painter->setPen(color);
        painter->drawText(rect, Qt::AlignCenter, QString(codePoint(m_icon)));
        painter->restore();
    }

    QSize actualSize(const QSize &size, QIcon::Mode, QIcon::State) override
    {
        return size;
    }

private:
    Icon m_icon;
    QColor m_color;
};

} // namespace

QIcon icon(Icon glyph)
{
    return QIcon(new FluentIconEngine(glyph));
}

QIcon icon(Icon glyph, const QColor &color)
{
    return QIcon(new FluentIconEngine(glyph, color));
}

bool isFluentIcon(const QIcon &icon)
{
    return icon.name().startsWith(QString::fromLatin1(fluentIconNamePrefix));
}

QPixmap iconPixmap(const QIcon &source, const QSize &size,
                   qreal devicePixelRatio, const QColor &foreground,
                   QIcon::Mode mode, QIcon::State state)
{
    if (source.isNull() || size.isEmpty())
        return {};
    QPixmap pixmap = source.pixmap(size, devicePixelRatio, mode, state);
    if (!isFluentIcon(source) || !foreground.isValid() || pixmap.isNull())
        return pixmap;

    QPainter painter(&pixmap);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(pixmap.rect(), foreground);
    return pixmap;
}

} // namespace WinUI3
