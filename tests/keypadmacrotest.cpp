#include <QtTest>

#include <QBuffer>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>

#include "keymap.h"
#include "keypadmacro.h"

namespace {
KeypadMacroEvent buttonEvent(quint64 offset, int id, bool pressed)
{
    KeypadMacroEvent event;
    event.type = KeypadMacroEventType::Button;
    event.offsetMs = offset;
    event.buttonId = id;
    event.pressed = pressed;
    return event;
}

KeypadMacroEvent touchEvent(quint64 offset, double x, double y, bool contact, bool down)
{
    KeypadMacroEvent event;
    event.type = KeypadMacroEventType::Touchpad;
    event.offsetMs = offset;
    event.x = x;
    event.y = y;
    event.contact = contact;
    event.down = down;
    return event;
}

QVector<KeypadMacroEvent> shortBalancedEvents()
{
    return {
        buttonEvent(0, 5, true),
        touchEvent(10, 0.25, 0.75, true, true),
        buttonEvent(20, 5, false),
        touchEvent(20, 0.25, 0.75, false, false)
    };
}
}

class KeypadMacroTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void recordsOrderedStatesAndBalancesStop();
    void modelMaintainsNamesSignalsAndIDs();
    void scriptParsesFormatsAndEditsModel();
    void scriptRejectsInvalidPrograms();
    void serializationAndSettingsRoundTrip();
    void corruptStreamsAreRejectedAtomically();
    void playbackUsesAbsoluteTimingAndStableOrder();
    void cancellationReleasesAndInvalidatesDeadline();
};

void KeypadMacroTest::initTestCase()
{
    qRegisterMetaTypeStreamOperators<KeypadMacroModel>();
    qRegisterMetaType<KeypadMacroModel>();
}

void KeypadMacroTest::recordsOrderedStatesAndBalancesStop()
{
    KeypadMacroController controller;
    controller.startRecording();
    controller.captureButton(5, true);
    controller.captureButton(5, true); // Duplicate effective state is ignored.
    controller.captureTouchpad(0.2, 0.3, true, false);
    controller.captureTouchpad(0.4, 0.5, true, true);
    QTest::qWait(3);
    controller.captureButton(5, false);
    controller.captureButton(7, true);

    QVector<KeypadMacroEvent> events;
    QVERIFY(controller.stopRecording(&events));
    QCOMPARE(events.size(), 7);
    QVERIFY(events.at(0).type == KeypadMacroEventType::Button);
    QCOMPARE(events.at(0).buttonId, 5);
    QVERIFY(events.at(0).pressed);
    QCOMPARE(events.at(0).offsetMs, quint64(0));

    QVERIFY(events.at(1).type == KeypadMacroEventType::Touchpad);
    QCOMPARE(events.at(1).x, 0.2);
    QCOMPARE(events.at(1).y, 0.3);
    QVERIFY(events.at(1).contact);
    QVERIFY(!events.at(1).down);
    QVERIFY(events.at(2).type == KeypadMacroEventType::Touchpad);
    QCOMPARE(events.at(2).x, 0.4);
    QVERIFY(events.at(2).down);

    QCOMPARE(events.at(3).buttonId, 5);
    QVERIFY(!events.at(3).pressed);
    QCOMPARE(events.at(4).buttonId, 7);
    QVERIFY(events.at(4).pressed);
    QCOMPARE(events.at(5).buttonId, 7);
    QVERIFY(!events.at(5).pressed);
    QVERIFY(events.at(6).type == KeypadMacroEventType::Touchpad);
    QVERIFY(!events.at(6).contact);
    QVERIFY(!events.at(6).down);
    QCOMPARE(events.at(5).offsetMs, events.at(6).offsetMs);

    for(int i = 1; i < events.size(); ++i)
        QVERIFY(events.at(i - 1).offsetMs <= events.at(i).offsetMs);

    controller.startRecording();
    events.append(buttonEvent(0, 1, true));
    QVERIFY(!controller.stopRecording(&events));
    QVERIFY(events.isEmpty());
}

