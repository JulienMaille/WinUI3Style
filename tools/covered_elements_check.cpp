// Phase 4 consistency gate: every element a renderer CLAIMS (from the
// `if (element != ...)` filter at the head of its draw* function, and from the
// per-element branches inside) must be listed in
// coveredPrimitive / coveredControl / coveredComplex. A covered element absent
// from that list makes the Q_ASSERT_X in Style::draw* unable to fire, so a
// covered control could silently reach QCommonStyle (METHODOLOGY section 1: no
// silent base-style fallback for covered controls).
//
// Single executable, no QApplication needed.

#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include <cstdio>

namespace {

QString readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly))
        return {};
    return QString::fromLocal8Bit(f.readAll());
}

// Elements the file claims, taken from the element filters and the per-element
// branches. `prefix` is the enum prefix ("PE_", "CE_", "CC_").
QSet<QString> claimedElements(const QString &source, const QString &prefix)
{
    QSet<QString> result;
    const QRegularExpression re(QStringLiteral("QStyle::(") + prefix + QStringLiteral("\\w+)"));
    auto it = re.globalMatch(source);
    while (it.hasNext())
        result.insert(it.next().captured(1));
    return result;
}

QSet<QString> coveredList(const QString &sources, const QString &functionName)
{
    const int start = sources.indexOf(QLatin1String("bool ") + functionName);
    if (start < 0) {
        QTextStream(stderr) << "MISSING function " << functionName << "\n";
        return {};
    }
    const int bodyStart = sources.indexOf(QLatin1Char('{'), start);
    int depth = 0;
    int bodyEnd = bodyStart;
    for (int i = bodyStart; i < sources.size(); ++i) {
        if (sources[i] == QLatin1Char('{'))
            ++depth;
        else if (sources[i] == QLatin1Char('}') && --depth == 0) {
            bodyEnd = i;
            break;
        }
    }
    QSet<QString> result;
    const QString body = sources.mid(bodyStart, bodyEnd - bodyStart);
    static const QRegularExpression caseRe(QStringLiteral("case\\s+QStyle::(\\w+)\\s*:"));
    auto it = caseRe.globalMatch(body);
    while (it.hasNext())
        result.insert(it.next().captured(1));
    return result;
}

// Deliberately PARTIAL elements: claimed by a renderer for specific widget
// conditions only, with falling through to QCommonStyle as the intended
// behavior for the remaining cases. They must NOT appear in the covered*
// lists (the assert would fire on a legitimate fallback). This set is the
// whole exception surface: adding an element here requires the same argument
// as removing it from a covered* list, and it is documented at
// coveredPrimitive in src/winui3style.cpp.
const QSet<QString> &partialElements()
{
    static const QSet<QString> partial = {
        // Generic QFrame keeps the base frame; the style owns popup windows,
        // item views and rich-text editors.
        QStringLiteral("PE_Frame"),
        // Owned only for winuiSettingsCard widgets.
        QStringLiteral("CE_ShapedFrame"),
        // Never painted; they only receive the shared button-family backdrop
        // pre-erase, so the base style draws the label.
        QStringLiteral("CE_CheckBoxLabel"),
        QStringLiteral("CE_RadioButtonLabel"),
    };
    return partial;
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc < 3) {
        QTextStream(stderr) << "usage: covered-elements-check <style.cpp> <renderer...>\n";
        return 2;
    }
    const QString styleSource = readFile(QString::fromLocal8Bit(argv[1]));

    QStringList rendererSources;
    for (int i = 2; i < argc; ++i)
        rendererSources << readFile(QString::fromLocal8Bit(argv[i]));
    // The PE_/CE_ elements are claimed by the renderers AND dispatched by the
    // facade (Style::drawPrimitive routes PE_Widget and the CE_ button family),
    // so both sources count as claims.
    const QString allRenderers = rendererSources.join(QLatin1Char('\n')) + QLatin1Char('\n')
            + styleSource;

    struct Entry { const char *prefix; const char *covered; };
    const Entry entries[] = { { "PE_", "coveredPrimitive" },
                              { "CE_", "coveredControl" },
                              { "CC_", "coveredComplex" } };

    // The covered* declarations live in winui3style.cpp (PE_/CE_) and
    // winui3complex_p.cpp (CC_); search every source.
    const QString allSources = styleSource + QLatin1Char('\n') + allRenderers;

    int failures = 0;
    for (const Entry &entry : entries) {
        const QString prefix = QLatin1String(entry.prefix);
        const QSet<QString> covered = coveredList(allSources, QLatin1String(entry.covered));
        const QSet<QString> claimed = claimedElements(allRenderers, prefix);
        for (const QString &element : claimed) {
            if (covered.contains(element) || partialElements().contains(element))
                continue;
            QTextStream(stdout) << "UNCOVERED " << element << " (claimed by a renderer, absent from "
                                << entry.covered << " and not a known partial element)\n";
            ++failures;
        }
        for (const QString &element : covered) {
            if (!claimed.contains(element))
                QTextStream(stdout) << "note: " << entry.covered << " lists " << element
                                    << " which no renderer claims\n";
        }
    }

    QTextStream(stdout) << (failures ? "FAIL " : "PASS ") << failures
                        << " uncovered element(s)\n";
    return failures ? 1 : 0;
}
