/*
  Copyright 2008 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include <cstdio>
#include "../jscript_massager.h"

int main(int argc, char **argv) {
  std::string input;
  char buffer[80];
  while (fgets(buffer, sizeof(buffer), stdin))
    input += buffer;

  bool debug = argc > 1 && argv[1][0] == '1';
  std::string result = ggadget::smjs::MassageJScript(input.c_str(), debug,
                                                     "", 1);
  printf("%s", result.c_str());
}
