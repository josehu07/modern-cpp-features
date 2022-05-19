CC:=g++
CXXFLAGS:=-Wall -Werror -O3 -DNDEBUG

BINS:=cpp11 cpp14 cpp17 cpp20


.PHONY: all
all: $(BINS)


cpp11 cpp14 cpp17: cpp%: cpp%.cpp
	$(CC) $(CXXFLAGS) -std=c++$* $< -o $@ -lpthread

cpp20: cpp%: cpp%.cpp
	$(CC) $(CXXFLAGS) -fcoroutines -std=c++$* $< -o $@ -lpthread


.PHONY: clean
clean:
	rm -f $(BINS)


.PHONY: run-all
run-all:
	@for bin in $(BINS); do ./$$bin; done
