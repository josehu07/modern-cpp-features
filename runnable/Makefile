CC=g++
CXXFLAGS=-Wall -Werror -O3 -DNDEBUG

STDS=11 14 17 20
BINS=$(patsubst %,cpp%,$(STDS))


.PHONY: all
all: $(BINS)

$(BINS): cpp%: cpp%.cpp
	$(CC) $(CXXFLAGS) -std=c++$* $< -o $@ -lpthread


.PHONY: clean
clean:
	rm -f $(BINS)
