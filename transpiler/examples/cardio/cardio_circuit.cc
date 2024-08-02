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

#include "cardio_circuit.h"

bool get_bit(uint8_t flags, uint8_t id) {
  return (flags >> id) & 0x1;
}

CardioAns Cardio::compute(
    uint8_t flags,
    uint8_t age,
    uint8_t hdl,
    uint8_t weight,
    uint8_t height,
    uint8_t physical_activity,
    uint8_t glasses_alcohol
) {
    bool man = get_bit(flags, 0);
    bool smoking = get_bit(flags, 1);
    bool diabetic = get_bit(flags, 2);
    bool high_bp = get_bit(flags, 3);

    uint8_t cond1 = man && (age > 50);
    uint8_t cond2 = !man && (age > 60);
    uint8_t cond3 = smoking;
    uint8_t cond4 = diabetic;
    uint8_t cond5 = high_bp;
    uint8_t cond6 = hdl < 40;
    uint8_t cond7 = weight > ((uint8_t) (height - 90));
    uint8_t cond8 = physical_activity < 30;
    uint8_t cond9 = man && (glasses_alcohol > 3);
    uint8_t cond10 = !man && (glasses_alcohol > 2);

    return CardioAns(cond1 + cond2 + cond3 + cond4 + cond5 + cond6 + cond7 + cond8 + cond9 + cond10);
}

// TODO: Way to mark Calculator::process() as main function
#pragma hls_top
CardioAns my_package(
    Cardio &chisq,
    uint8_t flags,
    uint8_t age,
    uint8_t hdl,
    uint8_t weight,
    uint8_t height,
    uint8_t physical_activity,
    uint8_t glasses_alcohol
) {
  return chisq.compute(flags, age, hdl, weight, height, physical_activity, glasses_alcohol);
}
