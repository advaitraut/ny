// Copyright (c) 2016 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <nytl/stringParam.hpp>
#include <cstdint>

namespace ny
{

//Modeled after linux/input.h

//Note that the keycodes therefore directly match with the linux keycodes defined
//in linux/input.h. They are also similiarly named.
//Keys which names cannot be used as enum name are usually changed a bit.
//If there is no possiblity to change the name (like e.g. for number keys 0-9) they
//have a k (like key) prefix.

//Note that e.g. Keycode::a should never be treated as if the user pressed a keyboard
//button labeled 'A'. The keycodes are only a platform and device independent way of
//representing hardware keys that could be located on a standard us keyboard.
//For keyboards with other character sets (like e.g. japanese or russian keyboards)
//Keycode::a will be generated when the key that is at the location where the 'A' key
//would be on an us keyboard. But that does neither mean that the key that was pressed
//is labeled 'A' nor that the user does now expect to e.g. see an 'A' on screen.
//One should not try to e.g. give keycodes names and represent them to the user
//in any way e.g. as application controls (which seems to make sense first, but
//users may not have any idea how to press the 'W' key for moving forward
//when they have a japanese keyboard).

//Therefore the only use case for Keycodes is when handling special input keys such
//as escape or leftshift that cannot be represented using unicode.
enum class Keycode : unsigned int
{
	none = 0,
	escape,

	k1,
	k2,
	k3,
	k4,
	k5,
	k6,
	k7,
	k8,
	k9,
	k0,
	minus,
	equals,
	backspace,
	tab,

	q,
	w,
	e,
	r,
	t,
	y,
	u,
	i,
	o,
	p,
	leftbrace,
	rightbrace,
	enter,
	leftctrl,

	a,
	s,
	d,
	f,
	g,
	h,
	j,
	k,
	l,
	semicolon,
	apostrophe,
	grave,
	leftshift,
	backslash,

	z,
	x,
	c,
	v,
	b,
	n,
	m,
	comma,
	period,
	slash,
	rightshift,
	kpmultiply,
	leftalt,
	space,
	capslock,

	f1,
	f2,
	f3,
	f4,
	f5,
	f6,
	f7,
	f8,
	f9,
	f10,

	numlock,
	scrollock,
	kp7,
	kp8,
	kp9,
	kpminus,
	kp4,
	kp5,
	kp6,
	kpplus,
	kp1,
	kp2,
	kp3,
	kp0,
	kpperiod,

	zenkakuhankaku = 85,
	nonushash,
	f11,
	f12,
	ro,
	katakana,
	hiragana,
	henkan,
	katakanahiragana,
	muhenkan,
	kpjpcomma,
	kpenter,
	rightctrl,
	kpdivide,
	sysrq,
	rightalt,
	linefeed,
	home,
	up,
	pageup,
	left,
	right,
	end,
	down,
	pagedown,
	insert,
	del,
	macro,
	mute,
	volumedown,
	volumeup,
	power,
	kpequals,
	kpplusminus,
	pause,
	scale,
	kpcomma,
	hangeul,
	hanguel = hangeul,
	hanja,
	yen,
	leftmeta,
	rightmeta,
	compose,
	stop,
	again,
	props,
	undo,
	front,
	copy,
	open,
	paste,
	find,
	cut,
	help,
	menu,
	calc,
	setup,
	sleep,
	wakeup,
	file,
	sendfile,
	deletefile,
	xfer,
	prog1,
	prog2,
	www,
	msdos,
	coffee,
	screenlock = coffee,
	rotateDisplay,
	direction = rotateDisplay,
	cyclewindows,
	mail,
	bookmarks,
	computer,
	back,
	forward,
	closecd,
	ejectcd,
	ejectclosecd,
	nextsong,
	playpause,
	previoussong,
	stopcd,
	record,
	rewind,
	phone,
	iso,
	config,
	homepage,
	refresh,
	exit,
	move,
	edit,
	scrollup,
	scrolldown,
	kpleftparen,
	kprightparen,
	knew,
	redo,
	f13,
	f14,
	f15,
	f16,
	f17,
	f18,
	f19,
	f20,
	f21,
	f22,
	f23,
	f24,

	playcd = 200,
	pausecd,
	prog3,
	prog4,
	dashboard,
	suspend,
	close,
	play,
	fastforward,
	bassboost,
	print,
	hp,
	camera,
	sound,
	question,
	email,
	chat,
	search,
	connect,
	finance,
	sport,
	shop,
	alterase,
	cancel,
	brightnessdown,
	brightnessup,
	media,
	switchvideomode,
	kbdillumtoggle,
	kbdillumdown,
	kbdillumup,
	send,
	reply,
	forwardmail,
	save,
	documents,
	battery,
	bluetooth,
	wlan,
	uwb,
	unkown,
	videoNext,
	videoPrev,
	brightnessCycle,
	brightnessAuto,
	brightnessZero = brightnessAuto,
	displayOff,
	wwan,
	wimax = wwan,
	rfkill,
	micmute,

