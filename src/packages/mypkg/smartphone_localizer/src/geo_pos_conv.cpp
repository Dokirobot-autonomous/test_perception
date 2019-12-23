/*
 *  Copyright (c) 2015, Nagoya University
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Autoware nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 *  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 *  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <geo_pos_conv.hpp>

geo_pos_conv::geo_pos_conv()
{

}

geo_pos_conv::geo_pos_conv(double lat, double lon)
{
    m_PLato = lat*M_PI/180.0;
    m_PLo = lon*M_PI/180.0;
}

double geo_pos_conv::x() const
{
  return m_x;
}

double geo_pos_conv::y() const
{

  return m_y;
}

double geo_pos_conv::z() const
{
  return m_z;
}

void geo_pos_conv::set_plane(double lat, double lon)
{
  m_PLato = lat*M_PI/180.0;
  m_PLo = lon*M_PI/180.0;
}

void geo_pos_conv::set_plane(int num)
{
  double lon_deg, lon_min, lat_deg, lat_min; // longitude and latitude of origin of each plane in Japan
  if (num == 1)
  {
    lon_deg = 33;
    lon_min = 0;
    lat_deg = 129;
    lat_min = 30;
  }
  else if (num == 2)
  {
    lon_deg = 33;
    lon_min = 0;
    lat_deg = 131;
    lat_min = 0;
  }
  else if (num == 3)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 132;
    lat_min = 10;
  }
  else if (num == 4)
  {
    lon_deg = 33;
    lon_min = 0;
    lat_deg = 133;
    lat_min = 30;
  }
  else if (num == 5)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 134;
    lat_min = 20;
  }
  else if (num == 6)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 136;
    lat_min = 0;
  }
  else if (num == 7)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 137;
    lat_min = 10;
  }
  else if (num == 8)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 138;
    lat_min = 30;
  }
  else if (num == 9)
  {
    lon_deg = 36;
    lon_min = 0;
    lat_deg = 139;
    lat_min = 50;
  }
  else if (num == 10)
  {
    lon_deg = 40;
    lon_min = 0;
    lat_deg = 140;
    lat_min = 50;
  }
  else if (num == 11)
  {
    lon_deg = 44;
    lon_min = 0;
    lat_deg = 140;
    lat_min = 15;
  }
  else if (num == 12)
  {
    lon_deg = 44;
    lon_min = 0;
    lat_deg = 142;
    lat_min = 15;
  }
  else if (num == 13)
  {
    lon_deg = 44;
    lon_min = 0;
    lat_deg = 144;
    lat_min = 15;
  }
  else if (num == 14)
  {
    lon_deg = 26;
    lon_min = 0;
    lat_deg = 142;
    lat_min = 0;
  }
  else if (num == 15)
  {
    lon_deg = 26;
    lon_min = 0;
    lat_deg = 127;
    lat_min = 30;
  }
  else if (num == 16)
  {
    lon_deg = 26;
    lon_min = 0;
    lat_deg = 124;
    lat_min = 0;
  }
  else if (num == 17)
  {
    lon_deg = 26;
    lon_min = 0;
    lat_deg = 131;
    lat_min = 0;
  }
  else if (num == 18)
  {
    lon_deg = 20;
    lon_min = 0;
    lat_deg = 136;
    lat_min = 0;
  }
  else if (num == 19)
  {
    lon_deg = 26;
    lon_min = 0;
    lat_deg = 154;
    lat_min = 0;
  }
  else if (num == 20)
  {
//    lon_deg = 35;
//    lon_min = 11.06556;
//    lat_deg = 137;
//    lat_min = 6.80682;
    lat_deg = 35;
    lat_min = 11.06556;
    lon_deg = 137;
    lon_min = 6.80682;
  }

  // convert longitude and latitude to rad
  m_PLato = M_PI * ((double)lat_deg + (double)lat_min / 60.0) / 180.0;
  m_PLo = M_PI * ((double)lon_deg + (double)lon_min / 60.0) / 180.0;
  m_Ph=0;
}

void geo_pos_conv::set_xyz(double cx, double cy, double cz)
{
  m_x = cx;
  m_y = cy;
  m_z = cz;
  conv_xyz2llh();
}

void geo_pos_conv::set_llh_nmea_degrees(double latd, double lond, double h)
{
  double lat, lad, lod, lon;
  // 1234.56 -> 12'34.56 -> 12+ 34.56/60

  lad = floor(latd / 100.);
  lat = latd - lad * 100.;
  lod = floor(lond / 100.);
  lon = lond - lod * 100.;

  // Changing Longitude and Latitude to Radians
  m_lat = (lad + lat / 60.0) * M_PI / 180.0;
  m_lon = (lod + lon / 60.0) * M_PI / 180.0;
  m_h = h;

  conv_llh2xyz();
}

void geo_pos_conv::llh_to_xyz(double lat, double lon, double ele)
{
  m_lat = lat * M_PI / 180.0;
  m_lon = lon * M_PI / 180.0;
  m_h = ele;

  conv_llh2xyz();
}

void geo_pos_conv::conv_llh2xyz(void)
{
  double PS;   //
  double PSo;  //
  double PDL;  //
  double Pt;   //
  double PN;   //
  double PW;   //

  double PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9;
  double PA, PB, PC, PD, PE, PF, PG, PH, PI;
  double Pe;   //
  double Pet;  //
  double Pnn;  //
  double AW, FW, Pmo;

  Pmo = 0.9999;

//  std::cout<<"Use function convECEF"<<std::endl;
  /*WGS84 Parameters*/
  AW = 6378137.0;            // Semimajor Axis
  FW = 1.0 / 298.257222101;  // 298.257223563 //Geometrical flattening
  double E2 = FW * (2 - FW);

  double x0,y0,z0,xe,ye,ze;

  // Compute x0,y0,z0
  double N0=AW/sqrt(1.0-E2*pow(sin(m_PLato),2));
  x0=(N0+m_Ph)*cos(m_PLato)*cos(m_PLo);
  y0=(N0+m_Ph)*cos(m_PLato)*sin(m_PLo);
  z0=(N0*(1.0-E2)+m_Ph)*sin(m_PLato);

  // Compute xe,ye,ze
  double N=AW/sqrt(1.0-E2*pow(sin(m_lat),2));
  xe=(N+m_h)*cos(m_lat)*cos(m_lon);
  ye=(N+m_h)*cos(m_lat)*sin(m_lon);
  ze=(N*(1.0-E2)+m_h)*sin(m_lat);

  // Convert ENU
  double SinLat=sin(m_PLato);
  double CosLat=cos(m_PLato);
  double SinLon=sin(m_PLo);
  double CosLon=cos(m_PLo);

  this->m_x = -SinLon*(xe - x0) + CosLon*(ye - y0); // East = x
  this->m_y = -SinLat*CosLon*(xe - x0) - SinLat*SinLon*(ye - y0) + CosLat*(ze - z0); // North = y
  this->m_z = CosLat*CosLon*(xe - x0) + CosLat*SinLon*(ye - y0) + SinLat*(ze - z0); // Up

//  std::cout<<this->m_x<<","<<this->m_y<<","<<this->m_z<<std::endl;

//  convECEF();
 
}


void geo_pos_conv::conv_xyz2llh(void)
{
  // n/a
}
