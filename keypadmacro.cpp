#include "keypadmacro.h"
#include "keymap.h"

#include <QDataStream>
#include <QDebug>
#include <QCoreApplication>
#include <QHash>
#include <QRegularExpression>

#include <algorithm>
#include <cmath>
#include <climits>

namespace {
constexpr quint32 SerializationVersion = 1;
constexpr int KeypadButtonCount = 88;

bool isValidButtonId(int buttonId)
{
    return buttonId >= 0 && buttonId < KeypadButtonCount;
}

bool isValidCoordinate(double value)
{
    return std::isfinite(value) && value >= 0.0 && value <= 1.0;
}

bool validateEvents(const QVector<KeypadMacroEvent> &events)
{
    if(events.isEmpty() || events.constFirst().offsetMs != 0)
        return false;

    QSet<int> pressedButtons;
    double touchX = 0.0;
    double touchY = 0.0;
    bool touchContact = false;
    bool touchDown = false;
    quint64 previousOffset = 0;

    for(int i = 0; i < events.size(); ++i)
    {
        const KeypadMacroEvent &event = events.at(i);
        if(event.offsetMs < previousOffset)
            return false;
        previousOffset = event.offsetMs;

        switch(event.type)
        {
        case KeypadMacroEventType::Button:
            if(!isValidButtonId(event.buttonId) ||
               pressedButtons.contains(event.buttonId) == event.pressed)
                return false;
            if(event.pressed)
                pressedButtons.insert(event.buttonId);
            else
                pressedButtons.remove(event.buttonId);
            break;
        case KeypadMacroEventType::Touchpad:
            if(!isValidCoordinate(event.x) || !isValidCoordinate(event.y) ||
               (touchX == event.x && touchY == event.y &&
                touchContact == event.contact && touchDown == event.down))
                return false;
            touchX = event.x;
            touchY = event.y;
            touchContact = event.contact;
            touchDown = event.down;
            break;
        default:
            return false;
        }
    }

    return pressedButtons.isEmpty() && !touchContact && !touchDown;
}

const QHash<QString, int> &scriptKeyNames()
{
    static const QHash<QString, int> names {
        {QStringLiteral("return"), keymap::ret},
        {QStringLiteral("ret"), keymap::ret},
        {QStringLiteral("enter"), keymap::enter},
        {QStringLiteral("neg"), keymap::neg},
        {QStringLiteral("space"), keymap::space},
        {QStringLiteral("a"), keymap::aa},
        {QStringLiteral("b"), keymap::ab},
        {QStringLiteral("c"), keymap::ac},
        {QStringLiteral("d"), keymap::ad},
        {QStringLiteral("e"), keymap::ae},
        {QStringLiteral("f"), keymap::af},
        {QStringLiteral("g"), keymap::ag},
        {QStringLiteral("h"), keymap::ah},
        {QStringLiteral("i"), keymap::ai},
        {QStringLiteral("j"), keymap::aj},
        {QStringLiteral("k"), keymap::ak},
        {QStringLiteral("l"), keymap::al},
        {QStringLiteral("m"), keymap::am},
        {QStringLiteral("n"), keymap::an},
        {QStringLiteral("o"), keymap::ao},
        {QStringLiteral("p"), keymap::ap},
        {QStringLiteral("q"), keymap::aq},
        {QStringLiteral("r"), keymap::ar},
        {QStringLiteral("s"), keymap::as},
        {QStringLiteral("t"), keymap::at},
        {QStringLiteral("u"), keymap::au},
        {QStringLiteral("v"), keymap::av},
        {QStringLiteral("w"), keymap::aw},
        {QStringLiteral("x"), keymap::ax},
        {QStringLiteral("y"), keymap::ay},
        {QStringLiteral("z"), keymap::az},
        {QStringLiteral("0"), keymap::n0},
        {QStringLiteral("1"), keymap::n1},
        {QStringLiteral("2"), keymap::n2},
        {QStringLiteral("3"), keymap::n3},
        {QStringLiteral("4"), keymap::n4},
        {QStringLiteral("5"), keymap::n5},
        {QStringLiteral("6"), keymap::n6},
        {QStringLiteral("7"), keymap::n7},
        {QStringLiteral("8"), keymap::n8},
        {QStringLiteral("9"), keymap::n9},
        {QStringLiteral("punct"), keymap::punct},
        {QStringLiteral("home"), keymap::on},
        {QStringLiteral("on"), keymap::on},
        {QStringLiteral("pi"), keymap::pi},
        {QStringLiteral("trig"), keymap::trig},
        {QStringLiteral("pow10"), keymap::pow10},
        {QStringLiteral("ee"), keymap::ee},
        {QStringLiteral("square"), keymap::squ},
        {QStringLiteral("squ"), keymap::squ},
        {QStringLiteral("divide"), keymap::div},
        {QStringLiteral("div"), keymap::div},
        {QStringLiteral("/"), keymap::div},
        {QStringLiteral("exp"), keymap::exp},
        {QStringLiteral("equals"), keymap::equ},
        {QStringLiteral("equ"), keymap::equ},
        {QStringLiteral("="), keymap::equ},
        {QStringLiteral("multiply"), keymap::mult},
        {QStringLiteral("mult"), keymap::mult},
        {QStringLiteral("*"), keymap::mult},
        {QStringLiteral("power"), keymap::pow},
        {QStringLiteral("pow"), keymap::pow},
        {QStringLiteral("^"), keymap::pow},
        {QStringLiteral("var"), keymap::var},
        {QStringLiteral("minus"), keymap::minus},
        {QStringLiteral("-"), keymap::minus},
        {QStringLiteral("rightparen"), keymap::pright},
        {QStringLiteral(")"), keymap::pright},
        {QStringLiteral("dot"), keymap::dot},
        {QStringLiteral("."), keymap::dot},
        {QStringLiteral("leftparen"), keymap::pleft},
        {QStringLiteral("("), keymap::pleft},
        {QStringLiteral("catalog"), keymap::cat},
        {QStringLiteral("cat"), keymap::cat},
        {QStringLiteral("matrix"), keymap::metrix},
        {QStringLiteral("delete"), keymap::del},
        {QStringLiteral("del"), keymap::del},
        {QStringLiteral("touchpad"), keymap::pad},
        {QStringLiteral("pad"), keymap::pad},
        {QStringLiteral("flag"), keymap::flag},
        {QStringLiteral("plus"), keymap::plus},
        {QStringLiteral("+"), keymap::plus},
        {QStringLiteral("doc"), keymap::doc},
        {QStringLiteral("menu"), keymap::menu},
        {QStringLiteral("escape"), keymap::esc},
        {QStringLiteral("esc"), keymap::esc},
        {QStringLiteral("tab"), keymap::tab},
        {QStringLiteral("shift"), keymap::shift},
        {QStringLiteral("ctrl"), keymap::ctrl},
        {QStringLiteral("control"), keymap::ctrl},
        {QStringLiteral("comma"), keymap::comma},
        {QStringLiteral(","), keymap::comma}
    };
    return names;
}

bool scriptButtonId(const QString &token, int *buttonId)
{
    bool numeric = false;
    const int numericId = token.toInt(&numeric);
    if(numeric && isValidButtonId(numericId))
    {
        *buttonId = numericId;
        return true;
    }

    const auto found = scriptKeyNames().constFind(token.toLower());
    if(found == scriptKeyNames().constEnd())
        return false;
    *buttonId = found.value();
    return true;
}

QString scriptButtonName(int buttonId)
{
    switch(buttonId)
    {
    case keymap::ret: return QStringLiteral("Return");
    case keymap::enter: return QStringLiteral("Enter");
    case keymap::neg: return QStringLiteral("Neg");
    case keymap::space: return QStringLiteral("Space");
    case keymap::aa: return QStringLiteral("A");
    case keymap::ab: return QStringLiteral("B");
    case keymap::ac: return QStringLiteral("C");
    case keymap::ad: return QStringLiteral("D");
    case keymap::ae: return QStringLiteral("E");
    case keymap::af: return QStringLiteral("F");
    case keymap::ag: return QStringLiteral("G");
    case keymap::ah: return QStringLiteral("H");
    case keymap::ai: return QStringLiteral("I");
    case keymap::aj: return QStringLiteral("J");
    case keymap::ak: return QStringLiteral("K");
    case keymap::al: return QStringLiteral("L");
    case keymap::am: return QStringLiteral("M");
    case keymap::an: return QStringLiteral("N");
    case keymap::ao: return QStringLiteral("O");
    case keymap::ap: return QStringLiteral("P");
    case keymap::aq: return QStringLiteral("Q");
    case keymap::ar: return QStringLiteral("R");
    case keymap::as: return QStringLiteral("S");
    case keymap::at: return QStringLiteral("T");
    case keymap::au: return QStringLiteral("U");
    case keymap::av: return QStringLiteral("V");
    case keymap::aw: return QStringLiteral("W");
    case keymap::ax: return QStringLiteral("X");
    case keymap::ay: return QStringLiteral("Y");
    case keymap::az: return QStringLiteral("Z");
    case keymap::n0: return QStringLiteral("0");
    case keymap::n1: return QStringLiteral("1");
    case keymap::n2: return QStringLiteral("2");
    case keymap::n3: return QStringLiteral("3");
    case keymap::n4: return QStringLiteral("4");
    case keymap::n5: return QStringLiteral("5");
    case keymap::n6: return QStringLiteral("6");
    case keymap::n7: return QStringLiteral("7");
    case keymap::n8: return QStringLiteral("8");
    case keymap::n9: return QStringLiteral("9");
    case keymap::punct: return QStringLiteral("Punct");
    case keymap::on: return QStringLiteral("Home");
    case keymap::pi: return QStringLiteral("Pi");
    case keymap::trig: return QStringLiteral("Trig");
    case keymap::pow10: return QStringLiteral("Pow10");
    case keymap::ee: return QStringLiteral("EE");
    case keymap::squ: return QStringLiteral("Square");
    case keymap::div: return QStringLiteral("Divide");
    case keymap::exp: return QStringLiteral("Exp");
    case keymap::equ: return QStringLiteral("Equals");
    case keymap::mult: return QStringLiteral("Multiply");
    case keymap::pow: return QStringLiteral("Power");
    case keymap::var: return QStringLiteral("Var");
    case keymap::minus: return QStringLiteral("Minus");
    case keymap::pright: return QStringLiteral("RightParen");
    case keymap::dot: return QStringLiteral("Dot");
    case keymap::pleft: return QStringLiteral("LeftParen");
    case keymap::cat: return QStringLiteral("Catalog");
    case keymap::metrix: return QStringLiteral("Matrix");
    case keymap::del: return QStringLiteral("Delete");
    case keymap::pad: return QStringLiteral("Touchpad");
    case keymap::flag: return QStringLiteral("Flag");
    case keymap::plus: return QStringLiteral("Plus");
    case keymap::doc: return QStringLiteral("Doc");
    case keymap::menu: return QStringLiteral("Menu");
    case keymap::esc: return QStringLiteral("Escape");
    case keymap::tab: return QStringLiteral("Tab");
    case keymap::shift: return QStringLiteral("Shift");
    case keymap::ctrl: return QStringLiteral("Ctrl");
    case keymap::comma: return QStringLiteral("Comma");
    default: return QString::number(buttonId);
    }
}

bool scriptDuration(const QString &token, quint64 *duration)
{
    QString number = token.trimmed();
    if(number.endsWith(QStringLiteral("ms"), Qt::CaseInsensitive))
        number.chop(2);
    if(number.isEmpty() || number.startsWith(QLatin1Char('-')))
        return false;
    bool ok = false;
    const quint64 parsed = number.toULongLong(&ok);
    if(!ok)
        return false;
    *duration = parsed;
    return true;
}

void scriptError(QString *error, int line, const QString &message)
{
    if(error)
        *error = QCoreApplication::translate("KeypadMacroScript", "Line %1: %2")
                .arg(line).arg(message);
}

void setCorrupt(QDataStream &stream)
{
    if(stream.status() != QDataStream::ReadCorruptData)
        qWarning() << QStringLiteral("Ignoring corrupt keypad macro data");
    stream.setStatus(QDataStream::ReadCorruptData);
}
}

