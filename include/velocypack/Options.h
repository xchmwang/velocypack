////////////////////////////////////////////////////////////////////////////////
/// @brief Library to build up VPack documents.
///
/// DISCLAIMER
///
/// Copyright 2015 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Jan Steemann
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef VELOCYPACK_OPTIONS_H
#define VELOCYPACK_OPTIONS_H 1

#include "velocypack/velocypack-common.h"

namespace arangodb {
  namespace velocypack {

    struct Options {
      Options () {
      }

      // validate UTF-8 strings when JSON-parsing with Parser
      bool validateUtf8Strings      = false;

      // validate that attribute names in Object values are actually
      // unique when creating objects via Builder. This also includes
      // creation of Object values via a Parser
      bool checkAttributeUniqueness = false;

      // whether or not attribute names should be sorted in Object
      // values created with a Builder. This also includes creation of
      // Object values via a Parser
      bool sortAttributeNames       = true;

      // escape forward slashes when serializing VPack values into
      // JSON with a Dumper
      bool escapeForwardSlashes     = false;

      // default options with the above settings
      static Options const Defaults;
    };
          
  }  // namespace arangodb::velocypack
}  // namespace arangodb

#endif