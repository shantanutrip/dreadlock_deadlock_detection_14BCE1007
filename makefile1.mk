COMPILE_OPTIONS := -g -std=c++11
LINKER_OPTIONS := -pthread -lboost_system -lboost_thread -lboost_unit_test_framework
SOURCE_FILES := dreadlock_test.cpp ut_starter.cpp 
HEADER_FILES := dreadlock.hpp bloom_filter.hpp
SOURCE_FILES1 := dreadlock_one_thread_no_deadlocks.cpp

dreadlock_test_one: $(SOURCE_FILES1) $(HEADER_FILES)
	g++ $(COMPILE_OPTIONS) -o $@ $(SOURCE_FILES1) $(LINKER_OPTIONS)

clean:
	rm dreadlock_test_one
