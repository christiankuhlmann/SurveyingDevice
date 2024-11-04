#include "utils.h"


void displayMat(const MatrixXf &m)
{
    int rows = m.rows();
    int cols = m.cols();
    for(int i=0; i<rows;i++)
    {
        Serial << "[\t";
        for(int j=0; j<cols;j++)
        {
            Serial.print(m(i,j),8);
            Serial.print("\t");
        }
        Serial << "\t]\n";
    }
    Serial << "    \n";
}

void displayVec(const VectorXf &v)
{
    int n = v.size();
    for(int i=0; i<n;i++)
    {
        Serial.print("[\t");
        Serial.print(v(i),8);
        Serial.print("\t");
        Serial.print("\t]\n");
    }
    Serial << "\n";
}