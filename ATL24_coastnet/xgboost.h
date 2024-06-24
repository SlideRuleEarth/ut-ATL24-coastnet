#pragma once

#include "utils.h"

namespace ATL24_coastnet
{

namespace xgboost
{

namespace constants
{
    constexpr unsigned max_depth = 8;
    //constexpr unsigned min_child_weight = 4;
    //constexpr double gamma = 0.280;
    //constexpr double colsample_bytree = 0.943;
    //constexpr double subsample = 0.360;
    //constexpr double eta = 0.360;
    //constexpr unsigned num_boosting_rounds = 100;
    constexpr float missing_data = std::numeric_limits<float>::max ();
} // namespace constants

template<typename F,typename... Args>
void call_xgboost (F f, Args... args)
{
    using namespace std;

    int err = invoke (f, args...);

    if (err != 0)
        throw runtime_error (string ("Exception: ")
            + string (__FILE__)
            + ": "
            + to_string (__LINE__)
            + ": "
            + XGBGetLastError ());

}

// Helper class for XGBoost DMatrix allocation
class dmatrix
{
    public:
    dmatrix (const std::vector<float> &features, const size_t rows, const size_t cols)
    {
        call_xgboost (XGDMatrixCreateFromMat, &features[0], rows, cols, constants::missing_data, &handle);
    }
    DMatrixHandle *get_handle_address ()
    {
        return &handle;
    }
    void add_labels (const std::vector<uint32_t> &labels)
    {
        call_xgboost (XGDMatrixSetUIntInfo, handle, "label", &labels[0], labels.size ());
    }
    void add_weights (const std::vector<uint32_t> &labels)
    {
        using namespace std;

        // Count occurrance of each class
        unordered_map<uint32_t,double> counts;
        for (size_t i = 0; i < labels.size (); ++i)
            ++counts[labels[i]];

        // Determine the weights from the counts
        vector<float> w (labels.size ());
        for (size_t i = 0; i < w.size (); ++i)
            w[i] = counts[labels[i]] / labels.size ();

        call_xgboost (XGDMatrixSetFloatInfo, handle, "weight", &w[0], w.size ());
    }
    ~dmatrix ()
    {
        call_xgboost (XGDMatrixFree, handle);
    }

    private:
    DMatrixHandle handle;
};

// XGBooster model interface
//
// This class is used to:
//
//     1. Train a model
//     2. Save a model to disk
//     3. Load a model from disk
//     4. Predict using the loaded model
//
// You can also warm start by loading a saved model and continuing to
// train it with new data.
class xgbooster
{
    public:
    explicit xgbooster (const bool verbose)
        : verbose (verbose)
        , initialized (false)
        , trained (false)
    {
    }
    ~xgbooster ()
    {
        if (initialized)
        {
            if (verbose)
                std::clog << "Destroying xgbooster" << std::endl;

            XGBoosterFree (booster);
        }
    }
    void train (const std::vector<float> &features,
        const std::vector<uint32_t> &labels,
        const size_t rows,
        const size_t cols,
        const size_t epochs = 100,
        const bool use_gpu = true)
    {
        using namespace std;

        if (verbose)
            clog << "Training" << endl;

        // Check invariants
        assert (!features.empty ());
        assert (features.size () == rows * cols);

        // Create the DMatrix
        dmatrix m (features, rows, cols);
        m.add_labels (labels);
        m.add_weights (labels);

        // Initialize booster if needed
        if (!initialized)
        {
            if (verbose)
                clog << "Creating booster using "
                    << (use_gpu ? "CUDA" : "CPU")
                    << endl;
            call_xgboost (XGBoosterCreate, m.get_handle_address (), 1, &booster);
            call_xgboost (XGBoosterSetParam, booster, "device", use_gpu ? "cuda" : "cpu");
            initialized = true;
        }

        // Set model parameters
        call_xgboost (XGBoosterSetParam, booster, "objective", "multi:softmax");
        call_xgboost (XGBoosterSetParam, booster, "num_class", "7");

        // These values were determined by the hyper-pararmeter search
        call_xgboost (XGBoosterSetParam, booster, "max_depth", to_string (constants::max_depth).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "min_child_weight", to_string (constants::min_child_weight).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "gamma", to_string (constants::gamma).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "colsample_bytree", to_string (constants::colsample_bytree).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "subsample", to_string (constants::subsample).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "eta", to_string (constants::eta).c_str ());
        //call_xgboost (XGBoosterSetParam, booster, "num_boosting_rounds", to_string (constants::num_boosting_rounds).c_str ());

        // Do the training
        for (size_t i = 0; i < epochs; ++i)
        {
            // Train
            call_xgboost (XGBoosterUpdateOneIter, booster, i, *m.get_handle_address ());

            // Evaluate
            const char* eval_names = "train";
            const char* eval_result = NULL;
            call_xgboost (XGBoosterEvalOneIter, booster, i, m.get_handle_address (), &eval_names, 1, &eval_result);

            if (verbose)
            {
                clog << "Epoch " << i+1 << "/" << epochs << " :";
                clog << eval_result << endl;
            }
        }

        trained = true;
    }
    void save_model (const std::string &filename) const
    {
        using namespace std;
        if (verbose)
            clog << "Saving model to " << filename << endl;

        call_xgboost (XGBoosterSaveModel, booster, filename.c_str ());
    }
    void load_model (const std::string &filename)
    {
        using namespace std;

        // Initialize booster if needed
        if (!initialized)
        {
            if (verbose)
                clog << "Creating booster" << filename << endl;

            call_xgboost (XGBoosterCreate, nullptr, 0, &booster);
            initialized = true;
        }

        if (verbose)
            clog << "Loading model from " << filename << endl;

        call_xgboost (XGBoosterLoadModel, booster, filename.c_str ());
    }
    std::vector<uint32_t> predict (const std::vector<float> &features,
        const size_t rows,
        const size_t cols,
        const bool use_gpu = false)
    {
        using namespace std;

        if (verbose)
            clog << "Getting predictions" << endl;

        // Check invariants
        assert (!features.empty ());
        assert (features.size () == rows * cols);

        // Set booster parameters
        call_xgboost (XGBoosterSetParam, booster, "device", use_gpu ? "cuda" : "cpu");

        // Create the DMatrix
        dmatrix m (features, rows, cols);

        char const config[] =
            "{\"training\": false,"
            " \"type\": 0,"
            " \"iteration_begin\": 0,"
            " \"iteration_end\": 0,"
            " \"strict_shape\": true}";
        const uint64_t *shape;
        uint64_t dim;
        const float *results = NULL;
        call_xgboost (XGBoosterPredictFromDMatrix, booster, *m.get_handle_address (), config, &shape, &dim, &results);

        // Check invariants
        assert(dim == 2);
        assert(shape[0] == rows);
        assert(shape[1] == 1);

        vector<uint32_t> predictions (rows);

        for (size_t i = 0; i < rows; ++i)
            predictions[i] = results[i];

        return predictions;
    }

    private:
    const bool verbose;
    bool initialized;
    BoosterHandle booster;
    bool trained;
};

} // namespace xgboost

} // namespace ATL24_coastnet
