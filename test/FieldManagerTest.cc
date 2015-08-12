
#include <diFieldManager.h>

#include <gtest/gtest.h>

#include <boost/foreach.hpp>
#include <fstream>
#include <string>

#define MILOGGER_CATEGORY "diField.test.FieldManagerTest"
#include "miLogger/miLogging.h"
#include <log4cpp/Category.hh>

//#define DEBUG_MESSAGES

TEST(FieldManagerTest, TestRW)
{
    const std::string fileName(TEST_BUILDDIR "testrun_fimexio_rw.nc");
    {
        const std::string origFileName(TEST_SRCDIR "/test_fimexio_rw.nc");
        std::ifstream orig(origFileName.c_str());
        ASSERT_TRUE(orig) << "no file '" << origFileName << "'";
        std::ofstream copy(fileName.c_str());
        copy << orig.rdbuf();
        ASSERT_TRUE(orig);
    }

    const int NEWDATA = 17;

    FieldRequest fieldrequest;
    fieldrequest.paramName = "lwe_precipitation_rate";
    fieldrequest.ptime = miutil::miTime("2013-02-27 00:00:00");
    fieldrequest.refTime = "2013-02-27T00:00:00";

    std::auto_ptr<FieldManager> fmanager(new FieldManager());
    {
        const std::string model_w = "fmtest_write";
        std::vector<std::string> modelConfigInfo;
        modelConfigInfo.push_back("model=" + model_w + " t=fimex sourcetype=netcdf file=" + fileName + " writeable=true");
        ASSERT_TRUE(fmanager->addModels(modelConfigInfo));

        fieldrequest.modelName = model_w;
        Field* fieldW = 0;
        ASSERT_TRUE(fmanager->makeField(fieldW, fieldrequest));
        ASSERT_TRUE(fieldW != 0);
        ASSERT_EQ(1, fieldW->data[0]);
        fieldW->data[0] = NEWDATA;
        ASSERT_TRUE(fmanager->writeField(fieldrequest, fieldW));
        delete fieldW;
    }
    {
        const std::string model_r = "fmtest_read";
        std::vector<std::string> modelConfigInfo;
        modelConfigInfo.push_back("model=" + model_r + " t=fimex sourcetype=netcdf file=" + fileName);
        ASSERT_TRUE(fmanager->addModels(modelConfigInfo));

        fieldrequest.modelName = model_r;
        Field* fieldR = 0;
        ASSERT_TRUE(fmanager->makeField(fieldR, fieldrequest));
        ASSERT_TRUE(fieldR != 0);
        ASSERT_EQ(NEWDATA, fieldR->data[0]);
        delete fieldR;
    }
}

namespace /* anonymous */ {
template<typename V, typename C>
C& operator<<(C& container, const V& value)
{ container.push_back(value); return container; }
} // anonymous namespace

