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
#include "transpiler/examples/cardio/cardio_interpreted_tfhe.h"
#include "transpiler/examples/cardio/cardio_interpreted_tfhe.types.h"
#else
#include "transpiler/examples/cardio/cardio_tfhe.h"
#include "transpiler/examples/cardio/cardio_tfhe.types.h"
#endif

typedef unsigned char uint8_t;

using namespace std;

const int kMainMinimumLambda = 120;

void calculate(
     bool man,
     bool smokes,
     bool diabetic,
     bool high_bp,
     uint8_t age,
     uint8_t hdl,
     uint8_t weight,
     uint8_t height,
     uint8_t physical_activity,
     uint8_t glasses_alcohol,
     TFHEParameters& params,
     TFHESecretKeySet& key
) {
  cout << "inputs are " 
     << man << " " 
     << smokes << " " 
     << diabetic << " "
     << high_bp << " "
     << age << " "
     << hdl << " "
     << weight << " "
     << height << " "
     << physical_activity << " "
     << glasses_alcohol << " "
     << endl;

  uint8_t flags = (uint8_t)man | ((uint8_t)smokes << 1) | ((uint8_t)diabetic << 2) | ((uint8_t)high_bp << 3);

  // Encrypt data
  auto encryptedFlags = Tfhe<uint8_t>::Encrypt(flags, key);
  auto encryptedAge = Tfhe<uint8_t>::Encrypt(age, key);
  auto encryptedHdl = Tfhe<uint8_t>::Encrypt(hdl, key);
  auto encryptedWeight = Tfhe<uint8_t>::Encrypt(weight, key);
  auto encryptedHeight = Tfhe<uint8_t>::Encrypt(height, key);
  auto encryptedPhysicalActivity = Tfhe<uint8_t>::Encrypt(physical_activity, key);
  auto encryptedGlassesAlcohol = Tfhe<uint8_t>::Encrypt(glasses_alcohol, key);

  cout << "Encryption done" << endl;
  cout << "Initial state check by decryption: " << endl;
  
  cout << "\t\t\t\t\tServer side computation:" << endl;
  // Perform computation
  Tfhe<CardioAns> encryptedResult(params);
  Tfhe<Cardio> cardio(params);
  cardio.SetUnencrypted(Cardio(), key.cloud());

  absl::Time start_time = absl::Now();
  double cpu_start_time = clock();
  XLS_CHECK_OK(
     my_package(
          encryptedResult,
          cardio,
          encryptedFlags,
          encryptedAge,
          encryptedHdl,
          encryptedWeight,
          encryptedHeight,
          encryptedPhysicalActivity,
          encryptedGlassesAlcohol,
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
  cout << (unsigned int)result.risk;
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

  calculate(
     false,
     false,
     true,
     true,
     40,
     50,
     70,
     170,
     1,
     1
     , params, key);
}