QDataStream &operator<<(QDataStream &out, const KeypadMacroEvent &event)
{
    out << SerializationVersion
        << static_cast<quint8>(event.type)
        << event.offsetMs;

    switch(event.type)
    {
    case KeypadMacroEventType::Button:
        out << event.buttonId << event.pressed;
        break;
    case KeypadMacroEventType::Touchpad:
        out << event.x << event.y << event.contact << event.down;
        break;
    }

    return out;
}

QDataStream &operator>>(QDataStream &in, KeypadMacroEvent &event)
{
    quint32 version = 0;
    quint8 rawType = 0;
    KeypadMacroEvent temporary;

    in >> version >> rawType >> temporary.offsetMs;
    if(in.status() != QDataStream::Ok || version != SerializationVersion)
    {
        setCorrupt(in);
        return in;
    }

    temporary.type = static_cast<KeypadMacroEventType>(rawType);
    switch(temporary.type)
    {
    case KeypadMacroEventType::Button:
        in >> temporary.buttonId >> temporary.pressed;
        if(in.status() == QDataStream::Ok && !isValidButtonId(temporary.buttonId))
            setCorrupt(in);
        break;
    case KeypadMacroEventType::Touchpad:
        in >> temporary.x >> temporary.y >> temporary.contact >> temporary.down;
        if(in.status() == QDataStream::Ok &&
           (!isValidCoordinate(temporary.x) || !isValidCoordinate(temporary.y)))
            setCorrupt(in);
        break;
    default:
        setCorrupt(in);
        break;
    }

    if(in.status() == QDataStream::Ok)
        event = temporary;
    return in;
}

