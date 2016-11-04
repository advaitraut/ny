#include <ny/backend/wayland/appContext.hpp>

#include <ny/backend/wayland/util.hpp>
#include <ny/backend/wayland/interfaces.hpp>
#include <ny/backend/wayland/windowContext.hpp>
#include <ny/backend/wayland/input.hpp>
#include <ny/backend/wayland/data.hpp>
#include <ny/backend/wayland/xdg-shell-client-protocol.h>

#include <ny/base/eventDispatcher.hpp>
#include <ny/base/loopControl.hpp>
#include <ny/base/log.hpp>

#ifdef NY_WithEGL
 #include <ny/backend/common/egl.hpp>
 #include <ny/backend/wayland/egl.hpp>
#endif //WithGL

#ifdef NY_WithVulkan
 #define VK_USE_PLATFORM_WAYLAND_KHR
 #include <ny/backend/wayland/vulkan.hpp>
 #include <vulkan/vulkan.h>
#endif //WithVulkan

#include <nytl/scope.hpp>

#include <wayland-client.h>
#include <wayland-cursor.h>
#include <wayland-client-protocol.h>

#include <poll.h>
#include <unistd.h>
#include <sys/eventfd.h>

#include <algorithm>

namespace ny
{

namespace
{

void callbackDestroy(void*, wl_callback* callback, unsigned int) 
{ 
	wl_callback_destroy(callback); 
}

const wl_callback_listener callbackDestroyListener { &callbackDestroy };

//LoopControl
class WaylandLoopControlImpl : public ny::LoopControlImpl
{
public:
	std::atomic<bool>* run;
	unsigned int evfd;

public:
	WaylandLoopControlImpl(std::atomic<bool>& prun, unsigned int evfd)
		: run(&prun), evfd(evfd)
	{
	}

