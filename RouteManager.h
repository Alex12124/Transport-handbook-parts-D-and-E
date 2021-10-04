#pragma once
#include <iostream>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <set>
#include <cmath>
#include <optional>
#include "graph.h"
#include <string_view>
#include "router.h"

using GraphHolder = std::unique_ptr<Graph::DirectedWeightedGraph<double>>;
using RouterHolder = std::unique_ptr<Graph::Router<double>>;

struct StringPairHasher {
    size_t operator()(const std::pair<std::string_view, std::string_view>& p) const {
    	size_t r1 = shash(p.first);
    	size_t r2 = shash(p.second);

    	const size_t coef = 2946901;
    	return r1 * coef + r2;
    }
    std::hash<std::string_view> shash;
};

struct RoutingSettings {
  int bus_wait_time;
  double bus_velocity;
};

struct BusStats {
  int stop_count;
  int unique_stop_count;
  double route_distance;
  double curvature;
};

struct ElementOfRoute {
  std::string_view bus_name;
  int span_count;
  std::string_view start_stop_name;
  double el_time;
};

template<typename Weight>
struct RouteStats {
  std::vector<ElementOfRoute> elements_of_route;
  Weight weight;
  int bus_wait_time;
};


struct DistanceStats {
  int real_dist;
  double dist;
  std::vector<int> real_distances;
  std::optional<std::vector<int>> reverse_distances;
};

struct DistanceToStop {

  int distance;
  std::string stop_name;
};

struct Coords {
  long double latitude;
  long double longitude;
};

class StopDataBase {
public:
  Coords GetCoords() const{
	return coords;
  }

  void SetCoords(const Coords& coords_) {
	coords = coords_;
  }

  std::set<std::string_view>& GetBuses() {
  	return buses;
  }

  const std::set<std::string_view>& GetBuses() const {
	return buses;
  }

  std::unordered_map<std::string_view, int>& GetDistance() {
  	return distance;
  }

  const std::unordered_map<std::string_view, int>& GetDistance() const {
	return distance;
  }

  void SetVertexId(size_t id) {
	vertex_id = id;
  }

  size_t GetVertexId() const{
  	return vertex_id;
  }
private:
  Coords coords;
  std::set<std::string_view> buses;
  std::unordered_map<std::string_view, int> distance;
  size_t vertex_id;
};

//---------------------Pattern Strategy-----------------------//
class Strategy {
public:
  virtual ~Strategy() = default;
  virtual int ComputeStopsOnRoute(const std::vector<std::string>& stops) const = 0;

  virtual DistanceStats ComputeDistancesOnRoute(const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db) const  = 0;

  double ComputeDistance(const Coords& lhs, const Coords& rhs) const;

  int ComputeUniqueStopsOnRoute(const std::vector<std::string>& stops) const;

  void FillBusesInStopDB(const std::vector<std::string>& stops,
		  std::string_view bus_name,
		  std::unordered_map<std::string_view, StopDataBase>& stop_db) {
	for(const std::string& stop_name: stops) {
	  stop_db[stop_name].GetBuses().insert(bus_name);
	}
  }

  virtual GraphHolder AddEdgesToGraph(std::string_view bus_name,
		  const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db,
		  std::vector<ElementOfRoute>& edge_to_element,
		  GraphHolder graph,
		  const RoutingSettings& settings,
		  const std::vector<int> real_distances,
		  std::optional<std::vector<int>> reverse_distances) const = 0;

  GraphHolder AddEdge(size_t vertex_from, size_t vertex_to,
		  GraphHolder graph, double dist_sum,
		  const RoutingSettings& settings,
		  std::string_view bus_name,
		  std::string_view start_stop_name,
		  std::vector<ElementOfRoute>& edge_to_element,
		  int span_count) const {
	if(vertex_from == vertex_to) {
	  return graph;
	}

    for(size_t edge_id: graph->GetIncidentEdges(vertex_from)) {
	  if(graph->GetEdge(edge_id).to == vertex_to) {
		if(graph->GetEdge(edge_id).weight > ((dist_sum * 60) /
		      (settings.bus_velocity * 1000)) + settings.bus_wait_time) {
		  graph->GetEdge(edge_id).weight = ((dist_sum * 60) /
			  (settings.bus_velocity * 1000)) + settings.bus_wait_time;
		  edge_to_element[edge_id] = {bus_name, span_count, start_stop_name,
			  (dist_sum * 60) / (settings.bus_velocity * 1000)};
		}
		return graph;
	  }
	}

	graph->AddEdge({vertex_from, vertex_to,
	  ((dist_sum * 60) / (settings.bus_velocity * 1000)) + settings.bus_wait_time});
	edge_to_element.push_back({bus_name, span_count, start_stop_name,
	  (dist_sum * 60) / (settings.bus_velocity * 1000)});

	return graph;
  }
};

