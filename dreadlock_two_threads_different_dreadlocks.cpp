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
    Dreadlock d1(m), d2(m);
    std::thread t1([&d1, &d2]{
   	for (int i = 0; i < 1000; ++i) {
        d1.lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        d1.unlock();
    	}
    });

    std::thread t2([&d1, &d2]{
        for (int i = 0; i < 1000; ++i) {
            d2.lock();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            d2.unlock();
        }
    });
    t1.join();
    t2.join();
}