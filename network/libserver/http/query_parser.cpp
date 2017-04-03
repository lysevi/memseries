#include <libserver/http/query_parser.h>
#include <extern/json/src/json.hpp>

using json = nlohmann::json;

namespace dariadb_parse_query_inner {
std::shared_ptr<dariadb::MeasArray>
read_meas_array(const dariadb::scheme::IScheme_Ptr &scheme, const json &js) {
  auto result = std::make_shared<dariadb::MeasArray>();
  auto values = js["append_values"];
  for (auto it = values.begin(); it != values.end(); ++it) {
    dariadb::MeasArray sub_result;
    auto id_str = it.key();
    auto val4id = it.value();

    dariadb::Id id = scheme->addParam(id_str);

    auto flag_js = val4id["F"];
    auto val_js = val4id["V"];
    auto time_js = val4id["T"];

    sub_result.resize(flag_js.size());
    size_t pos = 0;
    for (auto f_it = flag_js.begin(); f_it != flag_js.end(); ++f_it) {
      dariadb::Flag f = *f_it;
      sub_result[pos++].flag = f;
    }

    pos = 0;
    for (auto v_it = val_js.begin(); v_it != val_js.end(); ++v_it) {
      dariadb::Value v = *v_it;
      sub_result[pos++].value = v;
    }

    pos = 0;
    for (auto t_it = time_js.begin(); t_it != time_js.end(); ++t_it) {
      dariadb::Time t = *t_it;
      sub_result[pos++].time = t;
    }
    for (auto v : sub_result) {
      v.id = id;
      result->push_back(v);
    }
  }
  return result;
}

std::shared_ptr<dariadb::MeasArray>
read_single_meas(const dariadb::scheme::IScheme_Ptr &scheme, const json &js) {
  auto result = std::make_shared<dariadb::MeasArray>();
  result->resize(1);
  auto value = js["append_value"];
  result->front().time = value["T"];
  result->front().flag = value["F"];
  result->front().value = value["V"];
  std::string id_str = value["I"];
  result->front().id = scheme->addParam(id_str);
  return result;
}
} // namespace dariadb_parse_query_inner

dariadb::net::http::http_query
dariadb::net::http::parse_query(const dariadb::scheme::IScheme_Ptr &scheme,
                                const std::string &query) {
  http_query result;
  result.type = http_query_type::unknow;
  json js = json::parse(query);

  std::string type = js["type"];
  if (type == "append") {
    result.type = http_query_type::append;

    auto single_value = js.find("append_value");
    if (single_value == js.end()) {
      result.append_query = dariadb_parse_query_inner::read_meas_array(scheme, js);
    } else {
      result.append_query = dariadb_parse_query_inner::read_single_meas(scheme, js);
    }
    return result;
  }

  if (type == "readInterval") {
    result.type = http_query_type::readInterval;
    dariadb::Time from = js["from"];
    dariadb::Time to = js["to"];
    dariadb::Flag flag = js["flag"];
    std::vector<std::string> values = js["id"];
    dariadb::IdArray ids;
    ids.reserve(values.size());
    auto name_map = scheme->ls();
    for (auto v : values) {
      ids.push_back(name_map.idByParam(v));
    }
    result.interval_query = std::make_shared<QueryInterval>(ids, flag, from, to);
    return result;
  }
  return result;
}

std::string dariadb::net::http::status2string(const dariadb::Status &s) {
  json js;
  js["writed"] = s.writed;
  js["ignored"] = s.ignored;
  js["error"] = to_string(s.error);
  return js.dump();
}

std::string dariadb::net::http::scheme2string(const dariadb::scheme::DescriptionMap &dm) {
  json js;
  for (auto kv : dm) {
    js[kv.second.name] = kv.first;
  }
  return js.dump();
}

std::string dariadb::net::http::meases2string(const dariadb::scheme::IScheme_Ptr &scheme,
                                              const dariadb::MeasArray &ma) {
	auto nameMap = scheme->ls();
  std::map<dariadb::Id, dariadb::MeasArray> values;
  for (auto &m : ma) {
    values[m.id].push_back(m);
  }

  json js_result;
  for (auto &kv : values) {
	  std::list<json> js_values_list;
	  for (auto v : kv.second) {
		  json value_js;
		  value_js["F"] = v.flag;
		  value_js["T"] = v.time;
		  value_js["V"] = v.value;
		  js_values_list.push_back(value_js);
	  }
	  js_result[nameMap[kv.first].name] = js_values_list;
  }
  auto result = js_result.dump(1);
  return result;
}