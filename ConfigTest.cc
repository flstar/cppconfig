#include <stdio.h>
#include <iostream>
#include <gtest/gtest.h>

#include "Config.h"

using namespace std;

class TestableConfig : public Config
{
};

class ConfigTestEnv : public testing::Environment
{
  public:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

class ConfigTest : public testing::Test
{
  public:
    virtual void SetUp()
    {
    }
    virtual void TearDown()
    {
    }
};

int main(int argc, char *argv[])
{
  testing::AddGlobalTestEnvironment(new ConfigTestEnv);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

#define EXPECT_IN(elem, vect) do {\
	bool exists = false; \
	for (size_t i=0; i<vect.size(); i++) { \
		if (elem == vect[i]) { \
			exists = true; \
			break; \
		} \
	} \
	EXPECT_TRUE(exists); \
} while(0)

const string yaml_str = "\
config:\n\
  bitsflow:\n\
    agent: localhost:1234\n\
    service: 5678\n\
  testnum1: 100\n\
  testnum2: 1K\n\
  testnum3: 1m\n\
  testnum4: 1G\n\
  testnum5: 1T\n\
  testnum-invalid: 1x\n\
 \n\
  booltrue: true\n\
  boolfalse: FALSE\n\
  boolinvalid: invalid-bool\n\
";

 
TEST_F(ConfigTest, Dump)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);
	cfg.dump();
}

TEST_F(ConfigTest, LongValue)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	EXPECT_EQ("5678", cfg.get<string>("config.bitsflow.service"));
	EXPECT_EQ(5678L, cfg.get<long>("config.bitsflow.service"));
	try {
		cfg.get<long>("config.testnum-invalid");
		FAIL();
	}
	catch(const ConfigException &ex) {
		EXPECT_EQ(ConfigException::INVALID_LONG_VALUE, ex.code);
		printf("%s\n", ex.what());
	}
}

TEST_F(ConfigTest, GetNonExist)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);
	try {
		cfg.get<string>("non-exist");
		FAIL();
	}
	catch(const ConfigException &ex) {
		EXPECT_EQ(ConfigException::KEY_NOT_FOUND, ex.code);
	}
}

TEST_F(ConfigTest, BoolValue)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	EXPECT_EQ(true, cfg.get<bool>("config.booltrue"));
	EXPECT_EQ(false, cfg.get<bool>("config.boolfalse"));
	try {
		cfg.get<bool>("config.boolinvalid");
		FAIL();
	}
	catch(const ConfigException &ex) {
		EXPECT_EQ(ConfigException::INVALID_BOOL_VALUE, ex.code);
		printf("%s\n", ex.what());
	}
}

TEST_F(ConfigTest, LoadEnv)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	putenv((char*)"CONFIGTEST_config_bitsflow_service=8765");
	cfg.loadEnv("CONFIGTEST_");

	EXPECT_EQ(8765, cfg.get<long>("config.bitsflow.service"));
}

TEST_F(ConfigTest, List)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	vector<string> items = cfg.list("config.bitsflow");
	
	EXPECT_EQ(2, items.size());
	EXPECT_IN("agent", items);
	EXPECT_IN("service", items);

	items = cfg.list("config");
	EXPECT_IN("bitsflow", items);
}

TEST_F(ConfigTest, GetSubConfig)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	Config subcfg = cfg.getSubConfig("config.bitsflow");
	
	EXPECT_EQ("localhost:1234", subcfg.get<string>("bitsflow.agent"));
	EXPECT_EQ("5678", subcfg.get<string>("bitsflow.service"));
}

TEST_F(ConfigTest, GlobalInstance)
{
	Config *cfg = Config::getInstance();
	cfg->loadString(yaml_str);
	
	Config *cfg2 = Config::getInstance();
	EXPECT_EQ("localhost:1234", cfg2->get<string>("config.bitsflow.agent"));
}

TEST_F(ConfigTest, GetWithDefault)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	EXPECT_EQ(1L, cfg.get<long>("non-exist", 1L));
	EXPECT_EQ(true, cfg.get<bool>("non-exist", true));
	EXPECT_EQ("default", cfg.get<string>("non-exist", "default"));
}
