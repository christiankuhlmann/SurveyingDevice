#include "FileFuncs.h"
#include "debug_csd.h"

namespace FileFuncs
{

static Preferences preferences;

Preferences& getPreferences()
{
    return preferences;
}

bool locationExists(const char* fname, const char* vname)
{
  if (preferences.begin(fname, true))
  {
    if (preferences.isKey(vname))
    {
        preferences.end();
        return true;
    }
    Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_FILE, "Key does not exist in file");
    preferences.end();
    return false;
  }
  Debug_csd::log(Debug_csd::LOG_ERROR, Debug_csd::DEBUG_FILE, "File does not exist");
  preferences.end();
  return false;
}

bool writeToFile(const char* fname, const char* vname, const float data){
  if (!preferences.begin(fname, false)) return false;
  size_t written = preferences.putFloat(vname,data);
  preferences.end();
  return written > 0;
}
bool readFromFile(const char* fname, const char* vname, float& data){
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  data = preferences.getFloat(vname);
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const double data){
  if (!preferences.begin(fname, false)) return false;
  size_t written = preferences.putDouble(vname,data);
  preferences.end();
  return written > 0;
}
bool readFromFile(const char* fname, const char* vname, double& data){
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  data = preferences.getDouble(vname);
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const int data){
  if (!preferences.begin(fname, false)) return false;
  size_t written = preferences.putInt(vname,data);
  preferences.end();
  return written > 0;
}
bool readFromFile(const char* fname, const char* vname, int& data){
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  data = preferences.getInt(vname);
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const unsigned int data){
  if (!preferences.begin(fname, false)) return false;
  size_t written = preferences.putUInt(vname,data);
  preferences.end();
  return written > 0;
}
bool readFromFile(const char* fname, const char* vname, unsigned int& data){
  if (!locationExists(fname,vname)) {
    return false;
  }
  preferences.begin(fname, true);
  data = preferences.getUInt(vname);
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const String data){
  if (!preferences.begin(fname, false)) return false;
  size_t written = preferences.putString(vname,data);
  preferences.end();
  return written > 0;
}
bool readFromFile(const char* fname, const char* vname, String& data){
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  data = preferences.getString(vname);
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const float* data, int size)
{
    if (!preferences.begin(fname, false)) return false;
    size_t written = preferences.putBytes(vname,data,size*sizeof(float));
    preferences.end();
    return written > 0;
}
bool readFromFile(const char* fname, const char* vname, float* data, int size)
{
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  preferences.getBytes(vname,data,size*sizeof(float));
  preferences.end();
  return true;
}

bool writeToFile(const char* fname, const char* vname, const void* data, size_t size)
{
    if (!preferences.begin(fname, false)) return false;
    size_t written = preferences.putBytes(vname,data,size);
    preferences.end();
    return written > 0;
}
bool readFromFile(const char* fname, const char* vname, void* data, size_t size)
{
  if (!locationExists(fname,vname)) { return false; }
  preferences.begin(fname, true);
  preferences.getBytes(vname,data,size);
  preferences.end();
  return true;
}

bool isKey(const char* fname, const char* key)
{
    preferences.begin(fname, true);
    bool result = preferences.isKey(key);
    preferences.end();
    return result;
}

void erase_flash()
{
  Debug_csd::log(Debug_csd::LOG_WARN, Debug_csd::DEBUG_FILE, "ERASING FLASH...");
  nvs_flash_erase(); // erase the NVS partition and...
  nvs_flash_init(); // initialize the NVS partition.
}

void getStatus()
{
  nvs_stats_t nvs_stats;
  nvs_get_stats(NULL, &nvs_stats);
  Debug_csd::logf(Debug_csd::LOG_INFO, Debug_csd::DEBUG_FILE,
        "Count: UsedEntries = (%lu), FreeEntries = (%lu), NamespaceCount = (%lu), AllEntries = (%lu)",
        nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.namespace_count, nvs_stats.total_entries);
}

}
