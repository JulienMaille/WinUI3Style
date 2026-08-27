#include "winui3frameproperties_p.h"

#include <QCoreApplication>
#include <QObject>
#include <QtTest>

using WinUI3::Private::FramePropertyRegistry;

class FramePropertyRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void storesValuesWithoutDynamicProperties();
    void convertsRealValuesWithFallback();
    void clearsIndividualAndObjectValues();
    void purgesDestroyedObjects();
    void boundsPropertiesPerObject();
};

void FramePropertyRegistryTest::storesValuesWithoutDynamicProperties()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QObject object;

    registry.set(&object, "frame-value", 42);

    QCOMPARE(registry.value(&object, "frame-value").toInt(), 42);
    QVERIFY(!object.property("frame-value").isValid());
}

void FramePropertyRegistryTest::convertsRealValuesWithFallback()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QObject object;

    registry.set(&object, "real-value", 2.5);
    QCOMPARE(registry.real(&object, "real-value", -1.0), 2.5);
    QCOMPARE(registry.real(&object, "missing-value", -1.0), -1.0);

    registry.set(&object, "not-real", QStringLiteral("not a number"));
    QCOMPARE(registry.real(&object, "not-real", 7.0), 7.0);
}

void FramePropertyRegistryTest::clearsIndividualAndObjectValues()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QObject object;

    registry.set(&object, "one", 1);
    registry.set(&object, "two", 2);
    registry.clear(&object, "one");
    QVERIFY(!registry.value(&object, "one").isValid());
    QCOMPARE(registry.value(&object, "two").toInt(), 2);

    registry.clearObject(&object);
    QVERIFY(!registry.value(&object, "two").isValid());

    // An invalid QVariant has the same removal behavior as clear().
    registry.set(&object, "two", 2);
    registry.set(&object, "two", QVariant{});
    QVERIFY(!registry.value(&object, "two").isValid());
}

void FramePropertyRegistryTest::purgesDestroyedObjects()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    auto *object = new QObject;
    registry.set(object, "value", 1);
    QObject *destroyedObject = object;

    delete object;

    // value() only hashes the address; using the saved address here verifies
    // that destroyed() removed the old state before that address can be reused.
    QVERIFY(!registry.value(destroyedObject, "value").isValid());
}

void FramePropertyRegistryTest::boundsPropertiesPerObject()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QObject object;

    for (int i = 0; i < 80; ++i)
        registry.set(&object, QByteArrayLiteral("property-") + QByteArray::number(i), i);

    // The private registry's fixed per-object limit is 64.  Recent values
    // remain available, while the oldest values are evicted.
    QVERIFY(!registry.value(&object, "property-0").isValid());
    QVERIFY(!registry.value(&object, "property-15").isValid());
    QCOMPARE(registry.value(&object, "property-16").toInt(), 16);
    QCOMPARE(registry.value(&object, "property-79").toInt(), 79);
}

QTEST_MAIN(FramePropertyRegistryTest)
#include "tst_winui3frameproperties.moc"
