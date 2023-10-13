// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vtbackend/primitives.h>

#include <crispy/escape.h>
#include <crispy/overloaded.h>

#include <fmt/format.h>

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <libunicode/convert.h>

namespace vtbackend
{

/// Mutualy exclusive mouse protocls.
enum class MouseProtocol
{
    /// Old X10 mouse protocol
    X10 = 9,
    /// Normal tracking mode, that's X10 with mouse release events and modifiers
    NormalTracking = 1000,
    /// Highlight mouse tracking
    HighlightTracking = 1001,
    /// Button-event tracking protocol.
    ButtonTracking = 1002,
    /// Like ButtonTracking plus motion events.
    AnyEventTracking = 1003,
};

// {{{ Modifier
class Modifier
{
  public:
    enum Key : unsigned
    {
        None = 0,
        Shift = 1,
        Alt = 2,
        Control = 4,
        Super = 8,
        Hyper = 16,
        Meta = 32,
        CapsLock = 64,
        NumLock = 128,
    };

    constexpr Modifier(Key key): _mask { static_cast<unsigned>(key) } {}

    constexpr Modifier() = default;
    constexpr Modifier(Modifier&&) = default;
    constexpr Modifier(Modifier const&) = default;
    constexpr Modifier& operator=(Modifier&&) = default;
    constexpr Modifier& operator=(Modifier const&) = default;

    [[nodiscard]] constexpr unsigned value() const noexcept { return _mask; }

    [[nodiscard]] constexpr bool none() const noexcept { return value() == 0; }
    [[nodiscard]] constexpr bool some() const noexcept { return value() != 0; }
    [[nodiscard]] constexpr bool shift() const noexcept { return value() & Shift; }
    [[nodiscard]] constexpr bool alt() const noexcept { return value() & Alt; }
    [[nodiscard]] constexpr bool control() const noexcept { return value() & Control; }
    [[nodiscard]] constexpr bool super() const noexcept { return value() & Super; }
    // hyper is called hyperKey because in MSVC/Win32 there is a `typedef hyper __uint64;` :-(
    [[nodiscard]] constexpr bool hyperKey() const noexcept { return value() & Hyper; }
    [[nodiscard]] constexpr bool meta() const noexcept { return value() & Meta; }
    [[nodiscard]] constexpr bool capsLock() const noexcept { return value() & CapsLock; }
    [[nodiscard]] constexpr bool numLock() const noexcept { return value() & NumLock; }

    constexpr operator unsigned() const noexcept { return _mask; }

    [[nodiscard]] constexpr bool any() const noexcept { return _mask != 0; }

    constexpr Modifier& operator|=(Modifier const& other) noexcept
    {
        _mask |= other._mask;
        return *this;
    }

    [[nodiscard]] constexpr Modifier with(Modifier const& other) const noexcept
    {
        return Modifier(static_cast<Key>(_mask | other._mask));
    }

    [[nodiscard]] constexpr Modifier without(Modifier const& other) const noexcept
    {
        return Modifier(static_cast<Key>(_mask & ~other._mask));
    }

    [[nodiscard]] bool contains(Modifier const& other) const noexcept
    {
        return (_mask & other._mask) == other._mask;
    }

    constexpr void enable(Key key) noexcept { _mask |= key; }

    constexpr void disable(Key key) noexcept { _mask &= ~static_cast<unsigned>(key); }

  private:
    unsigned _mask = 0;
};

constexpr Modifier operator|(Modifier a, Modifier b) noexcept
{
    return Modifier(static_cast<Modifier::Key>(a.value() | b.value()));
}

constexpr bool operator<(Modifier lhs, Modifier rhs) noexcept
{
    return lhs.value() < rhs.value();
}

constexpr bool operator==(Modifier lhs, Modifier rhs) noexcept
{
    return lhs.value() == rhs.value();
}

std::optional<Modifier::Key> parseModifierKey(std::string const& key);

constexpr bool operator!(Modifier modifier) noexcept
{
    return modifier.none();
}

constexpr bool operator==(Modifier lhs, Modifier::Key rhs) noexcept
{
    return static_cast<Modifier::Key>(lhs.value()) == rhs;
}

constexpr Modifier operator+(Modifier::Key lhs, Modifier::Key rhs) noexcept
{
    return Modifier(static_cast<Modifier::Key>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs)));
}

