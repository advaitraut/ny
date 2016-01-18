#pragma once

#include <ny/include.hpp>
#include <ny/app/event.hpp>

#include <bitset>

namespace ny
{

//keyboard
class Keyboard
{

friend class App;

public:
    enum class Key
    {
        none = -1,
        a = 0, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
        num1, num2, num3, num4, num5, num6, num7, num8, num9, num0, 
		numpad1, numpad2, numpad3, numpad4, numpad5, numpad6, numpad7, numpad8, numpad9, numpad0,
        f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, 
		f20, f21, f22, f23, f24,
        play, stop, next, previous, escape, comma, dot, sharp, plus, minus, tab, 
		leftctrl, rightctrl, leftsuper, rightsuper, leftshift, rightshift,
        space, enter, backspace, del, end, insert, pageUp, pageDown,  home,  back, left, up, 
		down, right, volumeup, volumedown, leftalt, rightalt, capsLock
    };

	enum class Modifier
	{
		caps = 1,
		shift = 4,
		alt = 8,
		num = 16
	};

protected:
    static std::bitset<255> states_;
	static Modifier modifier_;

protected:
    static void keyPressed(Key key, bool pressed);
	static void modifier(Modifier mod, bool active);

public:
    static bool keyPressed(Key k);
	static bool modifier(Modifier mod);
};

//events
namespace eventType
{
	constexpr unsigned int key = 7;
}

class KeyEvent : public EventBase<KeyEvent, eventType::key>
{
public:
	using EvBase::EvBase;

    bool pressed;
    Keyboard::Key key;
	std::string text;
};

}
