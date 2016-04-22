#pragma once

#include <ny/backend/x11/include.hpp>
#include <ny/backend/windowContext.hpp>

typedef struct xcb_connection_t xcb_connection_t;

#include <cstdint>
#include <vector>

namespace ny
{

///Dummy delcaration to not include xcb_ewmh.h
///The xcb_ewmh_connection_t type cannot be forward declared since it is a unnamed
///struct typedef in the original xcb_ewmh header, which should not be included in a
///header file.
struct DummyEwmhConnection;

///Additional settings for a X11 Window.
class X11WindowSettings : public WindowSettings {};

///The X11 implementation of the WindowContext interface.
///Provides some extra functionality for x11.
///Tries to use xcb where possible, for some things (e.g. glx context) xlib is needed though.
class X11WindowContext : public WindowContext
{
protected:
	X11AppContext* appContext_ = nullptr;
	std::uint32_t xWindow_ = 0;
	std::uint32_t xVisualID_ = 0;

	///XXX Should they be stored?
	std::vector<std::uint32_t> states_;
    unsigned long mwmFuncHints_ = 0;
    unsigned long mwmDecoHints_ = 0;

protected:
	///Default Constructor only for derived classes that later call the create function.
	X11WindowContext() = default;

	///Creates the x11 window.
	///This extra function may be needed by derived drawType classes.
	void create(X11AppContext& ctx, const X11WindowSettings& settings);

	///Utility helper returning the xcbConnection of the app context.
	xcb_connection_t* xConnection() const;

	///Utility helper returning the ewmhConnection of the app context.
	DummyEwmhConnection* ewmhConnection() const;

	///The different drawType classes derived from this class may override this function to
	///select a custom visual for the window or query it in a different way connected with
	///more information. See the cairo or glx function overrides for examples.
	///Will automatically be called by the create function if the visualID member variable is
	///not set yet (since it is needed for window creation).
    virtual void initVisual();

public:
    X11WindowContext(X11AppContext& ctx, const X11WindowSettings& settings = {});
    virtual ~X11WindowContext();

    virtual void refresh() override;

    virtual DrawGuard draw() override;

    virtual void show() override;
    virtual void hide() override;

    virtual void size(const Vec2ui& size) override;
    virtual void position(const Vec2i& position) override;

    virtual void cursor(const Cursor& c) override;

    virtual void minSize(const Vec2ui& size) override;
    virtual void maxSize(const Vec2ui& size) override;

    virtual bool handleEvent(const Event& e) override;
	virtual NativeWindowHandle nativeHandle() const override;

	void droppable(const DataTypes&) override {};

    //toplevel window
    virtual void maximize() override;
    virtual void minimize() override;
    virtual void fullscreen() override;
    virtual void normalState() override;

    virtual void beginMove(const MouseButtonEvent* ev) override;
    virtual void beginResize(const MouseButtonEvent* ev, WindowEdges edges) override;

    virtual void title(const std::string& title) override;
	virtual void icon(const Image* img) override;
	virtual bool customDecorated() const override;

	virtual void addWindowHints(WindowHints hints) override;
	virtual void removeWindowHints(WindowHints hints) override;


    //x11-specific
	///Returns the associated X11AppContext.
	X11AppContext& appContext() const { return *appContext_; }

	///Returns the underlaying x11 window handle.
	std::uint32_t xWindow() const { return xWindow_; }

	///Queries the window size with a server request since it is not stored.
	Vec2ui querySize() const; 

	///Sets/unsets the override_redirect flag for the underlaying x11 window.
	///See some x11 manual for more information.
    void overrideRedirect(bool redirect);

	///Sets the window transient for some other window handle.
	///See some x11 manual for more information.
    void transientFor(std::uint32_t win);

	///Sets the window cursor to a given xCursorID.
    void cursor(unsigned int xCusrsorID);

	///Creates a request to raise the window.
    void raise();

	///Creates a request to lower the window.
    void lower();

	///Creates a request to bring focus to the window.
    void requestFocus();

	///Sets the motif deco and/or function hints for the window.
	///The hints are declared in ny/backend/x11/util.hpp.
	///Motif hints are outdated, so may not work on every compositor.
    void mwmHints(unsigned long deco, unsigned long func, bool d = true, bool f = true);

	///Returns the motif decoration hints for this window.
    unsigned long mwmDecorationHints() const;

	///Returns the motif function hints for this window.
    unsigned long mwmFunctionHints() const;

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void addStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void removeStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Adds the given state/states to the window.
	///Look at the ewmh specification for more information and allowed atoms.
	///The changed property is _NET_WM_STATE.
    void toggleStates(std::uint32_t state1, std::uint32_t state2 = 0);

	///Returns the states associated with this window.
	///For more information look in the ewmh specification for _NET_WM_STATE.
	std::vector<std::uint32_t> states() const { return states_; };

	///Reloads the stores window states. XXX: Needed? should they be stored?
    void refreshStates();

	///Sets the window type. 
	///For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
    void xWindowType(std::uint32_t type);

	///Returns the window type assocated with this window.
	///For more information look in the ewmh specification for _NET_WM_WINDOW_TYPE.
	std::uint32_t xWindowType();

	///Adds an allowed action for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    void addAllowedAction(std::uint32_t action); //only does something when custom handled

	///Removes an allowed action for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    void removeAllowedAction(std::uint32_t action);

	///Returns all allowed actions for the window.
	///For more information look in the ewmh specification for _NET_WM_ALLOWED_ACTIONS.
    std::vector<std::uint32_t> allowedActions() const;
};

}
