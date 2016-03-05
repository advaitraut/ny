#pragma once

#include <ny/backend/winapi/include.hpp>
#include <ny/backend/windowContext.hpp>
#include <nytl/rect.hpp>

#include <windows.h>

namespace ny
{

///Extents the WindowSettings class with extra winapi-specific settings.
class WinapiWindowSettings : public WindowSettings {};

///WindowContext for winapi windows using the winapi backend on a windows OS.
class WinapiWindowContext : public WindowContext
{
protected:
	WinapiAppContext* appContext_;

    HWND handle_;
    WNDCLASSEX wndClass_;

protected:
	void initWindowClass(const WinapiWindowSettings& settings);
	void initWindow(const WinapiWindowSettings& settings);

public:
    WinapiWindowContext(WinapiAppContext& ctx, const WinapiWindowSettings& settings = {});
    virtual ~WinapiWindowContext();

    virtual void refresh() override;

    virtual DrawGuard draw() override;

    virtual void show() override;
    virtual void hide() override;

	virtual void droppable(const DataTypes&) override {}

    virtual void addWindowHints(WindowHints hints) override;
    virtual void removeWindowHints(WindowHints hints) override;

    virtual void size(const Vec2ui& size) override;
    virtual void position(const Vec2i& position) override;

    virtual void cursor(const Cursor& c) override;
    virtual bool handleEvent(const Event& e) override;

	virtual NativeWindowHandle nativeHandle() const override;

    //toplevel
    virtual void maximize() override;
    virtual void minimize() override;
    virtual void fullscreen() override;
    virtual void normalState() override;

    virtual void minSize(const Vec2ui& size) override {};
    virtual void maxSize(const Vec2ui& size) override {};

    virtual void beginMove(const MouseButtonEvent* ev) override {};
    virtual void beginResize(const MouseButtonEvent* ev, WindowEdges edges) override {};

	virtual bool customDecorated() const override { return 0; };

    virtual void icon(const Image* img) override {};
    virtual void title(const std::string& title) override {};

    //winapi specific
	WinapiAppContext& appContext() const { return *appContext_; }

    HWND handle() const { return handle_; }
    WNDCLASSEX windowClass() const { return wndClass_; }

	Rect2i extents() const;
};


}