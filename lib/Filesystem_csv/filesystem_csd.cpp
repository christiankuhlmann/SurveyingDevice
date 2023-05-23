#include "filesystem_csd.h"
#ifdef ESP32
void write_to_file(const char* fname, const char* name, const float data){
  preferences.begin(fname, false);
  preferences.putFloat(name,data);
  preferences.end();
}

void write_to_file(const char* fname, const char* name, const double data){
  preferences.begin(fname, false);
  preferences.putDouble(name,data);
  preferences.end();
}

void write_to_file(const char* fname, const char* name, const int data){
  preferences.begin(fname, false);
  preferences.putInt(name,data);
  preferences.end();
}

void write_to_file(const char* fname, const char* name, const String data){
  preferences.begin(fname, false);
  preferences.putString(name,data);
  preferences.end();
}



//template<typename Derived>
// void write_to_file(const char* fname, const char* name, const Eigen::MatrixBase<Derived>& Data){
//     preferences.begin(fname, false);
//     preferences.putBytes(name,Data.data(),Data.size()*sizeof(Derived));
//     preferences.end();
// }
// template<typename Derived>
// void write_to_file(const char* fname, const char* name, const Eigen::MapBase<Derived>& Data){
//     preferences.begin(fname, false);
//     preferences.putBytes(name,Data.data(),Data.size()*sizeof(Derived));
//     preferences.end();
// }
// template<typename Derived>
// void read_from_file(const char* fname, const char* name, Eigen::MatrixBase<Derived>* Data){
//     preferences.begin(fname, true);
//     preferences.getBytes(name,Data.data(),Data.data()*sizeof(Derived));
//     preferences.end();
// }

// template<typename Derived>
// void read_from_file(const char* fname, const char* name, Eigen::MapBase<Derived>* Data){
//     preferences.begin(fname, true);
//     preferences.getBytes(name,Data.data(),Data.data()*sizeof(Derived));
//     preferences.end();
// }

void write_to_file(const char* fname, const char* name, const float* data, int size)
{
    preferences.begin(fname, false);
    preferences.putBytes(name,data,size*sizeof(float));
    preferences.end();
}

void read_from_file(const char* fname, const char* name, float* data, int size)
{
    preferences.begin(fname, true);
    preferences.getBytes(name,data,size*sizeof(float));
    preferences.end();
}

#else
#ifdef SEEED


#endif
#endif