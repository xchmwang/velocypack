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

#include <ostream>

#include "velocypack/velocypack-common.h"
#include "velocypack/AttributeTranslator.h"
#include "velocypack/Builder.h"
#include "velocypack/Dumper.h"
#include "velocypack/HexDump.h"
#include "velocypack/Iterator.h"
#include "velocypack/Parser.h"
#include "velocypack/Slice.h"
#include "velocypack/ValueType.h"

using namespace arangodb::velocypack;
using VT = arangodb::velocypack::ValueType;
  
VT const Slice::TypeMap[256] = {
    /* 0x00 */ VT::None,     /* 0x01 */ VT::Array,
    /* 0x02 */ VT::Array,    /* 0x03 */ VT::Array,
    /* 0x04 */ VT::Array,    /* 0x05 */ VT::Array,
    /* 0x06 */ VT::Array,    /* 0x07 */ VT::Array,
    /* 0x08 */ VT::Array,    /* 0x09 */ VT::Array,
    /* 0x0a */ VT::Object,   /* 0x0b */ VT::Object,
    /* 0x0c */ VT::Object,   /* 0x0d */ VT::Object,
    /* 0x0e */ VT::Object,   /* 0x0f */ VT::None,
    /* 0x10 */ VT::None,     /* 0x11 */ VT::None,
    /* 0x12 */ VT::None,     /* 0x13 */ VT::Array,
    /* 0x14 */ VT::Object,   /* 0x15 */ VT::None,
    /* 0x16 */ VT::None,     /* 0x17 */ VT::Illegal,
    /* 0x18 */ VT::Null,     /* 0x19 */ VT::Bool,
    /* 0x1a */ VT::Bool,     /* 0x1b */ VT::Double,
    /* 0x1c */ VT::UTCDate,  /* 0x1d */ VT::External,
    /* 0x1e */ VT::MinKey,   /* 0x1f */ VT::MaxKey,
    /* 0x20 */ VT::Int,      /* 0x21 */ VT::Int,
    /* 0x22 */ VT::Int,      /* 0x23 */ VT::Int,
    /* 0x24 */ VT::Int,      /* 0x25 */ VT::Int,
    /* 0x26 */ VT::Int,      /* 0x27 */ VT::Int,
    /* 0x28 */ VT::UInt,     /* 0x29 */ VT::UInt,
    /* 0x2a */ VT::UInt,     /* 0x2b */ VT::UInt,
    /* 0x2c */ VT::UInt,     /* 0x2d */ VT::UInt,
    /* 0x2e */ VT::UInt,     /* 0x2f */ VT::UInt,
    /* 0x30 */ VT::SmallInt, /* 0x31 */ VT::SmallInt,
    /* 0x32 */ VT::SmallInt, /* 0x33 */ VT::SmallInt,
    /* 0x34 */ VT::SmallInt, /* 0x35 */ VT::SmallInt,
    /* 0x36 */ VT::SmallInt, /* 0x37 */ VT::SmallInt,
    /* 0x38 */ VT::SmallInt, /* 0x39 */ VT::SmallInt,
    /* 0x3a */ VT::SmallInt, /* 0x3b */ VT::SmallInt,
    /* 0x3c */ VT::SmallInt, /* 0x3d */ VT::SmallInt,
    /* 0x3e */ VT::SmallInt, /* 0x3f */ VT::SmallInt,
    /* 0x40 */ VT::String,   /* 0x41 */ VT::String,
    /* 0x42 */ VT::String,   /* 0x43 */ VT::String,
    /* 0x44 */ VT::String,   /* 0x45 */ VT::String,
    /* 0x46 */ VT::String,   /* 0x47 */ VT::String,
    /* 0x48 */ VT::String,   /* 0x49 */ VT::String,
    /* 0x4a */ VT::String,   /* 0x4b */ VT::String,
    /* 0x4c */ VT::String,   /* 0x4d */ VT::String,
    /* 0x4e */ VT::String,   /* 0x4f */ VT::String,
    /* 0x50 */ VT::String,   /* 0x51 */ VT::String,
    /* 0x52 */ VT::String,   /* 0x53 */ VT::String,
    /* 0x54 */ VT::String,   /* 0x55 */ VT::String,
    /* 0x56 */ VT::String,   /* 0x57 */ VT::String,
    /* 0x58 */ VT::String,   /* 0x59 */ VT::String,
    /* 0x5a */ VT::String,   /* 0x5b */ VT::String,
    /* 0x5c */ VT::String,   /* 0x5d */ VT::String,
    /* 0x5e */ VT::String,   /* 0x5f */ VT::String,
    /* 0x60 */ VT::String,   /* 0x61 */ VT::String,
    /* 0x62 */ VT::String,   /* 0x63 */ VT::String,
    /* 0x64 */ VT::String,   /* 0x65 */ VT::String,
    /* 0x66 */ VT::String,   /* 0x67 */ VT::String,
    /* 0x68 */ VT::String,   /* 0x69 */ VT::String,
    /* 0x6a */ VT::String,   /* 0x6b */ VT::String,
    /* 0x6c */ VT::String,   /* 0x6d */ VT::String,
    /* 0x6e */ VT::String,   /* 0x6f */ VT::String,
    /* 0x70 */ VT::String,   /* 0x71 */ VT::String,
    /* 0x72 */ VT::String,   /* 0x73 */ VT::String,
    /* 0x74 */ VT::String,   /* 0x75 */ VT::String,
    /* 0x76 */ VT::String,   /* 0x77 */ VT::String,
    /* 0x78 */ VT::String,   /* 0x79 */ VT::String,
    /* 0x7a */ VT::String,   /* 0x7b */ VT::String,
    /* 0x7c */ VT::String,   /* 0x7d */ VT::String,
    /* 0x7e */ VT::String,   /* 0x7f */ VT::String,
    /* 0x80 */ VT::String,   /* 0x81 */ VT::String,
    /* 0x82 */ VT::String,   /* 0x83 */ VT::String,
    /* 0x84 */ VT::String,   /* 0x85 */ VT::String,
    /* 0x86 */ VT::String,   /* 0x87 */ VT::String,
    /* 0x88 */ VT::String,   /* 0x89 */ VT::String,
    /* 0x8a */ VT::String,   /* 0x8b */ VT::String,
    /* 0x8c */ VT::String,   /* 0x8d */ VT::String,
    /* 0x8e */ VT::String,   /* 0x8f */ VT::String,
    /* 0x90 */ VT::String,   /* 0x91 */ VT::String,
    /* 0x92 */ VT::String,   /* 0x93 */ VT::String,
    /* 0x94 */ VT::String,   /* 0x95 */ VT::String,
    /* 0x96 */ VT::String,   /* 0x97 */ VT::String,
    /* 0x98 */ VT::String,   /* 0x99 */ VT::String,
    /* 0x9a */ VT::String,   /* 0x9b */ VT::String,
    /* 0x9c */ VT::String,   /* 0x9d */ VT::String,
    /* 0x9e */ VT::String,   /* 0x9f */ VT::String,
    /* 0xa0 */ VT::String,   /* 0xa1 */ VT::String,
    /* 0xa2 */ VT::String,   /* 0xa3 */ VT::String,
    /* 0xa4 */ VT::String,   /* 0xa5 */ VT::String,
    /* 0xa6 */ VT::String,   /* 0xa7 */ VT::String,
    /* 0xa8 */ VT::String,   /* 0xa9 */ VT::String,
    /* 0xaa */ VT::String,   /* 0xab */ VT::String,
    /* 0xac */ VT::String,   /* 0xad */ VT::String,
    /* 0xae */ VT::String,   /* 0xaf */ VT::String,
    /* 0xb0 */ VT::String,   /* 0xb1 */ VT::String,
    /* 0xb2 */ VT::String,   /* 0xb3 */ VT::String,
    /* 0xb4 */ VT::String,   /* 0xb5 */ VT::String,
    /* 0xb6 */ VT::String,   /* 0xb7 */ VT::String,
    /* 0xb8 */ VT::String,   /* 0xb9 */ VT::String,
    /* 0xba */ VT::String,   /* 0xbb */ VT::String,
    /* 0xbc */ VT::String,   /* 0xbd */ VT::String,
    /* 0xbe */ VT::String,   /* 0xbf */ VT::String,
    /* 0xc0 */ VT::Binary,   /* 0xc1 */ VT::Binary,
    /* 0xc2 */ VT::Binary,   /* 0xc3 */ VT::Binary,
    /* 0xc4 */ VT::Binary,   /* 0xc5 */ VT::Binary,
    /* 0xc6 */ VT::Binary,   /* 0xc7 */ VT::Binary,
    /* 0xc8 */ VT::BCD,      /* 0xc9 */ VT::BCD,
    /* 0xca */ VT::BCD,      /* 0xcb */ VT::BCD,
    /* 0xcc */ VT::BCD,      /* 0xcd */ VT::BCD,
    /* 0xce */ VT::BCD,      /* 0xcf */ VT::BCD,
    /* 0xd0 */ VT::BCD,      /* 0xd1 */ VT::BCD,
    /* 0xd2 */ VT::BCD,      /* 0xd3 */ VT::BCD,
    /* 0xd4 */ VT::BCD,      /* 0xd5 */ VT::BCD,
    /* 0xd6 */ VT::BCD,      /* 0xd7 */ VT::BCD,
    /* 0xd8 */ VT::None,     /* 0xd9 */ VT::None,
    /* 0xda */ VT::None,     /* 0xdb */ VT::None,
    /* 0xdc */ VT::None,     /* 0xdd */ VT::None,
    /* 0xde */ VT::None,     /* 0xdf */ VT::None,
    /* 0xe0 */ VT::None,     /* 0xe1 */ VT::None,
    /* 0xe2 */ VT::None,     /* 0xe3 */ VT::None,
    /* 0xe4 */ VT::None,     /* 0xe5 */ VT::None,
    /* 0xe6 */ VT::None,     /* 0xe7 */ VT::None,
    /* 0xe8 */ VT::None,     /* 0xe9 */ VT::None,
    /* 0xea */ VT::None,     /* 0xeb */ VT::None,
    /* 0xec */ VT::None,     /* 0xed */ VT::None,
    /* 0xee */ VT::None,     /* 0xef */ VT::None,
    /* 0xf0 */ VT::Custom,   /* 0xf1 */ VT::Custom,
    /* 0xf2 */ VT::Custom,   /* 0xf3 */ VT::Custom,
    /* 0xf4 */ VT::Custom,   /* 0xf5 */ VT::Custom,
    /* 0xf6 */ VT::Custom,   /* 0xf7 */ VT::Custom,
    /* 0xf8 */ VT::Custom,   /* 0xf9 */ VT::Custom,
    /* 0xfa */ VT::Custom,   /* 0xfb */ VT::Custom,
    /* 0xfc */ VT::Custom,   /* 0xfd */ VT::Custom,
    /* 0xfe */ VT::Custom,   /* 0xff */ VT::Custom};

