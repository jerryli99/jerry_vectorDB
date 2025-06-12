# Compiler settings
CXX := g++
CXXFLAGS := -O3 -Wall -shared -std=c++14 -fPIC
PYTHON_INCLUDES := $(shell python3 -m pybind11 --includes)

# Add Eigen include path (replace /path/to/eigen with actual path)
EIGEN_INCLUDES := -I ./include/linear_algebra_lib/eigen

# Target name should match the module name in PYBIND11_MODULE()
TARGET := vector_db$(shell python3-config --extension-suffix)

all: $(TARGET)

$(TARGET): bindings.cpp vector_db.cpp vector_db.h
	$(CXX) $(CXXFLAGS) $(PYTHON_INCLUDES) $(EIGEN_INCLUDES) bindings.cpp vector_db.cpp -o $(TARGET)

clean:
	rm -f *.so *.o $(TARGET)

test: $(TARGET)
	python3 test.py

.PHONY: all clean test