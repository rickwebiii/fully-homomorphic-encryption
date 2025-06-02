// Copyright 2021 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "hamming_distance_circuit.h"

inline bool get_bit(uint8_t flags, unsigned int n)
{
    return ((flags >> n) & 0x1);
}

HammingDistanceAns HammingDistance::compute(
    uint8_t a[SIZE],
    uint8_t b[SIZE]
)
{
    uint8_t distance = 0;

#pragma hls_unroll yes
    for (int i = 0; i < SIZE; i++)
    {
#pragma hls_unroll yes
        for (int j = 0; j < 8; j++) {
            if (get_bit(a[i], j) != get_bit(b[i], j))
            {
                distance++;
            }
        }
    }

    return HammingDistanceAns(distance);
}

// TODO: Way to mark Calculator::process() as main function
#pragma hls_top
HammingDistanceAns my_package(
    HammingDistance &HammingDistance,
    uint8_t x[SIZE],
    uint8_t y[SIZE])
{
    return HammingDistance.compute(x, y);
}