unsigned int const Slice::WidthMap[32] = {
    0,  // 0x00, None
    1,  // 0x01, empty array
    1,  // 0x02, array without index table
    2,  // 0x03, array without index table
    4,  // 0x04, array without index table
    8,  // 0x05, array without index table
    1,  // 0x06, array with index table
    2,  // 0x07, array with index table
    4,  // 0x08, array with index table
    8,  // 0x09, array with index table
    1,  // 0x0a, empty object
    1,  // 0x0b, object with sorted index table
    2,  // 0x0c, object with sorted index table
    4,  // 0x0d, object with sorted index table
    8,  // 0x0e, object with sorted index table
    0};

unsigned int const Slice::FirstSubMap[32] = {
    0,  // 0x00, None
    1,  // 0x01, empty array
    2,  // 0x02, array without index table
    3,  // 0x03, array without index table
    5,  // 0x04, array without index table
    9,  // 0x05, array without index table
    3,  // 0x06, array with index table
    5,  // 0x07, array with index table
    9,  // 0x08, array with index table
    9,  // 0x09, array with index table
    1,  // 0x0a, empty object
    5,  // 0x0b, object with sorted index table
    9,  // 0x0c, object with sorted index table
    9,  // 0x0d, object with sorted index table
    9,  // 0x0e, object with sorted index table
    0};

