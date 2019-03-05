all: simd

CXXFLAGS = -std=c++11 -march=native -Isrc/mio/include
ifeq ($(DEBUG),1)
        CXXFLAGS += -g3 -O0
else
        CXXFLAGS += -g3 -O3
endif

simd.cc: src/simd.h

simd: simd.cc
	$(CXX) $(CXXFLAGS) -o $@ $^

#all:
	#@echo "make: Entering directory '/Users/jhester/p/vroom/src'"
	#@Rscript -e 'pkgload::load_all(quiet = FALSE)'
	#@echo "make: Leaving directory '/Users/jhester/p/vroom/src'"

#test:
	#@echo "make: Entering directory '/Users/jhester/p/vroom/tests/testthat'"
	#@Rscript -e 'devtools::test()'
	#@echo "make: Leaving directory '/Users/jhester/p/vroom/tests/testthat'"

clean:
	rm simd
	#@Rscript -e 'devtools::clean_dll()'
