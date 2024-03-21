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

ID=manual
INPUT_DIR=./input/$(ID)
INPUT_GLOB=$(INPUT_DIR)/*.csv

##############################################################################
# Surface
##############################################################################

.PHONY: train_surface # Train water surface model
train_surface: build
	@build=release ./train_surface.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-surface-$(ID).pt

.PHONY: classify_surface # Run water surface classifier
classify_surface: build
	@mkdir -p ./predictions/$(ID)
	@build=release ./classify_surface.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-surface-$(ID).pt \
		./predictions/$(ID)
	@./compute_scores.sh "./predictions/$(ID)/*_classified_surface.csv" 41

.PHONY: score_surface # Get water surface scores
score_surface:
	@echo "Surface Scores"
	@./summarize_scores.sh "./predictions/$(ID)/*_classified_surface_results.txt" 41

.PHONY: cross_val_surface # Cross validate water surface classifier
cross_val_surface: build
	@./cross_validate_surface.sh "$(INPUT_GLOB)"

##############################################################################
# Bathy
##############################################################################

.PHONY: train_bathy # Train bathy model
train_bathy: build
	@build=release ./train_bathy.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-bathy-$(ID).pt

.PHONY: classify_bathy # Run bathy classifier
classify_bathy: build
	@mkdir -p ./predictions/$(ID)
	@build=release ./classify_bathy.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-bathy-$(ID).pt \
		./predictions/$(ID)
	@./compute_scores.sh "./predictions/*_classified_bathy.csv" 40

.PHONY: score_bathy # Get bathy scores
score_bathy:
	@echo "Bathy Scores"
	@./summarize_scores.sh "./predictions/$(ID)/*_classified_bathy_results.txt" 40

.PHONY: cross_val_bathy # Cross validate bathy classifier
cross_val_bathy: build
	@./cross_validate_bathy.sh "$(INPUT_GLOB)"

##############################################################################
# Blunder detection
##############################################################################

.PHONY: check_surface # Run blunder detection
check_surface: build
	@build=debug ./check.sh \
		"./predictions/$(ID)/*_classified_surface.csv" \
		./predictions/$(ID)
	@./compute_scores.sh "./predictions/$(ID)/*_classified_surface_checked.csv" 41

.PHONY: check_bathy # Run blunder detection
check_bathy: build
	@build=debug ./check.sh \
		"./predictions/$(ID)/*_classified_bathy.csv" \
		./predictions/$(ID)
	@./compute_scores.sh "./predictions/$(ID)/*_classified_bathy_checked.csv" 40

.PHONY: score_checked # Generate scores on checked files
score_checked:
	@echo "Checked Surface Scores"
	@./summarize_scores.sh "./predictions/$(ID)/*_classified_surface_checked_results.txt" 41
	@echo "Checked Bathy Scores"
	@./summarize_scores.sh "./predictions/$(ID)/*_classified_bathy_checked_results.txt" 40

##############################################################################
#
# View data
#
##############################################################################

TRUTH_FNS=$(shell find $(INPUT_GLOB) | shuf | tail)

.PHONY: view_truth # View truth labels
view_truth:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_classifications.py -- --verbose {}" \
		::: ${TRUTH_FNS}

SURFACE_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_surface.csv | tail)

.PHONY: view_surface # View water surface prediction labels
view_surface:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${SURFACE_PREDICTION_FNS}

BATHY_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_bathy.csv | shuf | tail)

.PHONY: view_bathy # View bathy prediction labels
view_bathy:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${BATHY_PREDICTION_FNS}

CHECKED_SURFACE_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_surface_checked.csv | shuf | tail)

.PHONY: view_checked_surface # View checked water surface prediction labels
view_checked_surface:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${CHECKED_SURFACE_PREDICTION_FNS}

CHECKED_BATHY_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_bathy_checked.csv | shuf | tail)

.PHONY: view_checked_bathy # View checked bathy prediction labels
view_checked_bathy:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${CHECKED_BATHY_PREDICTION_FNS}

##############################################################################
#
# Full runs
#
##############################################################################

.PHONY: manual_full_run # Run everything with manually labelled inputs
manual_full_run:
	@$(MAKE) --no-print-directory train_surface
	@$(MAKE) --no-print-directory classify_surface
	@$(MAKE) --no-print-directory score_surface
	@$(MAKE) --no-print-directory train_bathy
	@$(MAKE) --no-print-directory classify_bathy
	@$(MAKE) --no-print-directory score_bathy

.PHONY: synthetic_full_run # Run everything with manually labelled inputs
synthetic_full_run:
	@$(MAKE) ID=synthetic --no-print-directory train_surface
	@$(MAKE) ID=synthetic --no-print-directory classify_surface
	@$(MAKE) ID=synthetic --no-print-directory score_surface
	@$(MAKE) ID=synthetic --no-print-directory train_bathy
	@$(MAKE) ID=synthetic --no-print-directory classify_bathy
	@$(MAKE) ID=synthetic --no-print-directory score_bathy

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
