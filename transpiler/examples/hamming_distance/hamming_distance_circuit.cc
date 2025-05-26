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

#define SIZE 64

inline bool get_bit(uint64_t flags, unsigned int n)
{
    return ((flags >> n) & 0x1);
}

HammingDistanceAns HammingDistance::compute(
    uint64_t a,
    uint64_t b)
{
    uint8_t distance = 0;

#pragma hls_unroll yes
    for (int i = 0; i < SIZE; i++)
    {
        if (get_bit(a, i) != get_bit(b, i))
        {
            distance++;
        }
    }

    return HammingDistanceAns(distance);
}

// TODO: Way to mark Calculator::process() as main function
#pragma hls_top
HammingDistanceAns my_package(
    HammingDistance &HammingDistance,
    uint64_t x,
    uint64_t y)
{
    return HammingDistance.compute(x, y);
}