void KeypadMacroTest::modelMaintainsNamesSignalsAndIDs()
{
    KeypadMacroModel model;
    QCOMPARE(model.rowCount(), 0);
    QSignalSpy changed(&model, &KeypadMacroModel::anythingChanged);
    QSignalSpy rowsInserted(&model, &QAbstractItemModel::rowsInserted);
    QSignalSpy rowsRemoved(&model, &QAbstractItemModel::rowsRemoved);
    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);

    const QVector<KeypadMacroEvent> events = shortBalancedEvents();
    QCOMPARE(model.addMacro(QStringLiteral("  First  "), events), 0);
    QCOMPARE(model.addMacro(QStringLiteral("Second"), events), 1);
    QCOMPARE(rowsInserted.count(), 2);
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::NameRole).toString(), QStringLiteral("First"));
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::IDRole).toUInt(), quint32(0));
    QCOMPARE(model.getDataRow(1, KeypadMacroModel::IDRole).toUInt(), quint32(1));
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::EventCountRole).toInt(), events.size());
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::DurationRole).toULongLong(), quint64(20));

    QVERIFY(!model.isNameAvailable(QString(), -1));
    QVERIFY(!model.isNameAvailable(QStringLiteral(" first "), -1));
    QVERIFY(model.isNameAvailable(QStringLiteral("first"), 0));
    QCOMPARE(model.addMacro(QStringLiteral("FIRST"), events), -1);
    QVERIFY(!model.rename(1, QStringLiteral("fIrSt")));
    model.setMutationLocked(true);
    QVERIFY(!model.rename(1, QStringLiteral("Locked")));
    QVERIFY(!model.remove(1));
    model.setMutationLocked(false);
    QVERIFY(model.rename(1, QStringLiteral(" Renamed ")));
    QCOMPARE(dataChanged.count(), 1);
    QCOMPARE(model.getDataRow(1, KeypadMacroModel::NameRole).toString(), QStringLiteral("Renamed"));

    QVERIFY(model.remove(0));
    QVERIFY(model.remove(0));
    QCOMPARE(rowsRemoved.count(), 2);
    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.addMacro(QStringLiteral("Third"), events), 0);
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::IDRole).toUInt(), quint32(2));
    QCOMPARE(changed.count(), 6);
}

