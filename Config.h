#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdarg.h>
#include <pthread.h>
#include <errno.h>

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

#include <yaml-cpp/yaml.h>

class ConfigException : public std::exception
{
public:
	enum {
		OK = 0,
		BASE = 10000,
		OPEN_FILE_FAIL,
		
		KEY_NOT_FOUND,
		INVALID_BOOL_VALUE,
		INVALID_LONG_VALUE,
		
	};
	
public:
	int code;
	static const int msglen = 256;
	char msg[msglen];
public:
	ConfigException(int errcode, const char *fmt, ...) throw();
	ConfigException(const ConfigException &ex) throw();
	virtual ~ConfigException() throw() {};
	virtual const char* what() const throw() { return msg; };
};

class Config
{
	friend YAML::Emitter & operator << (YAML::Emitter &em, const Config &cfg);
public:
	Config() {};
	~Config() {};

	void dump() const;
	void loadStream(std::istream &in);
	void loadString(const std::string &str);
	void loadFile(const std::string &fn);
	void loadEnv(const std::string &prefix);
	
	void loadUrl(const std::string &url);
	// load config urls from the last to the first
	void loadv(const std::vector<std::string> &urls);
	void load(const char *url, ...);
	
	template<typename T>
	T get(const std::string &key) const { return fromValue<T>(getItem(key)); };
	
	template<typename T>
	T get(const std::string &key, const T &defaultValue) const
	try {
		return get<T>(key);
	}
	catch(const ConfigException &ex) {
		if (ex.code == ConfigException::KEY_NOT_FOUND) {
			return defaultValue;
		}
		else {
			throw;
		}
	}
	
	std::vector<std::string> list(const std::string &dir) const;
	Config getSub(const std::string &dir);
private:
	std::string getItem(const std::string &key) const;
	void setItem(const std::string &key, const std::string &value);

	template<typename T>
	static T fromValue(const std::string &value);
protected:
	std::map<std::string, std::string> items;
protected:
	void loadYamlNode(std::string prefix, const YAML::Node &node);

// single instance
private:
	static std::auto_ptr<Config> theConfig;
public:
	static Config * getInstance() { return theConfig.get(); };
};

YAML::Emitter & operator << (YAML::Emitter &em, const Config &cfg);

#endif
