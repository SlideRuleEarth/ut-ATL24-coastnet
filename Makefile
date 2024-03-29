SHELL:=/usr/bin/bash

default: build test

##############################################################################
#
# Build
#
##############################################################################

# Recreate Makefiles when CMakeLists.txt changes
./build/debug/Makefile: CMakeLists.txt
	@./scripts/run_cmake.sh

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
	@./scripts/preprocess.sh  "./data/local/3DGL/*.csv" ./input/manual
	@./scripts/preprocess.sh  "./data/local/synthetic/*.csv" ./input/synthetic

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
	@./scripts/classify.sh | parallel --verbose --lb --jobs=15

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
	@build=release ./scripts/train_surface.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-surface-$(ID).pt

.PHONY: classify_surface # Run water surface classifier
classify_surface: build
	@mkdir -p ./predictions/$(ID)
	@build=release ./scripts/classify_surface.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-surface-$(ID).pt \
		./predictions/$(ID)
	@./scripts/compute_scores.sh "./predictions/$(ID)/*_classified_surface.csv" 41

.PHONY: score_surface # Get water surface scores
score_surface:
	@echo "Surface Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_surface_results.txt" 41

.PHONY: cross_val_surface # Cross validate water surface classifier
cross_val_surface: build
	@build=release ./scripts/cross_validate_surface.sh "$(INPUT_GLOB)" $(ID)

##############################################################################
# Bathy
##############################################################################

.PHONY: train_bathy # Train bathy model
train_bathy: build
	@build=release ./scripts/train_bathy.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-bathy-$(ID).pt

.PHONY: classify_bathy # Run bathy classifier
classify_bathy: build
	@mkdir -p ./predictions/$(ID)
	@build=release ./scripts/classify_bathy.sh \
		"$(INPUT_GLOB)" \
		./models/coastnet-bathy-$(ID).pt \
		./predictions/$(ID)
	@./scripts/compute_scores.sh "./predictions/$(ID)/*_classified_bathy.csv" 40

.PHONY: score_bathy # Get bathy scores
score_bathy:
	@echo "Bathy Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_bathy_results.txt" 40

.PHONY: cross_val_bathy # Cross validate bathy classifier
cross_val_bathy: build
	@build=release ./scripts/cross_validate_bathy.sh "$(INPUT_GLOB)" $(ID)

##############################################################################
# Combine bathy and surface predictions into a single file
##############################################################################

.PHONY: combine # Combine surface and bathy predictions into one
combine: build
	@build=debug ./scripts/combine.sh $(ID)
	@./scripts/compute_scores.sh "./predictions/$(ID)/*_classified_combined.csv"

.PHONY: score_combined # Generate scores on combined files
score_combined:
	@echo "Combined Surface Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_combined_results.txt" 41
	@echo "Combined Bathy Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_combined_results.txt" 40

##############################################################################
# Blunder detection
##############################################################################

.PHONY: check_combined # Run blunder detection
check_combined: build
	@build=debug ./scripts/check.sh \
		"./predictions/$(ID)/*_classified_combined.csv" \
		./predictions/$(ID)
	@./scripts/compute_scores.sh "./predictions/$(ID)/*_classified_combined_checked.csv"

.PHONY: score_checked # Generate scores on checked files
score_checked:
	@echo "Checked Surface Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_combined_checked_results.txt" 41
	@echo "Checked Bathy Scores"
	@./scripts/summarize_scores.sh "./predictions/$(ID)/*_classified_combined_checked_results.txt" 40

##############################################################################
#
# Mix real and synthetic data
#
##############################################################################

.PHONY: classify_surface_mixed # Run surface classifier using synthetic model
classify_surface_mixed:
	@mkdir -p ./predictions/mixed
	@build=release ./scripts/classify_surface.sh \
		"./input/manual/*.csv" \
		./models/coastnet-surface-synthetic.pt \
		./predictions/mixed
	@./scripts/compute_scores.sh "./predictions/mixed/*_classified_surface.csv" 41

