SHELL:=/usr/bin/bash

default: build test

##############################################################################
#
# Build
#
##############################################################################

# Recreate Makefiles when CMakeLists.txt changes
./build/debug/Makefile: CMakeLists.txt
	@./run_cmake.sh

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
		"find ./data/local/3DGL/*.csv | build/debug/train \
			--verbose \
			--num-classes=7 \
			--test-dataset={} \
			--random-seed=123 \
			--epochs=40 \
			--network-filename=coastnet_network-{}.pt \
			> coastnet_test_files-{}.txt" \
			::: $$(seq 0 4)

.PHONY: classify # Run classifier
classify: build
	@./classify.sh | parallel --verbose --lb --jobs=15
	@./get_bathy_scores.sh

.PHONY: train_coastnet_surface # Train a model
train_coastnet_surface: build
	@build=release ./train_coastnet_surface.sh \
		"./data/local/3DGL/*.csv" \
		./coastnet-surface.pt

.PHONY: classify_coastnet_surface # Run classifier
classify_coastnet_surface: build
	@mkdir -p ./predictions
	@build=release ./classify_coastnet_surface.sh \
		"./data/local/3DGL/*.csv" \
		./coastnet-surface.pt \
		./predictions

.PHONY: score_coastnet_surface # Get surface scores
score_coastnet_surface:
	@./get_coastnet_surface_scores.sh "./predictions/*_surface.txt"

.PHONY: cross_validate_surface # Cross validate surface classifier
cross_validate_surface: build
	@./cross_validate_surface.sh "./data/local/3DGL/ATL03_*.csv"

##############################################################################
#
# View data
#
##############################################################################

TRUTH_FNS=$(shell find ./data/local/3DGL/ATL03_*.csv | head)

.PHONY: view_truth # View truth labels
view_truth:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_classifications.py -- --verbose {}" \
		::: ${TRUTH_FNS}

PREDICTION_FNS=$(shell find ./predictions/ATL03_*_classified_?.csv | head)

.PHONY: view_predictions # View prediction labels
view_predictions:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_classifications.py -- --verbose {}" \
		::: ${PREDICTION_FNS}

SURFACE_PREDICTION_FNS=$(shell find ./predictions/ATL03_*_classified_surface.csv | tail)

.PHONY: view_surface_predictions # View water surface prediction labels
view_surface_predictions:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_classifications.py -- --verbose {}" \
		::: ${SURFACE_PREDICTION_FNS}

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
