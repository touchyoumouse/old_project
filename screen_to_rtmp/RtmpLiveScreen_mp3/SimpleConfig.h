/**
 * 简单的配置属性读取
 */

#ifndef _FRAMEWORK_UTIL_SIMPLE_CONFIG_H_
#define _FRAMEWORK_UTIL_SIMPLE_CONFIG_H_

#include <map>
#include <string>

class SimpleConfig
{
public:
    SimpleConfig();

    explicit SimpleConfig(const std::string& cfg_file);

    void SetCfgPath(const std::string& cfg_path)
    {
        if (false == cfg_path.empty())
            _cfg_filepath = cfg_path;
    }

    bool Load();

    bool Load(const std::string& cfg_file);

    bool Save();

    bool Save(const std::string& cfg_file);

    std::string GetProperty(const std::string& key);

    void SetProperty(const std::string& key, const std::string& val);

    int GetInt(const std::string& key, int default_val);

    void SetInt(const std::string& key, int val);

    double GetDouble(const std::string& key, double default_val);

    void SetDouble(const std::string& key, double val);

private:
    std::string _cfg_filepath;
    typedef std::map<std::string, std::string> CfgMapType;
    CfgMapType _cfg_map;
};

#endif // _FRAMEWORK_UTIL_SIMPLE_CONFIG_H_