QDataStream &operator<<(QDataStream &out, const KeypadMacro &macro)
{
    out << SerializationVersion << macro.id << macro.name << macro.events;
    return out;
}

QDataStream &operator>>(QDataStream &in, KeypadMacro &macro)
{
    quint32 version = 0;
    KeypadMacro temporary;

    in >> version;
    if(in.status() != QDataStream::Ok || version != SerializationVersion)
    {
        setCorrupt(in);
        return in;
    }

    in >> temporary.id >> temporary.name >> temporary.events;
    if(in.status() == QDataStream::Ok &&
       (temporary.name.isEmpty() || temporary.name != temporary.name.trimmed() ||
        temporary.events.isEmpty() || !validateEvents(temporary.events)))
        setCorrupt(in);

    if(in.status() == QDataStream::Ok)
        macro = temporary;
    return in;
}

bool KeypadMacroScript::parse(const QString &code, QVector<KeypadMacroEvent> *events, QString *error)
{
    QVector<KeypadMacroEvent> parsedEvents;
    QSet<int> pressedButtons;
    quint64 offsetMs = 0;
    double touchX = 0.0;
    double touchY = 0.0;
    bool touchContact = false;
    bool touchDown = false;
    bool endedWithWait = false;
    int lastCommandLine = 0;
    static const QRegularExpression whitespace(QStringLiteral("\\s+"));

    const QStringList lines = code.split(QLatin1Char('\n'));
    for(int lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
    {
        QString line = lines.at(lineIndex);
        const int comment = line.indexOf(QLatin1Char('#'));
        if(comment >= 0)
            line.truncate(comment);
        line = line.trimmed();
        if(line.isEmpty())
            continue;

        const int lineNumber = lineIndex + 1;
        lastCommandLine = lineNumber;
        const QStringList tokens = line.split(whitespace, Qt::SkipEmptyParts);
        const QString command = tokens.constFirst().toLower();

        if(command == QStringLiteral("wait"))
        {
            if(tokens.size() != 2)
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Expected: wait <milliseconds>"));
                return false;
            }
            if(parsedEvents.isEmpty())
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "The first event cannot be delayed"));
                return false;
            }
            quint64 duration = 0;
            if(!scriptDuration(tokens.at(1), &duration))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Duration must be a non-negative integer, optionally ending in ms"));
                return false;
            }
            if(duration > std::numeric_limits<quint64>::max() - offsetMs)
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Time offset is too large"));
                return false;
            }
            offsetMs += duration;
            endedWithWait = true;
            continue;
        }

        if(command == QStringLiteral("press") || command == QStringLiteral("release"))
        {
            if(tokens.size() != 2)
            {
                scriptError(error, lineNumber,
                            command == QStringLiteral("press")
                            ? QCoreApplication::translate("KeypadMacroScript",
                                                          "Expected: press <key>")
                            : QCoreApplication::translate("KeypadMacroScript",
                                                          "Expected: release <key>"));
                return false;
            }

            int buttonId = -1;
            if(!scriptButtonId(tokens.at(1), &buttonId))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Unknown key \"%1\"").arg(tokens.at(1)));
                return false;
            }
            const bool pressed = command == QStringLiteral("press");
            if(pressedButtons.contains(buttonId) == pressed)
            {
                scriptError(error, lineNumber,
                            pressed
                            ? QCoreApplication::translate("KeypadMacroScript",
                                                          "Key \"%1\" is already pressed").arg(tokens.at(1))
                            : QCoreApplication::translate("KeypadMacroScript",
                                                          "Key \"%1\" is not pressed").arg(tokens.at(1)));
                return false;
            }

            KeypadMacroEvent event;
            event.type = KeypadMacroEventType::Button;
            event.offsetMs = offsetMs;
            event.buttonId = buttonId;
            event.pressed = pressed;
            parsedEvents.append(event);
            if(pressed)
                pressedButtons.insert(buttonId);
            else
                pressedButtons.remove(buttonId);
            endedWithWait = false;
            continue;
        }

        if(command == QStringLiteral("tap"))
        {
            if(tokens.size() < 2 || tokens.size() > 3)
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Expected: tap <key> [duration]"));
                return false;
            }

            int buttonId = -1;
            if(!scriptButtonId(tokens.at(1), &buttonId))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Unknown key \"%1\"").arg(tokens.at(1)));
                return false;
            }
            if(pressedButtons.contains(buttonId))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Key \"%1\" is already pressed").arg(tokens.at(1)));
                return false;
            }

            quint64 duration = 50;
            if(tokens.size() == 3 && !scriptDuration(tokens.at(2), &duration))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Duration must be a non-negative integer, optionally ending in ms"));
                return false;
            }
            if(duration > std::numeric_limits<quint64>::max() - offsetMs)
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Time offset is too large"));
                return false;
            }

            KeypadMacroEvent pressEvent;
            pressEvent.type = KeypadMacroEventType::Button;
            pressEvent.offsetMs = offsetMs;
            pressEvent.buttonId = buttonId;
            pressEvent.pressed = true;
            parsedEvents.append(pressEvent);
            offsetMs += duration;
            pressEvent.offsetMs = offsetMs;
            pressEvent.pressed = false;
            parsedEvents.append(pressEvent);
            endedWithWait = false;
            continue;
        }

        if(command == QStringLiteral("touch"))
        {
            double x = touchX;
            double y = touchY;
            QString state;
            if(tokens.size() == 2 && tokens.at(1).compare(QStringLiteral("release"), Qt::CaseInsensitive) == 0)
                state = QStringLiteral("release");
            else if(tokens.size() == 4)
            {
                bool xOk = false;
                bool yOk = false;
                x = tokens.at(1).toDouble(&xOk);
                y = tokens.at(2).toDouble(&yOk);
                if(!xOk || !yOk || !isValidCoordinate(x) || !isValidCoordinate(y))
                {
                    scriptError(error, lineNumber,
                                QCoreApplication::translate("KeypadMacroScript",
                                                            "Touch coordinates must be numbers from 0 to 1"));
                    return false;
                }
                state = tokens.at(3).toLower();
            }
            else
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Expected: touch <x> <y> <contact|down|release> or touch release"));
                return false;
            }

            bool contact = false;
            bool down = false;
            if(state == QStringLiteral("contact"))
                contact = true;
            else if(state == QStringLiteral("down"))
                contact = down = true;
            else if(state != QStringLiteral("release"))
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Unknown touch state \"%1\"").arg(state));
                return false;
            }

            if(x == touchX && y == touchY && contact == touchContact && down == touchDown)
            {
                scriptError(error, lineNumber,
                            QCoreApplication::translate("KeypadMacroScript",
                                                        "Touch state is unchanged"));
                return false;
            }

            KeypadMacroEvent event;
            event.type = KeypadMacroEventType::Touchpad;
            event.offsetMs = offsetMs;
            event.x = x;
            event.y = y;
            event.contact = contact;
            event.down = down;
            parsedEvents.append(event);
            touchX = x;
            touchY = y;
            touchContact = contact;
            touchDown = down;
            endedWithWait = false;
            continue;
        }

        scriptError(error, lineNumber,
                    QCoreApplication::translate("KeypadMacroScript",
                                                "Unknown command \"%1\"").arg(tokens.constFirst()));
        return false;
    }

    if(parsedEvents.isEmpty())
    {
        if(error)
            *error = QCoreApplication::translate("KeypadMacroScript",
                                                  "Macro code contains no events");
        return false;
    }
    if(endedWithWait)
    {
        scriptError(error, lastCommandLine,
                    QCoreApplication::translate("KeypadMacroScript",
                                                "Macro code cannot end with wait"));
        return false;
    }
    if(!pressedButtons.isEmpty())
    {
        QList<int> pressed = pressedButtons.values();
        std::sort(pressed.begin(), pressed.end());
        if(error)
            *error = QCoreApplication::translate("KeypadMacroScript",
                                                  "Key \"%1\" is still pressed")
                    .arg(scriptButtonName(pressed.constFirst()));
        return false;
    }
    if(touchContact || touchDown)
    {
        if(error)
            *error = QCoreApplication::translate("KeypadMacroScript",
                                                  "Touchpad is still active");
        return false;
    }
    if(!validateEvents(parsedEvents))
    {
        if(error)
            *error = QCoreApplication::translate("KeypadMacroScript",
                                                  "Macro code produced an invalid event sequence");
        return false;
    }

    if(events)
        *events = parsedEvents;
    if(error)
        error->clear();
    return true;
}