ValueLength const Slice::seedTable[3 * 256] = { 
  0x5a27472f1245a7ffULL, 0xeddaba64abf7395cULL, 0xcf8be3d0165b6f7aULL,
  0x79a2b28300dbe918ULL, 0x277a445ce1007835ULL, 0x3108190bb5e267deULL,
  0x61d7646736301350ULL, 0xf421c6a52cc91b5dULL, 0x513bd42e53f01d5fULL,
  0xdeb7eae6408f6e87ULL, 0x1d4604aed518d37bULL, 0x7d622a7930857b90ULL,
  0x617486cf7811a9faULL, 0x23d5c4544470595dULL, 0xb1196ad9361214dcULL,
  0x136f45b1f41ebd00ULL, 0x6851b305831a45fcULL, 0x361db6a9779cb8ddULL,
  0x445e61691dbd5f60ULL, 0x58d26ccd4d5aa6d2ULL, 0x2c4abfaf6c6f40f1ULL,
  0x99e8efb38ba3d04aULL, 0x9be88ea64fe8e750ULL, 0x787b18142547cd55ULL,
  0x85b0fc78c2e0bbacULL, 0x999caedfbaceafd8ULL, 0xfe4385126a295417ULL,
  0xeca3ecf77d9b7177ULL, 0xd9f075d1bdae54bcULL, 0xab1f8f295481a4d6ULL,
  0xef698d125466958eULL, 0xde8f107ac167eb79ULL, 0x38dd60a797b80385ULL,
  0x4c18b0d685f7bcf3ULL, 0x3e860d6520f67392ULL, 0xb8b533204efac130ULL,
  0x841c44d8f7ce1701ULL, 0xf71743a79c27282cULL, 0x1eef1b9edcf4f3a9ULL,
  0xe4f0e6f627cccd2dULL, 0x6f4e8e19a6165a18ULL, 0x60b33a63f392a46dULL,
  0x4121407ba59517d2ULL, 0x64d9a2eac5c32071ULL, 0x82a6efcc45d140b6ULL,
  0x32fec00d34900c1bULL, 0x27d7ebfac6683882ULL, 0xcb51b9206334138aULL,
  0xcdd6a4bf530b5757ULL, 0x69dd7ec6236b7631ULL, 0xac4224d692e117beULL,
  0x205b8d1f22751b10ULL, 0xb068b38419052d8bULL, 0x169150a6c274e83eULL,
  0xbafd63cb54934243ULL, 0x7c6891341b63c54eULL, 0x2a18f6916d102d98ULL,
  0xf0c70eaa7536a39cULL, 0xbc74e924a78d7f0eULL, 0x6934198e95a3e549ULL,
  0xfba84e6892d142a2ULL, 0x384998f977ce7bf2ULL, 0xe7e84ec15d2b5cb5ULL,
  0xb4ba0909973c4ffdULL, 0x434b6868d7a1681bULL, 0x03a79309f3b163efULL,
  0x4b080fea59fb7c76ULL, 0xf86778965ec1a145ULL, 0x7ccf7627d1e4f2b2ULL,
  0x0a3d9a8a41298daeULL, 0x0e2b9b4a04997ce8ULL, 0xdc434f5a6dc6861eULL,
  0xb270d3f9474a5b2dULL, 0x20f38b3d1904e5aeULL, 0x3ef3b1e9442a57e9ULL,
  0x266a435d0c922dcaULL, 0x753795d113b256e3ULL, 0xbff434e2cf784ee9ULL,
  0xb6d5164c96e7dc16ULL, 0x430550d0d36c6300ULL, 0xf36ab6fe7064efb5ULL,
  0x07ec0ee830a1767cULL, 0x8a7a2cb95e34aaf2ULL, 0x4eb0bcb33b0026b1ULL,
  0x5e93428876dc5447ULL, 0xd26438b1b2c4100fULL, 0xe3efed064b3aed46ULL,
  0x18ba837598564c9bULL, 0x298024ccddd732b0ULL, 0x11f5c594b49a6ca1ULL,
  0x32ad47c782bb1d50ULL, 0x5fff0c820f39b8f2ULL, 0x755e477122565959ULL,
  0xc8ae37f097d89cb8ULL, 0xb24d1824b4f7ad71ULL, 0x70053e9e0e59bc99ULL,
  0x978c2bddc6bb741bULL, 0x602fad016df0a808ULL, 0x804dca176b26292aULL,
  0x58251c008c7edf13ULL, 0x0d17fceba691fd99ULL, 0x8649a5e5cefb105bULL,
  0xc250595118d043ddULL, 0x312e9ddf798898c4ULL, 0x5c2e38b5da45a719ULL,
  0x2f0caaa2eeba1987ULL, 0xa659235bc88e7101ULL, 0x2b87f4449854d19dULL,
  0xd6d731fea26ded84ULL, 0x3434c9a782f5d594ULL, 0x154d6d9b515c1f5fULL,
  0x6db75059ca2630efULL, 0x006ada5b46225ee9ULL, 0x2a54fd59effa748bULL,
  0x91d2b78084536596ULL, 0xf5f0f75e29cfc444ULL, 0x3a862dfef2d0f286ULL,
  0xec7b96e618bae21eULL, 0xbd0d63a1c1a98640ULL, 0xfa77f2cb3db0de55ULL,
  0x38293f6e834bc43dULL, 0x3a8b18b53a18363cULL, 0x8ad6cec289eeb313ULL,
  0xd66192a47340fde8ULL, 0xe8dc652ca9e0fd59ULL, 0x944e86c2efe1646cULL,
  0x6f36dddadf6968d2ULL, 0x131d797efd3fc44bULL, 0x2d47a6e4651d0b4bULL,
  0x7c5f630746dcc751ULL, 0x77c19f43f7215063ULL, 0x915d10c49752c481ULL,
  0xa66ce8ec129aa339ULL, 0x8479c8d59f5e44f5ULL, 0x2674be9d1ccadb7aULL,
  0xec882c352d8f2019ULL, 0xa28d1d33176e3ffcULL, 0x18175217054f43d0ULL,
  0x1b3563f8c9d3acedULL, 0x7682fab75ba851faULL, 0xc0d7b587f1574b49ULL,
  0xcb8038cf0014f1d2ULL, 0xac55ec88be788814ULL, 0x0cb97a80028fe419ULL,
  0x93a1e201e43c6674ULL, 0xc9d71afddf74c30eULL, 0x0cd6c1f02c4e2e92ULL,
  0x9523a254031101ebULL, 0x9c9f38f7bc597fc3ULL, 0x4897bc8323323b7dULL,
  0xc9ea57f06f8202ceULL, 0x69cc4c0b02fa87baULL, 0x606bdaa4b3ec9e08ULL,
  0xed818257521cf215ULL, 0xa8bbb5aa1deeb855ULL, 0xa55a2102317c9ba9ULL,
  0xd8e71eaf9f7d9fefULL, 0xaa6ef7175d0834c8ULL, 0xe9bb38b5649541e6ULL,
  0xf3a5c4bd1fefdf73ULL, 0x07d348d3c1b339f3ULL, 0x8186a7368c806656ULL,
  0x916ae69e876e6c9fULL, 0xe18fbe8b5c1f707eULL, 0x364374da37c8014dULL,
  0xb82c3ee48e5172e9ULL, 0x852e32079aa36c71ULL, 0x5c40aa309192c94aULL,
  0x9157a8c505a1533fULL, 0x2570c06d4e2c7612ULL, 0x3ebdfaac4943a92aULL,
  0xb1bf61f0ad96867eULL, 0xfe15d42582b16b16ULL, 0x7e90acc417b18f8eULL,
  0x5622f26c274e9786ULL, 0xebd547297577b88dULL, 0xd5ddc97d946f1be0ULL,
  0xb2ef7645383abb58ULL, 0xea1fdac5e10a40e6ULL, 0x5a0195e5bda4819dULL,
  0x2739a68b77c133a3ULL, 0x7d2012d920dcf157ULL, 0x8f18323850a86f89ULL,
  0x94d6536dd8250f76ULL, 0xaba3659c04fdbf69ULL, 0x3b5fa402e23d6993ULL,
  0xb332d492b8a3b744ULL, 0x48fb322f51460fdcULL, 0x7782f6bfd626fe57ULL,
  0x57bffbc650ead523ULL, 0xb91b97552b743f92ULL, 0xd1ed60463dba78a4ULL,
  0x57be08fa077b5844ULL, 0xc6408e53e49bf582ULL, 0xfc99cd4727d1df42ULL,
  0xed38fa5e878c55b3ULL, 0x5432ea023a69f99eULL, 0xc4468438dfc5a4dcULL,
  0x8e2d5e9fb6b758bfULL, 0xeb3073d58bce0c1eULL, 0x6b344610343ba385ULL,
  0x61872e368bd81e31ULL, 0x20e3c2507a8c3937ULL, 0x5eba081e398b3522ULL,
  0x3809ce85e9cc335eULL, 0x28c4c1d5f7c969c1ULL, 0xcac0c3640edc467cULL,
  0xcb5795570c17d10fULL, 0xd475ab2d98cc766fULL, 0xd7dd8a5590b06c1cULL,
  0x862e4760c5658ab0ULL, 0xee1c6e257623301fULL, 0xc53273caee86a8caULL,
  0xc13c3e545722f1c4ULL, 0x1ff3d041a0edce58ULL, 0xf575213cae7e174cULL,
  0xce4c2a02ce1653c3ULL, 0xc469e895e6146acfULL, 0xebf0fb1681cd9df2ULL,
  0x9c5fec6439be1172ULL, 0xd875737276fa0abdULL, 0xdeb4a99dcfb05822ULL,
  0x333c355cc8283847ULL, 0x35e7dd40e613e562ULL, 0x3aa1da4f7875cf19ULL,
  0x17f63a1172c70444ULL, 0xeda465021b019797ULL, 0x14b97c19251fc771ULL,
  0x39a090f35940abbeULL, 0xce3bc2fe92efe9b7ULL, 0xd992ce418042096fULL,
  0x67c54ee86e34282fULL, 0x7a4e847828da25faULL, 0x1f655ea252209742ULL,
  0x91a5658f089785adULL, 0x984c38d5fe57a342ULL, 0x794e4ce36aefacb7ULL,
  0x6e7dd01f458d58cbULL, 0x8e895e43f557ac9fULL, 0x1f8384ed848bb4bdULL,
  0x887f8be841cd1cf2ULL, 0x856e4d742088fe21ULL, 0x5207f2ec7e447848ULL,
  0x4b9e035a0c8d8857ULL, 0x8577d02d8ecd7369ULL, 0xe4d7e132feda262eULL,
  0x71b57dec609904eaULL, 0x0ea370024c688347ULL, 0x057ce443de4bc1f5ULL,
  0x497664b61852b3e4ULL, 0xc945acce54c1522aULL, 0x41efe14d607a2735ULL,
  0x99eb1b2e7d2b4a63ULL, 0x42bbc6a9e3144dc8ULL, 0x89c50aa4dd8b191aULL,
  0xda3596dbb12ab19bULL, 0xcd63c08eecf62a54ULL, 0xa127127f2a6f0b68ULL,
  0x0c5806c83642e502ULL, 0xd91125073f399b83ULL, 0x6dab93e5556db41cULL,
  0x01be658e1468934eULL, 0x2ec10fb37eb91d61ULL, 0x8d37bca1386e0596ULL,
  0x6f2c32ce30188cb7ULL, 0x8be0324e1598c434ULL, 0xc1444fe2962d6d03ULL,
  0xfc704bf248d36fc3ULL, 0x2389b5caf9b0bcf1ULL, 0x490cc951e85f40c5ULL,
  0x7015b39bcace294fULL, 0xd52680b1be873dc9ULL, 0xbe5ef2c48387d585ULL,
  0x19f7bc62ec77b8f0ULL, 0x5442a9d23fea40b2ULL, 0xe9461b9f76d4e108ULL,
  0x7e9d8ebf5d1eb82bULL, 0x9ef231f5c2fd2becULL, 0x5c5af47c5f82ddb5ULL,
  0xec5f087bcde51440ULL, 0x043d4a27b8412a75ULL, 0x580917928a2466bdULL,
  0xf33c0ed06e0139c1ULL, 0xdb9ff7b227bfcf2dULL, 0x31a0f5a71dc4aa95ULL,
  0x4914bcf22c0423d0ULL, 0xfa0b69358570660cULL, 0x4ab1fc945643957eULL,
  0x1141ce9421579e8dULL, 0xd2246801dca72a0bULL, 0xa507eeee73017fabULL,
  0x94a1ace2998caa8bULL, 0xf93c67d8b7ca9aa1ULL, 0x87d0c3809945d595ULL,
  0xfd1f69b11042adf6ULL, 0xfd8a5ae4c7f60149ULL, 0x67911cb6f3a34b73ULL,
  0xedff8d114a9203aeULL, 0x682255c5b74b269cULL, 0x28522f58145979ceULL,
  0xd032f4032cccc828ULL, 0xad72b2b499a2bf72ULL, 0x277ac1311604d728ULL,
  0x5301d1409e452ef3ULL, 0x6f32572a776cc9d2ULL, 0x9d3e5a1fda84e1d8ULL,
  0x197dd52929b11c10ULL, 0x3ffd34533bee649dULL, 0x8ad833b329d7dff2ULL,
  0x79da2dd924830781ULL, 0xcc3a599d4da08754ULL, 0x5eb52caf67542ea1ULL,
  0xf6277e794b846f0fULL, 0x904237ec32c91246ULL, 0xea4f1d5b3b1773e0ULL,
  0x8a29f992af20eb71ULL, 0xd9cf68298fcc0e61ULL, 0xcdd577b1eaebe80cULL,
  0xe02982d620c99a1dULL, 0x86b11ab12022a12fULL, 0xc8eff85ee5c7ef08ULL,
  0x786fd4c3ebd9638eULL, 0xa288d5a1f7ccf351ULL, 0xe28acd934460c5d5ULL,
  0x430e4e4010db365cULL, 0x3ed1c5c46081ed62ULL, 0xb897e70dceda5debULL,
  0x862085cc97732225ULL, 0x4c60478f131a75ebULL, 0x58646bca91d4efe8ULL,
  0xb7789478e82bc09bULL, 0x51512191afa1b60bULL, 0x7297e1dfc4b4d413ULL,
  0xd99ea6c8a0159374ULL, 0x64792685e5c8a923ULL, 0xa0e3e7bb537df62cULL,
  0xa674472eefb97e34ULL, 0x368edf4e6076772fULL, 0x1a3c9fea0f2c3fa9ULL,
  0x331917e5909b2449ULL, 0x4d58a50e2078b171ULL, 0xd56cf470daa27fb1ULL,
  0x1422e07ab6b64bd9ULL, 0xa996ae07205f6ba3ULL, 0x041856b8ebf9732fULL,
  0xc88a3f95f0229975ULL, 0x16324350d4682b38ULL, 0xb5fab3f3a6ae5bb6ULL,
  0x2e0a2dfbd9790e8fULL, 0xb84122813cfb80a7ULL, 0x7000ce215d63d283ULL,
  0x9597ca4f19ee6d16ULL, 0xffb7a7fc02adaa00ULL, 0xc7ec49a63779dc3bULL,
  0x2f799a3a2041d37eULL, 0x0e77b8863b07e357ULL, 0x5be99ef924cc0624ULL,
  0x1d1864e355290865ULL, 0x948ceec6c3a3260dULL, 0x925cf040ea95d691ULL,
  0x0350bb5d022f0167ULL, 0x05729d3ef356bab1ULL, 0x439d1a5689f0c458ULL,
  0x20b9aa23ccffd493ULL, 0xe76eb4621f8bb2cfULL, 0x238e99a1d8ed6224ULL,
  0x798befaef2cebdc4ULL, 0xe918d95d5a38da87ULL, 0x26d1c5d6e08ba92dULL,
  0x765f3ca0a3e2d96eULL, 0xdeb3084db17cda2dULL, 0x543b027d2904bb91ULL,
  0x4de434d7ef09f7e5ULL, 0x02684bdcba8a03d4ULL, 0x1128f8d98ea6a5acULL,
  0xf23d7ee683edc6f9ULL, 0x507751c96d55f33dULL, 0xc94b040f4f41d111ULL,
  0xc36c739dc9f57408ULL, 0x1c89448eff462f11ULL, 0xb763592b1df2360dULL,
  0xb1b97339bd51eb98ULL, 0x936fd25f45c76a90ULL, 0x826ae0e4ffbbf408ULL,
  0x41a51c2ec9c702d8ULL, 0x75dcf228ff301d28ULL, 0xf0ead5fdd2cd3dd1ULL,
  0xeeb1e73489231354ULL, 0x450d1aefbdcd10ccULL, 0xaa0f93e937db794aULL,
  0xfc1e82155262885cULL, 0x617d9b30097890cdULL, 0xb5dd7db8677499a3ULL,
  0x66ab34927a5207d4ULL, 0x9e7480158361eea7ULL, 0xc8871c8264316ee7ULL,
  0x228ec9f0938959c8ULL, 0xe70f8be8a6b2d886ULL, 0x61fbdc65dd0c60fbULL,
  0x70fcb9162dbe77bdULL, 0x837620f299f5a4acULL, 0xb24826d7bf88c1c9ULL,
  0xf0cacd80f141590dULL, 0xc897afc534861ad0ULL, 0x4b897250f1646680ULL,
  0xef1f8d9be9861c4fULL, 0x5bd8cf73531c20faULL, 0x9721ff044a430818ULL,
  0x8a2efe6045e7632bULL, 0x58bf9c8053cdef13ULL, 0x6985fc9c9d5bc7cdULL,
  0x55be707aa1a731eeULL, 0x03a0fa11137860e7ULL, 0xb5f7f733354b9bacULL,
  0xbe46778e622cc706ULL, 0xc20f6557e65b3c01ULL, 0xe992f662a93c94f8ULL,
  0xa0841aab50073d0cULL, 0x6f86ef625899c632ULL, 0x4154b558a40ee3bdULL,
  0x93f47d0fe2d936aeULL, 0x4f341bc52f35f164ULL, 0x367d35d1b87c204dULL,
  0x368b66a0465d4eeeULL, 0x67af26879390d210ULL, 0xa4fef44883979974ULL,
  0x59faf40ef3c85ebcULL, 0x8566fdea0b73be80ULL, 0x998e7d4afc91045bULL,
  0x70ab5ae7c60f9109ULL, 0xcb51988c8990bdefULL, 0x86bc70737c4f5916ULL,
  0x9447fd9f2952f349ULL, 0x49da95da0ce81735ULL, 0xdd24231710ab869fULL,
  0x3e294837ab307e23ULL, 0x473aeea20958d1a6ULL, 0x5142fc93b20ffe2bULL,
  0x1a04e04cf643cffbULL, 0x901b7059ff2e0d27ULL, 0x0d6f1d7ac988828aULL,
  0x5f583d312bccadfdULL, 0x9b5c46671d6b27ccULL, 0xeaa4836c9c447b88ULL,
  0x773a697841a10910ULL, 0x79d14ac4465c19d7ULL, 0x3a9690e07c54afe7ULL,
  0x69ee9028f7f6cd0fULL, 0x90968edf2dd96311ULL, 0x82c739a73b027102ULL,
  0x1549bf67db5f751dULL, 0x7dd6f71ae0cddccfULL, 0x7fdbacae8d958fb5ULL,
  0x2ebd8712f03c07b4ULL, 0x06d36e2cc08e5a33ULL, 0x2a876251df697a53ULL,
  0x99dfaee249ec02a1ULL, 0x1c67725a026ac286ULL, 0x95cd4535f788bec1ULL,
  0xd8dfaba469bc54a1ULL, 0xd36f140fc587240fULL, 0xd4ab9b76aeec842dULL,
  0xefd39c76fbba830eULL, 0x923624cd43bc9fc5ULL, 0xf6325533e5655924ULL,
  0xc208ca880327d5e7ULL, 0xf751e52d243ee000ULL, 0xe687c12b3ddd206bULL,
  0x7a171de8fd99c8a6ULL, 0xc8dc5e38851e23f0ULL, 0xb67b48f00de977c4ULL,
  0x198caa8c8f88c59cULL, 0xbdc2483df34c6e07ULL, 0x12546ac3b807baf8ULL,
  0x5153c5b4f97126e5ULL, 0xf3ecfbb3990e33f8ULL, 0xe73105b9ab9c009fULL,
  0x9727a49bc9449db0ULL, 0x391f77580ab133f2ULL, 0x0eb01a4b6e5b789eULL,
  0xa5147fec1b43ce6eULL, 0x87cbdb601008323eULL, 0x179832561cf06683ULL,
  0xf6df1fe607bc47b2ULL, 0x682d6c88c5c5cfffULL, 0xa458ce2e1b38add3ULL,
  0xdfc1fb1038093a92ULL, 0x0d9ceaaaca389f06ULL, 0x3f581f185597be3cULL,
  0xbf36458f6d7f5388ULL, 0xdff3811b1cbfa3a5ULL, 0xf4be7ad281dd0c3cULL,
  0xd479b96a8f8d99e6ULL, 0x5be5479bab93dd5aULL, 0x0910d2f10e35e6edULL,
  0xa5dc9cdba32d2d7aULL, 0xb74019f2fcad20bfULL, 0xcb56cbae8f51d9c3ULL,
  0x7f5c4107d630c174ULL, 0xe0e5f52b2c055da7ULL, 0xf17acc5d0d9744b9ULL,
  0x3f99f4e0bf7fcdd0ULL, 0x07b8f56d6e66aa98ULL, 0x3e2703622b5b3acfULL,
  0x4abd73907501112eULL, 0x0fa64fccdf835172ULL, 0xc60af557f99c31c0ULL,
  0x847b8690b7059da5ULL, 0x439f377336ca6c2bULL, 0x985e520644386a6eULL,
  0x2f61a9166a6bba00ULL, 0x0f5a0f97ee76baa3ULL, 0x979b61c6bc83c5e6ULL,
  0x9d28865f7ab96faaULL, 0x19a7750c2b104e0bULL, 0x330b2550592e80a2ULL,
  0xb89d432933e6fe7aULL, 0x2e9f32cda0137f88ULL, 0xab1e62ddd23e77b3ULL,
  0x1f77b4d6f62173faULL, 0x863e1aa2afa79cd3ULL, 0xd6017169c4c1942bULL,
  0x74887a1a2c729917ULL, 0xec265d4f23e02c2cULL, 0xdc3b5f748f3f3423ULL,
  0xc7dfe87aa1e02a4fULL, 0x83271f06d4baa0c2ULL, 0xfa183ad6005748d5ULL,
  0xa3e04662bec0585dULL, 0x830008fc1e07652bULL, 0x8cb3dbba04dcc452ULL,
  0x713363e3d79875e6ULL, 0xe5966abb931f27b6ULL, 0x80ae7eeb92188abcULL,
  0x5c13ec0dc1482247ULL, 0xe952ef5f2cc9f24dULL, 0x71bc90a5f3825060ULL,
  0xe7604f555e5e82afULL, 0xd4a9236ba8cffa56ULL, 0xdd514705032174cbULL,
  0x8a64b12e846ef793ULL, 0x5198cdeae41e1be8ULL, 0x4f5310eb8b33cfaeULL,
  0x01fa045ad33767beULL, 0x8881e77d982d2055ULL, 0x8ba499404c6c5723ULL,
  0x784640b95020b8b2ULL, 0x8e6f5a8c5cd6764eULL, 0x8b312b0867b1afd8ULL,
  0xd9dde65224d5532cULL, 0x58b510d2deb29c3cULL, 0xd05544eb8c9b73cbULL,
  0x475952834bfc777eULL, 0xc6e654aecf5fc5bdULL, 0x3ea747e63e723652ULL,
  0xe9df2d3e971a2eacULL, 0x28ddc0cb1d495929ULL, 0xe777de2235a831f4ULL,
  0x50ddf18b70552027ULL, 0xa2667c384a5f33deULL, 0x622e462aa17a724bULL,
  0x344095be42d0ec71ULL, 0xe3599a0ebba82156ULL, 0xc14840b95798481cULL,
  0x839e0bf89ccd5de5ULL, 0x1f172ff5031a1872ULL, 0xa415048fccf7d037ULL,
  0x7cec12f82df05cb0ULL, 0x9e63d1dce3fc803fULL, 0x8e42d5823887d01fULL,
  0xdbb9c557c85d4ce4ULL, 0xcfab43b0e28f4809ULL, 0x9085f6c81f9686beULL,
  0x7ee0a3abae2ffa32ULL, 0x8c6c9cf13bcd0d91ULL, 0x3c488cfd30e710f8ULL,
  0x41a220d99bd26069ULL, 0x671ca7dc90214783ULL, 0xc7db19f80ac0f648ULL,
  0x93e5bc785c928cddULL, 0x0d56e6d1caf4d243ULL, 0x603506884c1adf09ULL,
  0x436a15a927e39c6bULL, 0x55893d31e306fe2eULL, 0xe232c843426f9bf1ULL,
  0x8d3d12107ffb10bcULL, 0x922535aaa27c8b18ULL, 0x5ae07caf50e9c305ULL,
  0x456d83016c62566cULL, 0x0724924193117adaULL, 0x80e5bb7cc2ef825bULL,
  0x041e9dcfc0cf0641ULL, 0xb12cbab8e2c31b6fULL, 0x5164888e4eb8c322ULL,
  0x26c1d591ed1718acULL, 0xe517a0508b1baa99ULL, 0x152968c3b8e51239ULL,
  0xacc0598f02fe29b9ULL, 0x9cd7c2f313151e98ULL, 0x3a436ac49347a424ULL,
  0xf5f527d089263edbULL, 0x11f5e17f5db2b3f7ULL, 0xfb00e6ba85f763b7ULL,
  0xce993c7af9906abaULL, 0xebe6864a4ba2c507ULL, 0x651837e53cbe37ddULL,
  0xd654b6ccdf25d5b6ULL, 0xd314a7e16adb8c92ULL, 0x3fbec4356c844b37ULL,
  0x4c8dfd27a864f74eULL, 0x644360ccf75880f6ULL, 0x8c15381536a041d6ULL,
  0x556ee714a2bf3d81ULL, 0x81d65409d71d01d9ULL, 0x45d16bdc453cd52aULL,
  0xcb145f3ab68d33fbULL, 0x4b30ed5e1c8ce9f6ULL, 0x0d6388a92f4abd30ULL,
  0xbe78c16f7d142d3dULL, 0xb60b48868c376587ULL, 0xbb2ef7561ab22b9bULL,
  0x1257a03e96d0e149ULL, 0x3ee914b5adf92cbcULL, 0xe676a93ce9b79585ULL,
  0xd4bd0a7c21e9344aULL, 0x2563151a557838c2ULL, 0x688374a72563843fULL,
  0x90ca643d85b2ad6cULL, 0x96480743b4c5ec86ULL, 0x483ee5cb6f7f34e4ULL,
  0xa21effb5bf1b9276ULL, 0xb05ec6bb3b6bea97ULL, 0xf54874350deb330dULL,
  0x50c772c0e17e7b8fULL, 0x74fa555684319d03ULL, 0xb29f81f7afe4df8bULL,
  0x0d179ce00887463bULL, 0x4ada3e8aff51a904ULL, 0x0c409efade365f3cULL,
  0x3e4b2e10cd122801ULL, 0x6b2243a103368ca3ULL, 0xcfa31fbbdcd1ab40ULL,
  0x9a6a5d1a7e0e5ba0ULL, 0x8f8efdf697ec92daULL, 0x9f28398e828e001eULL,
  0xcc31af216577f0b3ULL, 0xbd4ff49d5ada2d73ULL, 0xcb233ef73ff53972ULL,
  0x7a875767d2b9f9e2ULL, 0xa1967d88722eba15ULL, 0x96a01373d011aefcULL,
  0xd2db66519403b7c8ULL, 0x7bce0ff02cf4fa3cULL, 0xf2c7467fc40ab866ULL,
  0x0355fda03b2c5bbfULL, 0x04af0f72322b4662ULL, 0xe585e4009469c2beULL,
  0xbcef119df6a5d0f7ULL, 0xacb3411273397a2bULL, 0x382ad14453da2e6bULL,
  0xb0a0b08482cd5ab4ULL, 0xe9a4942b9b694132ULL, 0x30db634fd81de4d2ULL,
  0x427741041cddd89bULL, 0x1849aa7b3684b143ULL, 0xe93b7dbedd24ba37ULL,
  0x2abb194b1501ebf0ULL, 0xd0fa24fdbd0e9dffULL, 0x5807dcce871c004bULL,
  0x8b9fc3efe303752aULL, 0x5eb0ab8de36b908aULL, 0x76b8f19b7d83be69ULL,
  0xa455e16c04da7d05ULL, 0xb0f12e7ab6d1a31aULL, 0x1f785b7bc1f40170ULL,
  0x5ef07235bcce6282ULL, 0xabd58f78df9d7423ULL, 0x16c128b093d65cf3ULL,
  0x73deacedb38ba2e8ULL, 0x0ecb6e9898876990ULL, 0x6c9d6798e1f81b44ULL,
  0xe5ce240a4b44fd88ULL, 0x5fe280fba7b25814ULL, 0xb11be175c6ecea70ULL,
  0xbb9dd23ec2e25aa2ULL, 0xfb4c6a1665b34063ULL, 0xec12f95432839fd4ULL,
  0xea0d08884c358a58ULL, 0x1d73f94c42a355efULL, 0x74ad10d4f3dffa76ULL,
  0xfdd401216aad06daULL, 0xbc25f93de7b622d1ULL, 0xc255ccb5887d556dULL,
  0x80060ed75dc747aeULL, 0xe93b702c41092e5cULL, 0x4fb89581787f87edULL,
  0x5574b03993ae5836ULL, 0xfe8f2cc99334784bULL, 0x0c9ae00e059a009aULL,
  0x6e3a112325c4dcadULL, 0x11026925a76da665ULL, 0xe17eeb8afbeb6d01ULL,
  0x2627bdbf09246a31ULL, 0x33b87d5b6a4e3defULL, 0x84c6c0d6b7d48b9dULL,
  0xf2638b1a4cfa9445ULL, 0xa643ffbb228a5954ULL, 0x0a6cb7c5ed495945ULL,
  0xed428a36770b993bULL, 0xac9cb3d4c8ef14baULL, 0x7e50692853b41cdcULL,
  0x272d4ed1d692621bULL, 0x4c8381a8ff0f443eULL, 0xc2a9779c2d293de2ULL,
  0x2c622d1de53aee3eULL, 0x000e31bf55d591d0ULL, 0x2e3b2a1e8db96bcaULL,
  0xf09157f9592433daULL, 0xbda6003dd1c19936ULL, 0xa909670cda376744ULL,
  0x198cf3207f06944cULL, 0xfcd67fb0f99cbf03ULL, 0x5f631f44616502a3ULL,
  0xcce9b2c803934d8eULL, 0xa00ef5999ddb325fULL, 0x687d7e71ac01fd32ULL,
  0xcbe0ea6220ad87d3ULL, 0xfc098055fbfa6291ULL, 0x6b7de440c87ede5aULL,
  0xe48cf4e0ddfae66dULL, 0x395c5a5a5c03265dULL, 0xe2db49b5fbe0e6a0ULL,
  0x21b6055ecfd21589ULL, 0xc24333ffc476af48ULL, 0x952099055475e0d6ULL,
  0xd2951a8698be2105ULL, 0xfe7ded32e21dc03eULL, 0x1d3f405e0567c8b5ULL,
  0x03bfb29fb1f9f04aULL, 0xa545a1ba497949fcULL, 0x070bb7fd63739ceaULL,
  0x6812587192bfa044ULL, 0x0da87456d0f1c15aULL, 0xba0c969c4e8f14b4ULL,
  0xfa821aa6ec641383ULL, 0x07e4a3f4e3d8ce08ULL, 0x92f66f26a9c0289cULL,
  0x78b680045238f8c6ULL, 0xc30d47d4d5fa4b75ULL, 0x4afc8d4b26e4fb36ULL,
  0x6942c31ccc456b88ULL, 0x34be37ddeb2fd33cULL, 0x22b6b33e8e25631bULL,
  0x0862ece62fbdc7bcULL, 0x0e242c065106daa8ULL, 0xb878e8d3170f75e9ULL,
  0x456ec7add8ef6cd5ULL, 0xfd6b5da413587ffcULL, 0xc72ed7f012623ed0ULL,
  0xffcdacf9b1573ae9ULL, 0x0590ea296523d71aULL, 0x6a45638451ca45a8ULL,
  0x8cd8fad4be1bc5e7ULL, 0x4fa5c223ce9a528dULL, 0xa4c5aac14d7fe68dULL,
  0x73ce2a42f0a0ed5fULL, 0x74967ffc0460c902ULL, 0x3c36e00e8d2d2ef5ULL
};

