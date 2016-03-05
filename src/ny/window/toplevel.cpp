#include <ny/window/toplevel.hpp>
#include <ny/app/mouse.hpp>
#include <ny/backend/windowContext.hpp>

namespace ny
{

ToplevelWindow::ToplevelWindow(App& app, const Vec2ui& size, const std::string& title,
		const WindowSettings& settings) : Window(), title_(title)
{
	create(app, size, title, settings);
}

ToplevelWindow::~ToplevelWindow()
{
}

void ToplevelWindow::create(App& app, const Vec2ui& size, const std::string& title,
		const WindowSettings& settings)
{
	title_ = title;
	Window::create(app, size, settings);
}

void ToplevelWindow::maximizeHint(bool set)
{
	constexpr auto hint = WindowHints::maximize;

	if(set && !nytl::bitsSet(hints_, hint))
	{
		hints_ |= hint;
		windowContext()->addWindowHints(hint);
	}
	else if(!set && nytl::bitsSet(hints_, hint))
	{
		hints_ &= ~hint;
		windowContext()->removeWindowHints(hint);
	}
}

void ToplevelWindow::minimizeHint(bool set)
{
	constexpr auto hint = WindowHints::maximize;

	if(set && !nytl::bitsSet(hints_, hint))
	{
		hints_ |= hint;
		windowContext()->addWindowHints(hint);
	}
	else if(!set && nytl::bitsSet(hints_, hint))
	{
		hints_ &= ~hint;
		windowContext()->removeWindowHints(hint);
	}
}
void ToplevelWindow::resizeHint(bool set)
{
	constexpr auto hint = WindowHints::maximize;

	if(set && !nytl::bitsSet(hints_, hint))
	{
		hints_ |= hint;
		windowContext()->addWindowHints(hint);
	}
	else if(!set && nytl::bitsSet(hints_, hint))
	{
		hints_ &= ~hint;
		windowContext()->removeWindowHints(hint);
	}
}
void ToplevelWindow::closeHint(bool set)
{
	constexpr auto hint = WindowHints::maximize;

	if(set && !nytl::bitsSet(hints_, hint))
	{
		hints_ |= hint;
		windowContext()->addWindowHints(hint);
	}
	else if(!set && nytl::bitsSet(hints_, hint))
	{
		hints_ &= ~hint;
		windowContext()->removeWindowHints(hint);
	}
}

bool ToplevelWindow::customDecorated(bool set)
{
	constexpr auto hint = WindowHints::maximize;

	if(set && !nytl::bitsSet(hints_, hint))
	{
		hints_ |= hint;
		windowContext()->addWindowHints(hint);
	}
	else if(!set && nytl::bitsSet(hints_, hint))
	{
		hints_ &= ~hint;
		windowContext()->removeWindowHints(hint);
	}

	return (windowContext()->customDecorated() == set);
}

void ToplevelWindow::icon(const Image& icon)
{
	windowContext()->icon(&icon);
}

void ToplevelWindow::title(const std::string& title)
{
	title_ = title;
	windowContext()->title(title);
}

void ToplevelWindow::mouseMoveEvent(const MouseMoveEvent& ev)
{
    Window::mouseMoveEvent(ev);
    if(!customDecorated()) return;

	Cursor::Type t = Cursor::Type::grab;

    int length = 100;

    if(ev.position.y > (int) size_.y - length)
    {
        t = Cursor::Type::sizeBottom;
    }

    else if(ev.position.y < length)
    {
        t = Cursor::Type::sizeTop;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(t == Cursor::Type::sizeTop) t = Cursor::Type::sizeTopRight;
        else if(t == Cursor::Type::sizeBottom) t = Cursor::Type::sizeBottomRight;
        else t = Cursor::Type::sizeRight;
    }

    if(ev.position.x < length)
    {
        if(t == Cursor::Type::sizeTop) t = Cursor::Type::sizeTopLeft;
        else if(t == Cursor::Type::sizeBottom) t = Cursor::Type::sizeBottomLeft;
        else t = Cursor::Type::sizeLeft;
    }

    cursor_.nativeType(t);
	windowContext_->cursor(cursor_);
}

void ToplevelWindow::mouseButtonEvent(const MouseButtonEvent& ev)
{
    Window::mouseButtonEvent(ev);
    if(!customDecorated()) return;

    WindowEdges medge = WindowEdges::unknown;

    int length = 100;

    bool found = 0;

    if(ev.position.y > (int) size_.y - length)
    {
        medge = WindowEdges::bottom;

        found = 1;
    }

    else if(ev.position.y < length)
    {
        medge = WindowEdges::top;

        found = 1;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(medge == WindowEdges::top) medge = WindowEdges::topRight;
        else if(medge == WindowEdges::bottom) medge = WindowEdges::bottomRight;
        else medge = WindowEdges::right;

        found = 1;
    }

    if(ev.position.x < length)
    {
        if(medge == WindowEdges::top) medge = WindowEdges::topLeft;
        else if(medge == WindowEdges::bottom) medge = WindowEdges::bottomLeft;
        else medge = WindowEdges::left;

        found = 1;
    }

    if(found) windowContext()->beginResize(&ev, medge);
    else windowContext()->beginMove(&ev);
}

}