	virtual void stop() override
	{
		run->store(0);
		std::int64_t v = 1;
		write(evfd, &v, 8);
	};
};

}

struct WaylandAppContext::Impl
{
#ifdef NY_WithEGL
	EglSetup eglSetup;
#endif //WithEGL
};

using namespace wayland;

WaylandAppContext::WaylandAppContext()
{
	impl_ = std::make_unique<Impl>();
    wlDisplay_ = wl_display_connect(nullptr);

    if(!wlDisplay_) throw std::runtime_error("ny::WaylandAC: could not connect to display");

    wlRegistry_ = wl_display_get_registry(wlDisplay_);
    wl_registry_add_listener(wlRegistry_, &globalRegistryListener, this);

    wl_display_dispatch(wlDisplay_);
    wl_display_roundtrip(wlDisplay_);

    //compositor added by registry Callback listener
	//note that if it is not there now it simply does not exist on the server since
	//we rountripped above
    if(!wlCompositor_) throw std::runtime_error("ny::WaylandAC: could not get compositor");
	eventfd_ = eventfd(0, EFD_NONBLOCK);

	if(wlSeat_ && wlDataManager_) dataDevice_ = std::make_unique<WaylandDataDevice>(*this);
}

WaylandAppContext::~WaylandAppContext()
{
	//we explicitly have to destroy/disconnect everything since wayland is plain c
	//note that additional (even RAII) members might have to be reset here too if there
	//destructor require the wayland display (or anything else) to be valid
	//therefor, we e.g. explicitly reset the egl unique ptrs
	impl_.reset();
	outputs_.clear();

	if(eventfd_) close(eventfd_);
	if(wlCursorTheme_) wl_cursor_theme_destroy(wlCursorTheme_);
	if(keyboardContext_) keyboardContext_.reset();
	if(mouseContext_) mouseContext_.reset();


	if(xdgShell_) xdg_shell_destroy(xdgShell_);
	if(wlShell_) wl_shell_destroy(wlShell_);
	if(wlSeat_) wl_seat_destroy(wlSeat_);
	if(wlDataManager_) wl_data_device_manager_destroy(wlDataManager_);
	if(wlShm_) wl_shm_destroy(wlShm_);
	if(wlSubcompositor_) wl_subcompositor_destroy(wlSubcompositor_);
	if(wlCompositor_) wl_compositor_destroy(wlCompositor_);

	if(wlRegistry_) wl_registry_destroy(wlRegistry_);
    if(wlDisplay_) wl_display_disconnect(wlDisplay_);
}

//TODO: exception safety!
bool WaylandAppContext::dispatchEvents()
{
	//dont use range-based for here, since they might insert new events
	for(auto i = 0u; i < pendingEvents_.size(); ++i) 
	{
		auto& e = pendingEvents_[i];
		if(e->handler) e->handler->handleEvent(*e);
	}

	pendingEvents_.clear();

	auto ret = wl_display_dispatch_pending(wlDisplay_);
	return ret != -1;
}

bool WaylandAppContext::dispatchLoop(LoopControl& control)
{
	auto guard = nytl::makeScopeGuard([&]{ control.impl_.reset(); });

	std::atomic<bool> run {true};
	control.impl_.reset(new WaylandLoopControlImpl(run, eventfd_));

	auto ret = 0;
	while(run && ret != -1)
	{
		//dont use range-based for here, since they might insert new events
		for(auto i = 0u; i < pendingEvents_.size(); ++i) 
		{
			auto& e = pendingEvents_[i];
			if(e->handler) e->handler->handleEvent(*e);
		}

		pendingEvents_.clear();

		ret = dispatchDisplay();
		if(ret == 0)
		{
			std::int64_t v;
			read(eventfd_, &v, 8);
		}
	}

	return ret != -1;
}

bool WaylandAppContext::threadedDispatchLoop(EventDispatcher& dispatcher, LoopControl& control)
{
	std::atomic<bool> run {true};
	control.impl_.reset(new WaylandLoopControlImpl(run, eventfd_));

	auto guard = nytl::makeScopeGuard([&]{
		control.impl_.reset();
	});

	//wake the loop up every time an event is dispatched from another thread
	//using the eventfd. See also dispatchDisplay() that polls for it (and the wl_display fd)
	auto conn = nytl::ConnectionGuard(dispatcher.onDispatch.add([&]{
		std::int64_t v = 1;
		write(eventfd_, &v, 8);
	}));


	auto ret = 0;
	while(run && ret != -1)
	{
		//dont use range-based for here, since they might insert new events
		for(auto i = 0u; i < pendingEvents_.size(); ++i) 
		{
			auto& e = pendingEvents_[i];
			if(e->handler) e->handler->handleEvent(*e);
		}

		pendingEvents_.clear();

		ret = dispatchDisplay();
		if(ret == 0)
		{
			std::int64_t v;
			read(eventfd_, &v, 8);
		}

		dispatcher.processEvents();
	}

	return ret != -1;
}

KeyboardContext* WaylandAppContext::keyboardContext()
{
	return keyboardContext_.get();
}

MouseContext* WaylandAppContext::mouseContext()
{
	return mouseContext_.get();
}

WindowContextPtr WaylandAppContext::createWindowContext(const WindowSettings& settings)
{
    WaylandWindowSettings waylandSettings;
    const auto* ws = dynamic_cast<const WaylandWindowSettings*>(&settings);

    if(ws) waylandSettings = *ws;
    else waylandSettings.WindowSettings::operator=(settings);

	auto contextType = settings.context;
	if(contextType == ContextType::vulkan)
	{
		#ifdef NY_WithVulkan
			return std::make_unique<WaylandVulkanWindowContext>(*this, waylandSettings);
		#else
			throw std::logic_error("ny::WaylandAC::createWC: ny built without vulkan support");
		#endif
	}
	else if(contextType == ContextType::gl)
	{
		#ifdef NY_WithGL
			if(!eglSetup()) throw std::runtime_error("ny::WaylandAC::createWC: cannot init egl");
			return std::make_unique<WaylandEglWindowContext>(*this, *eglSetup(), waylandSettings);
		#else
			throw std::logic_error("ny::WaylandAC::createWC: ny built without GL suppport");
		#endif
	}

	return std::make_unique<WaylandWindowContext>(*this, waylandSettings);
}

bool WaylandAppContext::clipboard(std::unique_ptr<DataSource>&& dataSource)
{
	auto src = new WaylandDataSource(*this, std::move(dataSource), false);
	wl_data_device_set_selection(&dataDevice_->wlDataDevice(), &src->wlDataSource(), 
		keyboardContext_->lastSerial());
	return true;
}

DataOffer* WaylandAppContext::clipboard()
{
	return dataDevice_->clipboardOffer();
}

bool WaylandAppContext::startDragDrop(std::unique_ptr<DataSource>&& dataSource)
{
	auto over = mouseContext_->over();
	if(!over) return false;

	//see WaylandDataSource documentation for a reason why <new> is used here.
	//this is not a leak!
	auto src = new WaylandDataSource(*this, std::move(dataSource), true);
	auto surf = &static_cast<WaylandWindowContext*>(over)->wlSurface();
	wl_data_device_start_drag(&dataDevice_->wlDataDevice(), &src->wlDataSource(), surf, nullptr, 
		mouseContext_->lastSerial());

	return true;
}

std::vector<const char*> WaylandAppContext::vulkanExtensions() const
{
	#ifdef NY_WithVulkan
		return {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
	#else
		return {};
	#endif
}

GlSetup* WaylandAppContext::glSetup() const
{
	return eglSetup();
}

EglSetup* WaylandAppContext::eglSetup() const
{
	#ifdef NY_WithEGL
		if(eglFailed_) return nullptr;

		if(!impl_->eglSetup.valid())
		{
			try
			{
				impl_->eglSetup = {static_cast<void*>(&wlDisplay())};
			}
			catch(const std::exception& error)
			{
				warning("WaylandAppContext::eglSetup: creating failed: ", error.what());
				eglFailed_ = true;
				impl_->eglSetup = {};
				return nullptr;
			}
		}

		return &impl_->eglSetup;

	#else
		return nullptr;

	#endif
}

nytl::Connection WaylandAppContext::fdCallback(int fd, unsigned int events, const FdCallback& func)
{
	return fdCallbacks_.add({fd, events, func});
}

int WaylandAppContext::dispatchDisplay()
{
	//In parts taken from wayland-client.c and modified to poll for the wayland fd as well as an 
	//eventfd. The wayland license:
	//
	// Copyright © 2008-2012 Kristian Høgsberg
	// Copyright © 2010-2012 Intel Corporation
	// 
	// Permission is hereby granted, free of charge, to any person obtaining
	// a copy of this software and associated documentation files (the
	// "Software"), to deal in the Software without restriction, including
	// without limitation the rights to use, copy, modify, merge, publish,
	// distribute, sublicense, and/or sell copies of the Software, and to
	// permit persons to whom the Software is furnished to do so, subject to
	// the following conditions:
	// 
	// The above copyright notice and this permission notice (including the
	// next paragraph) shall be included in all copies or substantial
	// portions of the Software.
	// 
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
	// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	// NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
	// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
	// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	// SOFTWARE.
	
	bool event = false;
	auto dpypoll = [&](int events){
		int ret;

		std::vector<pollfd> pfds;
		pfds.reserve(2 + fdCallbacks_.items.size());
		pfds.resize(2);
		
		//wl_display fd
		pfds[0].fd = wl_display_get_fd(wlDisplay_);
		pfds[0].events = events;

		//eventfd
		pfds[1].fd = eventfd_;
		pfds[1].events = POLLIN;

		//other fds
		for(auto& callback : fdCallbacks_.items)
		{
			pfds.emplace_back();
			pfds.back().fd = callback.fd;
			pfds.back().events = callback.events;
		}

		do ret = poll(pfds.data(), pfds.size(), -1);
		while(ret == -1 && errno == EINTR);

		//check if eventfd was triggered
		if(pfds[1].revents > POLLIN) event = true;

		//check for callback fds
		for(auto i = 0u; i < fdCallbacks_.items.size(); ++i)
		{
			auto& pfd = pfds[i + 2];
			if(pfd.revents == 0) continue;

			nytl::ConnectionRef connref(fdCallbacks_, fdCallbacks_.items[i].clID_);
			fdCallbacks_.items[i].callback(connref, pfd.fd, pfd.revents);
		}

		return ret;
	};

	int ret;
	if(wl_display_prepare_read(wlDisplay_) == -1)
		return wl_display_dispatch_pending(wlDisplay_);

	while(true) 
	{
		ret = wl_display_flush(wlDisplay_);

		if(ret != -1 || errno != EAGAIN)
			break;

		if(dpypoll(POLLOUT) == -1)
		{
			wl_display_cancel_read(wlDisplay_);
			return -1;
		}

		if(event)
		{
			wl_display_cancel_read(wlDisplay_);
			return 0;
		}
	}

	if(ret < 0 && errno != EPIPE) 
	{
		wl_display_cancel_read(wlDisplay_);
		return -1;
	}

	if(dpypoll(POLLIN) == -1) 
	{
		wl_display_cancel_read(wlDisplay_);
		return -1;
	}

	if(event) 
	{
		wl_display_cancel_read(wlDisplay_);
		return 0;
	}

	if(wl_display_read_events(wlDisplay_) == -1) return -1;
	return wl_display_dispatch_pending(wlDisplay_);
}

void WaylandAppContext::registryAdd(unsigned int id, const char* cinterface, unsigned int)
{
	std::string interface = cinterface;
    if(interface == "wl_compositor" && !wlCompositor_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_compositor_interface, 1);
        wlCompositor_ = {static_cast<wl_compositor*>(ptr), id};
    }
    else if(interface == "wl_shell" && !wlShell_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_shell_interface, 1);
        wlShell_ = {static_cast<wl_shell*>(ptr), id};
    }
    else if(interface == "wl_shm" && !wlShm_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_shm_interface, 1);
        wlShm_ = {static_cast<wl_shm*>(ptr), id};
        wl_shm_add_listener(wlShm_, &shmListener, this);

