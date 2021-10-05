#include <iostream>
#include "Requests.h"
#include "test_runner.h"
#include "RouteManager.h"
#include <fstream>

using namespace std;


void TestAll() {
  TestRunner tr;
  RUN_TEST(tr, TestModifyAndReadRequest);
  RUN_TEST(tr, TestComputeDistance);
  RUN_TEST(tr, TestBusStats);
  RUN_TEST(tr, TestStopStats);
}

void ModifyProcessing(const Visitor& visitor, const vector<RequestHolder>& requests) {
  for(const RequestHolder& r: requests) {
    if(r->type == Request::Type::MODIFY_STOP) {
	  r->Accept(visitor);
	}
  }
  for(const RequestHolder& r: requests) {
	if(r->type == Request::Type::MODIFY_BUS) {
	  r->Accept(visitor);
	}
  }
}

Json::Document ReadProcessing(const Visitor& visitor, const vector<RequestHolder>& requests) {
  using namespace Json;
  vector<Node> nodes;
  for(const RequestHolder& r: requests) {
	nodes.push_back(*(r->Accept(visitor)));
  }
  return Document (Node(nodes));
}

int main() {
  using namespace Json;
  //ofstream out("output.json");
  //ifstream input("input_main.json");
  //out.precision(6);
  TestAll();
  cout.precision(6);
  JsonParser jp;
  Document doc = Load(cin);
  RouteManager rm(jp.GetRoutingSettings(doc));

  Visitor visitor;
  visitor.SetRouteManager(&rm);

  const auto modify_requests = jp.ParseBaseRequests(doc);
  ModifyProcessing(visitor, modify_requests);
  const auto read_requests = jp.ParseStatRequests(doc);
  Document output_doc = ReadProcessing(visitor, read_requests);
  cout << output_doc.GetRoot().FromJsonToString();
  return 0;
}
