#include <ny/window/window.hpp>
#include <ny/window/events.hpp>
#include <ny/window/cursor.hpp>

#include <ny/app/event.hpp>
#include <ny/app/app.hpp>
#include <ny/backend/backend.hpp>
#include <ny/backend/windowContext.hpp>
#include <ny/draw/drawContext.hpp>
#include <ny/app/mouse.hpp>
#include <ny/app/keyboard.hpp>

#include <nytl/misc.hpp>
#include <ny/base/log.hpp>

#include <iostream>
#include <climits>

namespace ny
{

Window::Window()
{
}

Window::Window(const Vec2ui& size, const WindowSettings& settings) 
	: maxSize_(UINT_MAX, UINT_MAX)
{
    create(size, settings);
}

Window::~Window()
{
}

void Window::create(const Vec2ui& size, const WindowSettings& settings)
{
    size_ = size;

    if(!nyMainApp())
    {
		throw std::logic_error("Window::Window: Need an initialized app instance");
        return;
    }

	windowContext_ = nyMainApp()->backend().createWindowContext(*this, settings);
    hints_ |= windowContext_->additionalWindowHints();

	nyMainApp()->windowCreated();
}

void Window::close()
{
    onClose(*this);
    windowContext_.reset();

	nyMainApp()->windowClosed();
}

bool Window::handleEvent(const Event& ev)
{
    if(EventHandler::handleEvent(ev)) return 1;

    switch(ev.type())
    {
	case eventType::windowClose:
		closeEvent(static_cast<const CloseEvent&>(ev));
		return true;
    case eventType::mouseButton:
        mouseButtonEvent(static_cast<const MouseButtonEvent&>(ev));
        return true;
    case eventType::mouseMove:
        mouseMoveEvent(static_cast<const MouseMoveEvent&>(ev));
        return true;
    case eventType::mouseCross:
        mouseCrossEvent(static_cast<const MouseCrossEvent&>(ev));
        return true;
    case eventType::mouseWheel:
        mouseWheelEvent(static_cast<const MouseWheelEvent&>(ev));
        return true;
    case eventType::key:
        keyEvent(static_cast<const KeyEvent&>(ev));
        return true;
    case eventType::windowFocus:
        focusEvent(static_cast<const FocusEvent&>(ev));
        return true;
    case eventType::windowSize:
        sizeEvent(static_cast<const SizeEvent&>(ev));
        return true;
    case eventType::windowPosition:
        positionEvent(static_cast<const PositionEvent&>(ev));
        return true;
    case eventType::windowDraw:
        drawEvent(static_cast<const DrawEvent&>(ev));
        return true;
    case eventType::windowShow:
        showEvent(static_cast<const ShowEvent&>(ev));
        return true;
    case eventType::windowRefresh:
        refresh();
        return true;
    case eventType::windowContext:
        windowContext_->processEvent(static_cast<const ContextEvent&>(ev));
        return true;

    default:
        return false;
    }
}

void Window::refresh()
{
	if(!windowContext_) return;
    windowContext_->refresh();
}

void Window::size(const Vec2ui& size)
{
    size_ = size;
    windowContext_->size(size, 1);

    onResize(*this, size_);
}

void Window::position(const Vec2i& position)
{
    position_ = position;
    windowContext_->position(position_, 1);

	onMove(*this, position_);
}

void Window::move(const Vec2i& delta)
{
    position_ += delta;
    windowContext_->position(position_, 1);

    onMove(*this, position_);
}

void Window::show()
{
    windowContext_->show();
    shown_ = 1;
}

void Window::hide()
{
    windowContext_->hide();
    shown_ = false;
}

void Window::toggleShow()
{
    if(shown()) windowContext_->hide();
    else windowContext_->show();
}

void Window::maxSize(const Vec2ui& size)
{
    maxSize_ = size;
    windowContext_->maxSize(size);
}

void Window::minSize(const Vec2ui& size)
{
    minSize_ = size;
    windowContext_->minSize(size);
}

void Window::draw(DrawContext& dc)
{
	//dc.clear(Color::white);
    onDraw(*this, dc);
}

//event Callbacks
void Window::closeEvent(const CloseEvent&)
{
	close();
}
void Window::mouseMoveEvent(const MouseMoveEvent& e)
{
	onMouseMove(*this, e);
}
void Window::mouseCrossEvent(const MouseCrossEvent& e)
{
    mouseOver_ = e.entered;
	onMouseCross(*this, e);
}
void Window::mouseButtonEvent(const MouseButtonEvent& e)
{
    onMouseButton(*this, e);
}
void Window::mouseWheelEvent(const MouseWheelEvent& e)
{
    onMouseWheel(*this, e);
}
void Window::keyEvent(const KeyEvent& e)
{
    onKey(*this, e);
}
void Window::sizeEvent(const SizeEvent& e)
{
    size_ = e.size;
    windowContext_->size(size_, e.change);
    onResize.call(*this, size_);
}
void Window::positionEvent(const PositionEvent& e)
{
    position_ = e.position;
    windowContext_->position(position_, e.change);
    onMove(*this, position_);
}
void Window::drawEvent(const DrawEvent&)
{
    auto& dc = windowContext_->beginDraw();
    draw(dc);
    windowContext_->finishDraw();
}
void Window::showEvent(const ShowEvent& e)
{
    onShow(*this, e);
}
void Window::focusEvent(const FocusEvent& e)
{
    focus_ = e.gained;
    onFocus(*this, e);
}

void Window::cursor(const Cursor& curs)
{
    windowContext_->cursor(curs);
}

void Window::addWindowHints(unsigned long hints)
{
    hints &= ~hints_;
    hints_ |= hints;

    windowContext_->addWindowHints(hints);
}

void Window::removeWindowHints(unsigned int hints)
{
    hints &= hints_;
    hints_ &= ~hints;

    windowContext_->removeWindowHints(hints);
}

/*
void toplevelWindow::setIcon(const image* icon)
{
    if(!checkValid()) return;
    getWindowContext()->setIcon(icon);
}

void toplevelWindow::mouseButton(const mouseButtonEvent& ev)
{
    window::mouseButton(ev);

/*
    if(!isCustomResized() || !hasResizeHint())
        return;
* /

    windowEdge medge = windowEdge::Unknown;

    int length = 100;

    bool found = 0;

    if(ev.position.y > (int) size_.y - length)
    {
        medge = windowEdge::Bottom;

        found = 1;
    }

    else if(ev.position.y < length)
    {
        medge = windowEdge::Top;

        found = 1;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(medge == windowEdge::Top) medge = windowEdge::TopRight;
        else if(medge == windowEdge::Bottom) medge = windowEdge::BottomRight;
        else medge = windowEdge::Right;

        found = 1;
    }

    if(ev.position.x < length)
    {
        if(medge == windowEdge::Top) medge = windowEdge::TopLeft;
        else if(medge == windowEdge::Bottom) medge = windowEdge::BottomLeft;
        else medge = windowEdge::Left;

        found = 1;
    }

    if(found) getWindowContext()->beginResize(&ev, medge);
    else getWindowContext()->beginMove(&ev);
}

void toplevelWindow::mouseMove(const mouseMoveEvent& ev)
{
    window::mouseMove(ev);

/*
    if(!isCustomResized() || !hasResizeHint())
        return;
* /
    cursorType t = cursorType::grab;

    int length = 100;

    if(ev.position.y > (int) size_.y - length)
    {
        t = cursorType::sizeBottom;
    }

    else if(ev.position.y < length)
    {
        t = cursorType::sizeTop;
    }

    if(ev.position.x > (int) size_.x - length)
    {
        if(t == cursorType::sizeTop) t = cursorType::sizeTopRight;
        else if(t == cursorType::sizeBottom) t = cursorType::sizeBottomRight;
        else t = cursorType::sizeRight;
    }

    if(ev.position.x < length)
    {
        if(t == cursorType::sizeTop) t = cursorType::sizeTopLeft;
        else if(t == cursorType::sizeBottom) t = cursorType::sizeBottomLeft;
        else t = cursorType::sizeLeft;
    }

    cursor_.fromNativeType(t);
    windowContext_->updateCursor(nullptr);
}

void toplevelWindow::setTitle(const std::string& n)
{
    title_ = n;
    if(!checkValid()) return;

    getWindowContext()->setTitle(n);
};

/*
bool toplevelWindow::setCustomDecorated(bool set)
{
    hints_ |= windowHints::CustomDecorated;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomDecorated);
    else windowContext_->removeWindowHints(windowHints::CustomDecorated);

    return 1; //todo: this ans 2 following: corRect return, if it was successful
}

bool toplevelWindow::setCustomResized(bool set)
{
    hints_ |= windowHints::CustomResized;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomResized);
    else windowContext_->removeWindowHints(windowHints::CustomResized);

    return 1;
}

bool toplevelWindow::setCustomMoved(bool set)
{
    hints_ |= windowHints::CustomMoved;

    if(!checkValid()) return 0;

    if(set)windowContext_->addWindowHints(windowHints::CustomMoved);
    else windowContext_->removeWindowHints(windowHints::CustomMoved);

    return 1;
}
//*,///
void toplevelWindow::setMaximizeHint(bool hint)
{
    hints_ |= windowHints::Maximize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Maximize);
    else windowContext_->removeWindowHints(windowHints::Maximize);
}

void toplevelWindow::setMinimizeHint(bool hint)
{
    hints_ |= windowHints::Minimize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Minimize);
    else windowContext_->removeWindowHints(windowHints::Minimize);
}

void toplevelWindow::setResizeHint(bool hint)
{
    hints_ |= windowHints::Resize;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Resize);
    else windowContext_->removeWindowHints(windowHints::Resize);
}

void toplevelWindow::setMoveHint(bool hint)
{
    hints_ |= windowHints::Move;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Move);
    else windowContext_->removeWindowHints(windowHints::Move);
}

void toplevelWindow::setCloseHint(bool hint)
{
    hints_ |= windowHints::Close;

    if(!checkValid()) return;

    if(hint)windowContext_->addWindowHints(windowHints::Close);
    else windowContext_->removeWindowHints(windowHints::Close);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//childWindow///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
childWindow::childWindow() : window()
{
}

childWindow::childWindow(window& parent, Vec2i position, Vec2ui size, windowContextSettings settings) : window()
{
    create(parent, position, size, settings);
}

void childWindow::create(window& parent, Vec2i position, Vec2ui size, windowContextSettings settings)
{
    //hints_ |= windowHints::Child;
    window::create(parent, position, size, settings);
}

bool childWindow::isVirtual() const
{
    if(!checkValid()) return 0;
    return windowContext_->isVirtual();
}
*/

}