/// @returns CSI parameter for given function key modifier
constexpr size_t makeVirtualTerminalParam(Modifier modifier) noexcept
{
    return 1 + modifier.value();
}

std::string to_string(Modifier modifier);

// }}}
// {{{ KeyInputEvent, Key
enum class Key
{
    // function keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    F13,
    F14,
    F15,
    F16,
    F17,
    F18,
    F19,
    F20,
    F21,
    F22,
    F23,
    F24,
    F25,
    F26,
    F27,
    F28,
    F29,
    F30,
    F31,
    F32,
    F33,
    F34,
    F35,

    Escape,
    Enter,
    Tab,
    Backspace,

    // cursor keys
    DownArrow,
    LeftArrow,
    RightArrow,
    UpArrow,

    // 6-key editing pad
    Insert,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,

    // media keys
    MediaPlay,
    MediaStop,
    MediaPrevious,
    MediaNext,
    MediaPause,
    MediaTogglePlayPause,

    VolumeUp,
    VolumeDown,
    VolumeMute,

    // modifier keys
    LeftShift,
    RightShift,
    LeftControl,
    RightControl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper,
    LeftHyper,
    RightHyper,
    LeftMeta,
    RightMeta,
    IsoLevel3Shift,
    IsoLevel5Shift,

    // other special keys
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    Menu,
};

std::string to_string(Key key);

enum class KeyMode
{
    Normal,
    Application
};
// }}}
// {{{ Mouse
enum class MouseButton
{
    Left,
    Right,
    Middle,
    Release, // Button was released and/or no button is pressed.
    WheelUp,
    WheelDown,
    WheelLeft,
    WheelRight,
};

std::string to_string(MouseButton button);

enum class MouseTransport
{
    // CSI M Cb Cx Cy, with Cb, Cx, Cy incremented by 0x20
    Default,
    // CSI M Cb Coords, with Coords being UTF-8 encoded, Coords is a tuple, each value incremented by 0x20.
    Extended,
    // `CSI Cb Cx Cy M` and `CSI Cb Cx Cy m` (button release)
    SGR,
    // SGR-Pixels (1016), an xterm extension as of Patch #359 - 2020/08/17
    // This is just like SGR but reports pixels isntead of ANSI cursor positions.
    SGRPixels,
    // `CSI < Cb Cx Cy M` with Cb += 0x20
    URXVT,
};
// }}}

enum class KeyboardEventType
{
    Press = 1,
    Repeat = 2,
    Release = 3,
};

class KeyboardInputGenerator
{
  public:
    virtual ~KeyboardInputGenerator() = default;

    virtual bool generateChar(char32_t characterEvent, Modifier modifier, KeyboardEventType eventType) = 0;
    virtual bool generateKey(Key key, Modifier modifier, KeyboardEventType eventType) = 0;
};

class StandardKeyboardInputGenerator: public KeyboardInputGenerator
{
  public:
    bool generateChar(char32_t characterEvent, Modifier modifier, KeyboardEventType eventType) override;
    bool generateKey(Key key, Modifier modifier, KeyboardEventType eventType) override;

    [[nodiscard]] bool normalCursorKeys() const noexcept { return _cursorKeysMode == KeyMode::Normal; }
    [[nodiscard]] bool applicationCursorKeys() const noexcept { return !normalCursorKeys(); }

    [[nodiscard]] bool numericKeypad() const noexcept { return _numpadKeysMode == KeyMode::Normal; }
    [[nodiscard]] bool applicationKeypad() const noexcept { return !numericKeypad(); }
    void setCursorKeysMode(KeyMode mode) { _cursorKeysMode = mode; }
    void setNumpadKeysMode(KeyMode mode) { _numpadKeysMode = mode; }
    void setApplicationKeypadMode(bool enable)
    {
        _numpadKeysMode = enable ? KeyMode::Application : KeyMode::Normal;
    }

