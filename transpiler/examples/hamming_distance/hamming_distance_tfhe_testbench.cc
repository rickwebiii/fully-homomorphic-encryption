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

#define INT_DEF

#ifdef USE_INTERPRETED_TFHE
#include "transpiler/examples/hamming_distance/hamming_distance_interpreted_tfhe.h"
#include "transpiler/examples/hamming_distance/hamming_distance_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/hamming_distance/hamming_distance.h"
#include "transpiler/examples/hamming_distance/hamming_distance.types.h"
#endif

using namespace std;

const int kMainMinimumLambda = 120;

void calculate(
     uint64_t x,
     uint64_t y,
     TFHEParameters& params,
     TFHESecretKeySet& key
) {
  cout << "inputs are " 
     << x << " " << y << endl;
  // Encrypt data
  auto x_enc = Tfhe<uint64_t>::Encrypt(x, key);
  auto y_enc = Tfhe<uint64_t>::Encrypt(y, key);
  
  cout << "Encryption done" << endl;
  cout << "Initial state check by decryption: " << endl;
  
  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform computation
  Tfhe<HammingDistanceAns> encryptedResult(params);
  Tfhe<HammingDistance> hamming_distance(params);
  hamming_distance.SetUnencrypted(HammingDistance(), key.cloud());

  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(
     my_package(
          encryptedResult,
          hamming_distance,
          x_enc,
          y_enc,
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
  cout << (uint16_t)result.distance;
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

  calculate(0xFEEDF00D'CAFEBABE, 0x12345678'9ABCDEF0, params, key);
}
