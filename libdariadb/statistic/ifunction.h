#pragma once

#include <libdariadb/meas.h>
#include <memory>
#include <string>
namespace dariadb {
namespace statistic {
class IFunction {
public:
  IFunction(const std::string &s) : _kindname(s) {}
  virtual void apply(const Meas &ma) = 0;
  virtual Meas result() const = 0;
  std::string kind() const { return _kindname; };

protected:
  std::string _kindname;
};
using IFunction_ptr = std::shared_ptr<IFunction>;
} // namespace statistic
} // namespace dariadb