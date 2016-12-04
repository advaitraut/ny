// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/x11/bufferSurface.hpp>
#include <ny/x11/appContext.hpp>
#include <ny/x11/util.hpp>
#include <ny/log.hpp>
#include <nytl/vecOps.hpp>

#include <xcb/xcb_image.h>
#include <xcb/shm.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <cstring>

//sources:
//https://github.com/freedesktop-unofficial-mirror/xcb__util-image/blob/master/image/xcb_image.c#L158
//http://xcb.pdx.freedesktop.narkive.com/0u3XxxGY/xcb-put-image
//https://github.com/freedesktop-unofficial-mirror/xcb__util-image/blob/master/image/xcb_image.h
//https://www.x.org/releases/X11R7.6/doc/xproto/x11protocol.html#requests:PutImage
//https://tronche.com/gui/x/xlib/graphics/XPutImage.html

//Note that this is way harder to implement on x than it should be due to possible absence of
//the shm extension and additional required format querying (bpp != depth).
//Implementation mainly oriented at xcb-util-image. xcb-util-image was not used directly since
//we want to avoid additional overhead at resizing and since using it would make
//the implementation (shm switch) even more complex.

//When not using the shm version, we should have to check not to exceed the maximum
//request length, but since this is usually pretty low (even with big requests extensions
//somewhere around 4MB on test machine) but putting the image onto the window works
//nontheless and xcb-util-image does not check/split as well, we don't check for
//request length exceeding.

namespace ny
{

X11BufferSurface::X11BufferSurface(X11WindowContext& wc) : windowContext_(&wc)
{
	gc_ = xcb_generate_id(&xConnection());
	std::uint32_t value[] = {0, 0};
	auto c = xcb_create_gc_checked(&xConnection(), gc_, wc.xWindow(), XCB_GC_FOREGROUND, value);
	windowContext().errorCategory().checkThrow(c, "ny::X11BufferSurface: create_gc");

	//query the format
	//this is needed because the xserver may need a different bpp for an image
	//with the depth of the window.
	//For 24-bit depth images the xserver often required 32 bpp.
	auto setup = xcb_get_setup(&xConnection());
	auto fmtit = xcb_setup_pixmap_formats(setup);
	auto fmtend = fmtit + xcb_setup_pixmap_formats_length(setup);
	xcb_format_t* fmt {};

	while(fmtit != fmtend)
	{
		if(fmtit->depth == windowContext().visualDepth())
		{
			fmt = fmtit;
			break;
		}

		++fmtit;
	}

	if(!fmt)
		throw std::runtime_error("ny::X11BufferSurface: couldn't query depth format bpp");

	format_ = visualToFormat(*windowContext().xVisualType(), fmt->bits_per_pixel);
	if(format_ == ImageDataFormat::none)
		throw std::runtime_error("ny::X11BufferSurface: couldn't parse visual format");

	//check if the server has shm suport
	//it is also implemented without shm but the performance might be worse
	auto cookie = xcb_shm_query_version(&xConnection());
	auto reply = xcb_shm_query_version_reply(&xConnection(), cookie, nullptr);

	shm_ = reply;
	if(reply) free(reply);

	if(!shm_) warning("ny::X11BufferSurface: shm server does not support shm extension");
}

X11BufferSurface::~X11BufferSurface()
{
	if(active_) warning("ny::~X11BufferSurface: there is still an active BufferGuard");
	if(gc_) xcb_free_gc(&xConnection(), gc_);

	if(shmseg_)
	{
		xcb_shm_detach(&xConnection(), shmseg_);
		shmdt(data_);
		shmctl(shmid_, IPC_RMID, 0);
	}
}

BufferGuard X11BufferSurface::buffer()
{
	if(active_)
		throw std::logic_error("ny::X11BufferSurface::buffer: there is already a BufferGuard");

	//check if resize is needed
	auto size = windowContext().size();
	auto bpp = imageDataFormatSize(format_); //bytes per pixel (XXX NOT bits!)
	auto newBytes = size.x * size.y * bpp; //the needed size
	if(newBytes > byteSize_)
	{
		//we alloc more than is really needed because this will
		//speed up (especially the shm version) resizes. We don't have to reallocated
		//every time the window is resized and redrawn for the cost of higher memory
		//consumption
		byteSize_ = newBytes * 4;

		if(shm_)
		{
			if(shmseg_)
			{
				xcb_shm_detach(&xConnection(), shmseg_);
				shmdt(data_);
				shmctl(shmid_, IPC_RMID, 0);
			}
			else
			{
				shmseg_ = xcb_generate_id(&xConnection());
			}


			shmid_ = shmget(IPC_PRIVATE, byteSize_, IPC_CREAT | 0777);
			data_ = static_cast<uint8_t*>(shmat(shmid_, 0, 0));
			shmseg_ = xcb_generate_id(&xConnection());
			xcb_shm_attach(&xConnection(), shmseg_, shmid_, 0);
		}
		else
		{
			ownedBuffer_ = std::make_unique<uint8_t[]>(byteSize_);
			data_ = ownedBuffer_.get();
		}
	}

	size_ = size;
	active_ = true;

	return {*this, {data_, {size_.x, size_.y}, format_, size_.x * bpp}};
}

void X11BufferSurface::apply(const BufferGuard&) noexcept
{
	if(!active_)
	{
		warning("ny::X11BufferSurface::apply: no currently active BufferGuard");
		return;
	}

	active_ = false;

	//we use the checked versions here since those function are very error prone due to
	//the rather complex depth/visual/bpp x system. We catch invalid x request here
	//directly.

	auto depth = windowContext().visualDepth();
	auto window = windowContext().xWindow();
	if(shm_)
	{
		auto cookie = xcb_shm_put_image_checked(&xConnection(), window, gc_, size_.x, size_.y,
			0, 0, size_.x, size_.y, 0, 0, depth, XCB_IMAGE_FORMAT_Z_PIXMAP, 0, shmseg_, 0);
		windowContext().errorCategory().checkWarn(cookie, "ny::X11BufferSurface: shm_put_image");
	}
	else
	{
		auto bpp = imageDataFormatSize(format_); //Bytes per pixel (XXX NOT bits!)
		auto length = size_.x * size_.y * bpp;

		auto cookie = xcb_put_image_checked(&xConnection(), XCB_IMAGE_FORMAT_Z_PIXMAP, window,
			gc_, size_.x, size_.y, 0, 0, 0, depth, length, data_);
		windowContext().errorCategory().checkWarn(cookie, "ny::X11BufferSurface: put_image");
	}
}

//X11BufferWindowContext
X11BufferWindowContext::X11BufferWindowContext(X11AppContext& ac, const X11WindowSettings& settings)
	: X11WindowContext(ac, settings), bufferSurface_(*this)
{
	if(settings.buffer.storeSurface) *settings.buffer.storeSurface = &bufferSurface_;
}

Surface X11BufferWindowContext::surface()
{
	return {bufferSurface_};
}

}
