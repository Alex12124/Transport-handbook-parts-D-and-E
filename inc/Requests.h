#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>
#include <memory>
#include <set>
#include <string_view>
#include <sstream>
#include "RouteManager.h"
#include "json.h"

class Visitor;
class Request;
using RequestHolder = std::unique_ptr<Request>;

//------------------Request---------------------------//
class Request {
public:
  enum class Type {
	READ_STOP,
    READ_BUS,
	READ_ROUTE,
    MODIFY_BUS,
	MODIFY_STOP,
  };

  Request(Type type);
  static RequestHolder Create(Type type);
  virtual void ParseFrom(const Json::Node& node) = 0;
  virtual ~Request() = default;
  virtual std::optional<Json::Node> Accept(const Visitor& v) const = 0;
  const Type type;
};

const std::unordered_map<std::string_view, Request::Type> MODIFY_REQUEST_TYPE = {
    {"Bus", Request::Type::MODIFY_BUS},
    {"Stop", Request::Type::MODIFY_STOP}
};

const std::unordered_map<std::string_view, Request::Type> READ_REQUEST_TYPE = {
    {"Bus", Request::Type::READ_BUS},
	{"Stop", Request::Type::READ_STOP},
	{"Route", Request::Type::READ_ROUTE},
};

class ReadStopRequest : public Request {
public:
  ReadStopRequest();
  void ParseFrom(const Json::Node& node) override;
  std::optional<Json::Node> Accept(const Visitor& v) const override;
  int id;
  std::string stop_name;
};

class ReadBusRequest : public Request {
public:
  ReadBusRequest();
  void ParseFrom(const Json::Node& node) override;
  std::optional<Json::Node> Accept(const Visitor& v) const override;
  int id;
  std::string bus_name;
};

class ReadRouteRequest : public Request {
public:
  ReadRouteRequest();
  void ParseFrom(const Json::Node& node) override;
  std::optional<Json::Node> Accept(const Visitor& v) const override;
  int id;
  std::string from, to;
};

class ModifyBusRequest : public Request {
public:
  ModifyBusRequest();
  void ParseFrom(const Json::Node& node) override;
  std::optional<Json::Node> Accept(const Visitor& v) const override;

  std::vector<std::string> stops;
  std::string bus_name;
  bool cycle = false;
};

class ModifyStopRequest : public Request {
public:
  ModifyStopRequest();
  void ParseFrom(const Json::Node& node) override;
  std::optional<Json::Node> Accept(const Visitor& v) const override;


  double latitude, longitude;
  std::string stop_name;
  std::vector<DistanceToStop> distances;
};
//------------------Request---------------------------------------//

//------------------Parsing Functions-----------------------------//
std::optional<Request::Type> ConvertRequestTypeFromString(std::string_view type_str,
		const std::unordered_map<std::string_view, Request::Type>& str_to_type);

class JsonParser {
public:

  RoutingSettings GetRoutingSettings(const Json::Document& doc) {
	Json::Node node = doc.GetRoot().AsMap().at("routing_settings");
	return {node.AsMap().at("bus_wait_time").AsInt(),
		node.AsMap().at("bus_velocity").AsDouble()};
  }
  std::vector<RequestHolder> ParseBaseRequests(const Json::Document& doc) {
	return ParseRequests(doc.GetRoot().AsMap().at("base_requests"), true);
  }
  std::vector<RequestHolder> ParseStatRequests(const Json::Document& doc) {
	return ParseRequests(doc.GetRoot().AsMap().at("stat_requests"), false);
  }
private:
  std::vector<RequestHolder> ParseRequests(const Json::Node& node_,
		  bool is_modify) {
	using Json::Node;
	std::vector<RequestHolder> requests;
	for(const Node& node: node_.AsArray()) {
	  auto request_type = [is_modify, &node]{
	if(is_modify) {
	  return ConvertRequestTypeFromString(node.AsMap().
		at("type").AsString(), MODIFY_REQUEST_TYPE);
	} else {
	  return ConvertRequestTypeFromString(node.AsMap().
		at("type").AsString(), READ_REQUEST_TYPE);
	}
	}();

	if (!request_type) {
	continue;
	}

	RequestHolder request = Request::Create(*request_type);
	if (request) {
	request->ParseFrom(node);
	};
	requests.push_back(std::move(request));
	}
	return requests;
  }
};

Json::Node PrintBusResponse(std::optional<BusStats> stats, int id);

Json::Node PrintStopResponse(std::optional<std::set<std::string_view>> stats, int id);

Json::Node PrintRouteResponse(std::optional<RouteStats<double>> stats, int id);

//------------------Parsing Functions-----------------------------//

//-----------------------Visitor--------------------------------//

class Visitor {
public:
  Visitor();
  Json::Node Visit(const ReadBusRequest&) const;
  Json::Node Visit(const ReadRouteRequest&) const;
  Json::Node Visit(const ReadStopRequest&) const;
  void Visit(const ModifyBusRequest&) const;
  void Visit(const ModifyStopRequest&) const;
  void SetRouteManager(RouteManager* rm_);
private:
  RouteManager* rm;
  std::unique_ptr<Strategy> cycle_strategy;
  std::unique_ptr<Strategy> not_cycle_strategy;
};

//-------------------------Tests--------------------------------//
void TestModifyAndReadRequest();