class CycleStrategy : public Strategy {
public:
  int ComputeStopsOnRoute(const std::vector<std::string>& stops) const override {
    return stops.size();
  }

  DistanceStats ComputeDistancesOnRoute(const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db) const override {
    double sum = 0;
    int real_sum = 0;
    std::vector<int> real_distances;
    real_distances.reserve(ComputeStopsOnRoute(stops));
	for(auto it = begin(stops); it != prev(end(stops)); ++it) {
	  sum += ComputeDistance(stop_db.at(*it).GetCoords(),
			  stop_db.at(*next(it)).GetCoords());
      real_distances.push_back(stop_db.at(*it).GetDistance().at(*next(it)));
      real_sum += real_distances.back();
    }
	return {real_sum, sum, std::move(real_distances), std::nullopt};
  }

  GraphHolder AddEdgesToGraph(std::string_view bus_name,
		  const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db,
		  std::vector<ElementOfRoute>& edge_to_element,
		  GraphHolder graph,
		  const RoutingSettings& settings,
		  const std::vector<int> real_distances,
		  std::optional<std::vector<int>> reverse_distances) const override {

    for(auto it = begin(stops); it != prev(end(stops)); ++it) {
      double dist_sum = 0;
      for(auto it2 = next(it); it2 != end(stops); ++it2) {
	    dist_sum += real_distances[it2 - begin(stops) - 1];
	    graph = AddEdge(stop_db.at(*it).GetVertexId(), stop_db.at(*it2).GetVertexId(),
			std::move(graph), dist_sum, settings, bus_name,
			*it, edge_to_element, it2 - it);
      }
    }
    return std::move(graph);
  }
};

class NotCycleStrategy : public Strategy {
public:
  int ComputeStopsOnRoute(const std::vector<std::string>& stops) const override {
    return stops.size() * 2 - 1;
  }

  DistanceStats ComputeDistancesOnRoute(const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db) const override {
	double sum = 0;
	int real_sum = 0;
	std::vector<int> real_distances, reverse_distances;
	real_distances.reserve(ComputeStopsOnRoute(stops));
	reverse_distances.reserve(ComputeStopsOnRoute(stops));

	for(auto it = begin(stops); it != prev(end(stops)); ++it) {
	  sum += 2 * ComputeDistance(stop_db.at(*it).GetCoords(),
			  stop_db.at(*next(it)).GetCoords());

	  real_distances.push_back(stop_db.at(*it).GetDistance().at(*next(it)));
	  reverse_distances.push_back(stop_db.at(*next(it)).GetDistance().at(*it));
	  real_sum += (real_distances.back() + reverse_distances.back());
	}
	return {real_sum, sum, std::move(real_distances), std::move(reverse_distances)};
  }

  GraphHolder AddEdgesToGraph(std::string_view bus_name,
		  const std::vector<std::string>& stops,
		  const std::unordered_map<std::string_view, StopDataBase>& stop_db,
		  std::vector<ElementOfRoute>& edge_to_element,
		  GraphHolder graph,
		  const RoutingSettings& settings,
		  const std::vector<int> real_distances,
		  std::optional<std::vector<int>> reverse_distances) const override {

    for(auto it = begin(stops); it != prev(end(stops)); ++it) {
	  double dist_sum = 0;
	  for(auto it2 = next(it); it2 != end(stops); ++it2) {
		dist_sum += real_distances[it2 - begin(stops) - 1];
		graph = AddEdge(stop_db.at(*it).GetVertexId(), stop_db.at(*it2).GetVertexId(),
			std::move(graph), dist_sum, settings, bus_name,
			*it, edge_to_element, it2 - it);
	  }
	}

    for(auto it = rbegin(stops); it != prev(rend(stops)); ++it) {
	  double dist_sum = 0;
	  for(auto it2 = next(it); it2 != rend(stops); ++it2) {
		dist_sum += (*reverse_distances)[reverse_distances->size() - (it2 - rbegin(stops))];
		graph = AddEdge(stop_db.at(*it).GetVertexId(), stop_db.at(*it2).GetVertexId(),
			std::move(graph), dist_sum, settings, bus_name,
			*it, edge_to_element, it2 - it);
	  }
	}
    return std::move(graph);

  }
};
//---------------------Pattern Strategy-----------------------//


