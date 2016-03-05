#include <ny/wayland/waylandAppContext.hpp>

#include <ny/wayland/waylandUtil.hpp>
#include <ny/wayland/waylandInterfaces.hpp>
#include <ny/wayland/waylandWindowContext.hpp>

#include <ny/error.hpp>
#include <ny/app.hpp>
#include <ny/image.hpp>

#include <nytl/misc.hpp>
#include <nytl/eventLoop.hpp>

#include <ny/wayland/xdg-shell-client-protocol.h>

#ifdef NY_WithEGL
#include <ny/wayland/waylandEgl.hpp>
#include <wayland-egl.h>
#endif //NY_WithGL

#include <cassert>

namespace ny
{

using namespace wayland;

//waylandAppContext//////////////////////////////////////////////////////////////////////////
waylandAppContext::waylandAppContext() : appContext()
{
    init();
}

waylandAppContext::~waylandAppContext()
{
    if(wlDisplay_)
    {
        wl_display_disconnect(wlDisplay_);
    }
}

void waylandAppContext::init()
{
    //init display
    wlDisplay_ = wl_display_connect(nullptr);

    if(!wlDisplay_)
    {
        throw std::runtime_error("could not connect to wayland display");
        return;
    }

    //init registry
    wl_registry *registry = wl_display_get_registry(wlDisplay_);
    wl_registry_add_listener(registry, &globalRegistryListener, this);

    wl_display_dispatch(wlDisplay_);
    wl_display_roundtrip(wlDisplay_);

    //compositor added by registry Callback listener
    if(!wlCompositor_)
    {
        throw std::runtime_error("could not find wayland compositor");
        return;
    }

    wl_Callback* syncer = wl_display_sync(wlDisplay_);
    wl_Callback_add_listener(syncer, &displaySyncListener, this);

    //wlCursorSurface_ = wl_compositor_create_surface(wlCompositor_);

    #ifdef NY_WithEGL
    egl_ = new waylandEGLAppContext(this);
    #endif // NY_WithGL

    eventSource_.reset(new pollEventSource(nyMainApp()->getEventLoop(), wl_display_get_fd(wlDisplay_), UV_READABLE | UV_WRITABLE, 1));
    eventSource_->onNotify.add(memberCallback(&waylandAppContext::dispatchCB, this));
}

void waylandAppContext::dispatchCB()
{
    wl_display_dispatch(wlDisplay_);
}

void waylandAppContext::startDataOffer(dataSource& source, const image& img, const window& w, const event* ev)
{

    if(!wlDataDevice_)
    {
        if(!wlSeat_ || !wlDataManager_)
            return;

        wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
    }

    if(!dataIconBuffer_)
    {
        dataIconBuffer_ = new shmBuffer(img.getSize());
    }
    else
    {
        dataIconBuffer_->setSize(img.getSize());
    }

    waylandWindowContext* wwc = dynamic_cast<waylandWindowContext*>(w.getWindowContext());
    dataSourceSurface_ = wwc->getWlSurface();

    unsigned char* buffData = (unsigned char*) dataIconBuffer_->getData();
    unsigned char imgData[img.getBufferSize()];

    img.getData(imgData);

    for(unsigned int i(0); i < img.getBufferSize(); i++)
    {
        buffData[i] = imgData[i];
        //buffData[i] = 0x66;
    }

    if(!dataIconSurface_)
    {
        dataIconSurface_ = wl_compositor_create_surface(wlCompositor_);

    }

	dataSource_ = &source;

	wlDataSource_ = wl_data_device_manager_create_data_source(wlDataManager_);
	wl_data_source_add_listener(wlDataSource_, &dataSourceListener, &source);

    std::vector<std::string> Vec = dataTypesToString(source.getPossibleTypes(), 1); //only mime
	for(size_t i(0); i < Vec.size(); i++)
        wl_data_source_offer(wlDataSource_, Vec[i].c_str());

    waylandEventData* data;
    if(!ev->data || !(data = dynamic_cast<waylandEventData*>(ev->data.get())))
    {
        return;
    }

    wl_data_device_start_drag(wlDataDevice_, wlDataSource_, dataSourceSurface_, dataIconSurface_, data->serial);

    wl_surface_attach(dataIconSurface_, dataIconBuffer_->getWlBuffer(), 0, 0);
    wl_surface_damage(dataIconSurface_, 0, 0, img.getSize().x, img.getSize().y);
    wl_surface_commit(dataIconSurface_);
}

bool waylandAppContext::isOffering() const
{
	return (dataSource_ != nullptr);
}

void waylandAppContext::endDataOffer()
{
    if(isOffering())
    {
        //TODO
    }
}

dataOffer* waylandAppContext::getClipboard()
{
    return nullptr;
}

void waylandAppContext::setClipboard(ny::dataSource& source, const event* ev)
{

}

void waylandAppContext::registryHandler(wl_registry* registry, unsigned int id, std::string interface, unsigned int version)
{
    if (interface == "wl_compositor")
    {
        wlCompositor_ = (wl_compositor*) wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    }

    else if (interface == "wl_shell")
    {
        wlShell_ = (wl_shell*) wl_registry_bind(registry, id, &wl_shell_interface, 1);
    }

    else if (interface == "wl_shm")
    {
        wlShm_ = (wl_shm*) wl_registry_bind(registry, id, &wl_shm_interface, 1);
        assert(wlShm_);
        wl_shm_add_listener(wlShm_, &shmListener, this);

        wlCursorTheme_ = wl_cursor_theme_load("default", 32, wlShm_);
    }

    else if(interface == "wl_subcompositor")
    {
        wlSubcompositor_ = (wl_subcompositor*) wl_registry_bind(registry, id, &wl_subcompositor_interface, 1);
    }

    else if(interface == "wl_output")
    {
        wl_output* out = (wl_output*) wl_registry_bind(registry, id, &wl_output_interface, 1);
        wlOutputs_.push_back(wayland::output(out));
    }

    else if(interface == "wl_seat")
    {
        wlSeat_ = (wl_seat*) wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(wlSeat_, &seatListener, this);

        if(wlDataManager_ && !wlDataDevice_)
        {
            wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
            wl_data_device_add_listener(wlDataDevice_, &dataDeviceListener, this);
        }
    }
/*
    else if(interface == "wl_data_device_manager")
    {
        wlDataManager_ = (wl_data_device_manager*) wl_registry_bind(registry, id, &wl_data_device_manager_interface, 1);
        if(wlSeat_ && !wlDataDevice_)
        {
            wlDataDevice_ = wl_data_device_manager_get_data_device(wlDataManager_, wlSeat_);
            wl_data_device_add_listener(wlDataDevice_, &dataDeviceListener, this);
        }
    }
*/
    else if(interface == "xdg_shell")
    {
        xdgShell_ = (xdg_shell*) wl_registry_bind(registry, id, &xdg_shell_interface, 1);
        xdg_shell_add_listener(xdgShell_, &xdgShellListener, this);
    }

}

void waylandAppContext::registryRemover(wl_registry* registry, unsigned int id)
{
    //TODO
}

void waylandAppContext::seatCapabilities(wl_seat* seat, unsigned int caps)
{
    if ((caps & WL_SEAT_CAPABILITY_POINTER))
    {
        wlPointer_ = wl_seat_get_pointer(wlSeat_);
        wl_pointer_add_listener(wlPointer_, &pointerListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && wlPointer_)
    {
        wl_pointer_destroy(wlPointer_);
        wlPointer_ = nullptr;
    }

    if ((caps & WL_SEAT_CAPABILITY_KEYBOARD))
    {
        wlKeyboard_ = wl_seat_get_keyboard(wlSeat_);
        wl_keyboard_add_listener(wlKeyboard_, &keyboardListener, this);
    }
    else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD) && wlKeyboard_)
    {
        wl_keyboard_destroy(wlKeyboard_);
        wlKeyboard_ = nullptr;
    }

}