// creates a Slice from Json and adds it to a scope
Slice Slice::fromJson(SliceScope& scope, std::string const& json,
                      Options const* options) {
  Parser parser(options);
  parser.parse(json);

  Builder const& b = parser.builder();  // don't copy Builder contents here
  return scope.add(b.start(), b.size());
}

// translates an integer key into a string
Slice Slice::translate() const {
  if (!isSmallInt() && !isUInt()) {
    throw Exception(Exception::InvalidValueType,
                    "Cannot translate key of this type");
  }
  if (Options::Defaults.attributeTranslator == nullptr) {
    throw Exception(Exception::NeedAttributeTranslator);
  }
  return translateUnchecked();
}

// return the value for a UInt object, without checks!
// returns 0 for invalid values/types
uint64_t Slice::getUIntUnchecked() const {
  uint8_t const h = head();
  if (h >= 0x28 && h <= 0x2f) {
    // UInt
    return readInteger<uint64_t>(_start + 1, h - 0x27);
  }

  if (h >= 0x30 && h <= 0x39) {
    // Smallint >= 0
    return static_cast<uint64_t>(h - 0x30);
  }
  return 0;
}

// translates an integer key into a string, without checks
Slice Slice::translateUnchecked() const {
  uint8_t const* result = Options::Defaults.attributeTranslator->translate(getUIntUnchecked());
  if (result != nullptr) {
    return Slice(result);
  }
  return Slice();
}

