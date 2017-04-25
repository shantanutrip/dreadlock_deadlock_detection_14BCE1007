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
        Dreadlock d1(m);
        for (int i = 0; i < 100; ++i) {
           d1.lock();
           d1.unlock();
        }

        std::thread t1([&d1]{
          for (int i = 0; i < 1000; ++i) {
              d1.lock();
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
              d1.unlock();
          }
        });
        t1.join();
        return 0;
}