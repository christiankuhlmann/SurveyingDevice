// #include "RM3100SensorConnection.h"

// RM3100SensorConnection::RM3100SensorConnection(RM3100 &rm3100):rm3100_connection(rm3100){}

// void RM3100SensorConnection::init()
// {
//     rm3100_connection.begin();
// }

// Vector3f RM3100SensorConnection::getMeasurement()
// {
//     rm3100_connection.update();
//     Vector3f data;
//     // Div by 45 to come closer to normalised
//     // Convert from NED to ENU
//     data << rm3100_connection.getX()/50, -rm3100_connection.getY()/50, -rm3100_connection.getZ()/50;
//     // Serial << "RM3100 data: ";
//     // displayRowVec(data);
//     return data;
// }