void KeypadMacroTest::scriptParsesFormatsAndEditsModel()
{
    const QString code = QStringLiteral(
                "# Concurrent keys and touch input\n"
                "tap Home 25ms\n"
                "wait 25ms\n"
                "press Ctrl\n"
                "press A\n"
                "wait 10ms\n"
                "release A\n"
                "release Ctrl\n"
                "touch 0.25 0.75 contact\n"
                "wait 5ms\n"
                "touch 0.5 0.5 down\n"
                "wait 10ms\n"
                "touch release");
    QVector<KeypadMacroEvent> events;
    QString error;
    QVERIFY2(KeypadMacroScript::parse(code, &events, &error), qPrintable(error));
    QVERIFY(error.isEmpty());
    QCOMPARE(events.size(), 9);
    QCOMPARE(events.at(0).buttonId, keymap::on);
    QVERIFY(events.at(0).pressed);
    QCOMPARE(events.at(0).offsetMs, quint64(0));
    QCOMPARE(events.at(1).buttonId, keymap::on);
    QVERIFY(!events.at(1).pressed);
    QCOMPARE(events.at(1).offsetMs, quint64(25));
    QCOMPARE(events.at(2).buttonId, keymap::ctrl);
    QCOMPARE(events.at(2).offsetMs, quint64(50));
    QCOMPARE(events.at(3).buttonId, keymap::aa);
    QCOMPARE(events.at(3).offsetMs, quint64(50));
    QCOMPARE(events.at(4).buttonId, keymap::aa);
    QCOMPARE(events.at(4).offsetMs, quint64(60));
    QCOMPARE(events.at(5).buttonId, keymap::ctrl);
    QCOMPARE(events.at(5).offsetMs, quint64(60));
    QVERIFY(events.at(6).type == KeypadMacroEventType::Touchpad);
    QCOMPARE(events.at(6).x, 0.25);
    QCOMPARE(events.at(6).y, 0.75);
    QVERIFY(events.at(6).contact);
    QVERIFY(!events.at(6).down);
    QCOMPARE(events.at(7).offsetMs, quint64(65));
    QVERIFY(events.at(7).down);
    QCOMPARE(events.at(8).offsetMs, quint64(75));
    QVERIFY(!events.at(8).contact);
    QVERIFY(!events.at(8).down);

    const QString formatted = KeypadMacroScript::format(events);
    QVERIFY(formatted.contains(QStringLiteral("press Home")));
    QVERIFY(formatted.contains(QStringLiteral("wait 25ms")));
    QVERIFY(formatted.contains(QStringLiteral("touch 0.5 0.5 release")));

    QVector<KeypadMacroEvent> roundTripped;
    QVERIFY2(KeypadMacroScript::parse(formatted, &roundTripped, &error), qPrintable(error));
    QCOMPARE(roundTripped.size(), events.size());
    for(int index = 0; index < events.size(); ++index)
    {
        const KeypadMacroEvent &actual = roundTripped.at(index);
        const KeypadMacroEvent &expected = events.at(index);
        QVERIFY(actual.type == expected.type);
        QCOMPARE(actual.offsetMs, expected.offsetMs);
        QCOMPARE(actual.buttonId, expected.buttonId);
        QCOMPARE(actual.pressed, expected.pressed);
        QCOMPARE(actual.x, expected.x);
        QCOMPARE(actual.y, expected.y);
        QCOMPARE(actual.contact, expected.contact);
        QCOMPARE(actual.down, expected.down);
    }

    KeypadMacroModel model;
    QCOMPARE(model.addMacro(QStringLiteral("Editable"), shortBalancedEvents()), 0);
    const quint32 stableID = model.getDataRow(0, KeypadMacroModel::IDRole).toUInt();
    QSignalSpy changed(&model, &KeypadMacroModel::anythingChanged);
    QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);
    QVERIFY(model.replaceMacroEvents(0, events));
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::IDRole).toUInt(), stableID);
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::EventCountRole).toInt(), events.size());
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::DurationRole).toULongLong(), quint64(75));
    QCOMPARE(changed.count(), 1);
    QCOMPARE(dataChanged.count(), 1);
    QVERIFY(model.updateMacro(0, QStringLiteral("Edited"), shortBalancedEvents()));
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::NameRole).toString(), QStringLiteral("Edited"));
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::IDRole).toUInt(), stableID);
    QCOMPARE(model.getDataRow(0, KeypadMacroModel::EventCountRole).toInt(), shortBalancedEvents().size());
    QCOMPARE(changed.count(), 2);
    QCOMPARE(dataChanged.count(), 2);
    model.setMutationLocked(true);
    QVERIFY(!model.replaceMacroEvents(0, shortBalancedEvents()));
    QVERIFY(!model.updateMacro(0, QStringLiteral("Locked"), events));
}

void KeypadMacroTest::scriptRejectsInvalidPrograms()
{
    const QStringList invalidPrograms {
        QString(),
        QStringLiteral("wait 10ms\ntap 1"),
        QStringLiteral("press A"),
        QStringLiteral("release A"),
        QStringLiteral("press A\npress A\nrelease A"),
        QStringLiteral("tap UnknownKey"),
        QStringLiteral("touch 2 0 down\ntouch release"),
        QStringLiteral("touch 0.5 0.5 down"),
        QStringLiteral("tap 1\nwait 10ms"),
        QStringLiteral("launch 1")
    };

    for(const QString &code : invalidPrograms)
    {
        QVector<KeypadMacroEvent> unchanged = shortBalancedEvents();
        QString error;
        QVERIFY2(!KeypadMacroScript::parse(code, &unchanged, &error), qPrintable(code));
        QVERIFY2(!error.isEmpty(), qPrintable(code));
        QCOMPARE(unchanged.size(), shortBalancedEvents().size());
    }
}

