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

#ifndef FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_CALCULATOR_H_
#define FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_CALCULATOR_H_

typedef unsigned char uint8_t;

class CardioAns {
public: 
  uint8_t risk;

  CardioAns() {}

  CardioAns(uint8_t risk): risk(risk) {
  }
};

// Sums and subtracts values
class Cardio {
public:
  CardioAns compute(
    uint8_t flags,
    uint8_t age,
    uint8_t hdl,
    uint8_t weight,
    uint8_t height,
    uint8_t physical_activity,
    uint8_t glasses_alcohol
  );
};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_CALCULATOR_H_
