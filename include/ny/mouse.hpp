#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>
#include <nyutil/vec.hpp>
#include <nyutil/callback.hpp>
#include <nyutil/compFunc.hpp>

#include <bitset>

namespace ny
{

//grab
struct mouseGrab
{
    eventHandler* grabber_ {nullptr};
    std::unique_ptr<event> event_ {nullptr};
};

//mouse
class mouse
{

friend class app;

public:
    enum button : int
    {
        none = -1,
        left = 0,
        right,
        middle,
        custom1,
        custom2,
        custom3,
        custom4,
    };

protected:
    static std::bitset<8> states;
    static vec2i position;

    static void buttonPressed(button b);
    static void buttonReleased(button b);
    static void wheelMoved(float value);
    static void setPosition(vec2i pos);

    static callback<void(vec2i)> moveCallback_;
    static callback<void(button, bool)> buttonCallback_;
    static callback<void(float)> wheelCallback_;

public:
    static bool isButtonPressed(button b);
    static vec2i getPosition();

    template<typename F> static std::unique_ptr<connection> onMove(F&& func) { return moveCallback_.add(func); }
    template<typename F> static std::unique_ptr<connection> onButton(F&& func) { return buttonCallback_.add(func); }
    template<typename F> static std::unique_ptr<connection> onWheel(F&& func) { return wheelCallback_.add(func); }
};

//events
namespace eventType
{
constexpr unsigned int mouseMove = 2;
constexpr unsigned int mouseButton = 3;
constexpr unsigned int mouseWheel = 4;
constexpr unsigned int mouseCross = 5;
}

class mouseButtonEvent : public eventBase<mouseButtonEvent, eventType::mouseButton>
{
public:
    mouseButtonEvent(eventHandler* h = nullptr, mouse::button but = mouse::none, bool prssd = 0, vec2i pos = vec2i(), eventData* d = nullptr)
        : evBase(h, d), pressed(prssd), button(but), position(pos) {};

    bool pressed;
    mouse::button button;
    vec2i position;
};

class mouseMoveEvent : public eventBase<mouseMoveEvent, eventType::mouseMove>
{
public:
    mouseMoveEvent(eventHandler* h = nullptr, vec2i pos = vec2i(), vec2i spos = vec2i(), vec2i del = vec2i(), eventData* d = nullptr)
        : evBase(h, d), position(pos), screenPosition(spos), delta(del) {};

    vec2i position;
    vec2i screenPosition;
    vec2i delta;
};

class mouseCrossEvent : public eventBase<mouseCrossEvent, eventType::mouseCross>
{
public:
    mouseCrossEvent(eventHandler* h = nullptr, bool e = 0, vec2i pos = vec2i(), eventData* d = nullptr) : evBase(h, d), entered(e), position(pos) {};

    bool entered;
    vec2i position;
};

class mouseWheelEvent : public eventBase<mouseWheelEvent, eventType::mouseWheel>
{
public:
    mouseWheelEvent(eventHandler* h = nullptr, float val = 0.f, eventData* d = nullptr) : evBase(h, d), value(val) {};

    float value;
};


}
