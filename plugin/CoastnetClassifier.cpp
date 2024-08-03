/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "ATL24_coastnet/precompiled.h"
#include "ATL24_coastnet/confusion.h"
#include "ATL24_coastnet/utils.h"
#include "ATL24_coastnet/dataframe.h"
#include "ATL24_coastnet/raster.h"
#include "apps/features.h"
#include "xgboost.h"
#include "apps/classify_cmd.h"

#include "CoastnetClassifier.h"
#include "icesat2/BathyFields.h"

using BathyFields::extent_t;
using BathyFields::photon_t;

/******************************************************************************
 * EXTERNAL FUNCTION (to be moved)
 ******************************************************************************/

using namespace std;
using namespace ATL24_coastnet;

void classify (bool verbose, bool use_predictions, string model_filename, vector<ATL24_coastnet::classified_point2d>& p)
{
	xgboost::xgbooster xgb (verbose);
    xgb.load_model (model_filename);

    if (verbose)
    {
        clog << p.size () << " points read" << endl;
        clog << "Sorting points" << endl;
    }

    // Sort them by X
    sort (p.begin (), p.end (),
        [](const auto &a, const auto &b)
        { return a.x < b.x; });

    if (verbose)
        clog << "Classifying points" << endl;

    // Keep track of how many we skipped
    size_t used_predictions = 0;

    // Predict in batches
    const size_t batch_size = 1000;

    // For each point
    for (size_t i = 0; i < p.size (); )
    {
        // Get a vector of indexes
        vector<size_t> indexes;
        indexes.reserve (batch_size);

        // Fill the vector of indexes
        while (i < p.size () && indexes.size () != batch_size)
        {
            // Can we use the input predictions?
            if (use_predictions && p[i].cls != 0)
            {
                // Use the prediction
                p[i].prediction = p[i].cls;
                ++used_predictions;
            }
            else
            {
                // Save this index
                indexes.push_back (i);
            }

            // Go to the next one
            ++i;
        }

        // Get number of samples to predict
        const size_t rows = indexes.size ();

        // Create the features
        const size_t cols = features_per_sample ();
        vector<float> f (rows * cols);

        // Get the rasters for each point
        for (size_t j = 0; j < indexes.size (); ++j)
        {
            // Get point sample index
            assert (j < indexes.size ());
            const size_t index = indexes[j];
            assert (index < p.size ());

            // Create the raster at this point
            auto r = create_raster (p, index, sampling_params::patch_rows, sampling_params::patch_cols, sampling_params::aspect_ratio);

            // The first feature is the elevation
            f[j * cols] = p[index].z;

            // The rest of the features are the raster values
            for (size_t k = 0; k < r.size (); ++k)
            {
                const size_t n = j * cols + 1 + k;
                assert (n < f.size ());
                f[n] = r[k];
            }
        }

        // Process the batch
        const auto predictions = xgb.predict (f, rows, cols);
        assert (predictions.size () == rows);

        // Get the prediction for each batch point
        for (size_t j = 0; j < indexes.size (); ++j)
        {
            // Remap prediction
            const unsigned pred = reverse_label_map.at (predictions[j]);

            // Get point sample index
            assert (j < indexes.size ());
            const size_t index = indexes[j];
            assert (index < q.size ());

            // Save predicted value
            p[index].prediction = pred;
        }
    }
}

/******************************************************************************
 * DATA
 ******************************************************************************/

const char* CoastnetClassifier::CLASSIFIER_NAME = "coastnet";
const char* CoastnetClassifier::COASTNET_PARMS = "coastnet";
const char* CoastnetClassifier::DEFAULT_COASTNET_MODEL = "/data/coastnet_model-20240628.json";

static const char* COASTNET_PARM_MODEL = "model";
static const char* COASTNET_PARM_SET_CLASS = "set_class";
static const char* COASTNET_PARM_USE_PREDICTIONS = "use_predictions";
static const char* COASTNET_PARM_VERBOSE = "verbose";

