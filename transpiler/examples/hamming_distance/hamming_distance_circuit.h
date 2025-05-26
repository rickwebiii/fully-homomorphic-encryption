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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HAMMING_DISTANCE_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_HAMMING_DISTANCE_H_

#ifndef INT_DEF
typedef unsigned char uint8_t;
typedef unsigned long long uint64_t;
#endif

class HammingDistanceAns {
public: 
  uint8_t distance;

  HammingDistanceAns() {}

  HammingDistanceAns(uint8_t distance): distance(distance) {}
};

// Sums and subtracts values
class HammingDistance {
public:
  HammingDistanceAns compute(
    uint64_t x,
    uint64_t y
  );
};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_CALCULATOR_H_
