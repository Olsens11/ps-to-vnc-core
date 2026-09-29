# Host-side rung tests for the staged clean reconstruction.
# Each target links the actual product source under test against only the
# smallest host shims needed to exercise target-independent behavior.

CC ?= cc
CFLAGS ?=
COMMON_CFLAGS := -O2 -std=c99 -Wall -Wextra -Werror -pedantic -I../src
BUILD_DIR := .build

PROTOCOL_TEST := $(BUILD_DIR)/transport_protocol_test
PHYSICAL_TEST := $(BUILD_DIR)/transport_physical_stream_test
READINESS_TEST := $(BUILD_DIR)/transport_physical_readiness_test

.PHONY: unit clean
unit: $(PROTOCOL_TEST) $(PHYSICAL_TEST) $(READINESS_TEST)
	./$(PROTOCOL_TEST)
	./$(PHYSICAL_TEST)
	./$(READINESS_TEST)
	python3 unit/pi_wire_protocol_test.py

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(PROTOCOL_TEST): unit/transport_protocol_test.c ../src/transport/protocol.c ../src/transport/protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_CFLAGS) 		unit/transport_protocol_test.c 		../src/transport/protocol.c 		-o $@

$(PHYSICAL_TEST): unit/transport_physical_stream_test.c 		unit/transport_host_stubs/kernel.h 		unit/transport_host_stubs/ps2ip.h 		../src/transport/physical_stream.c 		../src/transport/physical_stream.h 		../src/transport/protocol.c 		../src/transport/protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_CFLAGS) 		-Iunit/transport_host_stubs 		unit/transport_physical_stream_test.c 		../src/transport/physical_stream.c 		../src/transport/protocol.c 		-o $@

$(READINESS_TEST): unit/transport_physical_readiness_test.c 		unit/transport_host_stubs/kernel.h 		unit/transport_host_stubs/ps2ip.h 		../src/transport/physical_stream.c 		../src/transport/physical_stream.h 		../src/transport/protocol.c 		../src/transport/protocol.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(COMMON_CFLAGS) 		-Iunit/transport_host_stubs 		unit/transport_physical_readiness_test.c 		../src/transport/physical_stream.c 		../src/transport/protocol.c 		-o $@

clean:
	rm -rf $(BUILD_DIR)