/******************************************************************************
 * METHODS
 ******************************************************************************/

/*----------------------------------------------------------------------------
 * luaCreate - create(parms)
 *----------------------------------------------------------------------------*/
int CoastnetClassifier::luaCreate (lua_State* L)
{
    try
    {
        return createLuaObject(L, new CoastnetClassifier(L, 1));
    }
    catch(const RunTimeException& e)
    {
        mlog(e.level(), "Error creating CoastnetClassifier: %s", e.what());
        return returnLuaStatus(L, false);
    }
}

/*----------------------------------------------------------------------------
 * init
 *----------------------------------------------------------------------------*/
void CoastnetClassifier::init (void)
{
}

/*----------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
CoastnetClassifier::CoastnetClassifier (lua_State* L, int index):
    BathyClassifier(L, CLASSIFIER_NAME)
{
    /* Build Parameters */
    if(lua_istable(L, index))
    {
        /* model */
        lua_getfield(L, index, COASTNET_PARM_MODEL);
        parms.model = LuaObject::getLuaString(L, -1, true, parms.model.c_str());
        lua_pop(L, 1);

        /* set class */
        lua_getfield(L, index, COASTNET_PARM_SET_CLASS);
        parms.set_class = LuaObject::getLuaBoolean(L, -1, true, parms.set_class);
        lua_pop(L, 1);

        /* use predictions */
        lua_getfield(L, index, COASTNET_PARM_USE_PREDICTIONS);
        parms.use_predictions = LuaObject::getLuaBoolean(L, -1, true, parms.use_predictions);
        lua_pop(L, 1);

        /* verbose */
        lua_getfield(L, index, COASTNET_PARM_VERBOSE);
        parms.verbose = LuaObject::getLuaBoolean(L, -1, true, parms.verbose);
        lua_pop(L, 1);
    }
}

/*----------------------------------------------------------------------------
 * Destructor
 *----------------------------------------------------------------------------*/
CoastnetClassifier::~CoastnetClassifier (void)
{
}

/*----------------------------------------------------------------------------
 * run
 *----------------------------------------------------------------------------*/
bool CoastnetClassifier::run (const vector<extent_t*>& extents)
{
    try
    {
        // Get number of samples
        size_t number_of_samples = 0;
        for(size_t i = 0; i < extents.size(); i++)
        {
            number_of_samples += extents[i]->photon_count;
        }

        // Preallocate samples and predictions vector
        std::vector<ATL24_coastnet::classified_point2d> samples;
        samples.reserve(number_of_samples);
        mlog(INFO, "Building %ld photon samples", number_of_samples);

        // Build and add samples
        for(size_t i = 0; i < extents.size(); i++)
        {
            photon_t* photons = extents[i]->photons;
            for(size_t j = 0; j < extents[i]->photon_count; j++)
            {
                // add samples
                ATL24_coastnet::classified_point2d p = {
                    .h5_index = (i << 32) | j, // TEMPORARY HACK to unsort results below
                    .x = photons[j].x_atc,
                    .z = photons[j].ortho_h,
                    .cls = static_cast<size_t>(photons[j].class_ph),
                    .prediction = BathyFields::UNCLASSIFIED
                };
                samples.push_back(p);
            }
        }

        // Run classification
        classify(parms.verbose, parms.use_predictions, parms.model, samples);

        // Update extents
        for(auto sample: samples)
        {
            size_t i = (sample.h5_index >> 32) & 0xFFFFFFFF;
            size_t j = sample.h5_index & 0xFFFFFFFF;
            if(parms.set_class) extents[i]->photons[j].class_ph = sample.prediction;
            extents[i]->photons[j].processing_result = sample.prediction;
        }
    }
    catch (const std::exception &e)
    {
        mlog(CRITICAL, "Failed to run coastnet classifier: %s", e.what());
        return false;
    }

    return true;
}