void waylandAppContext::eventMouseMove(wl_pointer *pointer, unsigned int time, int sx, int sy)
{
    auto pos = Vec2i(wl_fixed_to_int(sx), wl_fixed_to_int(sy));
    auto delta = pos - mouse::getPosition();

    nyMainApp()->mouseMove(make_unique<mouseMoveEvent>(nullptr, pos, Vec2i(), delta));
}

void waylandAppContext::eventMouseEnterSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface, int sx, int sy)
{
    assert(surface);

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();
    auto pos = Vec2i(wl_fixed_to_int(sx), wl_fixed_to_int(sy));

    nyMainApp()->mouseCross(make_unique<mouseCrossEvent>(handler, 1, pos, new waylandEventData(serial)));
}

void waylandAppContext::eventMouseLeaveSurface(wl_pointer *pointer, unsigned int serial, wl_surface *surface)
{
    assert(surface);

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->mouseCross(make_unique<mouseCrossEvent>(handler, 0, Vec2i(), new waylandEventData(serial)));
}

void waylandAppContext::eventMouseButton(wl_pointer *wl_pointer, unsigned int serial, unsigned int time, unsigned int button, unsigned int state)
{
    auto buttn = waylandToButton(button);
    nyMainApp()->mouseButton(make_unique<mouseButtonEvent>(nullptr, buttn, static_cast<bool>(state), mouse::getPosition(), new waylandEventData(serial)));
}

void waylandAppContext::eventMouseAxis(wl_pointer *pointer, unsigned int time, unsigned int axis, int value)
{
    //todo
    nyMainApp()->sendEvent(make_unique<mouseWheelEvent>(nullptr, value));
}

