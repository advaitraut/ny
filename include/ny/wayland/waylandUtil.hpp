#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <ny/mouse.hpp>
#include <ny/keyboard.hpp>
#include <ny/cursor.hpp>
#include <ny/backend.hpp>
#include <ny/window.hpp>
#include <ny/windowEvents.hpp>

#include <wayland-client-protocol.h>

#include <iostream>


namespace ny
{

namespace wayland
{

//frameEvent////////////////////////////////////
const unsigned int frameEvent = 11;

class waylandFrameEvent : public contextEvent
{
public:
    waylandFrameEvent() : contextEvent(Wayland, frameEvent) {};
};


//buffer//////////////////////////////////////////////////////////////
class shmBuffer
{
protected:
    static const unsigned int defaultSize_ = 1; //10MB

    vec2ui size_;
    wl_buffer* buffer_;
    wl_shm_pool* pool_;
    void* data_;

    void create();
    void destroy();

public:
    shmBuffer(vec2ui size, bufferFormat form = bufferFormat::argb8888);
    ~shmBuffer();

    const bufferFormat format;

    vec2ui getSize() const { return size_; }
    unsigned int getAbsSize() const { return size_.x * getBufferFormatSize(format) * size_.y; }
    wl_buffer* getWlBuffer() const { return buffer_; }
    void* getData(){ return data_; }

    void setSize(const vec2ui& size);
};

//serverCallback/////////////////
class serverCallback
{
protected:
    friend void callbackDone(void*, struct wl_callback*, uint32_t);
    void done(wl_callback*, unsigned int data);

    callback<void(wl_callback*, unsigned int)> callback_;
public:
    serverCallback(wl_callback* callback);

    connection& add(std::function<void(wl_callback*, unsigned int)> func);
    connection& add(std::function<void(unsigned int)> func);
    connection& add(std::function<void()> func);
};

//output
class output
{

friend void outputGeometry(void*, wl_output*, int, int, int, int, int, const char*, const char*, int);
friend void outputMode(void*, wl_output*, unsigned int, int, int, int);
friend void outputDone(void*, wl_output*);
friend void outputScale(void*, wl_output*, int);

protected:
    wl_output* wlOutput_ = nullptr;

    vec2i position_;
    vec2i size_;
    vec2i physicalSize_;

    int subpixel_;
    int refreshRate_;

    unsigned int flags_;

    std::string make_;
    std::string model_;

    int transform_;
    int scale_;

public:
    output(wl_output* outp);
    ~output();

    vec2i getPosition() const { return position_; }
    vec2i getSize() const { return size_; }
    vec2i getPhysicalSize() const { return physicalSize_; }

    int getSubpixel() const { return subpixel_; }
    int getRefreshRate() const { return refreshRate_; }

    unsigned int getFlags() const { return flags_; }

    std::string getMake() const { return make_; }
    std::string getModel() const { return model_; }

    int getTransform() const { return transform_; }
    int getScale() const { return scale_; }

    wl_output* getWlOutput() const { return wlOutput_; }
};

}//wayland

/////////////////////////////////////////////////////////////////////////
//utils convert function///
mouse::button waylandToButton(unsigned int id);
keyboard::key waylandToKey(unsigned int id);

std::string cursorToWayland(cursorType c);
cursorType waylandToCursor(std::string id);

int bufferFormatToWayland(bufferFormat format);
bufferFormat waylandToBufferFormat(unsigned int wlFormat);

}