#include <ny/backend/x11/input.hpp>
#include <ny/backend/x11/appContext.hpp>
#include <ny/backend/x11/windowContext.hpp>
#include <ny/base/log.hpp>

#include <xcb/xcb.h>

//the xkb header is not very c++ friendly...
#define explicit explicit_
#include <xcb/xkb.h>
#include <xkbcommon/xkbcommon-x11.h>
#undef explicit

namespace ny
{

//Mouse
nytl::Vec2ui X11MouseContext::position() const
{
	if(!over_) return {};
	auto cookie = xcb_query_pointer(appContext_.xConnection(), over_->xWindow());
	auto reply = xcb_query_pointer_reply(appContext_.xConnection(), cookie, nullptr);

	return nytl::Vec2ui(reply->win_x, reply->win_y);
}

bool X11MouseContext::pressed(MouseButton button) const
{
	return buttonStates_[static_cast<unsigned char>(button)];
}

WindowContext* X11MouseContext::over() const
{
	return over_;
}

//Keyboard
X11KeyboardContext::X11KeyboardContext(X11AppContext& ac) : appContext_(ac)
{
	//TODO: possibility for backend without xkb (if extension not supported?)
	//more error checking required
	auto xconn = appContext_.xConnection();

	xkbContext_ = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if(!xkbContext_) throw std::runtime_error("ny::X11KC: failed to create xkb_context");

	std::uint16_t major, minor;
	auto ret = xkb_x11_setup_xkb_extension(xconn, XKB_X11_MIN_MAJOR_XKB_VERSION,
        XKB_X11_MIN_MINOR_XKB_VERSION, XKB_X11_SETUP_XKB_EXTENSION_NO_FLAGS,
        &major, &minor, &eventType_, nullptr);
	if(!ret) throw std::runtime_error("X11KC: Failed to setup xkb extension");
	log("ny:: xkb version ", major, ".", minor, " supported");

	auto devid = xkb_x11_get_core_keyboard_device_id(xconn);
	auto flags = XKB_KEYMAP_COMPILE_NO_FLAGS;
	xkbKeymap_ = xkb_x11_keymap_new_from_device(xkbContext_, xconn, devid, flags);
	xkbState_ =  xkb_x11_state_new_from_device(xkbKeymap_, xconn, devid);

	//event mask
	constexpr auto reqEvents = 
		XCB_XKB_EVENT_TYPE_NEW_KEYBOARD_NOTIFY |
        XCB_XKB_EVENT_TYPE_MAP_NOTIFY |
        XCB_XKB_EVENT_TYPE_STATE_NOTIFY;
	constexpr auto reqNknDetails = XCB_XKB_NKN_DETAIL_KEYCODES;
	constexpr auto reqMapParts = 
		XCB_XKB_MAP_PART_KEY_TYPES |
		XCB_XKB_MAP_PART_KEY_SYMS |
		XCB_XKB_MAP_PART_MODIFIER_MAP |
		XCB_XKB_MAP_PART_EXPLICIT_COMPONENTS |
		XCB_XKB_MAP_PART_KEY_ACTIONS |
		XCB_XKB_MAP_PART_VIRTUAL_MODS |
		XCB_XKB_MAP_PART_VIRTUAL_MOD_MAP;
	constexpr auto reqStateDetails = 
		XCB_XKB_STATE_PART_MODIFIER_BASE |
		XCB_XKB_STATE_PART_MODIFIER_LATCH |
		XCB_XKB_STATE_PART_MODIFIER_LOCK |
		XCB_XKB_STATE_PART_GROUP_BASE |
		XCB_XKB_STATE_PART_GROUP_LATCH |
		XCB_XKB_STATE_PART_GROUP_LOCK;

    xcb_xkb_select_events_details_t details {};
	details.affectNewKeyboard = reqNknDetails;
    details.newKeyboardDetails = reqNknDetails;
    details.affectState = reqStateDetails;
    details.stateDetails = reqStateDetails;
	
	auto cookie = xcb_xkb_select_events_aux_checked(xconn, devid, reqEvents, 0, 0, 
		reqMapParts, reqMapParts, &details);         

    auto error = xcb_request_check(xconn, cookie);
    if(error) 
	{
        free(error);
		auto msg = "X11KC: failed to select xkb events: " + std::to_string((int) error->error_code);
		throw std::runtime_error(msg);
    }
}

bool X11KeyboardContext::pressed(Key key) const
{
	return keyStates_[static_cast<unsigned char>(key)];
}

void X11KeyboardContext::processXkbEvent(xcb_generic_event_t& ev)
{
	union XkbEvent 
	{
		struct 
		{
			std::uint8_t response_type; 
			std::uint8_t xkbType; 
			std::uint16_t sequence; 
			xcb_timestamp_t time; 
			uint8_t deviceID; 
		} any; 

		xcb_xkb_new_keyboard_notify_event_t newKeyboard; 
		xcb_xkb_map_notify_event_t map; 
		xcb_xkb_state_notify_event_t state;
	};

	XkbEvent& xkbev = reinterpret_cast<XkbEvent&>(ev);
	switch(xkbev.any.xkbType)
	{
		case XCB_XKB_STATE_NOTIFY:
		{
			xkb_state_update_mask(xkbState_,
				xkbev.state.baseMods, 
				xkbev.state.latchedMods, 
				xkbev.state.lockedMods, 
				xkbev.state.baseGroup, 
				xkbev.state.latchedGroup, 
				xkbev.state.lockedGroup);	
			break;
		}

		default: break;
	}
}

bool X11KeyboardContext::updateKeymap()
{
	return true;
}

std::string X11KeyboardContext::xkbUnicode(std::uint8_t keycode)
{
	char buffer[6] {};
	xkb_state_key_get_utf8(xkbState_, keycode, buffer, 6);
	return buffer;
}

Key X11KeyboardContext::xkbKey(std::uint8_t keycode)
{
	// keycode += 0x1B;
	debug("keycode: ", (void*) (keycode));
	auto keysym = xkb_state_key_get_one_sym(xkbState_, keycode);
	debug("keysym: ", (void*) (keysym));
	debug("keyname: ", xkb_keymap_key_get_name(xkbKeymap_, keycode));
	return xkbToKey(keysym);
}
	
}