//keyboard
void waylandAppContext::eventKeyboardKeymap(wl_keyboard *keyboard, unsigned int format, int fd, unsigned int size)
{
    //internal
}

void waylandAppContext::eventKeyboardEnterSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface, wl_array *keys)
{
    assert(surface);

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->windowFocus(make_unique<focusEvent>(handler, 1, new waylandEventData(serial)));
}

void waylandAppContext::eventKeyboardLeaveSurface(wl_keyboard *keyboard, unsigned int serial, wl_surface *surface)
{
    assert(surface);

    void* data = wl_surface_get_user_data(surface);
    if(!data) return;

    auto handler = &(static_cast<waylandWC*>(data))->getWindow();

    nyMainApp()->windowFocus(make_unique<focusEvent>(handler, 0, new waylandEventData(serial)));
}

void waylandAppContext::eventKeyboardKey(wl_keyboard *keyboard, unsigned int serial, unsigned int time, unsigned int key, unsigned int state)
{
    auto ky = waylandToKey(key);
    nyMainApp()->keyboardKey(make_unique<keyEvent>(nullptr, ky, static_cast<bool>(state), new waylandEventData(serial)));
}

void waylandAppContext::eventKeyboardModifiers(wl_keyboard *keyboard, unsigned int serial, unsigned int mods_depressed, unsigned int mods_latched, unsigned int mods_locked, unsigned int group)
{
    //Modifiers depressed %d, latched %d, locked %d, group %d\n", mods_depressed, mods_latched, mods_locked, group
}

void waylandAppContext::eventWindowResized(wl_shell_surface* shellSurface, unsigned int edges, unsigned int width, unsigned int height)
{
    if(!wl_shell_surface_get_user_data(shellSurface))
    {
        //warning
        return;
    }

    auto handler = &static_cast<waylandWindowContext*>(wl_shell_surface_get_user_data(shellSurface))->getWindow();
    auto size = Vec2ui(width, height);

    nyMainApp()->sendEvent(make_unique<sizeEvent>(handler, size, 1));
}

void waylandAppContext::shmFormat(wl_shm* shm, unsigned int format)
{
    supportedShm_.push_back(format);
}

bool waylandAppContext::bufferFormatSupported(unsigned int wlBufferType)
{
    //std::cout << wlBufferType << " " << supportedShm_.size() << std::endl;
    for(size_t i(0); i < supportedShm_.size(); i++)
    {
        if(supportedShm_[i] == wlBufferType)
            return 1;
    }

    return 0;
}

bool waylandAppContext::bufferFormatSupported(bufferFormat format)
{
    return bufferFormatSupported(bufferFormatToWayland(format));
}

void waylandAppContext::setCursor(std::string curse, unsigned int serial)
{
    //TODO
    wl_cursor* curs = wl_cursor_theme_get_cursor(wlCursorTheme_, curse.c_str());

    if(!curs || !wlCursorSurface_ || !wlCursorBuffer_) return;

/*
    wl_buffer* del = nullptr;
    if(cursorIsCustomImage_)
    {
        if(cursorImageBuffer_) delete cursorImageBuffer_;
    }
    else
    {
        del = wlCursorBuffer_;
    }
    cursorIsCustomImage_ = 0;
*/
    wl_cursor_image* image;

    image = curs->images[0];
    if(!image) return;

    wlCursorBuffer_ = wl_cursor_image_get_buffer(image);
    if(!wlCursorBuffer_) return;

    if(serial != 0) wl_pointer_set_cursor(wlPointer_, serial, wlCursorSurface_, image->hotspot_x, image->hotspot_y);
    wl_surface_attach(wlCursorSurface_, wlCursorBuffer_, 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, image->width, image->height);
    wl_surface_commit(wlCursorSurface_);

  //  if(del) wl_buffer_destroy(del);
}

void waylandAppContext::setCursor(const image* img, Vec2i hotspot, unsigned int serial)
{
    /*
    if(cursorIsCustomImage_)
    {
        if(cursorImageBuffer_) delete cursorImageBuffer_;
    }
    else
    {
        if(wlCursorBuffer_) wl_buffer_destroy(wlCursorBuffer_);
    }
    cursorIsCustomImage_ = 1;

    cursorImageBuffer_ = new wayland::shmBuffer(img->getSize(), bufferFormat::argb8888);
    //TODO: data
    if(serial != 0) wl_pointer_set_cursor(wlPointer_, serial, wlCursorSurface_, hotspot.x, hotspot.y);
    wl_surface_attach(wlCursorSurface_, cursorImageBuffer_->getWlBuffer(), 0, 0);
    wl_surface_damage(wlCursorSurface_, 0, 0, cursorImageBuffer_->getSize().x, cursorImageBuffer_->getSize().y);
    wl_surface_commit(wlCursorSurface_);
    */
}


}