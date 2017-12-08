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
    server:\n\
        host: localhost\n\
        port: 5678\n\
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
    var1: <<config.server.host>>\n\
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

	EXPECT_EQ("5678", cfg.get<string>("config.server.port"));
	EXPECT_EQ(5678L, cfg.get<long>("config.server.port"));
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

	putenv((char*)"CONFIGTEST_config_server_port=8765");
	cfg.loadEnv("CONFIGTEST_");

	EXPECT_EQ(8765, cfg.get<long>("config.server.port"));
}

TEST_F(ConfigTest, List)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	vector<string> items = cfg.list("config.server");
	
	EXPECT_EQ(2, items.size());
	EXPECT_IN("host", items);
	EXPECT_IN("port", items);

	items = cfg.list("config");
	EXPECT_IN("server", items);
}

TEST_F(ConfigTest, GetSub)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	Config subcfg = cfg.getSub("config.server");
	
	EXPECT_EQ("localhost", subcfg.get<string>("server.host"));
	EXPECT_EQ("5678", subcfg.get<string>("server.port"));
}

TEST_F(ConfigTest, GlobalInstance)
{
	Config *cfg = Config::getInstance();
	cfg->loadString(yaml_str);
	
	Config *cfg2 = Config::getInstance();
	EXPECT_EQ("localhost", cfg2->get<string>("config.server.host"));
}

TEST_F(ConfigTest, GetWithDefault)
{
	TestableConfig cfg;
	cfg.loadString(yaml_str);

	EXPECT_EQ(1L, cfg.get<long>("non-exist", 1L));
	EXPECT_EQ(true, cfg.get<bool>("non-exist", true));
	EXPECT_EQ("default", cfg.get<string>("non-exist", "default"));
}

TEST_F(ConfigTest, OpenNonExistFile)
{
	TestableConfig cfg;
	try {
		cfg.loadFile("/non-exist-xxxxx-yyyyyyy");
		FAIL();
	}
	catch(const ConfigException &ex) {
		EXPECT_EQ(ENOENT, ex.code);
	}
}