    [[nodiscard]] std::string_view peek() const noexcept { return std::string_view(_pendingSequence); }

    [[nodiscard]] std::string take() noexcept
    {
        auto result = std::move(_pendingSequence);
        _pendingSequence.clear();
        return result;
    }

    void reset()
    {
        _cursorKeysMode = KeyMode::Normal;
        _numpadKeysMode = KeyMode::Normal;
    }

  protected:
    struct FunctionKeyMapping
    {
        std::string_view std {};
        std::string_view mods {};
        std::string_view appCursor {};
        std::string_view appKeypad {};
    };

    [[nodiscard]] std::string select(Modifier modifier, FunctionKeyMapping mapping) const;
    void append(char ch) { _pendingSequence += ch; }
    void append(std::string_view sequence) { _pendingSequence += sequence; }

    template <typename... Args>
    void append(fmt::format_string<Args...> const& text, Args&&... args)
    {
#if defined(__APPLE__) || defined(_MSC_VER)
        append(fmt::vformat(text, fmt::make_format_args(args...)));
#else
        append(fmt::format(text, std::forward<Args>(args)...));
#endif
    }

    KeyMode _cursorKeysMode = KeyMode::Normal;
    KeyMode _numpadKeysMode = KeyMode::Normal;
    std::string _pendingSequence {};
};

class InputGenerator
{
  public:
    using Sequence = std::string;

    /// Changes the input mode for cursor keys.
    void setCursorKeysMode(KeyMode mode);

    /// Changes the input mode for numpad keys.
    void setNumpadKeysMode(KeyMode mode);

    void setApplicationKeypadMode(bool enable);

    [[nodiscard]] bool normalCursorKeys() const noexcept
    {
        return _keyboardInputGenerator.normalCursorKeys();
    }
    [[nodiscard]] bool applicationCursorKeys() const noexcept
    {
        return _keyboardInputGenerator.applicationCursorKeys();
    }

    [[nodiscard]] bool numericKeypad() const noexcept { return _keyboardInputGenerator.numericKeypad(); }
    [[nodiscard]] bool applicationKeypad() const noexcept
    {
        return _keyboardInputGenerator.applicationKeypad();
    }

    [[nodiscard]] bool bracketedPaste() const noexcept { return _bracketedPaste; }
    void setBracketedPaste(bool enable) { _bracketedPaste = enable; }

    void setMouseProtocol(MouseProtocol mouseProtocol, bool enabled);
    [[nodiscard]] std::optional<MouseProtocol> mouseProtocol() const noexcept { return _mouseProtocol; }

    // Sets mouse event transport protocol (default, extended, xgr, urxvt)
    void setMouseTransport(MouseTransport mouseTransport);
    [[nodiscard]] MouseTransport mouseTransport() const noexcept { return _mouseTransport; }

    enum class MouseWheelMode
    {
        // mouse wheel generates mouse wheel events as determined by mouse protocol + transport.
        Default,
        // mouse wheel generates normal cursor key events
        NormalCursorKeys,
        // mouse wheel generates application cursor key events
        ApplicationCursorKeys
    };

    void setMouseWheelMode(MouseWheelMode mode) noexcept;
    [[nodiscard]] MouseWheelMode mouseWheelMode() const noexcept { return _mouseWheelMode; }

    void setGenerateFocusEvents(bool enable) noexcept { _generateFocusEvents = enable; }
    [[nodiscard]] bool generateFocusEvents() const noexcept { return _generateFocusEvents; }

    void setPassiveMouseTracking(bool v) noexcept { _passiveMouseTracking = v; }
    [[nodiscard]] bool passiveMouseTracking() const noexcept { return _passiveMouseTracking; }

    bool generate(char32_t characterEvent, Modifier modifier, KeyboardEventType eventType);
    bool generate(Key key, Modifier modifier, KeyboardEventType eventType);
    void generatePaste(std::string_view const& text);
    bool generateMousePress(Modifier modifier,
                            MouseButton button,
                            CellLocation pos,
                            PixelCoordinate pixelPosition,
                            bool uiHandled);
    bool generateMouseMove(Modifier modifier,
                           CellLocation pos,
                           PixelCoordinate pixelPosition,
                           bool uiHandled);
    bool generateMouseRelease(Modifier modifier,
                              MouseButton button,
                              CellLocation pos,
                              PixelCoordinate pixelPosition,
                              bool uiHandled);

