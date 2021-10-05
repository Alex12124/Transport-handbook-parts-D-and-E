#include "Requests.h"
#include <iomanip>
#include "test_runner.h"
#include <fstream>

using namespace std;

void TestModifyAndReadRequest() {
  ifstream input("input.json");
  Json::Document doc = Json::Load(input);
  JsonParser jp;
  {
	const auto requests = jp.ParseBaseRequests(doc);

	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[2]).latitude, 3.1415926535 * 55.574371 / 180);
	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[2]).longitude, 3.1415926535 * 37.6517 / 180);
	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[2]).stop_name, "Biryulyovo Zapadnoye");

	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[3]).latitude, 3.1415926535 * 55.587655 / 180);
	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[3]).longitude, 3.1415926535 * 37.645687 / 180);
	ASSERT_EQUAL(static_cast<ModifyStopRequest&>(*requests[3]).stop_name, "Universam");


	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[0]).cycle, true);
	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[0]).bus_name, "297");
	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[0]).stops,
	vector<string>({"Biryulyovo Zapadnoye", "Biryulyovo Tovarnaya",
		"Universam", "Biryulyovo Zapadnoye"}));

	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[1]).cycle, false);
	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[1]).bus_name, "635");
	ASSERT_EQUAL(static_cast<ModifyBusRequest&>(*requests[1]).stops,
	vector<string>({"Biryulyovo Tovarnaya", "Universam", "Prazhskaya"}));
  }
  {
	const auto requests = jp.ParseStatRequests(doc);

	ASSERT_EQUAL(static_cast<ReadBusRequest&>(*requests[0]).bus_name, "297");
	ASSERT_EQUAL(static_cast<ReadBusRequest&>(*requests[0]).id, 1);


	ASSERT_EQUAL(static_cast<ReadBusRequest&>(*requests[1]).bus_name, "635");
	ASSERT_EQUAL(static_cast<ReadBusRequest&>(*requests[1]).id, 2);


	ASSERT_EQUAL(static_cast<ReadStopRequest&>(*requests[2]).id, 3);
	ASSERT_EQUAL(static_cast<ReadStopRequest&>(*requests[2]).stop_name, "Universam");


	ASSERT_EQUAL(static_cast<ReadRouteRequest&>(*requests[3]).id, 4);
	ASSERT_EQUAL(static_cast<ReadRouteRequest&>(*requests[3]).from, "Biryulyovo Zapadnoye");
	ASSERT_EQUAL(static_cast<ReadRouteRequest&>(*requests[3]).to, "Universam");
  }
}

