#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Main
#include <algorithm>
#include <boost/test/unit_test.hpp>

#include <compression.h>
#include <storage/manifest.h>
#include <storage/page.h>
#include <storage/page_manager.h>
#include <utils/fs.h>
#include <utils/utils.h>

using dariadb::storage::PageManager;
using dariadb::storage::Manifest;

BOOST_AUTO_TEST_CASE(ManifestFileTest) {
  const std::string fname = "manifest";
  if (dariadb::utils::fs::path_exists(fname)) {
    dariadb::utils::fs::rm(fname);
  }

  Manifest m(fname);
  std::list<std::string> names{"1", "2", "3"};
  for (auto n : names) {
    m.page_append(n);
  }
  auto lst = m.page_list();
  BOOST_CHECK_EQUAL(lst.size(), names.size());
  BOOST_CHECK_EQUAL_COLLECTIONS(lst.begin(), lst.end(), names.begin(),
                                names.end());

  if (dariadb::utils::fs::path_exists(fname)) {
    dariadb::utils::fs::rm(fname);
  }
}

BOOST_AUTO_TEST_CASE(PageManagerInstance) {
  const std::string storagePath = "testStorage";
  PageManager::start(PageManager::Params(storagePath, 1, 1));
  BOOST_CHECK(PageManager::instance() != nullptr);
  PageManager::stop();
}

dariadb::Time add_meases(dariadb::Id id, dariadb::Time t, size_t count,
                         dariadb::Meas::MeasList &addeded) {
  dariadb::Meas first;
  first.id = id;
  first.time = t;

  for (size_t i = 0; i < count; i++, t++) {
    first.flag = dariadb::Flag(i);
    first.time = t;
    first.value = dariadb::Value(i);

    auto res = PageManager::instance()->append(first);
    addeded.push_back(first);
    BOOST_CHECK(res.writed == 1);
  }

  return t;
}

BOOST_AUTO_TEST_CASE(PageManagerReadWrite) {
  const std::string storagePath = "testStorage/";
  const size_t chinks_count = 30;
  const size_t chunks_size = 256;

  if (dariadb::utils::fs::path_exists(storagePath)) {
    dariadb::utils::fs::rm(storagePath);
  }

  PageManager::start(
      PageManager::Params(storagePath, chinks_count, chunks_size));
  BOOST_CHECK(PageManager::instance() != nullptr);

  auto start_time = dariadb::Time(0);
  auto t = dariadb::Time(0);
  dariadb::Meas::MeasList addeded;
  const dariadb::Id id_count(2);
  dariadb::IdSet all_id_set;
  for (size_t i = 0; i < 3; i++) {
    auto cur_id = dariadb::Id(i % id_count);
    all_id_set.insert(cur_id);
    t = add_meases(cur_id, t, chinks_count, addeded);
  }
  
  dariadb::Time minTime(t);
  dariadb::IdArray all_id_array{all_id_set.begin(), all_id_set.end()};
  { // Chunks load
    // must return all of appended chunks;
    dariadb::storage::ChunksList all_chunks;

    PageManager::instance()->flush();
    PageManager::instance()
        ->chunksByIterval(
            dariadb::storage::QueryInterval(all_id_array, 0, 0, t))
        ->readAll(&all_chunks);
    auto readed_t = dariadb::Time(0);

    for (auto ch : all_chunks) {

      minTime = std::min(minTime, ch->info->minTime);
      ch->bw->reset_pos();

      dariadb::compression::CopmressedReader crr(ch->bw, ch->info->first);
      BOOST_CHECK_EQUAL(ch->info->first.value, addeded.front().value);
      addeded.pop_front();
      for (uint32_t i = 0; i < ch->info->count; i++) {
        auto m = crr.read();
        auto a = addeded.front();
        BOOST_CHECK_EQUAL(m.value, a.value);
        addeded.pop_front();
        readed_t++;
      }
    }
    dariadb::Time minT = std::numeric_limits<dariadb::Time>::max(),
                  maxT = std::numeric_limits<dariadb::Time>::min();
    BOOST_CHECK(
        PageManager::instance()->minMaxTime(dariadb::Id(0), &minT, &maxT));
    BOOST_CHECK_EQUAL(minT, dariadb::Time(0));

    {
      dariadb::Time end_time(t / 2);
      dariadb::storage::ChunksList chunk_list;
      PageManager::instance()
          ->chunksByIterval(dariadb::storage::QueryInterval(
              all_id_array, 0, start_time, end_time))
          ->readAll(&chunk_list);

      for (auto &v : chunk_list) {
        BOOST_CHECK(v->info->minTime <= end_time);
      }

      auto chunks_map = PageManager::instance()->chunksBeforeTimePoint(
          dariadb::storage::QueryTimePoint(all_id_array, 0, end_time));
      BOOST_CHECK_EQUAL(chunks_map.size(), size_t(id_count));

      for (auto &kv : chunks_map) {
        auto chunk = kv.second;
        auto is_in_interval = dariadb::utils::inInterval(
            chunk->info->minTime, chunk->info->maxTime, end_time);
        BOOST_CHECK(is_in_interval || chunk->info->maxTime < end_time);
      }

      auto ids_array = PageManager::instance()->getIds();
      BOOST_CHECK_EQUAL(ids_array.size(), size_t(2));
      BOOST_CHECK(std::find(ids_array.begin(), ids_array.end(),
                            dariadb::Id(0)) != ids_array.end());
      BOOST_CHECK(std::find(ids_array.begin(), ids_array.end(),
                            dariadb::Id(1)) != ids_array.end());
    }
  }
  BOOST_CHECK(dariadb::utils::fs::path_exists(storagePath));
  BOOST_CHECK(dariadb::utils::fs::ls(storagePath).size() ==
              3); // page +index+manifest

  PageManager::stop();

  if (dariadb::utils::fs::path_exists(storagePath)) {
    dariadb::utils::fs::rm(storagePath);
  }
}