QString KeypadMacroScript::format(const QVector<KeypadMacroEvent> &events)
{
    QStringList lines;
    quint64 previousOffset = 0;
    for(const KeypadMacroEvent &event : events)
    {
        if(event.offsetMs > previousOffset)
            lines.append(QStringLiteral("wait %1ms").arg(event.offsetMs - previousOffset));
        previousOffset = event.offsetMs;

        if(event.type == KeypadMacroEventType::Button)
        {
            lines.append(QStringLiteral("%1 %2")
                         .arg(event.pressed ? QStringLiteral("press") : QStringLiteral("release"),
                              scriptButtonName(event.buttonId)));
        }
        else if(event.type == KeypadMacroEventType::Touchpad)
        {
            QString state = QStringLiteral("release");
            if(event.down)
                state = QStringLiteral("down");
            else if(event.contact)
                state = QStringLiteral("contact");
            lines.append(QStringLiteral("touch %1 %2 %3")
                         .arg(QString::number(event.x, 'g', 15),
                              QString::number(event.y, 'g', 15),
                              state));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QDataStream &operator<<(QDataStream &out, const KeypadMacroModel &model)
{
    out << SerializationVersion << model.macros << model.nextID;
    return out;
}

QDataStream &operator>>(QDataStream &in, KeypadMacroModel &model)
{
    quint32 version = 0;
    QVector<KeypadMacro> macros;
    quint32 nextID = 0;

    in >> version;
    if(in.status() != QDataStream::Ok || version != SerializationVersion)
    {
        setCorrupt(in);
        return in;
    }

    in >> macros >> nextID;
    if(in.status() != QDataStream::Ok)
        return in;

    QSet<quint32> ids;
    QStringList names;
    quint32 maximumID = 0;
    bool hasID = false;
    for(const KeypadMacro &macro : macros)
    {
        if(ids.contains(macro.id))
        {
            setCorrupt(in);
            return in;
        }
        ids.insert(macro.id);
        maximumID = hasID ? qMax(maximumID, macro.id) : macro.id;
        hasID = true;

        for(const QString &name : names)
        {
            if(QString::compare(name, macro.name, Qt::CaseInsensitive) == 0)
            {
                setCorrupt(in);
                return in;
            }
        }
        names.append(macro.name);
    }

    if(hasID && maximumID != std::numeric_limits<quint32>::max() && nextID <= maximumID)
    {
        setCorrupt(in);
        return in;
    }
    if(hasID && maximumID == std::numeric_limits<quint32>::max() && nextID != maximumID)
    {
        setCorrupt(in);
        return in;
    }

    model.macros = macros;
    model.nextID = nextID;
    return in;
}

int KeypadMacroModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : macros.size();
}

QHash<int, QByteArray> KeypadMacroModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IDRole] = "id";
    roles[NameRole] = "name";
    roles[EventCountRole] = "eventCount";
    roles[DurationRole] = "durationMs";
    return roles;
}

