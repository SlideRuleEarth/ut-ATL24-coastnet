SHELL:=/usr/bin/bash

default: build test

##############################################################################
#
# Build
#
##############################################################################

# Recreate Makefiles when CMakeLists.txt changes
./build/debug/Makefile: CMakeLists.txt
	@mkdir -p ./build/debug
	@mkdir -p ./build/release
	@cd build/debug && cmake -D CMAKE_BUILD_TYPE=Debug ../..
	@cd build/release && cmake -D CMAKE_BUILD_TYPE=Release ../..

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
	@./scripts/preprocess.sh  "./data/local/3DGL/*.csv" ./input/manual
	@./scripts/preprocess.sh  "./data/local/OSU/*.csv" ./input/manual

##############################################################################
#
# Machine Learning
#
##############################################################################

INPUT=./input/manual/*.csv

.PHONY: train # Train a model
train: build
	find $(INPUT) | build/debug/train \
		--verbose \
		--num-classes=7 \
		--test-dataset=0 \
		--train-test-split=0.0 \
		--random-seed=123 \
		--epochs=100 \
		--model-filename=coastnet_model.json

.PHONY: classify # Run classifier
classify: build
	@mkdir -p predictions
	@find $(INPUT) | parallel --verbose --lb --jobs=4 --halt now,fail=1 \
		"build/debug/classify --verbose --num-classes=7 --model-filename=coastnet_model.json --results-filename=predictions/{/.}_results.txt < {} > predictions/{/.}_classified.csv"
	@./scripts/summarize_scores.sh "./predictions/*_results.txt" 41
	@./scripts/summarize_scores.sh "./predictions/*_results.txt" 40

.PHONY: xval # Cross-validate
xval: build
	@parallel --lb --jobs=1 --halt now,fail=1 \
		"find $(INPUT) | build/debug/train \
			--verbose \
			--num-classes=7 \
			--train-test-split=0.2 \
			--test-dataset={} \
			--random-seed=123 \
			--epochs=40 \
			--model-filename=coastnet_model-{}.json \
			> coastnet_test_files-{}.txt" \
			::: $$(seq 0 4)
	@./scripts/classify.sh | parallel --verbose --lb --jobs=4 --halt now,fail=1
	@./scripts/summarize_scores.sh "./predictions/*_results-?.txt" 41
	@./scripts/summarize_scores.sh "./predictions/*_results-?.txt" 40

##############################################################################
# Blunder detection
##############################################################################

.PHONY: check # Run blunder detection
check: build
	@build=debug ./scripts/check.sh \
		"./predictions/*_classified.csv" \
		./predictions
	@./scripts/compute_scores.sh "./predictions/*_classified_checked.csv"

.PHONY: score_checked # Generate scores on checked files
score_checked:
	@echo "Checked Surface Scores"
	@./scripts/summarize_scores.sh "./predictions/*_classified_checked_results.txt" 41
	@echo "Checked Bathy Scores"
	@./scripts/summarize_scores.sh "./predictions/*_classified_checked_results.txt" 40

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

##############################################################################
#
# Publication
#
##############################################################################

.PHONY: plots # Generate plots for publications
plots:
	./scripts/create_plots.sh
	./scripts/create_mixed_plots.sh

##############################################################################
#
# Get help by running
#
#     $ make help
#
##############################################################################
.PHONY: help # Generate list of targets with descriptions
help:
	@grep '^.PHONY: .* #' Makefile | sed 's/\.PHONY: \(.*\) # \(.*\)/\1	\2/' | expand -t25
