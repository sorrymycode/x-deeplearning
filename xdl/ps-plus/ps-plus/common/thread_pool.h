/* Copyright (C) 2016-2018 Alibaba Group Holding Limited

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#ifndef PS_PLUS_COMMON_THREAD_POOL_H
#define PS_PLUS_COMMON_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <cstring>

namespace ps {

class ThreadPool {
public:
  ThreadPool(size_t threads);
  ~ThreadPool();
  void Schedule(const std::function<void()>& func);

  static ThreadPool* Global();
private:
  void Loop();

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  
  std::mutex queue_mutex_;
  std::condition_variable condition_;
  bool stop_;
};

inline void QuickMemcpy(void* dest, const void* src, size_t count) {
  const size_t block_size = 1 << 30; // 1G
  if (count < block_size * 2) {
    memcpy(dest, src, count);
  } else {
    char* dest_ptr = (char*)dest;
    const char* src_ptr = (char*)src;
    std::promise<bool> ok;
    std::atomic<size_t> counter(count / block_size);
    while (count > 0) {
      size_t s = count < block_size * 2 ? count : block_size;
      ThreadPool::Global()->Schedule([=, &ok, &counter]{
        memcpy(dest_ptr, src_ptr, s);
        if (--counter == 0) {
          ok.set_value(true);
        }
      });
      dest_ptr += s;
      src_ptr += s;
      count -= s;
    }
    ok.get_future().wait();
  }
}

}

#endif

