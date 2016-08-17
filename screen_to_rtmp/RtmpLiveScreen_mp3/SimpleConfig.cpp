#include "stdafx.h"
#include "SimpleConfig.h"

#include <algorithm>

static void StrTrim(std::string& str)
{
    std::string::iterator it = std::find_if(str.begin(),str.end(), 
        std::not1(std::ptr_fun(isspace)));
    str.erase(str.begin(), it);
    std::string::reverse_iterator rit = std::find_if(str.rbegin(),str.rend(),
        std::not1(std::ptr_fun(isspace))); 
    str.erase(rit.base(), str.end());
}

SimpleConfig::SimpleConfig()
{

}

SimpleConfig::SimpleConfig(const std::string& cfg_file)
    : _cfg_filepath(cfg_file)
{
    Load();
}

bool SimpleConfig::Load()
{
    if (_cfg_filepath.empty()) return false;
    return Load(_cfg_filepath);
}

bool SimpleConfig::Load(const std::string& cfg_file)
{
    FILE *fp = fopen(cfg_file.c_str(), "r");
    if (fp)
    {
        char linebuf[2048];
        char *p = 0;
        while ((p = fgets(linebuf, 2048, fp)))
        {
            std::string line(linebuf);
            size_t comment_index = line.find_first_of('#');
            if (comment_index != std::string::npos)
                line = line.substr(0, comment_index);

            std::string::size_type pos = line.find('=');
            if (pos != std::string::npos)
            {
                std::string key = line.substr(0, pos);
                StrTrim(key);
                std::string val = line.substr(pos+1);
                StrTrim(val);
                if (!key.empty() && !val.empty())
                {
                    _cfg_map[key] = val;
                }
            }
        }

        fclose(fp);
        return true;
    }
    return false;
}

bool SimpleConfig::Save()
{
    if (_cfg_filepath.empty()) return false;
    return Save(_cfg_filepath);
}

bool SimpleConfig::Save(const std::string& cfg_file)
{
    FILE *fp = fopen(cfg_file.c_str(), "w");
    if (fp)
    {
        for (CfgMapType::iterator it = _cfg_map.begin();
            it != _cfg_map.end(); ++it)
        {
            fprintf(fp, "%s = %s\n", it->first.c_str(), it->second.c_str());
        }

        fclose(fp);
        return true;
    }
    return false;
}

std::string SimpleConfig::GetProperty(const std::string& key)
{
    CfgMapType::iterator it = _cfg_map.find(key);
    if (it != _cfg_map.end())
    {
        return it->second;
    }
    return "";
}

void SimpleConfig::SetProperty(const std::string& key, const std::string& val)
{
    _cfg_map[key] = val;
}

int SimpleConfig::GetInt(const std::string& key, int default_val)
{
    std::string val_str = GetProperty(key);

    if (val_str.empty())
    {
        return default_val;
    }
    else
    {
        return atoi(val_str.c_str());
    }
}

void SimpleConfig::SetInt(const std::string& key, int val)
{
    char tmpbuf[1024];
    sprintf(tmpbuf, "%d", val);
    SetProperty(key, tmpbuf);
}

double SimpleConfig::GetDouble(const std::string& key, double default_val)
{
    std::string val_str = GetProperty(key);

    if (val_str.empty())
    {
        return default_val;
    }
    else
    {
        return atof(val_str.c_str());
    }
}

void SimpleConfig::SetDouble(const std::string& key, double val)
{
    char buf[1024];
    sprintf(buf, "%.3f", val);
    SetProperty(key, std::string(buf));
}

