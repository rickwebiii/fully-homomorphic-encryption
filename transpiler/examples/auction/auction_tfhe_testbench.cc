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

#include <stdio.h>
#include <time.h>

#include <array>
#include <iostream>
#include <string>
#include <vector>

#include "tfhe/tfhe.h"
#include "tfhe/tfhe_io.h"
#include "transpiler/data/tfhe_data.h"
#include "xls/common/logging/logging.h"

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/auction/auction_interpreted_tfhe.h"
#include "transpiler/examples/auction/auction_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/auction/auction_tfhe.h"
#include "transpiler/examples/auction/auction_tfhe.types.h"
#endif

typedef unsigned short uint16_t;

using namespace std;

const int kMainMinimumLambda = 120;

void calculate(
     uint16_t bids[NUM_BIDS],
     TFHEParameters& params,
     TFHESecretKeySet& key
) {
  cout << "inputs are " 
     << bids;
  // Encrypt data
  auto bids_enc = TfheArray<uint16_t, NUM_BIDS>::Encrypt(absl::MakeSpan(bids, NUM_BIDS), key);
  
  cout << "Encryption done" << endl;
  cout << "Initial state check by decryption: " << endl;
  
  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform computation
  Tfhe<AuctionAns> encryptedResult(params);
  Tfhe<Auction> auction(params);
  auction.SetUnencrypted(Auction(), key.cloud());

  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(
     my_package(
          encryptedResult,
          auction,
          bids_enc,
          key.cloud()
  ));
  double cpu_end_time = clock();
  absl::Time end_time = absl::Now();
  cout << "\t\t\t\t\tComputation done" << endl;
  cout << "\t\t\t\t\tTotal time: "
       << absl::ToDoubleSeconds(end_time - start_time) << " secs" << endl;
  cout << "\t\t\t\t\t  CPU time: "
       << (cpu_end_time - cpu_start_time) / 1'000'000 << " secs" << endl;

  // Decrypt results.
  absl::Time decrypt_time_start = absl::Now();
  auto result = encryptedResult.Decrypt(key);
  absl::Time decrypt_time_end = absl::Now();

  cout << "Decrypted result: ";
  cout << result.bid << " " << result.index;
  cout << "\n";
  cout << "Decryption done" << endl << endl;

  cout << "Decryption time " << (decrypt_time_end - decrypt_time_start) << endl;
}

int main(int argc, char** argv) {
  // generate a keyset
  TFHEParameters params(kMainMinimumLambda);

  // generate a random key
  // Note: In real applications, a cryptographically secure seed needs to be
  // used.
  array<uint32_t, 3> seed = {314, 1592, 657};

  absl::Time keygen_time_start = absl::Now();
  TFHESecretKeySet key(params, seed);
  absl::Time keygen_time_end = absl::Now();

  cout << "Keygen time " << (keygen_time_end - keygen_time_start) << endl;

  uint16_t bids[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};

  calculate(bids, params, key);
}