	//extra keycodes that are usually not used in any way and just here for completeness.
	ok = 352,
	select,
	kgoto,
	clear,
	power2,
	option,
	info,
	time,
	vendor,
	archive,
	program,
	channel,
	favorites,
	epg,
	pvr,
	mhp,
	language,
	title,
	subtitle,
	angle,
	zoom,
	mode,
	keyboard,
	screen,
	pc,
	tv,
	tv2,
	vcr,
	vcr2,
	sat,
	sat2,
	cd,
	tape,
	radio,
	tuner,
	player,
	text,
	dvd,
	aux,
	mp3,
	audio,
	video,
	directory,
	list,
	memo,
	calendar,
	red,
	green,
	yellow,
	blue,
	channelup,
	channeldown,
	first,
	last,
	ab,
	next,
	restart,
	slow,
	shuffle,
	kbreak,
	previous,
	digits,
	teen,
	twen,
	videophone,
	games,
	zoomin,
	zoomout,
	zoomreset,
	wordprocessor,
	editor,
	spreadsheet,
	graphicseditor,
	presentation,
	database,
	news,
	voicemail,
	addressbook,
	messenger,
	displaytoggle,
	brightnessToggle = displaytoggle,
	spellcheck,
	logoff,

	dollar,
	euro,

	frameback,
	frameforward,
	contextMenu,
	mediaRepeat,
	channelsup10,
	channelsdown10,
	images,

	delEol = 0x1c0,
	delEos,
	insLine,
	delLine,

	fn = 0x1d0,
	fnEsc,
	fnF1,
	fnF2,
	fnF3,
	fnF4,
	fnF5,
	fnF6,
	fnF7,
	fnF8,
	fnF9,
	fnF10,
	fnF11,
	fnF12,
	fn1,
	fn2,
	fnD,
	fnE,
	fnF,
	fnS,
	fnB,

	brlDot1 = 0x1f1,
	brlDot2,
	brlDot3,
	brlDot4,
	brlDot5,
	brlDot6,
	brlDot7,
	brlDot8,
	brlDot9,
	brlDot10,

	numeric0 = 0x200,
	numeric1,
	numeric2,
	numeric3,
	numeric4,
	numeric5,
	numeric6,
	numeric7,
	numeric8,
	numeric9,
	numericStar,
	numericPound,
	numericA,
	numericB,
	numericC,
	numericD,

	cameraFocus,
	wpsButton,

	touchpadToggle,
	touchpadOn,
	touchpadOff,

	cameraZoomin,
	cameraZoomout,
	cameraUp,
	cameraDown,
	cameraLeft,
	cameraRight,

	attendantOn,
	attendantOff,
	attendantToggle,
	lightsToggle,

	alsToggle = 0x230,

	buttonconfig = 0x240,
	taskmanager,
	journal,
	controlpanel,
	appselect,
	screensaver,
	voicecommand,

	brightnessMin = 0x250,
	brightnessMax,

	kbdinputassistPrev = 0x260,
	kbdinputassistNext,
	kbdinputassistPrevgroup,
	kbdinputassistNextgroup,
	kbdinputassistAccept,
	kbdinputassistCancel,

	rightUp,
	rightDown,
	leftUp,
	leftDown,

	rootMenu,
	mediaTopMenu,
	numeric11,
	numeric12,

	audioDesc,
	mode3d,
	nextFavorite,
	stopRecord,
	pauseRecord,
	vod,
	unmute,
	fastreverse,
	slowreverse,

	data = fastreverse,

	extra = 0x10000,
};

//linux/input.h
//If one of those static_asserts fails for your compiler, please report it to the ny maintainers.
static_assert(static_cast<unsigned int>(Keycode::micmute) == 248, "Wrong enum numbering!");
static_assert(static_cast<unsigned int>(Keycode::data) == 0x275, "Wrong enum numbering!");

///Returns the name of a keycode.
///Basically just transforms the enumeration value into a string.
///Returns an empty ("") name for Keycode::none or invalid keycodes.
///\sa keycodeFromName
const char* keycodeName(Keycode keycode);

///Constructs a keycode value from a given name string.
///Returns the correspondingly named Keycode value or Keycode::none if there is no such value.
///\sa keycodeName
Keycode keycodeFromName(nytl::StringParam name);

}
