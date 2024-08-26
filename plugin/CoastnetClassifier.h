#ifndef __coastnet_classifier__
#define __coastnet_classifier__

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "LuaObject.h"
#include "OsApi.h"
#include "bathy/BathyClassifier.h"
#include "bathy/BathyParms.h"


/******************************************************************************
 * BATHY CLASSIFIER
 ******************************************************************************/

class CoastnetClassifier: public BathyClassifier
{
    public:

        /*--------------------------------------------------------------------
         * Constants
         *--------------------------------------------------------------------*/

        static const char* CLASSIFIER_NAME;
        static const char* COASTNET_PARMS;
        static const char* DEFAULT_COASTNET_MODEL;

        /*--------------------------------------------------------------------
         * Typedefs
         *--------------------------------------------------------------------*/

        struct parms_t {
            string model;           // filename for xgboost model
            bool set_class;         // whether to update class_ph in extent
            bool use_predictions;   // only classify photons that are marked unclassified
            bool verbose;           // verbose settin gin XGBoost library
            parms_t(): 
                model (DEFAULT_COASTNET_MODEL),
                set_class (true),
                use_predictions (false),
                verbose (true) {};
            ~parms_t() {};
        };

        /*--------------------------------------------------------------------
         * Methods
         *--------------------------------------------------------------------*/

        static int  luaCreate   (lua_State* L);
        static void init        (void);

        bool run (const vector<BathyParms::extent_t*>& extents) override;

    protected:

        /*--------------------------------------------------------------------
         * Methods
         *--------------------------------------------------------------------*/

        CoastnetClassifier (lua_State* L, int index);
        ~CoastnetClassifier (void) override;

        /*--------------------------------------------------------------------
         * Data
         *--------------------------------------------------------------------*/

        parms_t parms;

};

#endif  /* __coastnet_classifier__ */
