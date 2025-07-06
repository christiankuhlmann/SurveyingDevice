#ifndef HEADER_RM3100SC
#define HEADER_RM3100SC

#include <CaveSurveyDevice.h>
#include <RM3100.h>

class RM3100SensorConnection: public Magnetometer
{
public:
    RM3100SensorConnection(RM3100 &rm3100);
    Vector3f getMeasurement();
    void init();
private:
    RM3100 &rm3100_connection; /** Pointer to the rm3100_object associated with this connection*/
};



inline RM3100SensorConnection::RM3100SensorConnection(RM3100 &rm3100):rm3100_connection(rm3100){}

inline void RM3100SensorConnection::init()
{
    rm3100_connection.begin(false);  // Disable DRDY pin usage, use status register instead
}

inline Vector3f RM3100SensorConnection::getMeasurement()
{
    rm3100_connection.update();
    Vector3f data;
    // Div by 50 to come closer to normalised
    // RM3100 physical mounting requires [X, -Y, -Z] transformation
    // Due to sensor orientation on PCB, this aligns better with device ENU than standard NED->ENU conversion
    data << rm3100_connection.getX()/50.0, -rm3100_connection.getY()/50.0, -rm3100_connection.getZ()/50.0;
    // Serial << "RM3100 data: ";
    // displayRowVec(data);
    return data;
}

#endif