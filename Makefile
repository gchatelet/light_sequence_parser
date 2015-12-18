CFLAGS=-Iinclude -O3 -std=c++0x -Wall -Werror
ifdef DEBUG
CFLAGS+=-g -O0 -fno-omit-frame-pointer -fsanitize=address
endif

GTEST_FLAGS=-isystem ${GTEST_DIR}/include -pthread
BUILD_DIR=build

INCLUDES:=$(wildcard include/sequence/*.hpp include/sequence/details*.hpp)

SRC:=$(wildcard src/*.cpp)
OBJECTS:=$(patsubst %.cpp,$(BUILD_DIR)/%.o,$(notdir $(SRC)))

TEST_SRC:=$(wildcard test/*.cpp)
TEST_OBJECTS:=$(patsubst %.cpp,$(BUILD_DIR)/_%.o,$(notdir $(TEST_SRC)))

.PHONY: all
all: lss test

.PHONY: lss
lss: $(BUILD_DIR)/lss

$(BUILD_DIR)/lss: app/lss.cpp $(OBJECTS)
	$(CXX) $(CFLAGS) -static -static-libstdc++ $^ -o $@

$(BUILD_DIR)/%.o: src/%.cpp $(INCLUDES) | $(BUILD_DIR)
	$(CXX) $(CFLAGS) $(GTEST_FLAGS) -c $< -o $@

$(BUILD_DIR)/libsequenceparser.a: $(OBJECTS)
	ar -cvq $@ $^

.PHONY: test
test: $(BUILD_DIR)/test
	./$<

$(BUILD_DIR)/_%.o: test/%.cpp | $(BUILD_DIR) check-test-env
	$(CXX) $(CFLAGS) $(GTEST_FLAGS) -c $< -o $@

$(BUILD_DIR)/test: $(TEST_OBJECTS) $(OBJECTS) $(BUILD_DIR)/gtest-all.o $(BUILD_DIR)/gtest_main.o | check-test-env
	$(CXX) $(CFLAGS) $(GTEST_FLAGS) $^ -o $@

$(BUILD_DIR)/gtest-all.o: $(GTEST_DIR)/src/gtest-all.cc | $(BUILD_DIR) check-test-env
	$(CXX) $(GTEST_FLAGS) -I${GTEST_DIR} -c $< -o $@

$(BUILD_DIR)/gtest_main.o: $(GTEST_DIR)/src/gtest_main.cc | $(BUILD_DIR) check-test-env
	$(CXX) $(GTEST_FLAGS) -I${GTEST_DIR} -c $< -o $@

.PHONY: clean
clean:
	rm -Rf $(BUILD_DIR)

.PHONY: check-test-env
check-test-env:
	@if test -z "$$GTEST_DIR"; then echo "GTEST_DIR is undefined, unzip https://googletest.googlecode.com/files/gtest-1.7.0.zip somewhere and set GTEST_DIR env"; fi;

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