//---------------------Business Logic of Programm----------------//
class RouteManager {
public:
  RouteManager(RoutingSettings settings_): settings(settings_) {}

  void SetStopData(std::string_view stop_name, Coords coords,
		  const std::vector<DistanceToStop>& distances) {
	stop_db[stop_name].SetCoords(coords);
	for(const DistanceToStop& dist: distances) {
	  stop_db[stop_name].GetDistance()[dist.stop_name] = dist.distance;
	  if(!stop_db[dist.stop_name].GetDistance().count(stop_name)) {
		stop_db[dist.stop_name].GetDistance()[stop_name] = dist.distance;
	  }
	}
	stop_db[stop_name].SetVertexId(vertex_counter);
	++vertex_counter;
  }

  void SetBusData(std::string_view bus_name, const std::vector<std::string>& stops) {
	BuildGraphIfNotExists();
	BusStats stats;
	stats.stop_count = strategy->ComputeStopsOnRoute(stops);
	stats.unique_stop_count = strategy->ComputeUniqueStopsOnRoute(stops);
	DistanceStats dist_stats =
			strategy->ComputeDistancesOnRoute(stops, stop_db);
	stats.curvature = dist_stats.real_dist / dist_stats.dist;
	stats.route_distance = dist_stats.real_dist;
    bus_stats[bus_name] = stats;
    strategy->FillBusesInStopDB(stops, bus_name, stop_db);
    graph = strategy->AddEdgesToGraph(bus_name, stops, stop_db, edge_to_element, std::move(graph),
    		settings, dist_stats.real_distances, dist_stats.reverse_distances);
  }

  void BuildGraphIfNotExists() {
    if(!graph) {
      graph = std::make_unique<Graph::DirectedWeightedGraph
    		  <double>>(vertex_counter);
    }
  }

  void BuildRouterIfNotExists() {
    if(!router) {
      router = std::make_unique<Graph::Router<double>>(*graph);
    }
  }

  void SetStrategy(Strategy* strategy_) {
	strategy = strategy_;
  }

  std::optional<BusStats> GetBusStats(const std::string& bus_name) const {
	if(bus_stats.count(bus_name)) {
	  return bus_stats.at(bus_name);
	}
	return std::nullopt;
  }

  std::optional<std::set<std::string_view>> GetStopStats(const std::string& stop_name) {
	if(stop_db.count(stop_name)) {
	  return stop_db[stop_name].GetBuses();
	}
	return std::nullopt;
  }

  std::optional<RouteStats<double>> GetRouteStats(std::string_view from,
		  std::string_view to) {
	BuildRouterIfNotExists();
	if(route_db.count({from, to})) {
	  return route_db[{from, to}];
	}
	auto route_info = router->BuildRoute(stop_db.at(from).GetVertexId(),
			stop_db.at(to).GetVertexId());
	if(!route_info) {
	  return std::nullopt;
	}
	RouteStats<double> route_stats;
	std::vector<ElementOfRoute> elements_of_route;
	for(size_t idx = 0; idx < route_info->edge_count; ++idx) {
	  Graph::EdgeId edge_id = router->GetRouteEdge(route_info->id, idx);
	  elements_of_route.push_back(edge_to_element[edge_id]);
	}
	route_stats.elements_of_route = std::move(elements_of_route);
	route_stats.weight = route_info->weight;
	route_stats.bus_wait_time = settings.bus_wait_time;
    route_db[{from, to}] = route_stats;
    return route_stats;
  }

private:
  std::unordered_map<std::string_view, BusStats> bus_stats;
  std::unordered_map<std::string_view, StopDataBase> stop_db;
  std::unordered_map<std::pair<std::string_view, std::string_view>,
  	  RouteStats<double> , StringPairHasher> route_db;
  std::vector<ElementOfRoute> edge_to_element;
  Strategy* strategy;
  GraphHolder graph;
  RouterHolder router;
  size_t vertex_counter = 0;
  RoutingSettings settings;
};
//---------------------Business Logic of Programm----------------//

//---------------------Tests-----------------------------------//
void TestComputeDistance();
void TestBusStats();
void TestStopStats();
//---------------------Tests-----------------------------------//
