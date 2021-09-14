#pragma once

namespace common {

const int k_unknown = -1;
const unsigned long k_unknownUL = -1;

// float version of Arduino map()
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

}