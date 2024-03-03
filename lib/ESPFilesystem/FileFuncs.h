#ifndef ESP_FILESYSTEM_FILEFUNCS_H
#define ESP_FILESYSTEM_FILEFUNCS_H

#include <preferences.h>
#include <nvs_flash.h>


namespace FileFuncs
{

static Preferences preferences;
void writeToFile(const char* fname, const char* vname, const float data);
void readFromFile(const char* fname, const char* vname, float& data);

void writeToFile(const char* fname, const char* vname, const double data);
void readFromFile(const char* fname, const char* vname, double& data);

void writeToFile(const char* fname, const char* vname, const int data);
void readFromFile(const char* fname, const char* vname, int& data);

void writeToFile(const char* fname, const char* vname, const String data);
void readFromFile(const char* fname, const char* vname, String& data);

void writeToFile(const char* fname, const char* vname, const float* data, int size);
void readFromFile(const char* fname, const char* vname, float* data, int size);

void erase_flash();
void getStatus();
}




#endif