CXX = g++
CXXFLAGS = -Wall -Wextra -I../src -I.

all: bitmap_test tinymap_test
	@echo "Running tests..."
	@./bitmap_test --success
	@./tinymap_test --success
	@echo "All tests passed!"

bitmap_test: catch_amalgamated.cpp test_bitmapindex.cpp ../src/BitmapIndex.h
	$(CXX) $(CXXFLAGS) catch_amalgamated.cpp test_bitmapindex.cpp -o bitmap_test

tinymap_test: catch_amalgamated.cpp test_tinymap.cpp ../src/TinyMap.h
	$(CXX) $(CXXFLAGS) catch_amalgamated.cpp test_tinymap.cpp -o tinymap_test

clean:
	rm -f bitmap_test tinymap_test

.PHONY: all clean