// check if two Slices are equal on the binary level
bool Slice::equals(Slice const& other) const {
  if (head() != other.head()) {
    return false;
  }

  ValueLength const size = byteSize();

  if (size != other.byteSize()) {
    return false;
  }

  return (memcmp(start(), other.start(),
                 arangodb::velocypack::checkOverflow(size)) == 0);
}

std::string Slice::toJson(Options const* options) const {
  std::string buffer;
  StringSink sink(&buffer);
  Dumper dumper(&sink, options);
  dumper.dump(this);
  return buffer;
}

std::string Slice::toString(Options const* options) const {
  // copy options and set prettyPrint in copy
  Options prettyOptions = *options;
  prettyOptions.prettyPrint = true;

  std::string buffer;
  StringSink sink(&buffer);
  Dumper::dump(this, &sink, &prettyOptions);
  return buffer;
}

std::string Slice::hexType() const { return HexDump::toHex(head()); }
  
uint64_t Slice::normalizedHash(uint64_t seed) const {
  // FIXME: CUCKOO
  uint64_t value;

  if (isNumber()) {
    // upcast integer values to double
    double v = getNumericValue<double>();
    value = fasthash64(&v, sizeof(v), seed);
  } else if (isArray()) {
    // normalize arrays by hashing array length and iterating
    // over all array members
    uint64_t const n = length() ^ 0xba5bedf00d;
    value = fasthash64(&n, sizeof(n), seed);
    for (auto const& it : ArrayIterator(*this)) {
      value ^= it.normalizedHash(value);
    }
  } else if (isObject()) {
    // normalize objects by hashing object length and iterating
    // over all object members
    uint64_t const n = length() ^ 0xf00ba44ba5;
    uint64_t seed2 = fasthash64(&n, sizeof(n), seed);
    value = seed2;
    for (auto const& it : ObjectIterator(*this)) {
      value ^= it.key.normalizedHash(seed2);
      value ^= it.value.normalizedHash(seed2);
    }
  } else {
    // fall back to regular hash function
    value = hash(seed);
  }

  return value;
}

