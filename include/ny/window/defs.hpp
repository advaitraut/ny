#pragma once

#include <ny/include.hpp>
#include <nytl/vec.hpp>
#include <nytl/enumOps.hpp>
#include <nytl/clone.hpp>

#include <cstdint>
#include <bitset>
#include <memory>

namespace ny
{

///Toplevel window stsyle hints.
enum class WindowHints : unsigned int
{
    close = (1L << 1),
    maximize = (1L << 2),
    minimize = (1L << 3),
    resize = (1L << 4),
    customDecorated = (1L << 5),
    acceptDrop = (1L << 6),
    alwaysOnTop = (1L << 7),
    showInTaskbar = (1L << 8)
};

///Typesafe enum that specifies the edges of a window (e.g. for resizing).
///Note that e.g. (WindowEdge::top | WindowEdge::right) == (WindowEdge::topRight).
enum class WindowEdges : unsigned char
{
    unknown = 0,

    top = 1,
    right = 2,
    bottom = 4,
    left = 8,
    topRight = 3,
    bottomRight = 6,
    topLeft = 9,
    bottomLeft = 12,
};

///Result from a dialog.
enum class DialogResult
{
	none, ///not finished yet
	ok, ///Dialog finished as expected
	cancel ///Dialog was canceled
};

///Type of a natvie dialog.
enum class DialogType
{
	none, ///Default, no dialog type
	color, ///Returns a color
	path, ///Returns a file or directory path
	custom ///Returns some custom value
};

///DialogSettingsData
class DialogSettingsData : public Cloneable<DialogSettingsData> {};

///DialogSettings
class DialogSettings
{
public:
	DialogType type;
	std::unique_ptr<DialogSettingsData> data;

public:
	DialogSettings() = default;
	~DialogSettings() = default;
	
	DialogSettings(const DialogSettings& other) : type(other.type)
	{
		data = clone(*other.data);
	}

	DialogSettings& operator=(const DialogSettings& other)
	{
		type = other.type;
		data = clone(*other.data);
		return *this;
	}
};

///Typesafe enums that can be used for various settings with more control than just a bool.
///- must or mustNot: if the preference cannot be fulfilled an exception will be thrown or the
///  function will simply fail
///- should or shouldNot: if the preference cannot be fulfilled a warning will be raised but the
///  function will normally continue. Useful if there is method to check afterwards if preference
///  could be fulfilled.
///- dontCare: the function will decide what to do in this case. Usually the best choice.
enum class Preference : unsigned char
{
    must,
    should,
    dontCare = 0,
    shouldNot,
    mustNot
};

///Defines all possible native widgets that may be implemented on the specific backends.
///Note that none of them are guaranteed to exist, some backends to not have native widgets
///at all (linux backends).
enum class NativeWidgetType : unsigned char
{
	none = 0,

    button,
    textfield,
    text,
    checkbox,
    menuBar,
    toolbar,
    progressbar,
    dialog,
	dropdown
};

///Typesafe enums that represent the available drawing backends.
enum class DrawType : unsigned char
{
	dontCare = 0,
	none,
	gl,
	software,
	vulkan
};

///Typesafe enum for the current state of a toplevel window.
enum class ToplevelState : unsigned char
{
    unknown = 0,

    maximized,
    minimized,
    fullscreen,
    normal
};

///Class to reprsent the window handle of the underlaying window system api.
class NativeWindowHandle
{
protected:
	union
	{
		void* pointer_ = nullptr;
		std::uint64_t uint_;
	};

public:
	NativeWindowHandle(void* pointer = nullptr) : pointer_(pointer) {}
	NativeWindowHandle(std::uint64_t uint) : uint_(uint) {}

	void* pointer() const { return pointer_; }
	std::uint64_t uint() const { return uint_; }

	operator void*() const { return pointer_; }
	operator std::uint64_t() const { return uint_; }
	operator int() const { return uint_; }
	operator unsigned int() const { return uint_; }
};

///Settings for a Window.
///Backends usually have their own WindowSettings class derived from this one.
class WindowSettings
{
public:
    virtual ~WindowSettings() = default;

    DrawType draw = DrawType::dontCare;
	NativeWindowHandle nativeHandle = nullptr;
	NativeWindowHandle parent = nullptr;
	ToplevelState initState = ToplevelState::normal;
	DialogType dialogType = DialogType::none;
	Vec2ui size = {800, 500};
	Vec2i position = {~0, ~0};
	std::string title = "Some Random Window Title";
	bool initShown = true;

	NativeWidgetType nativeWidgetType = NativeWidgetType::none;
	DialogSettings dialogSettings;

	std::bitset<64> events = {1};
};

}

NYTL_ENABLE_ENUM_OPS(ny::WindowHints)
NYTL_ENABLE_ENUM_OPS(ny::WindowEdges)
