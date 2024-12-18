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
	@./scripts/preprocess.sh  "./data/remote/latest/*.csv" ./input/manual

##############################################################################
#
# Machine Learning
#
##############################################################################

INPUT=./input/manual/*.csv

.PHONY: train # Train a model
train: BUILD=debug
train: build
	find $(INPUT) | build/$(BUILD)/train \
		--verbose \
		--num-classes=7 \
		--test-dataset=0 \
		--train-test-split=0.0 \
		--random-seed=123 \
		--epochs=100 \
		--model-filename=coastnet_model.json

.PHONY: classify # Run classifier
classify: BUILD=debug
classify: build
	@mkdir -p predictions
	@find $(INPUT) | parallel --verbose --lb --jobs=4 --halt now,fail=1 \
		"build/$(BUILD)/classify --verbose --num-classes=7 --model-filename=coastnet_model.json < {} > predictions/{/.}_classified.csv"

.PHONY: score # Get scores
score: build
	@./scripts/get_scores.sh "./predictions/*_classified.csv" ""

.PHONY: xval # Cross-validate
xval: build
	@parallel --lb --jobs=1 --halt now,fail=1 \
		"find $(INPUT) | build/release/train \
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

.PHONY: score_xval # Compute xval scores
score_xval:
	@./scripts/get_scores.sh "./predictions/*_classified_0.csv" "_xval_0"
	@./scripts/get_scores.sh "./predictions/*_classified_1.csv" "_xval_1"
	@./scripts/get_scores.sh "./predictions/*_classified_2.csv" "_xval_2"
	@./scripts/get_scores.sh "./predictions/*_classified_3.csv" "_xval_3"
	@./scripts/get_scores.sh "./predictions/*_classified_4.csv" "_xval_4"

##############################################################################
#
# View results
#
##############################################################################

.PHONY: view # View predictions
view:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_viewer/view_predictions.py -- --verbose {}" \
		::: $$(find ./predictions/*_classified.csv | head)

##############################################################################
#
# Make everything
#
##############################################################################

.PHONY: everything # Make everything. This will take a while.
everything:
	@scripts/yesno.bash "This will clean, preprocess, retrain, classify, xval, ... etc."
	@echo "Build/test"
	@$(MAKE) --no-print-directory BUILD=release clean build test
	@echo "Cleaning local files"
	@rm -rf input/manual
	@rm -rf predictions/
	@echo "Preprocessing"
	@$(MAKE) --no-print-directory preprocess
	@echo "Training"
	@$(MAKE) --no-print-directory train BUILD=release
	@echo "Classifying/scoring"
	@$(MAKE) --no-print-directory classify score BUILD=release
	@echo "Cross-validating"
	@$(MAKE) --no-print-directory xval score_xval BUILD=release

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
