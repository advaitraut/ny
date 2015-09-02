#include <ny/eventHandler.hpp>

#include <ny/app.hpp>
#include <ny/error.hpp>
#include <ny/event.hpp>

#include <iostream>

namespace ny
{

eventHandler::eventHandler() : hierachyBase()
{
}

eventHandler::eventHandler(eventHandler& parent) : hierachyBase(parent)
{
}

bool eventHandler::processEvent(const event& event)
{
    if(event.type() == eventType::destroy)
    {
        destroy();
        return true;
    }
    else if(event.type() == eventType::reparent)
    {
        auto ev = event_cast<reparentEvent>(event);
        if(ev.newParent)
        {
            nyDebug("reparent");
            reparent(*ev.newParent); //else warning
        }
        return true;
    }

    return false;
}


}