BOOST_AUTO_TEST_CASE(PageManagerReadWriteWithContinue) {
  const std::string storagePath = "testStorage";
  const size_t chunks_count = 10;
  const size_t chunks_size = 200;
  auto t = dariadb::Time(0);

  if (dariadb::utils::fs::path_exists(storagePath)) {
    dariadb::utils::fs::rm(storagePath);
  }
  dariadb::Meas::MeasList addeded;
  PageManager::start(
      PageManager::Params(storagePath, chunks_count, chunks_size));
  dariadb::Meas first;
  first.id = 1;
  first.time = t;
  { add_meases(1, t, chunks_size / 10, addeded); }
  PageManager::stop();

  auto fname = dariadb::utils::fs::ls(storagePath, ".page").front();
  auto header = dariadb::storage::Page::readHeader(fname);
  BOOST_CHECK_EQUAL(header.chunk_per_storage, chunks_count);
  BOOST_CHECK_EQUAL(header.chunk_size, chunks_size);
  BOOST_CHECK_EQUAL(header.count_readers, size_t(0));

  auto iheader = dariadb::storage::Page::readIndexHeader(fname + "i");
  BOOST_CHECK_EQUAL(iheader.chunk_per_storage, chunks_count);
  BOOST_CHECK_EQUAL(iheader.chunk_size, chunks_size);
  BOOST_CHECK(iheader.is_sorted);

  PageManager::start(
      PageManager::Params(storagePath, chunks_count, chunks_size));

  // need to load current page;
  auto mintime_chunks = PageManager::instance()->chunksBeforeTimePoint(
      dariadb::storage::QueryTimePoint(dariadb::IdArray{1}, 0,
                                       PageManager::instance()->minTime()));
  BOOST_CHECK_GE(mintime_chunks.size(), size_t(0));

  PageManager::stop();

  if (dariadb::utils::fs::path_exists(storagePath)) {
    dariadb::utils::fs::rm(storagePath);
  }
}

BOOST_AUTO_TEST_CASE(PageManagerMultiPageRead) {
  const std::string storagePath = "testStorage";
  const size_t chunks_count = 10;
  const size_t chunks_size = 200;
  auto t = dariadb::Time(0);

  if (dariadb::utils::fs::path_exists(storagePath)) {
    dariadb::utils::fs::rm(storagePath);
  }
  dariadb::Meas::MeasList addeded;
  PageManager::start(
      PageManager::Params(storagePath, chunks_count, chunks_size));
  dariadb::Meas first;
  first.id = 1;
  first.time = t;
  const size_t page_count = 4;

  while (dariadb::utils::fs::ls(storagePath, ".page").size() <= page_count) {
    t=add_meases(1, t, chunks_size / 10, addeded);
  }

  dariadb::storage::QueryInterval qi(dariadb::IdArray{1}, 0,
                                     addeded.front().time, addeded.back().time);

  dariadb::storage::QueryTimePoint qt(dariadb::IdArray{ 1 }, 0,
	  addeded.front().time + (addeded.back().time - addeded.front().time) / 2);

  dariadb::storage::ChunksList chlist;
  PageManager::instance()->chunksByIterval(qi)->readAll(&chlist);
  //BOOST_CHECK_GE(chlist.size(), page_count*chunks_count);

  size_t writed=addeded.size();
  size_t readed=0;
  for (auto ch : chlist) {
    ch->bw->reset_pos();

    dariadb::compression::CopmressedReader crr(ch->bw, ch->info->first);
    BOOST_CHECK_EQUAL(ch->info->first.value, addeded.front().value);
    addeded.pop_front();
    readed++;
    for (uint32_t i = 0; i < ch->info->count; i++) {
      auto m = crr.read();
      readed++;
      auto a = addeded.front();
      BOOST_CHECK_EQUAL(m.value, a.value);
      addeded.pop_front();
    }
  }
  BOOST_CHECK_EQUAL(readed,writed);

  
  auto tp_chunks=PageManager::instance()->chunksBeforeTimePoint(qt);
  BOOST_CHECK_EQUAL(tp_chunks.size(), size_t(1));
  BOOST_CHECK_LE(tp_chunks[1]->info->minTime, qt.time_point);
  BOOST_CHECK_GE(tp_chunks[1]->info->maxTime, qt.time_point);
  PageManager::stop();

  if (dariadb::utils::fs::path_exists(storagePath)) {
	  dariadb::utils::fs::rm(storagePath);
  }
}
