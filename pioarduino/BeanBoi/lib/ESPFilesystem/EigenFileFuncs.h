#ifndef ESP_FILESYSTEM_EIGEN_FILEFUNCS_H
#define ESP_FILESYSTEM_EIGEN_FILEFUNCS_H

#include <preferences.h>
#include <nvs_flash.h>
#include <ArduinoEigen.h>
#include "FileFuncs.h"
using namespace Eigen;

namespace EigenFileFuncs
{
using namespace FileFuncs;

inline void writeToFile(const char* fname, const char* name, const Ref<const MatrixXf> &mat)
{
  Preferences& preferences = getPreferences();
  preferences.begin(fname, false);
  preferences.putBytes(name,mat.data(),mat.size()*sizeof(float));
  preferences.end();
}

inline bool readFromFile(const char* fname, const char* name, Ref<MatrixXf> mat)
{
  Preferences& preferences = getPreferences();
  preferences.begin(fname, true);
  size_t expected_size = mat.size() * sizeof(float);
  size_t actual_size = preferences.getBytesLength(name);
  
  if (actual_size == 0 || actual_size != expected_size) {
    preferences.end();
    return false; // Data doesn't exist or size mismatch
  }
  
  preferences.getBytes(name, mat.data(), expected_size);
  preferences.end();
  return true; // Success
}


}

#endif