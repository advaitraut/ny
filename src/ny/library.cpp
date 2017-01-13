// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#include <ny/library.hpp>
#include <ny/log.hpp>
#include <nytl/utf.hpp>

#ifdef _WIN32
	#define UNICODE
	#include <windows.h>
#else
	#include <dlfcn.h>
#endif

namespace ny {
namespace detail {

#ifdef WIN32 // Windows

void* dlopen(const char* name)
{
	auto wname = nytl::toUtf16(name);
	auto ret = ::LoadLibrary(reinterpret_cast<const wchar_t*>(wname.c_str()));

	if(!ret) {
		wchar_t buffer[512] = {};
		auto code = ::GetLastError();
		::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
			0, buffer, sizeof(buffer), nullptr);

		auto msg = nytl::toUtf8(reinterpret_cast<const char16_t*>(buffer));
		log("ny::Library: failed to open ", name, " with error ", code, ": ", msg);
	}

	return ret;
}

void dlclose(void* handle)
{
	::FreeLibrary(static_cast<HMODULE>(handle));
}

void* dlsym(void* handle, const char* name)
{
	return reinterpret_cast<void*>(::GetProcAddress(static_cast<HMODULE>(handle), name));
}

#else // UNIX

void* dlopen(const char* name)
{
	::dlerror(); //reset any error
	auto ret = ::dlopen(name, 0);
	if(!ret) log("ny::Library: failed to open ", name, ": ", dlerror());

	return ret;
}

void dlclose(void* handle)
{
	::dlclose(handle);
}

void* dlsym(void* handle, const char* name)
{
	return ::dlsym(handle, name);
}

#endif // Windows or Unix

} // namespace detail

Library::Library(nytl::StringParam name)
{
	handle_ = detail::dlopen(name);
}

Library::~Library()
{
	if(handle_) detail::dlclose(handle_);
}

Library::Library(Library&& other) noexcept : handle_(other.handle_)
{
	other.handle_ = nullptr;
}

Library& Library::operator=(Library&& other) noexcept
{
	if(handle_) detail::dlclose(handle_);
	handle_ = other.handle_;
	other.handle_ = nullptr;
	return *this;
}

void* Library::symbol(nytl::StringParam name) const
{
	if(!handle_) return nullptr;
	return detail::dlsym(handle_, name);
}

} // namespace ny