QVariant KeypadMacroModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
        return QVariant();
    return getDataRow(index.row(), role);
}

QVariant KeypadMacroModel::getDataRow(int row, int role) const
{
    const KeypadMacro *macro = macroAt(row);
    if(!macro)
        return QVariant();

    switch(role)
    {
    case IDRole:
        return macro->id;
    case NameRole:
        return macro->name;
    case EventCountRole:
        return macro->events.size();
    case DurationRole:
        return macro->events.isEmpty() ? quint64(0) : macro->events.constLast().offsetMs;
    default:
        return QVariant();
    }
}

bool KeypadMacroModel::rename(int row, QString name)
{
    name = name.trimmed();
    if(mutationLocked || row < 0 || row >= macros.size() || !isNameAvailable(name, row))
        return false;
    if(macros[row].name == name)
        return true;

    macros[row].name = name;
    emit dataChanged(index(row), index(row), QVector<int>({NameRole}));
    emit anythingChanged();
    return true;
}

bool KeypadMacroModel::remove(int row)
{
    if(mutationLocked || row < 0 || row >= macros.size())
        return false;

    beginRemoveRows(QModelIndex(), row, row);
    macros.removeAt(row);
    endRemoveRows();
    emit anythingChanged();
    return true;
}

bool KeypadMacroModel::isNameAvailable(QString name, int exceptRow) const
{
    name = name.trimmed();
    if(name.isEmpty() || exceptRow < -1 || exceptRow >= macros.size())
        return false;

    for(int row = 0; row < macros.size(); ++row)
    {
        if(row != exceptRow && QString::compare(macros.at(row).name, name, Qt::CaseInsensitive) == 0)
            return false;
    }
    return true;
}

