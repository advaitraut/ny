#include <ny/app/eventDispatcher.hpp>
#include <ny/base/eventHandler.hpp>
#include <ny/base/loopControl.hpp>

#include <ny/base/log.hpp>

namespace ny
{

struct DispatcherControlImpl : public LoopControlImpl
{
	std::atomic<bool>* stop_;
	std::condition_variable* cv_;

	DispatcherControlImpl(std::atomic<bool>& stop, std::condition_variable& cv)
		: stop_(&stop), cv_(&cv) {}
	virtual void stop() override { if(!stop_) return; stop_->store(1); cv_->notify_one(); }
};

//Default
void EventDispatcher::sendEvent(const Event& event)
{
	auto it = onEvent.find(event.type());
	if(it != onEvent.cend())
	{
		it->second(event);
	}

	if(event.handler) event.handler->handleEvent(event);
	else noEventHandler(event);
}

void EventDispatcher::noEventHandler(const Event& event) const
{
	sendWarning("ny::EventDispatcher: Received Event with no handler of type ", event.type());
}

//Threaded
ThreadedEventDispatcher::ThreadedEventDispatcher()
{
}

ThreadedEventDispatcher::~ThreadedEventDispatcher()
{
}

void ThreadedEventDispatcher::dispatchEvents()
{
    std::unique_lock<std::mutex> lck(eventMtx_);
	while(!events_.empty())
	{
        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        sendEvent(*ev);
		lck.lock();
	}
}

void ThreadedEventDispatcher::dispatchLoop(LoopControl& control)
{
	std::atomic<bool> stop {0};
	control.impl_ = std::make_unique<DispatcherControlImpl>(stop, eventCV_);
    std::unique_lock<std::mutex> lck(eventMtx_);

    while(1)
    {
		if(events_.empty())
		{
			//Just signal all stored promises since there cannot be any that are associated with
			//queued events, since there are none.
			for(auto& promise : promises_) promise.second.set_value();
			promises_.clear();

	        while(events_.empty() && !stop.load())
			{
				eventCV_.wait(lck);
			}
		}

        if(stop.load())
		{
			break;
		}

        auto ev = std::move(events_.front());
        events_.pop_front();

        lck.unlock();
        sendEvent(*ev);
		lck.lock();

		for(auto it = promises_.begin(); it < promises_.end();)
		{
			if(it->first == ev.get())
			{
				it->second.set_value();
				it = promises_.erase(it);
			}
			else
			{
				++it;
			}
		}
    }

	control.impl_.reset();
}

void ThreadedEventDispatcher::dispatch(EventPtr&& event)
{
    if(!event.get())
    {
		sendWarning("EventDispatcher::dispatch: invalid event");
        return;
    }

	if(!event->handler)
	{
		noEventHandler(*event);
		return;
	}

    {
		std::lock_guard<std::mutex> lck(eventMtx_);
        if(event->overrideable())
        {
            for(auto& stored : events_)
            {
                if(stored->type() == event->type())
                {
                    stored = std::move(event);
					break;
                }
            }
        }

		if(event) events_.emplace_back(std::move(event));
    }

    eventCV_.notify_one();
}

void ThreadedEventDispatcher::dispatch(const Event& event)
{
	dispatch(clone(event));
}

void ThreadedEventDispatcher::dispatch(Event&& event)
{
	dispatch(cloneMove(std::move(event)));
}

void ThreadedEventDispatcher::dispatchSync(EventPtr&& event)
{
	dispatch(std::move(event));
	auto fut = sync(); //XXX: may acually call sync after some additional events where queued.
	fut.wait();
}

void ThreadedEventDispatcher::dispatchSync(const Event& event)
{
	dispatchSync(clone(event));
}

void ThreadedEventDispatcher::dispatchSync(Event&& event)
{
	dispatchSync(cloneMove(std::move(event)));
}

std::future<void> ThreadedEventDispatcher::sync()
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	if(events_.empty())
	{
		std::promise<void> prom;
		prom.set_value();
		return prom.get_future();
	}

	promises_.emplace_back();
	promises_.back().first = events_.back().get();
	return promises_.back().second.get_future();
}

std::future<void> ThreadedEventDispatcher::waitIdle()
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	if(events_.empty())
	{
		std::promise<void> prom;
		prom.set_value();
		return prom.get_future();
	}

	promises_.emplace_back();
	promises_.back().first = nullptr;
	return promises_.back().second.get_future();
}

std::size_t ThreadedEventDispatcher::eventCount() const
{
	std::lock_guard<std::mutex> lck(eventMtx_);
	return events_.size();
}

}
