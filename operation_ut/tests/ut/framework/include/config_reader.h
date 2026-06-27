#ifndef _CONFIG_READER_H_
#define _CONFIG_READER_H_

#include "nlohmann/json.hpp"
#include <string>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <iostream>

void InitConfigJson(const std::string& path);
const std::string& GetConfigValueByKey(const std::string& key);
const std::string& GetConfigValueDirByKey(const std::string& key);

#endif
