#ifndef HEADER_LDK2MSensorConnection
#define HEADER_LDK2MSensorConnection

#include <CaveSurveyDevice.h>
#include <LDK_2M.h>

class LDK2MSensorConnection : public Laser
{
public:
    LDK2MSensorConnection(LDK_2M &ptr_LDK2M);
    void init();
    float getMeasurement();
    void toggleLaser();
    void toggleLaser(bool mode);
    void beep();
private:
    LDK_2M &LDK2M_connection;
};

inline LDK2MSensorConnection::LDK2MSensorConnection(LDK_2M &ldk2m): LDK2M_connection(ldk2m){}

inline void LDK2MSensorConnection::init()
{
    LDK2M_connection.init();
}

inline void LDK2MSensorConnection::beep()
{
    LDK2M_connection.beep();
}

inline float LDK2MSensorConnection::getMeasurement()
{
    return LDK2M_connection.getMeasurement();
}

inline void LDK2MSensorConnection::toggleLaser()
{
    LDK2M_connection.toggleLaser();
}
inline void LDK2MSensorConnection::toggleLaser(bool mode)
{
    LDK2M_connection.toggleLaser(mode);
}








#endif