// look for the specified attribute inside an Object
// returns a Slice(ValueType::None) if not found
Slice Slice::get(std::string const& attribute) const {
  if (!isObject()) {
    throw Exception(Exception::InvalidValueType, "Expecting Object");
  }

  auto const h = head();
  if (h == 0x0a) {
    // special case, empty object
    return Slice();
  }

  if (h == 0x14) {
    // compact Object
    return getFromCompactObject(attribute);
  }

  ValueLength const offsetSize = indexEntrySize(h);
  ValueLength end = readInteger<ValueLength>(_start + 1, offsetSize);
  ValueLength nrSlots;
  ValueLength htBase;
  uint8_t seed;
  if (offsetSize < 4) {
    nrSlots = readInteger<ValueLength>(_start + 1 + 2 * offsetSize, offsetSize);
    htBase = end - nrSlots * offsetSize;
    seed = _start[1 + 3 * offsetSize];
  } else {
    nrSlots = readInteger<ValueLength>(_start+end-1-offsetSize, offsetSize);
    htBase = end - nrSlots * offsetSize - 1 - (offsetSize == 4 ? 8 : 16);
    seed = _start[end-1];
  }
  bool small = nrSlots <= 0x1000000;
  Slice s;
  Slice k;
  char const* st;
  ValueLength stLength;
  ValueLength offset;
  ValueLength pos[3];
  fasthash64x3(attribute.c_str(), attribute.size(), seedTable + 3 * seed, pos);
  pos[0] = small ? fastModulo32Bit(pos[0], nrSlots) : pos[0] % nrSlots;
  offset = readInteger<ValueLength>(_start + htBase + pos[0] * offsetSize,
                                    offsetSize);
  if (offset != 0) {
    s = Slice(_start + offset);
    k = s.makeKey();
    st = k.getString(stLength);
    if (attribute.size() == stLength &&
        memcmp(attribute.c_str(), st, stLength) == 0) {
      return Slice(s._start + s.byteSize());
    }
  }
  pos[1] = small ? fastModulo32Bit(pos[1], nrSlots) : pos[1] % nrSlots;
  offset = readInteger<ValueLength>(_start + htBase + pos[1] * offsetSize,
                                    offsetSize);
  if (offset != 0) {
    s = Slice(_start + offset);
    k = s.makeKey();
    st = k.getString(stLength);
    if (attribute.size() == stLength &&
        memcmp(attribute.c_str(), st, stLength) == 0) {
      return Slice(s._start + s.byteSize());
    }
  }
  pos[2] = small ? fastModulo32Bit(pos[2], nrSlots) : pos[2] % nrSlots;
  offset = readInteger<ValueLength>(_start + htBase + pos[1] * offsetSize,
                                    offsetSize);
  if (offset != 0) {
    s = Slice(_start + offset);
    k = s.makeKey();
    st = k.getString(stLength);
    if (attribute.size() == stLength &&
        memcmp(attribute.c_str(), st, stLength) == 0) {
      return Slice(s._start + s.byteSize());
    }
  }
  return noneSlice();
}