void KeypadMacroTest::serializationAndSettingsRoundTrip()
{
    const QVector<KeypadMacroEvent> events = shortBalancedEvents();
    KeypadMacroEvent event = events.at(1);
    KeypadMacro macro{42, QStringLiteral("Round Trip"), events};
    KeypadMacroModel model;
    QCOMPARE(model.addMacro(QStringLiteral("One"), events), 0);
    QCOMPARE(model.addMacro(QStringLiteral("Two"), events), 1);

    QByteArray bytes;
    QDataStream out(&bytes, QIODevice::WriteOnly);
    out << event << macro << model;
    QCOMPARE(out.status(), QDataStream::Ok);

    KeypadMacroEvent readEvent;
    KeypadMacro readMacro;
    KeypadMacroModel readModel;
    QDataStream in(&bytes, QIODevice::ReadOnly);
    in >> readEvent >> readMacro >> readModel;
    QCOMPARE(in.status(), QDataStream::Ok);
    QVERIFY(readEvent.type == event.type);
    QCOMPARE(readEvent.offsetMs, event.offsetMs);
    QCOMPARE(readEvent.x, event.x);
    QCOMPARE(readEvent.y, event.y);
    QCOMPARE(readEvent.contact, event.contact);
    QCOMPARE(readEvent.down, event.down);
    QCOMPARE(readMacro.id, macro.id);
    QCOMPARE(readMacro.name, macro.name);
    QCOMPARE(readMacro.events.size(), macro.events.size());
    QCOMPARE(readModel.rowCount(), 2);
    QCOMPARE(readModel.getDataRow(1, KeypadMacroModel::NameRole).toString(), QStringLiteral("Two"));
    QCOMPARE(readModel.addMacro(QStringLiteral("Three"), events), 2);
    QCOMPARE(readModel.getDataRow(2, KeypadMacroModel::IDRole).toUInt(), quint32(2));

    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("macros.ini"));
    {
        QSettings settings(path, QSettings::IniFormat);
        settings.setValue(QStringLiteral("keypadMacros"), QVariant::fromValue(model));
        settings.sync();
        QCOMPARE(settings.status(), QSettings::NoError);
    }
    {
        QSettings settings(path, QSettings::IniFormat);
        KeypadMacroModel restored = settings.value(QStringLiteral("keypadMacros")).value<KeypadMacroModel>();
        QCOMPARE(restored.rowCount(), 2);
        QCOMPARE(restored.getDataRow(0, KeypadMacroModel::NameRole).toString(), QStringLiteral("One"));
        QCOMPARE(restored.addMacro(QStringLiteral("After Restart"), events), 2);
        QCOMPARE(restored.getDataRow(2, KeypadMacroModel::IDRole).toUInt(), quint32(2));
    }
}

void KeypadMacroTest::corruptStreamsAreRejectedAtomically()
{
    KeypadMacroEvent unchanged = buttonEvent(9, 3, true);

    QByteArray unknownVersionBytes;
    QDataStream unknownVersionOut(&unknownVersionBytes, QIODevice::WriteOnly);
    unknownVersionOut << quint32(99) << quint8(1) << quint64(0) << 1 << true;
    QDataStream unknownVersionIn(&unknownVersionBytes, QIODevice::ReadOnly);
    unknownVersionIn >> unchanged;
    QCOMPARE(unknownVersionIn.status(), QDataStream::ReadCorruptData);
    QCOMPARE(unchanged.offsetMs, quint64(9));
    QCOMPARE(unchanged.buttonId, 3);

    QByteArray unknownTypeBytes;
    QDataStream unknownTypeOut(&unknownTypeBytes, QIODevice::WriteOnly);
    unknownTypeOut << quint32(1) << quint8(99) << quint64(0);
    QDataStream unknownTypeIn(&unknownTypeBytes, QIODevice::ReadOnly);
    unknownTypeIn >> unchanged;
    QCOMPARE(unknownTypeIn.status(), QDataStream::ReadCorruptData);
    QCOMPARE(unchanged.offsetMs, quint64(9));

    const QVector<KeypadMacroEvent> unbalanced = {buttonEvent(0, 2, true)};
    QByteArray unbalancedBytes;
    QDataStream unbalancedOut(&unbalancedBytes, QIODevice::WriteOnly);
    unbalancedOut << quint32(1) << quint32(4) << QStringLiteral("Broken") << unbalanced;
    KeypadMacro unchangedMacro{7, QStringLiteral("Keep"), shortBalancedEvents()};
    QDataStream unbalancedIn(&unbalancedBytes, QIODevice::ReadOnly);
    unbalancedIn >> unchangedMacro;
    QCOMPARE(unbalancedIn.status(), QDataStream::ReadCorruptData);
    QCOMPARE(unchangedMacro.id, quint32(7));
    QCOMPARE(unchangedMacro.name, QStringLiteral("Keep"));

    KeypadMacroModel unchangedModel;
    QCOMPARE(unchangedModel.addMacro(QStringLiteral("Keep"), shortBalancedEvents()), 0);
    QByteArray modelBytes;
    QDataStream modelOut(&modelBytes, QIODevice::WriteOnly);
    modelOut << quint32(99);
    QDataStream modelIn(&modelBytes, QIODevice::ReadOnly);
    modelIn >> unchangedModel;
    QCOMPARE(modelIn.status(), QDataStream::ReadCorruptData);
    QCOMPARE(unchangedModel.rowCount(), 1);
    QCOMPARE(unchangedModel.getDataRow(0, KeypadMacroModel::NameRole).toString(), QStringLiteral("Keep"));
}