    bool generateFocusInEvent();
    bool generateFocusOutEvent();

    /// Generates raw input, usually used for sending reply VT sequences.
    bool generateRaw(std::string_view const& raw);

    /// Peeks into the generated output, returning it as string view.
    ///
    /// @return a view into the generated buffer sequence.
    [[nodiscard]] std::string_view peek() const noexcept
    {
        return std::string_view(_pendingSequence.data() + _consumedBytes,
                                size_t(_pendingSequence.size() - size_t(_consumedBytes)));
    }

    void consume(int n)
    {
        _consumedBytes += n;
        if (_consumedBytes == static_cast<int>(_pendingSequence.size()))
        {
            _consumedBytes = 0;
            _pendingSequence.clear();
        }
    }

    enum class MouseEventType
    {
        Press,
        Drag,
        Release
    };

    /// Resets the input generator's state, as required by the RIS (hard reset) VT sequence.
    void reset();

  private:
    bool generateMouse(MouseEventType eventType,
                       Modifier modifier,
                       MouseButton button,
                       CellLocation pos,
                       PixelCoordinate pixelPosition,
                       bool uiHandled);

    bool mouseTransport(MouseEventType eventType,
                        uint8_t button,
                        uint8_t modifier,
                        CellLocation pos,
                        PixelCoordinate pixelPosition,
                        bool uiHandled);

    bool mouseTransportX10(uint8_t button, uint8_t modifier, CellLocation pos);
    bool mouseTransportExtended(uint8_t button, uint8_t modifier, CellLocation pos);

    bool mouseTransportSGR(
        MouseEventType type, uint8_t button, uint8_t modifier, int x, int y, bool uiHandled);

    bool mouseTransportURXVT(MouseEventType type, uint8_t button, uint8_t modifier, CellLocation pos);

    inline bool append(std::string_view sequence);
    inline bool append(char asciiChar);
    inline bool append(uint8_t byte);
    inline bool append(unsigned int asciiChar);

    // private fields
    //
    bool _bracketedPaste = false;
    bool _generateFocusEvents = false;
    std::optional<MouseProtocol> _mouseProtocol = std::nullopt;
    bool _passiveMouseTracking = false;
    MouseTransport _mouseTransport = MouseTransport::Default;
    MouseWheelMode _mouseWheelMode = MouseWheelMode::Default;
    Sequence _pendingSequence {};
    int _consumedBytes {};

    std::set<MouseButton> _currentlyPressedMouseButtons {};
    CellLocation _currentMousePosition {}; // current mouse position
    StandardKeyboardInputGenerator _keyboardInputGenerator {};
};

inline std::string to_string(InputGenerator::MouseEventType value)
{
    switch (value)
    {
        case InputGenerator::MouseEventType::Press: return "Press";
        case InputGenerator::MouseEventType::Drag: return "Drag";
        case InputGenerator::MouseEventType::Release: return "Release";
    }
    return "???";
}

} // namespace vtbackend

// {{{ fmtlib custom formatter support