// return the value for an Int object
int64_t Slice::getInt() const {
  uint8_t const h = head();
  if (h >= 0x20 && h <= 0x27) {
    // Int  T
    uint64_t v = readInteger<uint64_t>(_start + 1, h - 0x1f);
    if (h == 0x27) {
      return toInt64(v);
    } else {
      int64_t vv = static_cast<int64_t>(v);
      int64_t shift = 1LL << ((h - 0x1f) * 8 - 1);
      return vv < shift ? vv : vv - (shift << 1);
    }
  }

  if (h >= 0x28 && h <= 0x2f) {
    // UInt
    uint64_t v = getUInt();
    if (v > static_cast<uint64_t>(INT64_MAX)) {
      throw Exception(Exception::NumberOutOfRange);
    }
    return static_cast<int64_t>(v);
  }

  if (h >= 0x30 && h <= 0x3f) {
    // SmallInt
    return getSmallInt();
  }

  throw Exception(Exception::InvalidValueType, "Expecting type Int");
}

// return the value for a UInt object
uint64_t Slice::getUInt() const {
  uint8_t const h = head();
  if (h >= 0x28 && h <= 0x2f) {
    // UInt
    return readInteger<uint64_t>(_start + 1, h - 0x27);
  }

  if (h >= 0x20 && h <= 0x27) {
    // Int
    int64_t v = getInt();
    if (v < 0) {
      throw Exception(Exception::NumberOutOfRange);
    }
    return static_cast<int64_t>(v);
  }

  if (h >= 0x30 && h <= 0x39) {
    // Smallint >= 0
    return static_cast<uint64_t>(h - 0x30);
  }

  if (h >= 0x3a && h <= 0x3f) {
    // Smallint < 0
    throw Exception(Exception::NumberOutOfRange);
  }

  throw Exception(Exception::InvalidValueType, "Expecting type UInt");
}

