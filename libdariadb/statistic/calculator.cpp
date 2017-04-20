#include <libdariadb/statistic/calculator.h>
#include <libdariadb/statistic/functions.h>
#include <libdariadb/utils/utils.h>

using namespace dariadb;
using namespace dariadb::storage;
using namespace dariadb::statistic;

IFunction_ptr FunctionFactory::make_one(const std::string &k) {
  if (k == "average") {
    return std::make_shared<Average>("average");
  }
  if (k == "median") {
    return std::make_shared<Median>("median");
  }
  if (k == "percentile90") {
    return std::make_shared<Percentile90>("percentile90");
  }
  if (k == "percentile99") {
    return std::make_shared<Percentile99>("percentile99");
  }
  return nullptr;
}

std::vector<IFunction_ptr> FunctionFactory::make(const std::vector<std::string> &kinds) {
  std::vector<IFunction_ptr> result(kinds.size());
  size_t i = 0;
  for (const auto k : kinds) {
    result[i++] = make_one(k);
  }
  return result;
}

std::vector<std::string> FunctionFactory::functions() {
  return {"average", "median", "percentile90", "percentile99"};
}

Calculator::Calculator(const IEngine_Ptr &storage) : _storage(storage) {}

MeasArray Calculator::apply(const IdArray &ids, Time from, Time to, Flag f,
                            const std::vector<std::string> &functions,
                            const MeasArray &ma) {
  return MeasArray();
}