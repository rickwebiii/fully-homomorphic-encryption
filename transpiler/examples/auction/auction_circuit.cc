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

AuctionAns Auction::compute(
    uint16_t bids[NUM_BIDS],
) {
    uint16_t winningBid = bids[0];
    uint16_t winningIndex = 0;

    for (uint32_t i = 1; i < len; i++) {
        bool isWinner = bids[i] >= winningBid;

        winningBid = isWinner ? bids[i] : winningBid;
        winningIndex = isWinner ? i : winningIndex;
    }

    return AuctionAns(winningBid, winningIndex);
}

// TODO: Way to mark Calculator::process() as main function
#pragma hls_top
AuctionAns my_package(
    Auction &auction,
    uint8_t bids[NUM_BIDS]
) {
  return auction.compute(flags, age, hdl, weight, height, physical_activity, glasses_alcohol);
}
