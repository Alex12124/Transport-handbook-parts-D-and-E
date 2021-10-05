#include "Requests.h"
#include <set>

using namespace std;

//------------------Request---------------------------//

Request::Request(Type type) : type(type) {}

RequestHolder Request::Create(Request::Type type) {
  switch (type) {
    case Request::Type::MODIFY_BUS:
      return std::make_unique<ModifyBusRequest>();
    case Request::Type::MODIFY_STOP:
      return std::make_unique<ModifyStopRequest>();
    case Request::Type::READ_BUS:
	  return std::make_unique<ReadBusRequest>();
    case Request::Type::READ_STOP:
    	  return std::make_unique<ReadStopRequest>();
    case Request::Type::READ_ROUTE:
		  return std::make_unique<ReadRouteRequest>();
    default:
      return nullptr;
  }
}

ReadStopRequest::ReadStopRequest() : Request(Type::READ_STOP){}

void ReadStopRequest::ParseFrom(const Json::Node& node) {
  stop_name = node.AsMap().at("name").AsString();
  id = node.AsMap().at("id").AsInt();
}

ReadBusRequest::ReadBusRequest() : Request(Type::READ_BUS) {}

void ReadBusRequest::ParseFrom(const Json::Node& node) {
  bus_name = node.AsMap().at("name").AsString();
  id = node.AsMap().at("id").AsInt();
}

ReadRouteRequest::ReadRouteRequest() :Request(Type::READ_ROUTE){}
void ReadRouteRequest::ParseFrom(const Json::Node& node) {
  id = node.AsMap().at("id").AsInt();
  from = node.AsMap().at("from").AsString();
  to = node.AsMap().at("to").AsString();
}

ModifyBusRequest::ModifyBusRequest() : Request(Type::MODIFY_BUS) {}

void ModifyBusRequest::ParseFrom(const Json::Node& node) {
  bus_name = node.AsMap().at("name").AsString();
  cycle = node.AsMap().at("is_roundtrip").AsBool();
  for(const Json::Node& node: node.AsMap().at("stops").AsArray()) {
	stops.push_back(node.AsString());
  }
}

ModifyStopRequest::ModifyStopRequest() : Request(Type::MODIFY_STOP) {}

void ModifyStopRequest::ParseFrom(const Json::Node& node) {
  for(auto [key, value] :node.AsMap().at("road_distances").AsMap()) {
    distances.push_back({value.AsInt(), std::move(key)});
  }
  longitude = 3.1415926535 * node.AsMap().at("longitude").AsDouble() / 180;
  stop_name = node.AsMap().at("name").AsString();
  latitude = 3.1415926535 * node.AsMap().at("latitude").AsDouble() / 180;
}

optional<Json::Node> ReadStopRequest::Accept(const Visitor& v) const {
  return v.Visit(*this);

}
optional<Json::Node> ReadBusRequest::Accept(const Visitor& v) const {

  return v.Visit(*this);
}

optional<Json::Node> ReadRouteRequest::Accept(const Visitor& v) const {
  return v.Visit(*this);

}
optional<Json::Node> ModifyBusRequest::Accept(const Visitor& v) const {
  v.Visit(*this);
  return nullopt;
}

optional<Json::Node> ModifyStopRequest::Accept(const Visitor& v) const {
  v.Visit(*this);
  return nullopt;
}

//------------------Request---------------------------//

//------------------Parsing Functions-----------------//

optional<Request::Type> ConvertRequestTypeFromString(string_view type_str,
		const unordered_map<string_view, Request::Type>& str_to_type) {

  if (const auto it = str_to_type.find(type_str);
	it != str_to_type.end()) {
	return it->second;
  } else {
	return nullopt;
  }

}
//------------------Parsing Functions-----------------------------//

//-----------------------PrintResults-------------------------------//

Json::Node PrintBusResponse(optional<BusStats> stats, int id) {
  using Json::Node;
  map<std::string, Json::Node> result;
  result["request_id"] = Node(id);
  if(!stats) {
	  result["error_message"] = Node("not found"s);
  } else {
	result["route_length"] = Node(stats->route_distance);
	result["curvature"] = Node(stats->curvature);
	result["stop_count"] = Node(stats->stop_count);
	result["unique_stop_count"] = Node(stats->unique_stop_count);
  }
  return Json::Node(result);
}

Json::Node PrintStopResponse(optional<set<string_view>> stats, int id) {
  using Json::Node;
  map<std::string, Json::Node> result;
  result["request_id"] = Node(id);
  if(!stats) {
	result["error_message"] = Node("not found"s);
  } else {
	std::vector<Json::Node> v;
	for(string_view str: (*stats)) {
	  v.push_back(Node(string(str)));
	}
	result["buses"] = Node(v);
  }
  return Json::Node(result);
}

Json::Node PrintRouteResponse(optional<RouteStats<double>> stats, int id) {
  using Json::Node;
  map<std::string, Json::Node> result;
  result["request_id"] = Node(id);
  if(!stats) {
  	result["error_message"] = Node("not found"s);
  } else {
	result["total_time"] = Node(stats->weight);
	vector<Json::Node> nodes;
	nodes.reserve(stats->elements_of_route.size() * 2);
	for(ElementOfRoute& element: stats->elements_of_route) {
	  map<std::string, Json::Node> wait;
	  wait["type"] = Node("Wait"s);
	  wait["stop_name"] = Node(string(element.start_stop_name));
	  wait["time"] = Node(stats->bus_wait_time);
	  nodes.push_back(Node(wait));

	  map<std::string, Json::Node> bus;
	  bus["type"] = Node("Bus"s);
	  bus["bus"] = Node(string(element.bus_name));
	  bus["span_count"] = Node(element.span_count);
	  bus["time"] = Node(element.el_time);
	  nodes.push_back(Node(bus));
	}
	result["items"] = Node(nodes);
  }
  return Json::Node(result);
}
//-----------------------PrintResults-------------------------------//


//---------------Visitor------------------------------//

Json::Node Visitor::Visit(const ReadBusRequest& request) const {
  auto result = PrintBusResponse(rm->GetBusStats(request.bus_name), request.id);
  return result;
}

Json::Node Visitor::Visit(const ReadStopRequest& request) const {
  auto result = PrintStopResponse(rm->GetStopStats(request.stop_name), request.id);
  return result;
}

Json::Node Visitor::Visit(const ReadRouteRequest& request) const {
  return PrintRouteResponse(rm->GetRouteStats(request.from, request.to), request.id);
}

void Visitor::Visit(const ModifyBusRequest& request) const {
  if(request.cycle) {
	rm->SetStrategy(cycle_strategy.get());
  } else {
	rm->SetStrategy(not_cycle_strategy.get());
  }
  rm->SetBusData(request.bus_name, request.stops);
}
void Visitor::Visit(const ModifyStopRequest& request) const {
  rm->SetStopData(request.stop_name, Coords{request.latitude, request.longitude},
		  request.distances);
}

void Visitor::SetRouteManager(RouteManager* rm_) {
  rm = rm_;
}

Visitor::Visitor() {
  cycle_strategy = make_unique<CycleStrategy>();
  not_cycle_strategy = make_unique<NotCycleStrategy>();
}

//---------------Visitor------------------------------//
