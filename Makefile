SHELL:=/usr/bin/bash

default: build test

##############################################################################
#
# Build
#
##############################################################################

# Recreate Makefiles when CMakeLists.txt changes
./build/debug/Makefile: CMakeLists.txt
	@bash ./run_cmake.sh

.PHONY: build # Build all targets
build: ./build/debug/Makefile
	@cd build/debug && make -j
	@cd build/release && make -j

.PHONY: clean # Remove all build dependencies
clean:
	@rm -rf build

##############################################################################
#
# Test
#
##############################################################################

.PHONY: unit_test
unit_test: BUILD=debug
unit_test:
	@parallel --jobs 24 --halt now,fail=1 "echo {} && {}" ::: build/$(BUILD)/test_*

.PHONY: test # Run tests
test:
	@echo "Testing..."
	@$(MAKE) --no-print-directory unit_test BUILD=debug
	@$(MAKE) --no-print-directory unit_test BUILD=release

##############################################################################
#
# Machine Learning
#
##############################################################################

.PHONY: train # Train a model
train: build
	@parallel --lb --jobs=15 \
		"find ./data/local/OSU/*.csv | build/debug/train \
			--verbose \
			--num-classes=8 \
			--test-dataset={} \
			--random-seed=123 \
			--network-filename=resnet_network-{}.pt \
			> resnet_test_files-{}.txt" \
			::: $$(seq 0 5)

.PHONY: classify # Run classifier
classify: build
	@bash run_classify_atl24.sh | parallel

##############################################################################
#
# Get help by running
#
#     $ make help
#
##############################################################################
.PHONY: help # Generate list of targets with descriptions
help:
	@grep '^.PHONY: .* #' Makefile | sed 's/\.PHONY: \(.*\) # \(.*\)/\1	\2/' | expand -t20