template <>
struct fmt::formatter<vtbackend::KeyboardEventType>: formatter<std::string_view>
{
    auto format(vtbackend::KeyboardEventType value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::KeyboardEventType::Press: name = "Press"; break;
            case vtbackend::KeyboardEventType::Repeat: name = "Repeat"; break;
            case vtbackend::KeyboardEventType::Release: name = "Release"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::MouseProtocol>: formatter<std::string_view>
{
    auto format(vtbackend::MouseProtocol value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::MouseProtocol::X10: name = "X10"; break;
            case vtbackend::MouseProtocol::HighlightTracking: name = "HighlightTracking"; break;
            case vtbackend::MouseProtocol::ButtonTracking: name = "ButtonTracking"; break;
            case vtbackend::MouseProtocol::NormalTracking: name = "NormalTracking"; break;
            case vtbackend::MouseProtocol::AnyEventTracking: name = "AnyEventTracking"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::Modifier>: formatter<std::string>
{
    auto format(vtbackend::Modifier modifier, format_context& ctx) -> format_context::iterator
    {
        std::string s;
        auto const advance = [&](bool cond, std::string_view text) {
            if (!cond)
                return;
            if (!s.empty())
                s += ',';
            s += text;
        };
        advance(modifier.alt(), "Alt");
        advance(modifier.shift(), "Shift");
        advance(modifier.control(), "Control");
        advance(modifier.meta(), "Meta");
        if (s.empty())
            s = "None";
        return formatter<std::string>::format(s, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::InputGenerator::MouseWheelMode>: formatter<std::string_view>
{
    auto format(vtbackend::InputGenerator::MouseWheelMode value, format_context& ctx)
        -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::InputGenerator::MouseWheelMode::Default: name = "Default"; break;
            case vtbackend::InputGenerator::MouseWheelMode::NormalCursorKeys:
                name = "NormalCursorKeys";
                break;
            case vtbackend::InputGenerator::MouseWheelMode::ApplicationCursorKeys:
                name = "ApplicationCursorKeys";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::KeyMode>: public formatter<std::string_view>
{
    auto format(vtbackend::KeyMode value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::KeyMode::Normal: name = "Normal"; break;
            case vtbackend::KeyMode::Application: name = "Application"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::MouseButton>: formatter<std::string_view>
{
    auto format(vtbackend::MouseButton value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::MouseButton::Left: name = "Left"; break;
            case vtbackend::MouseButton::Right: name = "Right"; break;
            case vtbackend::MouseButton::Middle: name = "Middle"; break;
            case vtbackend::MouseButton::Release: name = "Release"; break;
            case vtbackend::MouseButton::WheelUp: name = "WheelUp"; break;
            case vtbackend::MouseButton::WheelDown: name = "WheelDown"; break;
            case vtbackend::MouseButton::WheelLeft: name = "WheelLeft"; break;
            case vtbackend::MouseButton::WheelRight: name = "WheelRight"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::MouseTransport>: formatter<std::string_view>
{
    auto format(vtbackend::MouseTransport value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::MouseTransport::Default: name = "Default"; break;
            case vtbackend::MouseTransport::Extended: name = "Extended"; break;
            case vtbackend::MouseTransport::SGR: name = "SGR"; break;
            case vtbackend::MouseTransport::URXVT: name = "URXVT"; break;
            case vtbackend::MouseTransport::SGRPixels: name = "SGR-Pixels"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};

template <>
struct fmt::formatter<vtbackend::Key>: formatter<std::string_view>
{
    auto format(vtbackend::Key value, format_context& ctx) -> format_context::iterator
    {
        string_view name;
        switch (value)
        {
            case vtbackend::Key::F1: name = "F1"; break;
            case vtbackend::Key::F2: name = "F2"; break;
            case vtbackend::Key::F3: name = "F3"; break;
            case vtbackend::Key::F4: name = "F4"; break;
            case vtbackend::Key::F5: name = "F5"; break;
            case vtbackend::Key::F6: name = "F6"; break;
            case vtbackend::Key::F7: name = "F7"; break;
            case vtbackend::Key::F8: name = "F8"; break;
            case vtbackend::Key::F9: name = "F9"; break;
            case vtbackend::Key::F10: name = "F10"; break;
            case vtbackend::Key::F11: name = "F11"; break;
            case vtbackend::Key::F12: name = "F12"; break;
            case vtbackend::Key::F13: name = "F13"; break;
            case vtbackend::Key::F14: name = "F14"; break;
            case vtbackend::Key::F15: name = "F15"; break;
            case vtbackend::Key::F16: name = "F16"; break;
            case vtbackend::Key::F17: name = "F17"; break;
            case vtbackend::Key::F18: name = "F18"; break;
            case vtbackend::Key::F19: name = "F19"; break;
            case vtbackend::Key::F20: name = "F20"; break;
            case vtbackend::Key::F21: name = "F21"; break;
            case vtbackend::Key::F22: name = "F22"; break;
            case vtbackend::Key::F23: name = "F23"; break;
            case vtbackend::Key::F24: name = "F24"; break;
            case vtbackend::Key::F25: name = "F25"; break;
            case vtbackend::Key::F26: name = "F26"; break;
            case vtbackend::Key::F27: name = "F27"; break;
            case vtbackend::Key::F28: name = "F28"; break;
            case vtbackend::Key::F29: name = "F29"; break;
            case vtbackend::Key::F30: name = "F30"; break;
            case vtbackend::Key::F31: name = "F31"; break;
            case vtbackend::Key::F32: name = "F32"; break;
            case vtbackend::Key::F33: name = "F33"; break;
            case vtbackend::Key::F34: name = "F34"; break;
            case vtbackend::Key::F35: name = "F35"; break;
            case vtbackend::Key::Escape: name = "Escape"; break;
            case vtbackend::Key::Enter: name = "Enter"; break;
            case vtbackend::Key::Tab: name = "Tab"; break;
            case vtbackend::Key::Backspace: name = "Backspace"; break;
            case vtbackend::Key::DownArrow: name = "DownArrow"; break;
            case vtbackend::Key::LeftArrow: name = "LeftArrow"; break;
            case vtbackend::Key::RightArrow: name = "RightArrow"; break;
            case vtbackend::Key::UpArrow: name = "UpArrow"; break;
            case vtbackend::Key::Insert: name = "Insert"; break;
            case vtbackend::Key::Delete: name = "Delete"; break;
            case vtbackend::Key::Home: name = "Home"; break;
            case vtbackend::Key::End: name = "End"; break;
            case vtbackend::Key::PageUp: name = "PageUp"; break;
            case vtbackend::Key::PageDown: name = "PageDown"; break;
            case vtbackend::Key::MediaPlay: name = "MediaPlay"; break;
            case vtbackend::Key::MediaStop: name = "MediaStop"; break;
            case vtbackend::Key::MediaPrevious: name = "MediaPrevious"; break;
            case vtbackend::Key::MediaNext: name = "MediaNext"; break;
            case vtbackend::Key::MediaPause: name = "MediaPause"; break;
            case vtbackend::Key::MediaTogglePlayPause: name = "MediaTogglePlayPause"; break;
            case vtbackend::Key::VolumeDown: name = "VolumeDown"; break;
            case vtbackend::Key::VolumeUp: name = "VolumeUp"; break;
            case vtbackend::Key::VolumeMute: name = "VolumeMute"; break;
            case vtbackend::Key::LeftShift: name = "LeftShift"; break;
            case vtbackend::Key::RightShift: name = "RightShift"; break;
            case vtbackend::Key::LeftControl: name = "LeftControl"; break;
            case vtbackend::Key::RightControl: name = "RightControl"; break;
            case vtbackend::Key::LeftAlt: name = "LeftAlt"; break;
            case vtbackend::Key::RightAlt: name = "RightAlt"; break;
            case vtbackend::Key::LeftSuper: name = "LeftSuper"; break;
            case vtbackend::Key::RightSuper: name = "RightSuper"; break;
            case vtbackend::Key::LeftHyper: name = "LeftHyper"; break;
            case vtbackend::Key::RightHyper: name = "RightHyper"; break;
            case vtbackend::Key::LeftMeta: name = "LeftMeta"; break;
            case vtbackend::Key::RightMeta: name = "RightMeta"; break;
            case vtbackend::Key::IsoLevel3Shift: name = "IsoLevel3Shift"; break;
            case vtbackend::Key::IsoLevel5Shift: name = "IsoLevel5Shift"; break;
            case vtbackend::Key::CapsLock: name = "CapsLock"; break;
            case vtbackend::Key::ScrollLock: name = "ScrollLock"; break;
            case vtbackend::Key::NumLock: name = "NumLock"; break;
            case vtbackend::Key::PrintScreen: name = "PrintScreen"; break;
            case vtbackend::Key::Pause: name = "Pause"; break;
            case vtbackend::Key::Menu: name = "Menu"; break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
// }}}
