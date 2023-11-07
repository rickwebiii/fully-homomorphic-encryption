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

#include "chisq_circuit.h"

ChiSqAns ChiSq::compute(short n_0, short n_1, short n_2) {
    short a = 4 * n_0 * n_2 - n_1 * n_1;
    short a_sq = a * a;

    short b_1 = 2 * n_0 + n_1;
    short b_1_sq = 2 * b_1 * b_1;

    short b_2 = (2 * n_0 + n_1) * (2 * n_2 + n_1);
    short b_3 = 2 * (2 * n_2 + n_1) * (2 * n_2 + n_1);

    return ChiSqAns(a_sq, b_1_sq, b_2, b_3);
}

// TODO: Way to mark Calculator::process() as main function
#pragma hls_top
ChiSqAns my_package(ChiSq &chisq, short n_0, short n_1, short n_2) {
  return chisq.compute(n_0, n_1, n_2);
}
