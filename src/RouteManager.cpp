#include "RouteManager.h"
#include <sstream>
#include "test_runner.h"
#include <iomanip>
using namespace std;

double Strategy::ComputeDistance(const Coords& lhs, const Coords& rhs) const {

  return acos(sin(lhs.latitude) * sin(rhs.latitude) +
		      cos(lhs.latitude) * cos(rhs.latitude) *
		      cos(abs(lhs.longitude - rhs.longitude))) *
		      6371000;
}

int Strategy::ComputeUniqueStopsOnRoute(const std::vector<std::string>& stops) const {
  std::set<std::string> s(begin(stops), end(stops));
  return s.size();
}

void TestComputeDistance() {
  ostringstream os;
  os.precision(6);
  CycleStrategy strategy;
  os << strategy.ComputeDistance(Coords{55.611087 * 3.1415926535 / 180,
		37.20829 * 3.1415926535 / 180}, Coords{55.595884 * 3.1415926535 / 180,
		37.209755 * 3.1415926535 / 180});

  ASSERT_EQUAL(os.str(), "1693");
}

void TestBusStats() {
  RouteManager manager(RoutingSettings{6, 40});

  vector<string> stops = {"Biryulyovo Tovarnaya", "Universam", "Prazhskaya"};

  unique_ptr<Strategy> not_cycle = make_unique<NotCycleStrategy>();
  unique_ptr<Strategy> cycle = make_unique<CycleStrategy>();
  vector<DistanceToStop> v1 = vector<DistanceToStop>({{2600, "Biryulyovo Tovarnaya"}});
  vector<DistanceToStop> v2 = vector<DistanceToStop>({{890, "Universam"}});
  vector<DistanceToStop> v3 = vector<DistanceToStop>({{4650, "Prazhskaya"},
	  {2500, "Biryulyovo Zapadnoye"}, {1380, "Biryulyovo Tovarnaya"}});

  manager.SetStopData("Biryulyovo Zapadnoye", Coords{55.574371 * 3.1415926535 / 180,
		37.6517 * 3.1415926535 / 180}, v1);
  manager.SetStopData("Biryulyovo Tovarnaya", Coords{55.592028 * 3.1415926535 / 180,
	    37.653656 * 3.1415926535 / 180}, v2);
  manager.SetStopData("Universam", Coords{55.587655 * 3.1415926535 / 180,
	    37.645687 * 3.1415926535 / 180}, v3);
  manager.SetStopData("Prazhskaya", Coords{55.611717 * 3.1415926535 / 180,
	    37.603938 * 3.1415926535 / 180}, {});

  manager.SetStrategy(not_cycle.get());
  manager.SetBusData("635", stops);
  BusStats stats = *manager.GetBusStats("635");
  ASSERT_EQUAL(stats.stop_count, 5);
  ASSERT_EQUAL(stats.unique_stop_count, 3);
  ASSERT_EQUAL(stats.route_distance, 11570);

  {
	ostringstream os;
	os << setprecision(6);
	os << stats.curvature;
	ASSERT_EQUAL(os.str(), "1.30156");
  }

  stops = {"Biryulyovo Zapadnoye", "Biryulyovo Tovarnaya", "Universam",
	        "Biryulyovo Zapadnoye"};

  manager.SetStrategy(cycle.get());
  manager.SetBusData("297", stops);
  stats = *manager.GetBusStats("297");
  ASSERT_EQUAL(stats.stop_count, 4);
  ASSERT_EQUAL(stats.unique_stop_count, 3);
  ASSERT_EQUAL(stats.route_distance, 5990);

  ostringstream os;
  os << setprecision(6);
  os << stats.curvature;
  ASSERT_EQUAL(os.str(), "1.42963");

}

void TestStopStats() {
  RouteManager manager(RoutingSettings{6, 40});
  vector<string> stops = {"Tolstopaltsevo", "Marushkino", "Rasskazovka"};
  {

    vector<DistanceToStop> v1 = vector<DistanceToStop>({{3900, "Marushkino"}});
	vector<DistanceToStop> v2 = vector<DistanceToStop>({{9900, "Rasskazovka"}});



    unique_ptr<Strategy> not_cycle = make_unique<NotCycleStrategy>();
    manager.SetStopData("Tolstopaltsevo", Coords{55.611087 * 3.1415926535 / 180,
		37.20829 * 3.1415926535 / 180}, v1);
    manager.SetStopData("Marushkino", Coords{55.595884 * 3.1415926535 / 180,
		37.209755 * 3.1415926535 / 180}, v2);
    manager.SetStopData("Rasskazovka", Coords{55.632761 * 3.1415926535 / 180,
		37.333324 * 3.1415926535 / 180}, {});
    manager.SetStopData("Extra stop", Coords{53.632761 * 3.1415926535 / 180,
		37.333324 * 3.1415926535 / 180}, {});
    manager.SetStrategy(not_cycle.get());
    manager.SetBusData("750", stops);
    ASSERT(manager.GetStopStats("Extra stop")->empty());
    ASSERT(!manager.GetStopStats("250"));
    ASSERT_EQUAL(*manager.GetStopStats("Tolstopaltsevo"), set<std::string_view>({{"750"}}));
  }
}

