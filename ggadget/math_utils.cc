/*
  Copyright 2007 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "math_utils.h"

namespace ggadget {

void ChildCoordFromParentCoord(double parent_x, double parent_y,
                               double child_x_pos, double child_y_pos,
                               double child_pin_x, double child_pin_y,
                               double rotation_radians,
                               double *child_x, double *child_y) {
  double sin_theta = sin(rotation_radians);
  double cos_theta = cos(rotation_radians);
  double a_13 = child_pin_x - child_y_pos * sin_theta - child_x_pos * cos_theta;
  double a_23 = child_pin_y + child_x_pos * sin_theta - child_y_pos * cos_theta;
  
  *child_x = parent_x * cos_theta + parent_y * sin_theta + a_13;
  *child_y = parent_y * cos_theta - parent_x * sin_theta + a_23;
}

ChildCoordCalculator::ChildCoordCalculator(double child_x_pos, double child_y_pos,
                                           double child_pin_x, double child_pin_y,
                                           double rotation_radians) {  
  sin_theta = sin(rotation_radians);
  cos_theta = cos(rotation_radians);
  a_13 = child_pin_x - child_y_pos * sin_theta - child_x_pos * cos_theta;
  a_23 = child_pin_y + child_x_pos * sin_theta - child_y_pos * cos_theta;  
}

void ChildCoordCalculator::Convert(double parent_x, double parent_y, 
                                   double *child_x, double *child_y) {
  *child_x = parent_x * cos_theta + parent_y * sin_theta + a_13;
  *child_y = parent_y * cos_theta - parent_x * sin_theta + a_23;    
}  

double ChildCoordCalculator::GetChildX(double parent_x, double parent_y) {
  return parent_x * cos_theta + parent_y * sin_theta + a_13;
}

double ChildCoordCalculator::GetChildY(double parent_x, double parent_y) {
  return parent_y * cos_theta - parent_x * sin_theta + a_23;  
}

double DegreesToRadians(double degrees) {
  return degrees * M_PI / 180.;  
}

bool IsPointInElement(double x, double y, double width, double height) {
  return 0. <= x && 0. <= y && x < width && y < height;  
}

int RoundToInt(double x) {
  int value = static_cast<int>(x); 
  if ((x - value) >= .5) {
    value++;    
  }
  return value;
}

} // namespace ggadget