int KeypadMacroModel::addMacro(QString name, QVector<KeypadMacroEvent> events)
{
    name = name.trimmed();
    if(!isNameAvailable(name, -1) || events.isEmpty() || !validateEvents(events))
        return -1;

    if(nextID == std::numeric_limits<quint32>::max())
    {
        for(const KeypadMacro &macro : macros)
            if(macro.id == nextID)
                return -1;
    }

    const int row = macros.size();
    beginInsertRows(QModelIndex(), row, row);
    macros.append({nextID, name, events});
    if(nextID != std::numeric_limits<quint32>::max())
        ++nextID;
    endInsertRows();
    emit anythingChanged();
    return row;
}

bool KeypadMacroModel::replaceMacroEvents(int row, QVector<KeypadMacroEvent> events)
{
    if(mutationLocked || row < 0 || row >= macros.size() ||
       events.isEmpty() || !validateEvents(events))
        return false;

    macros[row].events = events;
    emit dataChanged(index(row), index(row), QVector<int>({EventCountRole, DurationRole}));
    emit anythingChanged();
    return true;
}

bool KeypadMacroModel::updateMacro(int row, QString name, QVector<KeypadMacroEvent> events)
{
    name = name.trimmed();
    if(mutationLocked || row < 0 || row >= macros.size() ||
       !isNameAvailable(name, row) || events.isEmpty() || !validateEvents(events))
        return false;

    macros[row].name = name;
    macros[row].events = events;
    emit dataChanged(index(row), index(row),
                     QVector<int>({NameRole, EventCountRole, DurationRole}));
    emit anythingChanged();
    return true;
}