// return the value for a SmallInt object
int64_t Slice::getSmallInt() const {
  uint8_t const h = head();

  if (h >= 0x30 && h <= 0x39) {
    // Smallint >= 0
    return static_cast<int64_t>(h - 0x30);
  }

  if (h >= 0x3a && h <= 0x3f) {
    // Smallint < 0
    return static_cast<int64_t>(h - 0x3a) - 6;
  }

  if ((h >= 0x20 && h <= 0x27) || (h >= 0x28 && h <= 0x2f)) {
    // Int and UInt
    // we'll leave it to the compiler to detect the two ranges above are
    // adjacent
    return getInt();
  }

  throw Exception(Exception::InvalidValueType, "Expecting type SmallInt");
}

int Slice::compareString(std::string const& attribute) const {
  ValueLength keyLength;
  char const* k = getString(keyLength);
  size_t const attributeLength = attribute.size();
  size_t const compareLength =
      (std::min)(static_cast<size_t>(keyLength), attributeLength);
  int res = memcmp(k, attribute.c_str(), compareLength);

  if (res == 0) {
    if (keyLength != attributeLength) {
      return (keyLength > attributeLength) ? 1 : -1;
    }
  }
  return res;
}

bool Slice::isEqualString(std::string const& attribute) const {
  ValueLength keyLength;
  char const* k = getString(keyLength);
  if (static_cast<size_t>(keyLength) != attribute.size()) {
    return false;
  }
  return (memcmp(k, attribute.c_str(), attribute.size()) == 0);
}

Slice Slice::getFromCompactObject(std::string const& attribute) const {
  ObjectIterator it(*this);
  while (it.valid()) {
    Slice key = it.key();
    if (key.makeKey().isEqualString(attribute)) {
      return Slice(key.start() + key.byteSize());
    }

    it.next();
  }
  // not found
  return Slice();
}

// get the offset for the nth member from an Array or Object type
ValueLength Slice::getNthOffset(ValueLength index) const {
  VELOCYPACK_ASSERT(isArray() || isObject());

  auto const h = head();

  if (h == 0x13 || h == 0x14) {
    // compact Array or Object
    return getNthOffsetFromCompact(index);
  }
  
  if (h == 0x01 || h == 0x0a) {
    // special case: empty Array or empty Object
    throw Exception(Exception::IndexOutOfBounds);
  }

  ValueLength const offsetSize = indexEntrySize(h);
  ValueLength end = readInteger<ValueLength>(_start + 1, offsetSize);

  ValueLength dataOffset = 0;

  // find the number of items or number of slots:
  ValueLength n;
  ValueLength ieBase = 0;   // base of index table
  if (h <= 0x05) {  // No offset table or length, need to compute:
    dataOffset = findDataOffset(h);
    Slice first(_start + dataOffset);
    n = (end - dataOffset) / first.byteSize();
  } else if (h <= 0x09) {   // array with offset table
    if (offsetSize < 8) {
      n = readInteger<ValueLength>(_start + 1 + offsetSize, offsetSize);
      ieBase = end - n * offsetSize;
    } else {
      n = readInteger<ValueLength>(_start + end - offsetSize, offsetSize);
      ieBase = end - n * offsetSize - 8;
    }
  } else {   // only remaining: types 0x0b-0x0e, cuckoo hashed objects
    if (offsetSize < 4) {
      n = readInteger<ValueLength>(_start + 1 + 2 * offsetSize, offsetSize);
      ieBase = end - n * offsetSize;
    } else {
      n = readInteger<ValueLength>(_start + end - 1 - offsetSize, offsetSize);
      ieBase = end - n * offsetSize - 1 - (offsetSize == 4 ? 8 : 16);
    }
    // Note that here, n is the number of slots in the hash table and
    // will be larger than the number of subitems!
  }

  if (index >= n) {
    throw Exception(Exception::IndexOutOfBounds);
  }

  // empty array case was already covered
  VELOCYPACK_ASSERT(n > 0);

  if (h <= 0x05 || n == 1) {
    // no index table, but all array items have the same length
    // now fetch first item and determine its length
    if (dataOffset == 0) {
      dataOffset = findDataOffset(h);
    }
    return dataOffset + index * Slice(_start + dataOffset).byteSize();
  }

  return readInteger<ValueLength>(_start + ieBase + index * offsetSize, 
                                  offsetSize);
}

// extract the nth member from an Array
Slice Slice::getNth(ValueLength index) const {
  VELOCYPACK_ASSERT(isArray());

  return Slice(_start + getNthOffset(index));
}

// extract the nth member from an Object
Slice Slice::getNthKey(ValueLength index, bool translate) const {
  VELOCYPACK_ASSERT(type() == ValueType::Object);

  Slice s(_start + getNthOffset(index));

  if (translate) {
    return s.makeKey();
  }

  return s;
}

Slice Slice::makeKey() const {
  if (isString()) {
    return *this;
  }
  if (isSmallInt() || isUInt()) {
    if (Options::Defaults.attributeTranslator == nullptr) {
      throw Exception(Exception::NeedAttributeTranslator);
    }
    return translateUnchecked();
  }

  throw Exception(Exception::InvalidValueType,
                  "Cannot translate key of this type");
}

// get the offset for the nth member from a compact Array or Object type
ValueLength Slice::getNthOffsetFromCompact(ValueLength index) const {
  ValueLength end = readVariableValueLength<false>(_start + 1);
  ValueLength n = readVariableValueLength<true>(_start + end - 1);
  if (index >= n) {
    throw Exception(Exception::IndexOutOfBounds);
  }

  auto const h = head();
  ValueLength offset = 1 + getVariableValueLength(end);
  ValueLength current = 0;
  while (current != index) {
    uint8_t const* s = _start + offset;
    offset += Slice(s).byteSize();
    if (h == 0x14) {
      offset += Slice(_start + offset).byteSize();
    }
    ++current;
  }
  return offset;
}

SliceScope::SliceScope() : _allocations() {}

SliceScope::~SliceScope() {
  for (auto& it : _allocations) {
    delete[] it;
  }
}

Slice SliceScope::add(uint8_t const* data, ValueLength size) {
  size_t const s = checkOverflow(size);
  std::unique_ptr<uint8_t[]> copy(new uint8_t[s]);
  memcpy(copy.get(), data, s);
  _allocations.push_back(copy.get());
  return Slice(copy.release());
}

std::ostream& operator<<(std::ostream& stream, Slice const* slice) {
  stream << "[Slice " << valueTypeName(slice->type()) << " ("
         << slice->hexType() << "), byteSize: " << slice->byteSize() << "]";
  return stream;
}

std::ostream& operator<<(std::ostream& stream, Slice const& slice) {
  return operator<<(stream, &slice);
}

static_assert(sizeof(arangodb::velocypack::Slice) ==
              sizeof(void*), "Slice has an unexpected size");
