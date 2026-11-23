CC ?= gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -O2
SAN_FLAGS := -std=c11 -Wall -Wextra -Wpedantic -Werror -g -O1 -fsanitize=address,undefined

BUILD_DIR := build
APP_BIN := $(BUILD_DIR)/sistema_bancario
TEST_BIN := $(BUILD_DIR)/test_conta
SAN_TEST_BIN := $(BUILD_DIR)/test_conta_san

.PHONY: all run test sanitize clean

all: $(APP_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(APP_BIN): src/main.c src/conta.c src/conta.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) src/main.c src/conta.c -o $@

$(TEST_BIN): tests/test_conta.c src/conta.c src/conta.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) tests/test_conta.c src/conta.c -o $@

$(SAN_TEST_BIN): tests/test_conta.c src/conta.c src/conta.h | $(BUILD_DIR)
	$(CC) $(SAN_FLAGS) tests/test_conta.c src/conta.c -o $@

run: $(APP_BIN)
	./$(APP_BIN)

test: $(APP_BIN) $(TEST_BIN)
	./$(TEST_BIN)
	bash tests/test_cli.sh ./$(APP_BIN)

sanitize: $(SAN_TEST_BIN)
	./$(SAN_TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)
