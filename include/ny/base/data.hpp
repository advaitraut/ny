#pragma once

#include <ny/include.hpp>
#include <ny/base/event.hpp>

#include <nytl/callback.hpp>

#include <vector>
#include <memory>

// #include <experimental/any>
// namespace std { using namespace experimental; }
// #include <any>
#include <ny/base/any.hpp>

namespace ny
{

//Adds a new event
namespace eventType
{
    constexpr auto dataOffer = 31u;
}

///This namespace holds constants for all data formats in which data from a DataSource/DataOffer
///may be represented.
///Applications or libraries using ny may specify their own dataTypes. Custom dataTypes
///are always represented with a std:vector<std::uint8_t> holding the serialized data.
namespace dataType
{
	constexpr auto none = 0u; //meta symbolic constant, dont actually use it.
	constexpr auto custom = 1u; //meta symbolic constant, dont actually use it.

	constexpr auto raw = 2u; //std:vector<std::uint8_t>, raw unspecified data buffer
	constexpr auto text = 3u; //std::string encoded utf8
    constexpr auto filePaths = 4u; //std::vector<c++17 ? std::path : std::string>
	constexpr auto image = 5u; //ny:Image
	constexpr auto timePoint = 6u; //ny(tl)::TimePoint
	constexpr auto timeDuration = 7u; //ny(tl)::TimeDuration
}

///Represents multiple data type formats in which certain data can be retrieved.
///Used by DataSource and DataOffer to signal in which types the data is available.
///The constant types used should be defined as constexpr std::uint8_t in the ny::dataType
///namespace. ny already provides the most common datatypes, applications can extent them with
///their own definitions starting with number 100.
///\sa dataTypes
class DataTypes
{
public:
    std::vector<unsigned int> types;

public:
    void add(unsigned int type);
    void remove(unsigned int type);
    bool contains(unsigned int type) const;
};

///The DataSource class is an interface implemented by the application to start drag and drop
///actions or copy data into the clipboard.
///The interface gives information about in which formats data can be represented and then
///provides the data for a given format.
class DataSource
{
public:
    virtual ~DataSource() = default;

	///Returns all supported dataTypes format constants as a DataTypes object.
	virtual DataTypes types() const = 0;

	///Returns a std::any holding the data in the given format.
	///The std::any must contain a object of the type specified at the dataType constant
	///declaration.
	virtual std::any data(unsigned int format) const = 0;
};

///Class that allows app to retrieve data from other apps
///The DataOffer interface is usually implemented by the backends and will be passed to the
///application either as result from a clipboard request or with a DataOfferEvent, if there
///was data dropped onto a window.
///It can then be used to determine the different data types in which the data can be represented
///or to retrieve the data in a supported format.
class DataOffer
{
public:
	// using DataFunction = CompFunc<void(DataOffer& off, unsigned int fmt, const std::any& data)>;
	using DataFunction = std::function<void(DataOffer& off, unsigned int fmt, const std::any& data)>;

public:
	///Will be called everytime a new format is signaled.
	Callback<bool(DataOffer& off, unsigned int fmt)> onFormat;

public:
	virtual ~DataOffer() = default;

	///Returns all currently known supported data representation formats.
	///Note that on some backends the supported types are queried async, therefore
	///this list might grow over time, add a function to the onFormat callback to
	///retrieve new supported types.
	virtual DataTypes types() const = 0;

	///Requests conversion of the data into the given format and registers a function
	///that should be asynchronous called when the data arrives.
	///On some backends, the given function will be called immedietly, returning
	///a disconnected connection while on other backends a valid one is returned and the
	///function might be called at some point in the future from the event (backend) thread.
	///The Connection will be automatically destroyed after the function has been
	///called once or earlier.
	///If the requested format cannot be retrieved, the function will be called with an
	///empty any object.
	///Note that on some backends this function might not return (running an internal event loop)
	///until the data is retrieved.
	virtual CbConn data(unsigned int fmt, const DataFunction& func) = 0;
};

///Event which will be sent when the application recieves data from another application.
///If the event is sent as effect from a drag and drop action, the event will be sent
///to the window on which the data was dropped, otherwise (e.g. clipboard) it will
///be sent to the specified event handler.
class DataOfferEvent : public EventBase<eventType::dataOffer, DataOfferEvent>
{
public:
    DataOfferEvent(EventHandler* handler = {}, std::unique_ptr<DataOffer> xoffer = {})
		: EvBase(handler), offer(std::move(xoffer)) {}
	~DataOfferEvent() = default;

	DataOfferEvent(DataOfferEvent&&) noexcept = default;
	DataOfferEvent& operator=(DataOfferEvent&&) noexcept = default;

    std::unique_ptr<DataOffer> offer;
	//XXX: some source indication? clipboard or dnd?
};

///Converts the given string to a dataType constant. If the given string is not recognized,
///0 is returned.
unsigned int stringToDataType(const std::string& type);

///Returns a number of string that describe the given data type.
///Will return an empty vector if the type is not known.
std::vector<std::string> dataTypeToString(unsigned int type);

///Gives a number of strings for a given DataTypes object.
///Will return an empty vector if the DataType holds no known types.
std::vector<std::string> dataTypesToString(DataTypes types);

}
