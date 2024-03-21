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
# Preprocessing
#
##############################################################################

.PHONY: preprocess # Preprocess input data
preprocess:
	@mkdir -p ./input/manual
	@mkdir -p ./input/synthetic
	@./preprocess.sh  "./data/local/3DGL/*.csv" ./input/manual
	@./preprocess.sh  "./data/local/synthetic/*.csv" ./input/synthetic

##############################################################################
#
# Machine Learning
#
##############################################################################

##############################################################################
#
# Multi-class models
#
##############################################################################

MC_INPUT=./input/manual/*.csv

.PHONY: train_mc # Train a multi-class model
train_mc: build
	@parallel --lb --jobs=15 \
		"find $(MC_INPUT) | build/debug/train \
			--verbose \
			--num-classes=7 \
			--test-dataset={} \
			--random-seed=123 \
			--epochs=40 \
			--network-filename=coastnet_network-{}.pt \
			> coastnet_test_files-{}.txt" \
			::: $$(seq 0 4)

.PHONY: classify_mc # Run multi-class classifier
classify_mc: build
	@./classify.sh | parallel --verbose --lb --jobs=15

##############################################################################
#
# Binary models
#
##############################################################################

BINARY_INPUT=./input/manual/*.csv

##############################################################################
# Surface
##############################################################################

.PHONY: train_surface # Train water surface model
train_surface: build
	@build=release ./train_surface.sh \
		"$(BINARY_INPUT)" \
		./models/coastnet-surface.pt

.PHONY: classify_surface # Run water surface classifier
classify_surface: build
	@mkdir -p ./predictions
	@build=release ./classify_surface.sh \
		"$(BINARY_INPUT)" \
		./models/coastnet-surface.pt \
		./predictions
	@./compute_scores.sh "./predictions/*_classified_surface.csv" 41

.PHONY: score_surface # Get water surface scores
score_surface:
	@echo "Surface Scores"
	@./summarize_scores.sh "./predictions/*_classified_surface_results.txt" 41

.PHONY: cross_val_surface # Cross validate water surface classifier
cross_val_surface: build
	@./cross_validate_surface.sh "$(BINARY_INPUT)"

##############################################################################
# Bathy
##############################################################################

.PHONY: train_bathy # Train bathy model
train_bathy: build
	@build=release ./train_coastnet_bathy.sh \
		"$(BINARY_INPUT)" \
		./models/coastnet-bathy.pt

.PHONY: classify_bathy # Run bathy classifier
classify_bathy: build
	@build=release ./classify_bathy.sh \
		"$(BINARY_INPUT)" \
		./models/coastnet-bathy.pt \
		./predictions
	@./compute_scores.sh "./predictions/*_classified_bathy.csv" 40

.PHONY: score_bathy # Get bathy scores
score_bathy:
	@echo "Bathy Scores"
	@./summarize_scores.sh "./predictions/*_classified_bathy_results.txt" 40

.PHONY: cross_val_bathy # Cross validate bathy classifier
cross_val_bathy: build
	@./cross_validate_bathy.sh "$(BINARY_INPUT)"

##############################################################################
# Blunder detection
##############################################################################

.PHONY: check_surface # Run blunder detection
check_surface: build
	@build=debug ./check.sh \
		"./predictions/*_classified_surface.csv" \
		./predictions
	@./compute_scores.sh "./predictions/*_classified_surface_checked.csv" 41

.PHONY: check_bathy # Run blunder detection
check_bathy: build
	@build=debug ./check.sh \
		"./predictions/*_classified_bathy.csv" \
		./predictions
	@./compute_scores.sh "./predictions/*_classified_bathy_checked.csv" 40

.PHONY: score_checked # Generate scores on checked files
score_checked:
	@echo "Checked Surface Scores"
	@./summarize_scores.sh "./predictions/*_classified_surface_checked_results.txt" 41
	@echo "Checked Bathy Scores"
	@./summarize_scores.sh "./predictions/*_classified_bathy_checked_results.txt" 40

##############################################################################
#
# View data
#
##############################################################################

TRUTH_FNS=$(shell find ./input/ATL03_*.csv | tail)

.PHONY: view_truth # View truth labels
view_truth:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_classifications.py -- --verbose {}" \
		::: ${TRUTH_FNS}

PREDICTION_FNS=$(shell find ./predictions/ATL03_*_classified_?.csv | head)

.PHONY: view_predictions # View prediction labels
view_predictions:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${PREDICTION_FNS}

SURFACE_PREDICTION_FNS=$(shell find ./predictions/*_classified_surface.csv | tail)

.PHONY: view_surface # View water surface prediction labels
view_surface:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${SURFACE_PREDICTION_FNS}

BATHY_PREDICTION_FNS=$(shell find ./predictions/*_classified_bathy.csv | shuf | tail)

.PHONY: view_bathy # View bathy prediction labels
view_bathy:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${BATHY_PREDICTION_FNS}

CHECKED_PREDICTION_FNS=$(shell find ./predictions/*_checked_bathy.csv | shuf | tail)

.PHONY: view_checked # View checked prediction labels
view_checked:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${CHECKED_PREDICTION_FNS}

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