const KeypadMacro *KeypadMacroModel::macroAt(int row) const
{
    return row >= 0 && row < macros.size() ? &macros.at(row) : nullptr;
}

KeypadMacroController::KeypadMacroController(QObject *parent) : QObject(parent)
{
    playbackDeadlineTimer.setSingleShot(true);
    playbackDeadlineTimer.setTimerType(Qt::PreciseTimer);
}

bool KeypadMacroController::validButtonId(int buttonId)
{
    return isValidButtonId(buttonId);
}

bool KeypadMacroController::validTouchpad(double x, double y)
{
    return isValidCoordinate(x) && isValidCoordinate(y);
}

void KeypadMacroController::startRecording()
{
    cancelRecording();
    recording = true;
    recordingTimer.start();
    firstEventElapsedMs = -1;
    recordedEvents.clear();
    recordedButtons.clear();
    recordedTouchX = recordedTouchY = 0.0;
    recordedTouchContact = recordedTouchDown = false;
}

quint64 KeypadMacroController::recordingOffset()
{
    const qint64 now = recordingTimer.elapsed();
    if(firstEventElapsedMs < 0)
    {
        firstEventElapsedMs = now;
        return 0;
    }
    return static_cast<quint64>(qMax<qint64>(0, now - firstEventElapsedMs));
}

void KeypadMacroController::appendButton(int buttonId, bool pressed, quint64 offsetMs)
{
    KeypadMacroEvent event;
    event.type = KeypadMacroEventType::Button;
    event.offsetMs = offsetMs;
    event.buttonId = buttonId;
    event.pressed = pressed;
    recordedEvents.append(event);
}

void KeypadMacroController::appendTouchpad(double x, double y, bool contact, bool down, quint64 offsetMs)
{
    KeypadMacroEvent event;
    event.type = KeypadMacroEventType::Touchpad;
    event.offsetMs = offsetMs;
    event.x = x;
    event.y = y;
    event.contact = contact;
    event.down = down;
    recordedEvents.append(event);
}

void KeypadMacroController::captureButton(int buttonId, bool pressed)
{
    if(!recording || !validButtonId(buttonId) || recordedButtons.contains(buttonId) == pressed)
        return;

    if(pressed)
        recordedButtons.insert(buttonId);
    else
        recordedButtons.remove(buttonId);
    appendButton(buttonId, pressed, recordingOffset());
}

void KeypadMacroController::captureTouchpad(double x, double y, bool contact, bool down)
{
    if(!recording || !validTouchpad(x, y) ||
       (recordedTouchX == x && recordedTouchY == y &&
        recordedTouchContact == contact && recordedTouchDown == down))
        return;

    recordedTouchX = x;
    recordedTouchY = y;
    recordedTouchContact = contact;
    recordedTouchDown = down;
    appendTouchpad(x, y, contact, down, recordingOffset());
}

