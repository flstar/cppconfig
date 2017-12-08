#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Config.h"

using namespace std;

#define LOG(fmt, ...) fprintf(stderr, "[%s@%s:%d]" fmt "\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__)

// static members
auto_ptr<Config> Config::theConfig(new Config);

ConfigException::ConfigException(int errcode, const char *fmt, ...) throw()
	: exception()
{
	code = errcode;
	sprintf(msg, "[%d] ", code);
	int usedlen = strlen(msg);
	va_list args;
	va_start(args, fmt);
	vsnprintf(msg + usedlen, msglen - usedlen, fmt, args);
	va_end(args);
}

ConfigException::ConfigException(const ConfigException &ex) throw()
{
	code = ex.code;
	strncpy(msg, ex.msg, msglen-1);
	msg[msglen-1] = '\0';
}

void Config::dump() const
{
	printf("==================== Dump config ====================\n");
	map<string, string>::const_iterator iter;
	for (iter = itemMap_.begin(); iter != itemMap_.end(); ++iter) {
		printf("%s = %s\n", iter->first.c_str(), iter->second.c_str());
	}
	printf("=====================================================\n");
	return;
}

YAML::Emitter & operator << (YAML::Emitter &em, const Config &cfg)
{
	string prefix = ".";
	em << YAML::BeginMap;
	map<string, string>::const_iterator iter;
	for (iter = cfg.itemMap_.begin(); iter != cfg.itemMap_.end(); ++iter) {
		const string &key = "." + iter->first;
		const string &value = iter->second;
		
		while (true) {
			// If prefix is the same
			int len = prefix.size();
			if (key.compare(0, len, prefix) == 0) {
				// search next '.' or the end of string
				size_t pos = key.find('.', len);
				if (pos == string::npos) {
					// We get a leaf node, break to next item
					string seg = key.substr(len);
					em	<< YAML::Key << seg
						<< YAML::Value <<value;
					break;
				}
				else {
					// Add one level
					string seg = key.substr(len, pos - len);
					em	<< YAML::Key << seg
						<< YAML::Value
						<< YAML::BeginMap;
					prefix = key.substr(0, pos + 1);
				}
			}
			else {
				// Reduce one level
				em << YAML::EndMap;
				size_t pos = prefix.rfind('.', prefix.size() - 2);
				if (pos == string::npos) {
					break;
				}
				else {
					prefix = prefix.substr(0, pos + 1);
				}
			}
		} // while
	} // for
	
    // End map
	for (size_t pos = 0; pos != string::npos; pos = prefix.find('.', pos + 1)) {
		em	<< YAML::EndMap;
	}
}

// prefix does not have tailing '.'
void Config::loadYamlNode(string prefix, const YAML::Node &node)
{
	YAML::Emitter em;

	switch(node.Type()) {
	case YAML::NodeType::Null:
		itemMap_[prefix] = "";
		break;
	case YAML::NodeType::Scalar:
		itemMap_[prefix] = node.to<string>();
		break;
	case YAML::NodeType::Sequence:
		em << YAML::Flow;
		em << node;
		itemMap_[prefix] = em.c_str();
		break;
	case YAML::NodeType::Map:
		if (prefix.size() != 0) {
			prefix += ".";
		}
		for (YAML::Iterator it=node.begin(); it!=node.end(); ++it) {
			string key = it.first().to<string>();
			loadYamlNode(prefix + key, node[key]);
		}
		break;
	default:
		throw ("Unknown YAML node type: " + node.Type());
		break;
	}
}

void Config::loadStream(istream &in)
{
  YAML::Parser parser;
  try {
    YAML::Node doc;
    parser.Load(in);
    while (parser.GetNextDocument(doc)) {
      loadYamlNode("", doc);
    }
  }
  catch(YAML::ParserException &e) {
	LOG("%s", e.what());
    throw;
  }
  return;
}

void Config::loadString(const string &str)
{
  istringstream in(str);
  loadStream(in);
}

void Config::loadFile(const string &fn)
{
	ifstream fin(fn.c_str(), ifstream::in);
	if (fin.good()) {
		loadStream(fin);
	}
	else {
		throw string("Failed to read the content of file: ") + fn;
	}
}

void Config::loadEnv(const string &prefix)
{
	extern char **environ;  // system environment vars
	for (int i=0; ; i++) {
		const char *env = environ[i];
		if (env == NULL) {
			break;
		}
		// Check prefix
		else if (string(env).compare(0, prefix.size(), prefix) != 0) {
			continue;
		}
		// Chop prefix
		string name(env + prefix.size());
		size_t pos = name.find_first_of('=');
		string value = name.substr(pos + 1);
		name = name.substr(0, pos);
		// Convert '_' to '.'
		for (size_t i = 0; i<name.size(); i++) {
			if (name[i] == '_') {
				name[i] = '.';
			}
		}
		// Insert into map
		itemMap_[name] = value;
	}
}