void KeypadMacroTest::playbackUsesAbsoluteTimingAndStableOrder()
{
    KeypadMacroController controller;
    QVector<QString> order;
    QVector<qint64> times;
    QElapsedTimer elapsed;
    connect(&controller, &KeypadMacroController::playbackButtonState,
            this, [&](int id, bool pressed) {
        order.append(QStringLiteral("b%1:%2").arg(id).arg(pressed));
        times.append(elapsed.elapsed());
    });
    connect(&controller, &KeypadMacroController::playbackTouchpadState,
            this, [&](double, double, bool contact, bool down) {
        order.append(QStringLiteral("t:%1:%2").arg(contact).arg(down));
        times.append(elapsed.elapsed());
    });
    QSignalSpy finished(&controller, &KeypadMacroController::playbackFinished);

    const QVector<KeypadMacroEvent> events = {
        buttonEvent(0, 1, true),
        touchEvent(50, 0.3, 0.7, true, true),
        buttonEvent(100, 1, false),
        touchEvent(150, 0.3, 0.7, false, false)
    };
    elapsed.start();
    controller.startPlayback(events);
    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 700);

    QCOMPARE(order, QVector<QString>({QStringLiteral("b1:1"), QStringLiteral("t:1:1"),
                                      QStringLiteral("b1:0"), QStringLiteral("t:0:0")}));
    QCOMPARE(times.size(), 4);
    QVERIFY(times.at(0) <= 80);
    QVERIFY(times.at(1) >= 20 && times.at(1) <= 140);
    QVERIFY(times.at(2) >= 60 && times.at(2) <= 190);
    QVERIFY(times.at(3) >= 100 && times.at(3) <= 260);
}

void KeypadMacroTest::cancellationReleasesAndInvalidatesDeadline()
{
    KeypadMacroController controller;
    QVector<QString> order;
    connect(&controller, &KeypadMacroController::playbackButtonState,
            this, [&](int id, bool pressed) {
        order.append(QStringLiteral("b%1:%2").arg(id).arg(pressed));
    });
    connect(&controller, &KeypadMacroController::playbackTouchpadState,
            this, [&](double, double, bool contact, bool down) {
        order.append(QStringLiteral("t:%1:%2").arg(contact).arg(down));
    });
    QSignalSpy finished(&controller, &KeypadMacroController::playbackFinished);

    const QVector<KeypadMacroEvent> events = {
        buttonEvent(0, 8, true),
        touchEvent(0, 0.4, 0.6, true, true),
        buttonEvent(500, 8, false),
        touchEvent(500, 0.4, 0.6, false, false)
    };
    controller.startPlayback(events);
    QTRY_COMPARE_WITH_TIMEOUT(order.size(), 2, 200);
    QElapsedTimer cancellationElapsed;
    cancellationElapsed.start();
    controller.cancelPlayback();
    QVERIFY(cancellationElapsed.elapsed() < 100);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(order, QVector<QString>({QStringLiteral("b8:1"), QStringLiteral("t:1:1"),
                                      QStringLiteral("b8:0"), QStringLiteral("t:0:0")}));

    QTest::qWait(550);
    QCOMPARE(order.size(), 4);
    QCOMPARE(finished.count(), 1);
}

QTEST_GUILESS_MAIN(KeypadMacroTest)
#include "keypadmacrotest.moc"
