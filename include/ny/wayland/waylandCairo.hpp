#pragma once

#include <ny/wayland/waylandInclude.hpp>

#include <nyutil/nonCopyable.hpp>
#include <nyutil/vec.hpp>
#include <ny/cairo.hpp>

namespace ny
{

////
class waylandCairoDrawContext: public cairoDrawContext
{
protected:
    const waylandWindowContext& wc_;

    wayland::shmBuffer* buffer_[2] {nullptr, nullptr};
    unsigned int frontID_ {0};

    cairo_surface_t* cairoBackSurface_ {nullptr};
    cairo_t* cairoBackCR_ {nullptr};

    wayland::shmBuffer* frontBuffer() const { return buffer_[frontID_]; }
    wayland::shmBuffer* backBuffer() const { return buffer_[frontID_^1]; }

public:
    waylandCairoDrawContext(const waylandWindowContext& wc);
    virtual ~waylandCairoDrawContext();

    void attach(const vec2i& pos = vec2i());
    void updateSize(const vec2ui& size);

    void swapBuffers();
    bool frontBufferUsed() const;
};


}