		//TODO
        wlCursorTheme_ = wl_cursor_theme_load("default", 32, wlShm_);
    }
    else if(interface == "wl_subcompositor" && !wlSubcompositor_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_subcompositor_interface, 1);
        wlSubcompositor_ = {static_cast<wl_subcompositor*>(ptr), id};
    }
    else if(interface == "wl_output")
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_output_interface, 2);
        outputs_.emplace_back(*this, *static_cast<wl_output*>(ptr), id);
    }
    else if(interface == "wl_data_device_manager" && !wlDataManager_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_data_device_manager_interface, 3);
        wlDataManager_ = {static_cast<wl_data_device_manager*>(ptr), id};
    }
    else if(interface == "wl_seat" && !wlSeat_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &wl_seat_interface, 5);
        wlSeat_ = {static_cast<wl_seat*>(ptr), id};
        wl_seat_add_listener(wlSeat_, &seatListener, this);
    }
    else if(interface == "xdg_shell" && !xdgShell_)
    {
		auto ptr = wl_registry_bind(wlRegistry_, id, &xdg_shell_interface, 5);
        xdgShell_ = {static_cast<xdg_shell*>(ptr), id};
        xdg_shell_add_listener(xdgShell_, &xdgShellListener, this);
    }
}

void WaylandAppContext::registryRemove(unsigned int id)
{
	//TODO: stop the application/main loop here.
	//TODO: check other globals here
	if(id == wlCompositor_.name)
	{
		wl_compositor_destroy(wlCompositor_);
		wlCompositor_ = {};
	}
	else if(id == wlSubcompositor_.name)
	{
		wl_subcompositor_destroy(wlSubcompositor_);
		wlSubcompositor_ = {};
	}
	else if(id == wlShell_.name)
	{
		wl_shell_destroy(wlShell_);
		wlShell_ = {};
	}
	else
	{
		outputs_.erase(std::remove_if(outputs_.begin(), outputs_.end(),
			[=](const Output& output){ return output.name() == id; }), outputs_.end());
	}
}

