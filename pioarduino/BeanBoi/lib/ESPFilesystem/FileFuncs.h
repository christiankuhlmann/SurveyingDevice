#ifndef ESP_FILESYSTEM_FILEFUNCS_H
#define ESP_FILESYSTEM_FILEFUNCS_H

#include <preferences.h>
#include <nvs_flash.h>

#define FNAME_LENGTH 6
#define VARNAME_LENGTH 4

namespace FileFuncs
{

Preferences& getPreferences();

bool locationExists(const char* fname, const char* vname);

bool writeToFile(const char* fname, const char* vname, const float data);
bool readFromFile(const char* fname, const char* vname, float& data);

bool writeToFile(const char* fname, const char* vname, const double data);
bool readFromFile(const char* fname, const char* vname, double& data);

bool writeToFile(const char* fname, const char* vname, const int data);
bool readFromFile(const char* fname, const char* vname, int& data);

bool writeToFile(const char* fname, const char* vname, const unsigned int data);
bool readFromFile(const char* fname, const char* vname, unsigned int& data);

bool writeToFile(const char* fname, const char* vname, const String data);
bool readFromFile(const char* fname, const char* vname, String& data);

bool writeToFile(const char* fname, const char* vname, const float* data, int size);
bool readFromFile(const char* fname, const char* vname, float* data, int size);

bool writeToFile(const char* fname, const char* vname, const void* data, size_t size);
bool readFromFile(const char* fname, const char* vname, void* data, size_t size);

bool isKey(const char* fname, const char* key);

void erase_flash();
void getStatus();
}




#endif