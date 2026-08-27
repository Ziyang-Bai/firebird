#ifndef KEYPADMACRO_H
#define KEYPADMACRO_H

#include <QAbstractListModel>
#include <QDataStream>
#include <QElapsedTimer>
#include <QMetaObject>
#include <QSet>
#include <QTimer>
#include <QVector>

#include <limits>

enum class KeypadMacroEventType : quint8
{
    Button = 1,
    Touchpad = 2
};

struct KeypadMacroEvent
{
    KeypadMacroEventType type = KeypadMacroEventType::Button;
    quint64 offsetMs = 0;

    int buttonId = 0;
    bool pressed = false;

    double x = 0.0;
    double y = 0.0;
    bool contact = false;
    bool down = false;
};

struct KeypadMacro
{
    quint32 id = 0;
    QString name;
    QVector<KeypadMacroEvent> events;

    KeypadMacro() = default;
    KeypadMacro(quint32 id, const QString &name, const QVector<KeypadMacroEvent> &events)
        : id(id), name(name), events(events) {}
};

QDataStream &operator<<(QDataStream &out, const KeypadMacroEvent &event);
QDataStream &operator>>(QDataStream &in, KeypadMacroEvent &event);
QDataStream &operator<<(QDataStream &out, const KeypadMacro &macro);
QDataStream &operator>>(QDataStream &in, KeypadMacro &macro);

class KeypadMacroScript
{
public:
    static bool parse(const QString &code, QVector<KeypadMacroEvent> *events, QString *error);
    static QString format(const QVector<KeypadMacroEvent> &events);
};

class KeypadMacroModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Role {
        IDRole = Qt::UserRole + 1,
        NameRole,
        EventCountRole,
        DurationRole
    };
    Q_ENUMS(Role)

    KeypadMacroModel() = default;
    KeypadMacroModel(const KeypadMacroModel &other) : QAbstractListModel()
    {
        macros = other.macros;
        nextID = other.nextID;
    }
    KeypadMacroModel &operator=(const KeypadMacroModel &other)
    {
        macros = other.macros;
        nextID = other.nextID;
        return *this;
    }

    Q_INVOKABLE int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    Q_INVOKABLE QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE QVariant getDataRow(int row, int role = Qt::DisplayRole) const;
    Q_INVOKABLE bool rename(int row, QString name);
    Q_INVOKABLE bool remove(int row);
    Q_INVOKABLE bool isNameAvailable(QString name, int exceptRow = -1) const;

    int addMacro(QString name, QVector<KeypadMacroEvent> events);
    bool replaceMacroEvents(int row, QVector<KeypadMacroEvent> events);
    bool updateMacro(int row, QString name, QVector<KeypadMacroEvent> events);
    const KeypadMacro *macroAt(int row) const;
    void setMutationLocked(bool locked) { mutationLocked = locked; }

    friend QDataStream &operator<<(QDataStream &out, const KeypadMacroModel &model);
    friend QDataStream &operator>>(QDataStream &in, KeypadMacroModel &model);

signals:
    void anythingChanged();

private:
    QVector<KeypadMacro> macros;
    quint32 nextID = 0;
    bool mutationLocked = false;
};

QDataStream &operator<<(QDataStream &out, const KeypadMacroModel &model);
QDataStream &operator>>(QDataStream &in, KeypadMacroModel &model);

class KeypadMacroController : public QObject
{
    Q_OBJECT
public:
    explicit KeypadMacroController(QObject *parent = nullptr);

    void startRecording();
    void captureButton(int buttonId, bool pressed);
    void captureTouchpad(double x, double y, bool contact, bool down);
    bool stopRecording(QVector<KeypadMacroEvent> *events);
    void cancelRecording();

    void startPlayback(const QVector<KeypadMacroEvent> &events);
    void cancelPlayback();

    bool isRecording() const { return recording; }
    bool isPlaying() const { return playing; }
    bool hasRecordedEvents() const { return !recordedEvents.isEmpty(); }

signals:
    void playbackButtonState(int buttonId, bool pressed);
    void playbackTouchpadState(double x, double y, bool contact, bool down);
    void playbackFinished();

private slots:
    void processPlayback();

private:
    static bool validButtonId(int buttonId);
    static bool validTouchpad(double x, double y);
    quint64 recordingOffset();
    void appendButton(int buttonId, bool pressed, quint64 offsetMs);
    void appendTouchpad(double x, double y, bool contact, bool down, quint64 offsetMs);
    void schedulePlayback();
    void finishPlayback(bool notify);
    void releasePlaybackState();

    bool recording = false;
    QElapsedTimer recordingTimer;
    qint64 firstEventElapsedMs = -1;
    QVector<KeypadMacroEvent> recordedEvents;
    QSet<int> recordedButtons;
    double recordedTouchX = 0.0;
    double recordedTouchY = 0.0;
    bool recordedTouchContact = false;
    bool recordedTouchDown = false;

    bool playing = false;
    QElapsedTimer playbackTimer;
    QTimer playbackDeadlineTimer;
    QMetaObject::Connection playbackTimerConnection;
    quint64 playbackGeneration = 0;
    QVector<KeypadMacroEvent> playbackEvents;
    int playbackIndex = 0;
    QSet<int> playbackButtons;
    double playbackTouchX = 0.0;
    double playbackTouchY = 0.0;
    bool playbackTouchContact = false;
    bool playbackTouchDown = false;
};

Q_DECLARE_METATYPE(KeypadMacroModel)

#endif // KEYPADMACRO_H
