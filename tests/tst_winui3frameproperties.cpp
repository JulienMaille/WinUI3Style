// SPDX-License-Identifier: LGPL-2.1-or-later
#include "winui3frameproperties_p.h"

#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QThread>
#include <QtTest>

#include <type_traits>

using WinUI3::Private::FramePropertyRegistry;

static_assert(!std::is_copy_constructible<FramePropertyRegistry>::value);
static_assert(!std::is_copy_assignable<FramePropertyRegistry>::value);
static_assert(!std::is_move_constructible<FramePropertyRegistry>::value);
static_assert(!std::is_move_assignable<FramePropertyRegistry>::value);

class FramePropertyRegistryTest : public QObject
{
    Q_OBJECT

private slots:
    void storesValuesWithoutDynamicProperties();
    void convertsRealValuesWithFallback();
    void clearsIndividualAndObjectValues();
    void purgesDestroyedObjects();
    void boundsPropertiesPerObject();
    void offThreadPublicAccessIsRejected();
    void destroyedOnOwningWorkerThreadCleansUpOnGuiThread();
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

void FramePropertyRegistryTest::offThreadPublicAccessIsRejected()
{
#ifndef QT_NO_DEBUG
    // checkGuiThread() asserts in debug builds; the reject-without-touching
    // contract below only applies to release builds, where the guard returns
    // false instead of aborting.
    QSKIP("off-thread registry access asserts in debug builds.");
#else
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QObject object;
    registry.set(&object, "seed", 7);

    struct Probe
    {
        bool valueReadInvalid = false;
        bool realReadFallback = false;
    };
    Probe probe;
    QThread *thread = QThread::create([&object, &probe] {
        FramePropertyRegistry &registry = FramePropertyRegistry::instance();
        // Every one of these must be rejected without touching shared state.
        registry.set(&object, "seed", 999);
        probe.valueReadInvalid = !registry.value(&object, "seed").isValid();
        probe.realReadFallback = registry.real(&object, "seed", -1.0) == -1.0;
        registry.clear(&object, "seed");
        registry.clearObject(&object);
    });
    thread->start();
    QVERIFY(thread->wait(5000));
    delete thread;

    QVERIFY(probe.valueReadInvalid);
    QVERIFY(probe.realReadFallback);
    // The GUI-thread entry survived every off-thread attempt untouched.
    QCOMPARE(registry.value(&object, "seed").toInt(), 7);
    registry.clearObject(&object);
#endif
}

void FramePropertyRegistryTest::destroyedOnOwningWorkerThreadCleansUpOnGuiThread()
{
    FramePropertyRegistry &registry = FramePropertyRegistry::instance();
    QThread worker;
    worker.start();

    auto *object = new QObject;
    object->moveToThread(&worker);
    registry.set(object, "value", 1);
    QCOMPARE(registry.value(object, "value").toInt(), 1);
    QObject *address = object;

    // Destroy on the owning worker thread: destroyed() is delivered there,
    // so the registry must defer the shared-hash removal to the GUI thread
    // instead of mutating the hashes off-thread.
    QSignalSpy destroyedSpy(object, &QObject::destroyed);
    QMetaObject::invokeMethod(object, "deleteLater", Qt::QueuedConnection);
    QVERIFY(destroyedSpy.wait(5000));
    worker.quit();
    QVERIFY(worker.wait(5000));

    // value() only hashes the address; the deferred GUI-thread cleanup must
    // have removed the entry before that address can be reused.
    QTRY_VERIFY_WITH_TIMEOUT(!registry.value(address, "value").isValid(), 5000);

    // The registry stays coherent for later GUI-thread use.
    QObject probe;
    registry.set(&probe, "value", 2);
    QCOMPARE(registry.value(&probe, "value").toInt(), 2);
    registry.clearObject(&probe);
}

QTEST_MAIN(FramePropertyRegistryTest)
#include "tst_winui3frameproperties.moc"
