/*
The sensor outputs provided by the library are the raw 16-bit values
obtained by concatenating the 8-bit high and low accelerometer and
magnetometer data registers. They can be converted to units of g and
gauss using the conversion factors specified in the datasheet for your
particular device and full scale setting (gain).

Example: An LSM303D gives a magnetometer X axis reading of 1982 with
its default full scale setting of +/- 4 gauss. The M_GN specification
in the LSM303D datasheet (page 10) states a conversion factor of 0.160
mgauss/LSB (least significant bit) at this FS setting, so the raw
reading of -1982 corresponds to 1982 * 0.160 = 317.1 mgauss =
0.3171 gauss.

In the LSM303DLHC, LSM303DLM, and LSM303DLH, the acceleration data
registers actually contain a left-aligned 12-bit number, so the lowest
4 bits are always 0, and the values should be shifted right by 4 bits
(divided by 16) to be consistent with the conversion factors specified
in the datasheets.

Example: An LSM303DLH gives an accelerometer Z axis reading of -16144
with its default full scale setting of +/- 2 g. Dropping the lowest 4
bits gives a 12-bit raw value of -1009. The LA_So specification in the
LSM303DLH datasheet (page 11) states a conversion factor of 1 mg/digit
at this FS setting, so the value of -1009 corresponds to -1009 * 1 =
1009 mg = 1.009 g.
*/

#include <Wire.h>
#include <LSM303.h>
#include <L3G.h>

LSM303 compass;
L3G gyro;
float acc_x = 0.0;
float acc_y = 0.0;
float acc_z = 0.0;
int count = 0;
float x_offset = 0.67;
float y_offset = 2.04;
float z_offset = -0.11;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  if (!gyro.init())
  {
    Serial.println("Failed to autodetect gyro type!");
    while (1);
  }
  gyro.enableDefault();
  compass.init();
  compass.enableDefault();
  compass.m_min = (LSM303::vector<int16_t>){-32767, -32767, -32767};
  compass.m_max = (LSM303::vector<int16_t>){+32767, +32767, +32767};

}

void loop()
{
  compass.read();
  gyro.read();
  double x = (compass.a.x / 1600.0);
  double y = (compass.a.y / 1600.0);
  double z = (compass.a.z / 1600.0);
  float g_x = gyro.g.x * 8.75/1000 + x_offset;
  float g_y = gyro.g.y * 8.75/1000 + y_offset;
  float g_z = gyro.g.z * 8.75/1000 + z_offset;
//  count = count + 1;
//  acc_x = acc_x + g_x + x_offset;
//  acc_y = acc_y + g_y + y_offset;
//  acc_z = acc_z + g_z + z_offset;
  // float heading = compass.heading((LSM303::vector<int>){0, -1, 0});
  float heading = compass.heading();
  
  Serial.print(x);
  Serial.print(",");
  Serial.print(y);
  Serial.print(",");
  Serial.print(z);
  Serial.print(",");
  Serial.print(g_x);
  Serial.print(",");
  Serial.print(g_y);
  Serial.print(",");
  Serial.print(g_z);
  Serial.print(",");
  Serial.print(heading);
  Serial.println();

  delay(20);
}
