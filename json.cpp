#include "json.h"

using namespace std;

namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }

  variant<int, double> ReadNumber(istream& input) {
  	int integer;
  	double rational;
  	input >> integer;
  	if(input.peek() == '.'){
  	  input >> rational;
  	  if(integer < 0) {
  		return - rational + integer;
  	  }
  	  return rational + integer;
  	} else {
  	  return integer;
  	}
  }

  Node LoadNumber(istream& input) {
    auto result = ReadNumber(input);
    if(holds_alternative<int>(result)) {
      return Node(get<int>(result));
    }
    return Node(get<double>(result));
  }

  Node LoadBool(istream& input) {
    bool result;
    input >> boolalpha >> result;
    return Node(result);
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else if(c == '-' || isdigit(c)){
      input.putback(c);
      return LoadNumber(input);
    } else {
      input.putback(c);
      return LoadBool(input);
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }

}