bool KeypadMacroController::stopRecording(QVector<KeypadMacroEvent> *events)
{
    if(!recording)
        return false;

    recording = false;
    if(recordedEvents.isEmpty())
    {
        if(events)
            events->clear();
        cancelRecording();
        return false;
    }

    const quint64 offsetMs = recordingOffset();
    QList<int> buttons = recordedButtons.values();
    std::sort(buttons.begin(), buttons.end());
    for(int buttonId : buttons)
        appendButton(buttonId, false, offsetMs);
    if(recordedTouchContact || recordedTouchDown)
        appendTouchpad(recordedTouchX, recordedTouchY, false, false, offsetMs);

    if(events)
        *events = recordedEvents;
    cancelRecording();
    return true;
}

void KeypadMacroController::cancelRecording()
{
    recording = false;
    recordingTimer.invalidate();
    firstEventElapsedMs = -1;
    recordedEvents.clear();
    recordedButtons.clear();
    recordedTouchX = recordedTouchY = 0.0;
    recordedTouchContact = recordedTouchDown = false;
}

void KeypadMacroController::startPlayback(const QVector<KeypadMacroEvent> &events)
{
    if(playing)
        finishPlayback(true);

    playbackEvents = events;
    playbackIndex = 0;
    playbackButtons.clear();
    playbackTouchX = playbackTouchY = 0.0;
    playbackTouchContact = playbackTouchDown = false;
    playing = !playbackEvents.isEmpty();
    ++playbackGeneration;

    if(!playing)
    {
        emit playbackFinished();
        return;
    }

    playbackTimer.start();
    schedulePlayback();
}

void KeypadMacroController::schedulePlayback()
{
    if(!playing || playbackIndex >= playbackEvents.size())
    {
        finishPlayback(true);
        return;
    }

    const quint64 elapsed = static_cast<quint64>(qMax<qint64>(0, playbackTimer.elapsed()));
    const quint64 deadline = playbackEvents.at(playbackIndex).offsetMs;
    const quint64 delay = deadline > elapsed ? deadline - elapsed : 0;
    const int interval = static_cast<int>(qMin<quint64>(delay, static_cast<quint64>(INT_MAX)));
    const quint64 generation = playbackGeneration;

    if(playbackTimerConnection)
        disconnect(playbackTimerConnection);
    playbackTimerConnection = connect(&playbackDeadlineTimer, &QTimer::timeout, this,
                                      [this, generation]() {
        if(playing && generation == playbackGeneration)
            processPlayback();
    });
    playbackDeadlineTimer.start(interval);
}

void KeypadMacroController::processPlayback()
{
    if(!playing)
        return;

    const quint64 elapsed = static_cast<quint64>(qMax<qint64>(0, playbackTimer.elapsed()));
    while(playbackIndex < playbackEvents.size() &&
          playbackEvents.at(playbackIndex).offsetMs <= elapsed)
    {
        const KeypadMacroEvent event = playbackEvents.at(playbackIndex++);
        if(event.type == KeypadMacroEventType::Button)
        {
            if(event.pressed)
                playbackButtons.insert(event.buttonId);
            else
                playbackButtons.remove(event.buttonId);
            emit playbackButtonState(event.buttonId, event.pressed);
        }
        else
        {
            playbackTouchX = event.x;
            playbackTouchY = event.y;
            playbackTouchContact = event.contact;
            playbackTouchDown = event.down;
            emit playbackTouchpadState(event.x, event.y, event.contact, event.down);
        }
    }

    if(playbackIndex >= playbackEvents.size())
        finishPlayback(true);
    else
        schedulePlayback();
}

void KeypadMacroController::releasePlaybackState()
{
    QList<int> buttons = playbackButtons.values();
    std::sort(buttons.begin(), buttons.end());
    for(int buttonId : buttons)
        emit playbackButtonState(buttonId, false);
    playbackButtons.clear();

    if(playbackTouchContact || playbackTouchDown)
        emit playbackTouchpadState(playbackTouchX, playbackTouchY, false, false);
    playbackTouchContact = playbackTouchDown = false;
}

void KeypadMacroController::finishPlayback(bool notify)
{
    if(!playing)
        return;

    playing = false;
    ++playbackGeneration;
    playbackDeadlineTimer.stop();
    if(playbackTimerConnection)
    {
        disconnect(playbackTimerConnection);
        playbackTimerConnection = QMetaObject::Connection();
    }
    releasePlaybackState();
    playbackTimer.invalidate();
    playbackEvents.clear();
    playbackIndex = 0;
    if(notify)
        emit playbackFinished();
}

void KeypadMacroController::cancelPlayback()
{
    finishPlayback(true);
}
