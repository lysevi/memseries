#include <libdariadb/engine.h>
#include <libdariadb/timeutil.h>
#include <libdariadb/storage/manifest.h>
#include <libdariadb/utils/fs.h>
#include <libdariadb/utils/logger.h>

#include <boost/program_options.hpp>

#include <iostream>

namespace po = boost::program_options;

std::string storage_path = "dariadb_storage";
bool verbose = false;
std::string set_var;
std::string new_base_name;

std::string compact_from, compact_to;
bool time_in_iso_format=false;

class CtlLogger : public dariadb::utils::ILogger {
public:
  CtlLogger() {}
  ~CtlLogger() {}
  void message(dariadb::utils::LOG_MESSAGE_KIND kind, const std::string &msg) {
    switch (kind) {
    case dariadb::utils::LOG_MESSAGE_KIND::FATAL:
      std::cerr << msg << std::endl;
      break;
    case dariadb::utils::LOG_MESSAGE_KIND::INFO:
      if (verbose) {
        std::cout << msg << std::endl;
      }
      break;
    case dariadb::utils::LOG_MESSAGE_KIND::MESSAGE:
      if (verbose) {
        std::cout << msg << std::endl;
      }
      break;
    }
  }
};

void check_path_exists() {
  if (!dariadb::utils::fs::path_exists(storage_path)) {
    std::cerr << "path " << storage_path << " not exists" << std::endl;
    std::exit(1);
  }
}

int main(int argc, char *argv[]) {
  po::options_description desc("Allowed options");
  auto aos = desc.add_options();
  aos("help", "produce help message.");
  aos("settings", "print all settings.");
  aos("format", "print storage format version.");
  aos("verbose", "verbose output.");
  aos("compress", "compress all aof files.");
  aos("time-iso-format", "if set, all time param is in iso format (\"20020131T235959\")");
  aos("compact", "compact all page files to one.");
  aos("storage-path",po::value<std::string>(&storage_path)->default_value(storage_path),"path to storage.");
  aos("set", po::value<std::string>(&set_var)->default_value(set_var),"change setting variable.\nexample: --set=\"strategy=MEMORY\"");
  aos("create",po::value<std::string>(&new_base_name)->default_value(new_base_name),"create new database in selected folder.");
  aos("compact-from",po::value<std::string>(&compact_from)->default_value(compact_from),"compaction period start date (example 2002-01-20 00:00:00.000).");
  aos("compact-to",po::value<std::string>(&compact_to)->default_value(compact_to),"compaction period end date (example 2002-01-20 23:59:59.000).");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
  } catch (std::exception &ex) {
    dariadb::logger("Error: ", ex.what());
    exit(1);
  }
  po::notify(vm);

  if (vm.count("help") || argc == 1) {
    std::cout << desc << std::endl;
    std::exit(0);
  }

  if (vm.count("verbose")) {
    std::cout << "verbose=on" << std::endl;
    verbose = true;
  }

  if (vm.count("time-iso-format")) {
    std::cout << "time-iso-format=on" << std::endl;
    time_in_iso_format = true;
  }

  dariadb::utils::ILogger_ptr log_ptr{new CtlLogger()};
  dariadb::utils::LogManager::start(log_ptr);

  if (new_base_name.size() != 0) {
      std::cout<<"create "<<new_base_name<<std::endl;
    auto settings = dariadb::storage::Settings_ptr{
        new dariadb::storage::Settings(new_base_name)};
    auto e = std::make_unique<dariadb::storage::Engine>(settings);
    e = nullptr;
    std::exit(1);
  }

  if (vm.count("format")) {
    check_path_exists();
    dariadb::storage::Manifest m(dariadb::utils::fs::append_path(
        storage_path, dariadb::storage::MANIFEST_FILE_NAME));
    std::cout << "version: " << m.get_version() << std::endl;
    std::exit(0);
  }

  if (vm.count("settings")) {
    check_path_exists();
    dariadb::storage::Settings s(storage_path);
    std::cout << s.dump() << std::endl;
    std::exit(0);
  }

  if (set_var.size() != 0) {
    check_path_exists();
    dariadb::storage::Settings s(storage_path);
    s.change(set_var);
    s.save();
    std::exit(0);
  }

  if (vm.count("compress")) {
    auto settings = dariadb::storage::Settings_ptr{
        new dariadb::storage::Settings(storage_path)};
    auto e = std::make_unique<dariadb::storage::Engine>(settings);
    e->compress_all();
    e->stop();
  }

  if (vm.count("compact")) {
    if(compact_from.size()==0 && compact_to.size()==0){
        auto settings = dariadb::storage::Settings_ptr{
            new dariadb::storage::Settings(storage_path)};

        auto e = std::make_unique<dariadb::storage::Engine>(settings);

        e->compactTo(0);
        e->stop();
        std::exit(0);
    }

    if(compact_from.size()!=0 && compact_to.size()!=0){
        dariadb::Time from=0, to=0;
        if(time_in_iso_format){
            from=dariadb::timeutil::from_iso_string(compact_from);
            to=dariadb::timeutil::from_iso_string(compact_to);
        }else{
            from=dariadb::timeutil::from_string(compact_from);
            to=dariadb::timeutil::from_string(compact_to);
        }
        std::cout<<"compaction from "<<dariadb::timeutil::to_string(from)
                <<" to "<<dariadb::timeutil::to_string(to)<<std::endl;

        auto settings = dariadb::storage::Settings_ptr{
            new dariadb::storage::Settings(storage_path)};
        auto e = std::make_unique<dariadb::storage::Engine>(settings);

        e->compactbyTime(from,to);
        e->stop();
        std::exit(0);
    }

    std::cerr<<"you must set compact_from and compact_to."<<std::endl;
    std::exit(1);
  }
}