#pragma once

#include <ny/include.hpp>
#include <nytl/clone.hpp>
#include <memory>

namespace ny
{

///All eventType constants should go in this namespace as constexpr unsigned int.
///ny-internal event ranges:
/// - 1 - 20 abstract
/// - 20 - 100 app/backend/input (mouse, keyboard, data)
/// - 100 - 200 window
/// - 1000 - 1100 backend wayland
/// - 1100 - 1200 backend x11
/// - 1200 - 1300 backend winapi
namespace eventType {};

///Classes derived from the EventData class are used by backends to put their custom
///information (like e.g. native event objects) in Event objects, since every Event stores
///an owned EventData pointer.
///Has a virtual destructor which makes RTTI possible.
class EventData
{
public:
	virtual ~EventData() = default;
};

///Event base class.
///Events can be used for (potentially) threadsafe communication without knowing the exact type
///of the event handler.
///Custom Event classes should rather be derived from EventBase, since the template
///makes implementing the virtual functions easier.
///All Events must be move constructible and are made cloneMovable i.e. one can
///move construct a std::unique_ptr<Event> from an Event object while preserving the type.
///Custom Events can carry their own information but every event has a EventData pointer
///as member, which can be used by backends (or custom senders) to send additional
///information eventType-agnostic.
class Event : public AbstractCloneMovable<Event>
{
public:
    EventHandler* handler {nullptr}; ///The EventHandler this event should be delivered to.
    std::unique_ptr<EventData> data {nullptr}; ///Custom backend data. Should not be changed.

public:
    Event(EventHandler* h = nullptr, EventData* d = nullptr) : handler(h), data(d) {};
    virtual ~Event() = default;

    Event(Event&& other) noexcept = default;
    Event& operator=(Event&& other) noexcept = default;

    virtual unsigned int type() const = 0;
    virtual bool overrideable() const { return 0; }
};

using EventPtr = std::unique_ptr<Event>;

///Custom Event classes should be derived from this class, which automatically implements
///the pure virtual type() function given on the Type template paramter and cares about
///correct cloneMovable deriving.
///This template can also be used for events typedefs, if the events carry no custom data,
///then just specify the correct type and use void as derived type.
///\tparam Type The type id of the event class, e.g. eventType::mouseMove.
///\tparam T The type deriving from this base class used for the CRTP. Pass void
///if EventBase is used for an custom event typedef instead of deriving from it.
///\tparam Override Specifies whether newer events objects of the same type can override
///older ones. E.g. for mouseMove events this is true, since if there is an old event
///and a new one it is ok if only the new one is sent, but e.g. for key events this is
///false since all key events should be sent.
template<unsigned int Type, typename T = void, bool Override = 0>
class EventBase : public DeriveCloneMovable<Event, T>
{
public:
	using EvBase = EventBase<Type, T,  Override>;
	using typename DeriveCloneMovable<Event, T>::CloneMovableBase;

public:
	using CloneMovableBase::CloneMovableBase;

    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

//Void type EventBase specialization makeing EventBase typedefs possible.
template<unsigned int Type, bool Override>
class EventBase<Type, void, Override>
	: public DeriveCloneMovable<Event, EventBase<Type, void, Override>>
{
public:
	using EvBase = EventBase<Type, void, Override>;
	using typename DeriveCloneMovable<Event, EventBase>::CloneMovableBase;

public:
	using CloneMovableBase::CloneMovableBase;

    virtual unsigned int type() const override final { return Type; }
    virtual bool overrideable() const override final { return Override; }
};

}