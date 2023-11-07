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

class ChiSqAns {
public: 
  short alpha;
  short beta1_sq;
  short beta2;
  short beta3;

  ChiSqAns() {}

  ChiSqAns(short alpha, short beta1_sq, short beta2, short beta3): alpha(alpha), beta1_sq(beta1_sq), beta2(beta2), beta3(beta3) {
  }
};

// Sums and subtracts values
class ChiSq {
public:
  ChiSqAns compute(short n_0, short n_1, short n_2);
};

#endif  // FULLY_HOMOMORPHIC_ENCRYPTION_TRANSPILER_EXAMPLES_CALCULATOR_H_
