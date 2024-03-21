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
# Cross-validated models
#
##############################################################################

XVAL_INPUT=./input/manual/*.csv

.PHONY: train_xval # Train a model with cross-validation
train_xval: build
	@parallel --lb --jobs=15 \
		"find $(XVAL_INPUT) | build/debug/train \
			--verbose \
			--num-classes=7 \
			--test-dataset={} \
			--random-seed=123 \
			--epochs=40 \
			--network-filename=coastnet_network-{}.pt \
			> coastnet_test_files-{}.txt" \
			::: $$(seq 0 4)

.PHONY: classify_xval # Run classifier with cross-validation
classify_xval: build
	@./classify.sh | parallel --verbose --lb --jobs=15
	@./get_bathy_scores.sh

##############################################################################
#
# Deployable models
#
##############################################################################

DEPLOY_INPUT=./input/manual/*.csv

##############################################################################
# Surface
##############################################################################

.PHONY: train_surface # Train water surface model
train_surface: build
	@build=release ./train_coastnet_surface.sh \
		"$(DEPLOY_INPUT)" \
		./models/coastnet-surface.pt

.PHONY: classify_surface # Run water surface classifier
classify_surface: build
	@mkdir -p ./predictions
	@build=release ./classify_coastnet_surface.sh \
		"$(DEPLOY_INPUT)" \
		./models/coastnet-surface.pt \
		./predictions

.PHONY: score_surface # Get water surface scores
score_surface:
	@./get_coastnet_surface_scores.sh "./predictions/*_results_surface.txt"

.PHONY: cross_val_surface # Cross validate water surface classifier
cross_val_surface: build
	@./cross_validate_surface.sh "$(DEPLOY_INPUT)"

##############################################################################
# Bathy
##############################################################################

.PHONY: train_bathy # Train bathy model
train_bathy: build
	@build=release ./train_coastnet_bathy.sh \
		"$(DEPLOY_INPUT)" \
		./models/coastnet-bathy.pt

.PHONY: classify_bathy # Run bathy classifier
classify_bathy: build
	@build=release ./classify_coastnet_bathy.sh \
		"$(DEPLOY_INPUT)" \
		./models/coastnet-bathy.pt \
		./predictions

.PHONY: score_bathy # Get bathy scores
score_bathy:
	@./get_coastnet_bathy_scores.sh "./predictions/*_results_bathy.txt"

.PHONY: cross_val_bathy # Cross validate bathy classifier
cross_val_bathy: build
	@./cross_validate_bathy.sh "$(DEPLOY_INPUT)"

##############################################################################
# Blunder detection
##############################################################################

.PHONY: check_surface # Run blunder detection
check_surface: build
	@build=debug ./check_coastnet_surface.sh \
		"./predictions/*_classified_surface.csv" \
		./predictions

.PHONY: check_bathy # Run blunder detection
check_bathy: build
	@build=debug ./check_coastnet_bathy.sh \
		"./predictions/*_classified_bathy.csv" \
		./predictions

.PHONY: get_checked_scores # Generate scores on checked files
get_checked_scores: build
	@build=debug ./get_checked_surface_scores.sh \
		"./predictions/*_classified_surface.csv" \
		./predictions
	@build=debug ./get_checked_bathy_scores.sh \
		"./predictions/*_classified_bathy.csv" \
		./predictions

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