void WaylandAppContext::seatCapabilities(unsigned int caps)
{
	//TODO: some kind of notification or warning if no pointer/keyboard
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !mouseContext_)
    {
		mouseContext_ = std::make_unique<WaylandMouseContext>(*this, *wlSeat_.global);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && mouseContext_)
    {
		mouseContext_.reset();
    }

    if((caps & WL_SEAT_CAPABILITY_KEYBOARD) && !keyboardContext_)
    {
		keyboardContext_ = std::make_unique<WaylandKeyboardContext>(*this, *wlSeat_.global);
    }
    else if(!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && keyboardContext_)
    {
		keyboardContext_.reset();
    }
}

void WaylandAppContext::seatName(const char* name)
{
	seatName_ = name;
}

void WaylandAppContext::addShmFormat(unsigned int format)
{
    shmFormats_.push_back(format);
}

bool WaylandAppContext::shmFormatSupported(unsigned int wlShmFormat)
{
    for(auto format : shmFormats_) if(format == wlShmFormat) return true;
    return false;
}

void WaylandAppContext::dispatch(Event&& event)
{
	pendingEvents_.push_back(nytl::cloneMove(event));
}

WaylandWindowContext* WaylandAppContext::windowContext(wl_surface& surface) const
{
	auto data = wl_surface_get_user_data(&surface);
	return static_cast<WaylandWindowContext*>(data);
}

wl_pointer* WaylandAppContext::wlPointer() const
{
	if(!mouseContext_) return nullptr;
	return mouseContext_->wlPointer();
}

wl_keyboard* WaylandAppContext::wlKeyboard() const
{
	if(!keyboardContext_) return nullptr;
	return keyboardContext_->wlKeyboard();
}

}