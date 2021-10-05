#pragma once

#include <istream>
#include <map>
#include <string>
#include <variant>
#include <vector>
#include <sstream>

namespace Json {

  class Node : std::variant<std::vector<Node>,
                            std::map<std::string, Node>,
                            int,
                            std::string,
							double,
							bool> {
  public:
    using variant::variant;

    const auto& AsArray() const {
      return std::get<std::vector<Node>>(*this);
    }
    const auto& AsMap() const {
      return std::get<std::map<std::string, Node>>(*this);
    }
    int AsInt() const {
      return std::get<int>(*this);
    }
    double AsDouble() const {
	  if (IsDouble())
	    return std::get<double>(*this);
	  else
	    return AsInt();
	}
    const auto& AsString() const {
      return std::get<std::string>(*this);
    }
    bool AsBool() const {
	  return std::get<bool>(*this);
	}

    bool IsArray() const {
	  return std::holds_alternative<std::vector<Node>>(*this);
	}
    bool IsMap() const {
	  return std::holds_alternative<std::map<std::string, Node>>(*this);
	}
    bool IsInt() const {
	  return std::holds_alternative<int>(*this);
	}
    bool IsDouble() const {
	  return std::holds_alternative<double>(*this);
	}
    bool IsString() const {
	  return std::holds_alternative<std::string>(*this);
	}
	bool IsBool() const {
	  return std::holds_alternative<bool>(*this);
	}

	std::string FromJsonToString() const{
		std::ostringstream os;
		if(IsArray()){
			os << "[\n" ;
			for(auto it = begin(AsArray()); it != end(AsArray()); ++it) {
				if(it != prev(end(AsArray()))) {
					os << it->FromJsonToString() << ",\n";
				} else {
					os << it->FromJsonToString() << "\n";
				}
			}
			std::string out = os.str();
			out.push_back(']');
			return out;
		} else if (IsMap()) {
			os << "{\n" ;
			for(auto it = begin(AsMap()); it != end(AsMap()); ++it) {
				if(it != prev(end(AsMap()))) {
					os << '"' << it->first << "\": " << it->second.FromJsonToString() << ",\n";
				} else {
					os << '"' << it->first << "\": " << it->second.FromJsonToString() << "\n";
				}
			}
			std::string out = os.str();
			out.append("}");
			return out;
		} else if (IsString()) {
			os << '"' << AsString() << '"';
			return os.str();
		} else if (IsInt()) {
			os << AsInt();
			return os.str();
		} else if (IsDouble()) {
			os << AsDouble();
			return os.str();
		} else if (IsBool()) {
			os << std::boolalpha << AsBool();
			return os.str();
		} else {
			return "";
		}
	}

  };

  class Document {
  public:
    explicit Document(Node root);

    const Node& GetRoot() const;

  private:
    Node root;
  };

  Document Load(std::istream& input);

}