.PHONY: classify_bathy_mixed # Run bathy classifier using synthetic model
classify_bathy_mixed:
	@mkdir -p ./predictions/mixed
	@build=release ./scripts/classify_bathy.sh \
		"./input/manual/*.csv" \
		./models/coastnet-bathy-synthetic.pt \
		./predictions/mixed
	@./scripts/compute_scores.sh "./predictions/mixed/*_classified_bathy.csv" 40

.PHONY: score_mixed # Generate scores on mixed files
score_mixed:
	@./scripts/summarize_scores.sh \
		"./predictions/mixed/*_classified_surface_results.txt" 41 \
		> mixed-surface_summary.txt
	@./scripts/summarize_scores.sh \
		"./predictions/mixed/*_classified_bathy_results.txt" 40 \
		> mixed-bathy_summary.txt
	@cat mixed-surface_summary.txt
	@cat mixed-bathy_summary.txt

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

COMBINED_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_combined.csv | shuf | tail)

.PHONY: view_combined # View combined prediction labels
view_combined:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${COMBINED_PREDICTION_FNS}

CHECKED_PREDICTION_FNS=$(shell find ./predictions/$(ID)/*_classified_combined_checked.csv | shuf | tail)

.PHONY: view_checked # View checked prediction labels
view_checked:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${CHECKED_PREDICTION_FNS}

MIXED_BATHY_PREDICTION_FNS=$(shell find ./predictions/mixed/*_classified_bathy.csv | shuf | tail)

.PHONY: view_mixed_bathy # View mixed bathy prediction labels
view_mixed_bathy:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose {}" \
		::: ${MIXED_BATHY_PREDICTION_FNS}

SELECTIONS=\
		ATL03_20200212214153_07360601_005_01_gt2r \
		ATL03_20210704023000_01601201_005_01_gt3r \
		ATL03_20230213042035_08341807_006_01_gt1l \
		ATL03_20200606090624_10970705_005_01_gt3l

.PHONY: view_selected # View selected prediction labels
view_selected:
	@parallel --lb --jobs=100 \
		"streamlit run ../ATL24_rasters/apps/view_predictions.py -- --verbose ./predictions/$(ID)/{}_classified_combined_checked.csv" \
		::: ${SELECTIONS}

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

.PHONY: synthetic_full_run # Run everything with synthetically labelled inputs
synthetic_full_run:
	@$(MAKE) ID=synthetic --no-print-directory train_surface
	@$(MAKE) ID=synthetic --no-print-directory classify_surface
	@$(MAKE) ID=synthetic --no-print-directory score_surface
	@$(MAKE) ID=synthetic --no-print-directory train_bathy
	@$(MAKE) ID=synthetic --no-print-directory classify_bathy
	@$(MAKE) ID=synthetic --no-print-directory score_bathy

.PHONY: manual_cross_val # Cross validate manually labelled inputs
manual_cross_val:
	@$(MAKE) --no-print-directory cross_val_surface
	@$(MAKE) --no-print-directory cross_val_bathy

.PHONY: synthetic_cross_val # Cross validate synthetically labelled inputs
synthetic_cross_val:
	@$(MAKE) ID=synthetic --no-print-directory cross_val_surface
	@$(MAKE) ID=synthetic --no-print-directory cross_val_bathy

.PHONY: classify_full_run # Classify all datasets
classify_full_run:
	@$(MAKE) --no-print-directory classify_surface
	@$(MAKE) --no-print-directory classify_bathy
	@$(MAKE) ID=synthetic --no-print-directory classify_surface
	@$(MAKE) ID=synthetic --no-print-directory classify_bathy
	@$(MAKE) --no-print-directory classify_surface_mixed
	@$(MAKE) --no-print-directory classify_bathy_mixed

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
