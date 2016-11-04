#pragma once

#include <ny/include.hpp>
#include <ny/event.hpp>

#include <nytl/callback.hpp>

namespace ny
{

///Keyboard interface.
///Implemented by the different backends. Can be used to handle keycodes and their unicode
///representations correctly as well as receiving general information about the keyboard.
class KeyboardContext
{
public:
	///Returns whether the given key code is pressed.
	///This functions may be async to any received events and callbacks e.g. if checked
	///here a key might appear press although the application has not yet received (and maybe will
	///never receive) a matching KeyEvent.
	///\exception std::logic_error for invalid keycodes.
	virtual bool pressed(Keycode keycode) const = 0;

	///Converts the given Keycode to its default utf8 encoded characters.
	///If the Keycode cannot be represented using unicode (e.g. leftshift or escape) or it
	///is not recognized (e.g. because it is invalid) an empty string will be returned.
	///Usually the returned string should contain not more than one utf8 encoded unicode
	///value. Remember that std::string[0] does NOT return the first unicode code point of
	///a string but the first 8-bit char.
	///\warning This function cannot be used in exchange for the utf8 field of a KeyEvent.
	///It does only map a keycode to SOME (usually the "default") unicode string this
	///keycode could produce with the used keyboard layout and not to the one the
	///user expects to see regarding modifiers or dead key composition.
	virtual std::string utf8(Keycode) const = 0;

	///Returns the WindowContext that has the current keyboard focus or nullptr if there
	///is none.
	///This function may be async to any received events and callbacks i.e. the WindowContext
	///might be returned as the focused one here although it has not yet handled the focus
	///event.
	virtual WindowContext* focus() const = 0;

public:
	///Will be called every time a key status changes.
	// Callback<void(Keycode keycode, std::uint32_t utf32, bool pressed)> onKey;
	nytl::Callback<void(const KeyboardContext&, Keycode, std::string utf8, bool pressed)> onKey;

	///Will be called every time the keyboard focus changes.
	///Note that both parameters might be a nullptr
	///It is guaranteed that both parameters will have different values.
	nytl::Callback<void(const KeyboardContext&, WindowContext* prev, WindowContext* now)> onFocus;
};

namespace eventType
{
	constexpr auto key = 25u;
	constexpr auto focus = 26u;
}

///Event that will be sent everytime a key is pressed or released.
class KeyEvent : public EventBase<eventType::key, KeyEvent>
{
public:
	using EvBase::EvBase;

	bool pressed; //whether it was pressed or released
	Keycode keycode; //the raw keycode of the pressed key
	std::string unicode; //utf-8 encoded, keyboard state dependent
};

///Event that will be sent everytime a WindowContext gains or loses focus.
class FocusEvent : public EventBase<eventType::focus, FocusEvent>
{
public:
	using EvBase::EvBase;
	bool focus; //whether it gained focus
};


}
