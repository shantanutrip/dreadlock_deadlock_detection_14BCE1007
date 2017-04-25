#define BOOST_TEST_MODULE Dreadlock Test

#include <iostream>
#include <thread>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <atomic>
#include <vector>
#include <boost/test/unit_test.hpp>
#include "dreadlock.hpp"

int main() {
  DreadlockManager m;
  Dreadlock d(m);
  std::thread t1([&d]{
    for (int i = 0; i < 100; ++i) {
      d.lock();
      //  std::cout << "thread 1 lock\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      d.unlock();
    }
    d.lock();
    //    std::cout << "thread 1 lock PREDEADLOCK CONDITION\n";
    BOOST_CHECK_THROW(d.lock(), std::logic_error);
    d.unlock();
  });

  std::thread t2([&d]{
    for (int i = 0; i < 100; ++i) {
      d.lock();
      //      std::cout << "thread 2 lock\n";
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      d.unlock();
    }
  });
  t1.join();
  t2.join();
  return 0;
}