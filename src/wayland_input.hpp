#pragma once

#include "key_resolver.hpp"

#include <cstdint>
#include <optional>

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QRect>
#include <QString>

#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>

struct zwlr_virtual_pointer_manager_v1;
struct zwlr_virtual_pointer_v1;
struct zwp_virtual_keyboard_manager_v1;
struct zwp_virtual_keyboard_v1;

namespace hkcf {

class WaylandInput {
  public:
    struct XkbModifiers {
        std::uint32_t depressed = 0;
        std::uint32_t latched = 0;
        std::uint32_t locked = 0;
        std::uint32_t group = 0;

        [[nodiscard]] bool operator==(const XkbModifiers& other) const = default;
    };

    WaylandInput();
    ~WaylandInput();

    WaylandInput(const WaylandInput&) = delete;
    WaylandInput& operator=(const WaylandInput&) = delete;

    [[nodiscard]] bool ensureReady();
    [[nodiscard]] QString lastError() const;

    bool pointerMotion(double dx, double dy);
    bool pointerMotionAbsolute(double x, double y);
    bool pointerButton(std::uint32_t button, bool pressed);
    bool pointerAxis(double dx, double dy);
    bool pointerAxisDiscrete(std::uint32_t axis, int steps);
    bool keyboardKeycode(std::uint32_t keycode, bool pressed);
    bool keyboardKeysym(std::uint32_t keysym, bool pressed);
    [[nodiscard]] QRect logicalBounds() const;

    // The keymap the compositor uses for this seat, so that remote clients encode
    // keycodes the same way the compositor will decode them.
    [[nodiscard]] QByteArray keymapText();
    [[nodiscard]] XkbModifiers modifierState() const;

    static void keyboardKeymap(void* data, wl_keyboard* keyboard, std::uint32_t format, int32_t fd, std::uint32_t size);
    static void keyboardEnter(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface, wl_array* keys);
    static void keyboardLeave(void* data, wl_keyboard* keyboard, std::uint32_t serial, wl_surface* surface);
    static void keyboardKey(void* data, wl_keyboard* keyboard, std::uint32_t serial, std::uint32_t time, std::uint32_t key, std::uint32_t state);
    static void keyboardModifiers(void* data,
                                  wl_keyboard* keyboard,
                                  std::uint32_t serial,
                                  std::uint32_t depressed,
                                  std::uint32_t latched,
                                  std::uint32_t locked,
                                  std::uint32_t group);
    static void keyboardRepeatInfo(void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay);

    static void handleGlobal(void* data, wl_registry* registry, std::uint32_t name, const char* interface, std::uint32_t version);
    static void handleGlobalRemove(void* data, wl_registry* registry, std::uint32_t name);

    static void outputGeometry(void* data,
                               wl_output* output,
                               int32_t x,
                               int32_t y,
                               int32_t physicalWidth,
                               int32_t physicalHeight,
                               int32_t subpixel,
                               const char* make,
                               const char* model,
                               int32_t transform);
    static void outputMode(void* data, wl_output* output, std::uint32_t flags, int32_t width, int32_t height, int32_t refresh);
    static void outputDone(void* data, wl_output* output);
    static void outputScale(void* data, wl_output* output, int32_t factor);
    static void outputName(void* data, wl_output* output, const char* name);
    static void outputDescription(void* data, wl_output* output, const char* description);

  private:
    struct OutputInfo {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] bool connect();
    [[nodiscard]] bool createDevices();
    [[nodiscard]] bool sendKeyboardKeymap();
    [[nodiscard]] std::uint32_t timeMs() const;
    [[nodiscard]] QRect outputBounds() const;
    bool flush();
    void cleanup();
    void setError(const QString& error);

    void ensureKeymapState();
    bool sendModifiers();

    wl_display* m_display = nullptr;
    wl_registry* m_registry = nullptr;
    wl_seat* m_seat = nullptr;
    wl_keyboard* m_seatKeyboard = nullptr;
    wl_output* m_output = nullptr;
    zwlr_virtual_pointer_manager_v1* m_virtualPointerManager = nullptr;
    zwp_virtual_keyboard_manager_v1* m_virtualKeyboardManager = nullptr;
    zwlr_virtual_pointer_v1* m_pointer = nullptr;
    zwp_virtual_keyboard_v1* m_keyboard = nullptr;
    std::uint32_t m_pointerManagerVersion = 1;
    std::uint32_t m_keyboardManagerVersion = 1;
    OutputInfo m_outputInfo;
    QByteArray m_seatKeymapText;
    QByteArray m_fallbackKeymapText;
    xkb_context* m_xkbContext = nullptr;
    xkb_keymap* m_xkbKeymap = nullptr;
    xkb_state* m_xkbState = nullptr;
    XkbModifiers m_modifiers;
    QElapsedTimer m_timer;
    QString m_lastError;
    KeyResolver m_keyResolver;
    QHash<std::uint32_t, int> m_keysymShiftCounts;
    int m_shiftedKeysDown = 0;
};

} // namespace hkcf