void Config::loadUrl(const string &url)
{
	// Recognize file:// and env://
	string filePattern("file://");
	string envPattern("env://");
	if (url.substr(0, filePattern.size()) == filePattern) {
		string path = url.substr(filePattern.size());
		loadFile(path);
	}
	else if (url.substr(0, envPattern.size()) == envPattern) {
		string prefix = url.substr(envPattern.size());
		loadEnv(prefix);
	}
	else {
		// try as local file
		loadFile(url);
	}
}

void Config::loadv(const vector<string> &urls)
{
	bool loaded = false;

	for (int i=urls.size()-1; i>=0; i--) {
		try {
			loadUrl(urls[i]);
			loaded = true;
		} catch(...) {
			// Do NOT need all urls exist
		}
	}
	if (!loaded) {
		// At least one succeeded
		throw string("Need at least one URL");
	}
}

void Config::load(const char *url, ...)
{ 
  vector<string> urls;
  const char *curr = NULL;
  
  va_list args;
  va_start(args, url);
  curr = url;
  do {
    urls.push_back(curr);
    curr = va_arg(args, const char *);
  } while (curr != NULL);
  
  va_end(args);
  
  loadv(urls);
}

string Config::getItem(const string &key) const
{
	try {
		return itemMap_.at(key);
	}
	catch(out_of_range) {
		throw ConfigException(ConfigException::KEY_NOT_FOUND, "No such key: %s", key.c_str());
	}
}

void Config::setItem(const string &key, const string &value)
{
	itemMap_[key] = value;
}

template<>
string Config::fromValue<string>(const string &value)
{
	return value;
}

template<>
bool Config::fromValue<bool>(const string &value)
{
	if (strcasecmp(value.c_str(), "true") == 0) {
		return true;
	}
	else if (strcasecmp(value.c_str(), "false") == 0) {
		return false;
	}
	else {
		throw ConfigException(ConfigException::INVALID_BOOL_VALUE, "Invalid bool value: %s", value.c_str());
	}
}

template<>
long Config::fromValue<long>(const string &value)
{
	// Recognize tailing 'k', 'K', 'm', 'M', 'g', 'G', 't', 'T'
	long unit = 1;
	string nvalue;
	switch(value[value.size()-1]) {
		case 'k':
		case 'K':
			unit = 1024L;
			nvalue = value.substr(0, value.size() - 1);
			break;
		case 'm':
		case 'M':
			unit = 1024L * 1024;
			nvalue = value.substr(0, value.size() - 1);
			break;
		case 'g':
		case 'G':
			unit = 1024L * 1024 * 1024;
			nvalue = value.substr(0, value.size() - 1);
			break;
		case 't':
		case 'T':
			unit = 1024L * 1024 * 1024 * 1024;
			nvalue = value.substr(0, value.size() - 1);
			break;
		default:
			nvalue = value;
			break;
	}
	// convert value to long
	char *endptr = NULL;
	long num = strtol(nvalue.c_str(), &endptr, 10);
	if (*endptr != '\0') {
		throw ConfigException(ConfigException::INVALID_LONG_VALUE, "Invalid long value: %s", value.c_str());
	}

	return num *= unit;
}

vector<string> Config::list(const string &dir) const
{
	string prefix = dir + ".";
	map<string, int> listmap;
	vector<string> listvect;

	// Compare the key of each item with "dir."
	for (map<string, string>::const_iterator iter = itemMap_.begin(); iter != itemMap_.end(); ++iter) {
		string key = iter->first;
		if (key.compare(0, prefix.size(), prefix) == 0) {
			// Get next segemnt in key
			string seg;
			for (size_t pos = prefix.size(); pos < key.size() && key[pos] != '.'; pos++) {
				seg += key[pos];
			}
			// Put into a map to de-duplicate
			listmap[seg]++;
		}
	}
	// Copy key of map to a vector for returning
	for (map<string, int>::const_iterator iter = listmap.begin(); iter != listmap.end(); ++iter) {
		listvect.push_back(iter->first);
	}

	return listvect;
}

Config Config::getSubConfig(const string &dir)
{
	Config cfg;

	// Find the position of the last segment of dir
	// We need this last segment as part of new key of cfg
	size_t pos = dir.rfind('.');
	if (pos == string::npos) {
		pos = 0;
	}
	else {
		pos++;
	}
	// If current item is an item under dir, put it into new config
	string prefix = dir + '.';
	for (map<string, string>::const_iterator iter = itemMap_.begin(); iter != itemMap_.end(); ++iter) {
		const string &key = iter->first;
		if (key.compare(0, prefix.size(), prefix) == 0) {
			cfg.setItem(key.substr(pos), iter->second);
		}
	}

	return cfg;
}
