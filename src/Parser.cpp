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

#include "velocypack/velocypack-common.h"
#include "velocypack/Parser.h"
#include "velocypack/Date.h"
#include "asm-functions.h"

#include <cstdlib>
#include <chrono>
#include <iostream>
#include <sstream>

using namespace arangodb::velocypack;

static uint8_t const Base64DecodeTable[128] = {
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
  255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,  62, 255,  62, 255,  63, 
  52,  53,  54,  55,  56,  57,  58,  59,  60,  61, 255, 255,   0, 255, 255, 255, 
  255,   0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25, 255, 255, 255, 255,  63,
  255,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40, 
  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,  51, 255, 255, 255, 255, 255
};

// The following function does the actual parse. It gets bytes
// via peek, consume and reset appends the result to the Builder
// in *_b. Errors are reported via an exception.
// Behind the scenes it runs two parses, one to collect sizes and
// check for parse errors (scan phase) and then one to actually
// build the result (build phase).

ValueLength Parser::parseInternal(bool multi) {
  // skip over optional BOM
  if (_size >= 3 && _start[0] == 0xef && _start[1] == 0xbb &&
      _start[2] == 0xbf) {
    // found UTF-8 BOM. simply skip over it
    _pos += 3;
  }

  ValueLength nr = 0;
  do {
    bool haveReported = false;
    if (!_b->_stack.empty()) {
      ValueLength const tos = _b->_stack.back();
      if (_b->_start[tos] == 0x0b || _b->_start[tos] == 0x14) {
        if (! _b->_keyWritten) {
          throw Exception(Exception::BuilderKeyMustBeString);
        }
        else {
          _b->_keyWritten = false;
        }
      }
      else {
        _b->reportAdd();
        haveReported = true;
      }
    }
    try {
      parseJson();
    }
    catch (...) {
      if (haveReported) {
        _b->cleanupAdd();
      }
      throw;
    }
    nr++;
    while (_pos < _size && isWhiteSpace(_start[_pos])) {
      ++_pos;
    }
    if (!multi && _pos != _size) {
      consume();  // to get error reporting right
      throw Exception(Exception::ParseError, "Expecting EOF");
    }
  } while (multi && _pos < _size);
  return nr;
}

// skips over all following whitespace tokens but does not consume the
// byte following the whitespace
int Parser::skipWhiteSpace(char const* err) {
  if (_pos >= _size) {
    throw Exception(Exception::ParseError, err);
  }
  uint8_t c = _start[_pos];
  if (!isWhiteSpace(c)) {
    return c;
  }
  if (c == ' ') {
    if (_pos + 1 >= _size) {
      _pos++;
      throw Exception(Exception::ParseError, err);
    }
    c = _start[_pos + 1];
    if (!isWhiteSpace(c)) {
      _pos++;
      return c;
    }
  }
  size_t remaining = _size - _pos;
  if (remaining >= 16) {
    size_t count = JSONSkipWhiteSpace(_start + _pos, remaining - 15);
    _pos += count;
  }
  do {
    if (!isWhiteSpace(_start[_pos])) {
      return static_cast<int>(_start[_pos]);
    }
    _pos++;
  } while (_pos < _size);
  throw Exception(Exception::ParseError, err);
}

// parses a number value
void Parser::parseNumber() {
  size_t startPos = _pos;
  ParsedNumber numberValue;
  bool negative = false;
  int i = consume();
  // We know that a character is coming, and it's a number if it
  // starts with '-' or a digit. otherwise it's invalid
  if (i == '-') {
    i = getOneOrThrow("Incomplete number");
    negative = true;
  }
  if (i < '0' || i > '9') {
    throw Exception(Exception::ParseError, "Expecting digit");
  }

  if (i != '0') {
    unconsume();
    scanDigits(numberValue);
  }
  i = consume();
  if (i < 0 || (i != '.' && i != 'e' && i != 'E')) {
    if (i >= 0) {
      unconsume();
    }
    if (!numberValue.isInteger) {
      if (negative) {
        _b->addDouble(-numberValue.doubleValue);
      } else {
        _b->addDouble(numberValue.doubleValue);
      }
    } else if (negative) {
      if (numberValue.intValue <= static_cast<uint64_t>(INT64_MAX)) {
        _b->addInt(-static_cast<int64_t>(numberValue.intValue));
      } else if (numberValue.intValue == toUInt64(INT64_MIN)) {
        _b->addInt(INT64_MIN);
      } else {
        _b->addDouble(-static_cast<double>(numberValue.intValue));
      }
    } else {
      _b->addUInt(numberValue.intValue);
    }
    return;
  }

  double fractionalPart;
  if (i == '.') {
    // fraction. skip over '.'
    i = getOneOrThrow("Incomplete number");
    if (i < '0' || i > '9') {
      throw Exception(Exception::ParseError, "Incomplete number");
    }
    unconsume();
    fractionalPart = scanDigitsFractional();
    if (negative) {
      fractionalPart = -numberValue.asDouble() - fractionalPart;
    } else {
      fractionalPart = numberValue.asDouble() + fractionalPart;
    }
    i = consume();
    if (i < 0) {
      _b->addDouble(fractionalPart);
      return;
    }
  } else {
    if (negative) {
      fractionalPart = -numberValue.asDouble();
    } else {
      fractionalPart = numberValue.asDouble();
    }
  }
  if (i != 'e' && i != 'E') {
    unconsume();
    // use conventional atof() conversion here, to avoid precision loss
    // when interpreting and multiplying the single digits of the input stream
    // _b->addDouble(fractionalPart);
    _b->addDouble(atof(reinterpret_cast<char const*>(_start) + startPos));
    return;
  }
  i = getOneOrThrow("Incomplete number");
  negative = false;
  if (i == '+' || i == '-') {
    negative = (i == '-');
    i = getOneOrThrow("Incomplete number");
  }
  if (i < '0' || i > '9') {
    throw Exception(Exception::ParseError, "Incomplete number");
  }
  unconsume();
  ParsedNumber exponent;
  scanDigits(exponent);
  if (negative) {
    fractionalPart *= pow(10, -exponent.asDouble());
  } else {
    fractionalPart *= pow(10, exponent.asDouble());
  }
  if (std::isnan(fractionalPart) || !std::isfinite(fractionalPart)) {
    throw Exception(Exception::NumberOutOfRange);
  }
  // use conventional atof() conversion here, to avoid precision loss
  // when interpreting and multiplying the single digits of the input stream
  // _b->addDouble(fractionalPart);
  _b->addDouble(atof(reinterpret_cast<char const*>(_start) + startPos));
}

void Parser::parseString() {
  // When we get here, we have seen a " character and now want to
  // find the end of the string and parse the string value to its
  // VPack representation. We assume that the string is short and
  // insert 8 bytes for the length as soon as we reach 127 bytes
  // in the VPack representation.

  ValueLength const base = _b->_pos;
  _b->reserveSpace(1);
  _b->_start[_b->_pos++] = 0x40;  // correct this later

  bool large = false;          // set to true when we reach 128 bytes
  uint32_t highSurrogate = 0;  // non-zero if high-surrogate was seen

  while (true) {
    size_t remainder = _size - _pos;
    if (remainder >= 16) {
      _b->reserveSpace(remainder);
      size_t count;
      // Note that the SSE4.2 accelerated string copying functions might
      // peek up to 15 bytes over the given end, because they use 128bit
      // registers. Therefore, we have to subtract 15 from remainder
      // to be on the safe side. Further bytes will be processed below.
      if (options->validateUtf8Strings) {
        count = JSONStringCopyCheckUtf8(_b->_start + _b->_pos, _start + _pos,
                                        remainder - 15);
      } else {
        count = JSONStringCopy(_b->_start + _b->_pos, _start + _pos,
                               remainder - 15);
      }
      _pos += count;
      _b->_pos += count;
    }
    int i = getOneOrThrow("Unfinished string");
    if (!large && _b->_pos - (base + 1) > 126) {
      large = true;
      _b->reserveSpace(8);
      ValueLength len = _b->_pos - (base + 1);
      memmove(_b->_start + base + 9, _b->_start + base + 1, checkOverflow(len));
      _b->_pos += 8;
    }
    switch (i) {
      case '"':
        ValueLength len;
        if (!large) {
          len = _b->_pos - (base + 1);
          _b->_start[base] = 0x40 + static_cast<uint8_t>(len);
          // String is ready
        } else {
          len = _b->_pos - (base + 9);
          _b->_start[base] = 0xbf;
          for (ValueLength i = 1; i <= 8; i++) {
            _b->_start[base + i] = len & 0xff;
            len >>= 8;
          }
        }
        return;
      case '\\':
        // Handle cases or throw error
        i = consume();
        if (i < 0) {
          throw Exception(Exception::ParseError, "Invalid escape sequence");
        }
        switch (i) {
          case '"':
          case '/':
          case '\\':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = static_cast<uint8_t>(i);
            highSurrogate = 0;
            break;
          case 'b':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = '\b';
            highSurrogate = 0;
            break;
          case 'f':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = '\f';
            highSurrogate = 0;
            break;
          case 'n':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = '\n';
            highSurrogate = 0;
            break;
          case 'r':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = '\r';
            highSurrogate = 0;
            break;
          case 't':
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = '\t';
            highSurrogate = 0;
            break;
          case 'u': {
            uint32_t v = 0;
            for (int j = 0; j < 4; j++) {
              i = consume();
              if (i < 0) {
                throw Exception(Exception::ParseError,
                                "Unfinished \\uXXXX escape sequence");
              }
              if (i >= '0' && i <= '9') {
                v = (v << 4) + i - '0';
              } else if (i >= 'a' && i <= 'f') {
                v = (v << 4) + i - 'a' + 10;
              } else if (i >= 'A' && i <= 'F') {
                v = (v << 4) + i - 'A' + 10;
              } else {
                throw Exception(Exception::ParseError,
                                "Illegal \\uXXXX escape sequence");
              }
            }
            if (v < 0x80) {
              _b->reserveSpace(1);
              _b->_start[_b->_pos++] = static_cast<uint8_t>(v);
              highSurrogate = 0;
            } else if (v < 0x800) {
              _b->reserveSpace(2);
              _b->_start[_b->_pos++] = 0xc0 + (v >> 6);
              _b->_start[_b->_pos++] = 0x80 + (v & 0x3f);
              highSurrogate = 0;
            } else if (v >= 0xdc00 && v < 0xe000 && highSurrogate != 0) {
              // Low surrogate, put the two together:
              v = 0x10000 + ((highSurrogate - 0xd800) << 10) + v - 0xdc00;
              _b->_pos -= 3;
              _b->reserveSpace(4);
              _b->_start[_b->_pos++] = 0xf0 + (v >> 18);
              _b->_start[_b->_pos++] = 0x80 + ((v >> 12) & 0x3f);
              _b->_start[_b->_pos++] = 0x80 + ((v >> 6) & 0x3f);
              _b->_start[_b->_pos++] = 0x80 + (v & 0x3f);
              highSurrogate = 0;
            } else {
              if (v >= 0xd800 && v < 0xdc00) {
                // High surrogate:
                highSurrogate = v;
              } else {
                highSurrogate = 0;
              }
              _b->reserveSpace(3);
              _b->_start[_b->_pos++] = 0xe0 + (v >> 12);
              _b->_start[_b->_pos++] = 0x80 + ((v >> 6) & 0x3f);
              _b->_start[_b->_pos++] = 0x80 + (v & 0x3f);
            }
            break;
          }
          default:
            throw Exception(Exception::ParseError, "Invalid escape sequence");
        }
        break;
      default:
        if ((i & 0x80) == 0) {
          // non-UTF-8 sequence
          if (i < 0x20) {
            // control character
            throw Exception(Exception::UnexpectedControlCharacter);
          }
          highSurrogate = 0;
          _b->reserveSpace(1);
          _b->_start[_b->_pos++] = static_cast<uint8_t>(i);
        } else {
          if (!options->validateUtf8Strings) {
            highSurrogate = 0;
            _b->reserveSpace(1);
            _b->_start[_b->_pos++] = static_cast<uint8_t>(i);
          } else {
            // multi-byte UTF-8 sequence!
            int follow = 0;
            if ((i & 0xe0) == 0x80) {
              throw Exception(Exception::InvalidUtf8Sequence);
            } else if ((i & 0xe0) == 0xc0) {
              // two-byte sequence
              follow = 1;
            } else if ((i & 0xf0) == 0xe0) {
              // three-byte sequence
              follow = 2;
            } else if ((i & 0xf8) == 0xf0) {
              // four-byte sequence
              follow = 3;
            } else {
              throw Exception(Exception::InvalidUtf8Sequence);
            }

            // validate follow up characters
            _b->reserveSpace(1 + follow);
            _b->_start[_b->_pos++] = static_cast<uint8_t>(i);
            for (int j = 0; j < follow; ++j) {
              i = getOneOrThrow("scanString: truncated UTF-8 sequence");
              if ((i & 0xc0) != 0x80) {
                throw Exception(Exception::InvalidUtf8Sequence);
              }
              _b->_start[_b->_pos++] = static_cast<uint8_t>(i);
            }
            highSurrogate = 0;
          }
        }
        break;
    }
  }
}

void Parser::parseArray() {
  _b->addArray();

  int i = skipWhiteSpace("Expecting item or ']'");
  if (i == ']') {
    // empty array
    ++_pos;  // the closing ']'
    _b->close();
    return;
  }

  increaseNesting();

  while (true) {
    // parse array element itself
    _b->reportAdd();
    parseJson();
    i = skipWhiteSpace("Expecting ',' or ']'");
    if (i == ']') {
      // end of array
      ++_pos;  // the closing ']'
      _b->close();
      decreaseNesting();
      return;
    }
    // skip over ','
    if (i != ',') {
      throw Exception(Exception::ParseError, "Expecting ',' or ']'");
    }
    ++_pos;  // the ','
  }

  // should never get here
  VELOCYPACK_ASSERT(false);
}

void Parser::parseObject() {
  _b->addObject();

  int i = skipWhiteSpace("Expecting item or '}'");
  if (i == '}') {
    // empty object
    consume();  // the closing ']'
    if (_nesting != 0 || !options->keepTopLevelOpen) {
      // only close if we've not been asked to keep top level open
      _b->close();
    }
    return;
  }

  increaseNesting();

  while (true) {
    // always expecting a string attribute name here
    if (i != '"') {
      throw Exception(Exception::ParseError, "Expecting '\"' or '}'");
    }
    // get past the initial '"'
    ++_pos;

    _b->reportAdd();
    bool excludeAttribute = false;
    auto const lastPos = _b->_pos;
    // always call parseString() method of JsonParser!
    Parser::parseString();
    if (options->attributeExcludeHandler != nullptr &&
        options->attributeExcludeHandler->shouldExclude(
              Slice(_b->_start + lastPos), _nesting)) {
      excludeAttribute = true;
    }

    if (!excludeAttribute && options->attributeTranslator != nullptr) {
      // check if a translation for the attribute name exists
      Slice key(_b->_start + lastPos);

      if (key.isString()) {
        ValueLength keyLength;
        char const* p = key.getString(keyLength);
        uint8_t const* translated =
            options->attributeTranslator->translate(p, keyLength);

        if (translated != nullptr) {
          // found translation... now reset position to old key position
          // and simply overwrite the existing key with the numeric translation
          // id
          _b->_pos = lastPos;
          _b->addUInt(Slice(translated).getUInt());
        }
      }
    }

    i = skipWhiteSpace("Expecting ':'");
    // always expecting the ':' here
    if (i != ':') {
      throw Exception(Exception::ParseError, "Expecting ':'");
    }
    ++_pos;  // skip over the colon

    parseJson();

    if (excludeAttribute) {
      _b->removeLast();
    }

    i = skipWhiteSpace("Expecting ',' or '}'");
    if (i == '}') {
      // end of object
      ++_pos;  // the closing '}'
      if (_nesting != 1 || !options->keepTopLevelOpen) {
        // only close if we've not been asked to keep top level open
        _b->close();
      }
      decreaseNesting();
      return;
    }
    if (i != ',') {
      throw Exception(Exception::ParseError, "Expecting ',' or '}'");
    }
    // skip over ','
    ++_pos;  // the ','
    i = skipWhiteSpace("Expecting '\"' or '}'");
  }

  // should never get here
  VELOCYPACK_ASSERT(false);
}

void Parser::parseJson() {
  skipWhiteSpace("Expecting item");

  int i = consume();
  if (i < 0) {
    return;
  }
  switch (i) {
    case '{':
      parseObject();  // this consumes the closing '}' or throws
      break;
    case '[':
      parseArray();  // this consumes the closing ']' or throws
      break;
    case 't':
      parseTrue();  // this consumes "rue" or throws
      break;
    case 'f':
      parseFalse();  // this consumes "alse" or throws
      break;
    case 'n':
      parseNull();  // this consumes "ull" or throws
      break;
    case '"':
      parseString(); // virtual method call!
      break;
    default: {
      // everything else must be a number or is invalid...
      // this includes '-' and '0' to '9'. scanNumber() will
      // throw if the input is non-numeric
      unconsume();
      parseNumber();  // this consumes the number or throws
      break;
    }
  }
}

// specialized parsing routine for VJson strings
void VJsonParser::parseString() {
  // When we get here, we have seen a " character and now want to
  // find the end of the string and parse the string value to its
  // VPack representation. We assume that the string is short and
  // insert 8 bytes for the length as soon as we reach 127 bytes
  // in the VPack representation.

  if (_size - _pos < 2) {
    throw Exception(Exception::ParseError, "Invalid VJSON string value");
  }

  // type byte comes first
  uint8_t const* p = _start + _pos;
  // the character following the type byte must be a ':'
  uint8_t const type = *p++;
  if (*p != ':') {
    throw Exception(Exception::ParseError, "Invalid VJSON string value. Expecting ':'");
  }
  
  switch (type) {
    case 's': {
      _pos += 2; // skip over type description and pretend this is a regular string
      Parser::parseString();
      break;
    }
    case 'b': {
      _pos += 2; // skip over type description and pretend this is a regular string
      parseBase64();
      consume(); // we already validated before that the next character is a "
      break;
    }
    case 'd': {
      _pos += 2; // skip over type description and pretend this is a regular string
      parseUTCDate();
      consume(); // we already validated before that the next character is a "
      break;
    }
    default: {
      throw Exception(Exception::ParseError, "Invalid VJSON data type");
    }
  }
}

void VJsonParser::parseBase64() {
  uint8_t const* end = static_cast<uint8_t const*>(memchr(_start + _pos, '"', _size - _pos));

  if (end == nullptr) {
    throw Exception(Exception::ParseError, "Invalid VJSON base64 value");
  }
  
  size_t const length = end - _start - _pos;
  size_t remainder = length;

  // skip trailing = chars
  while (remainder > 0 && _start[_pos + remainder - 1] == '=') {
    --remainder;
  }

  size_t const decodedLength = 3 * remainder / 4;

  _b->reserveSpace(9 + decodedLength); // reserve enough space for header and payload
  _b->appendUInt(decodedLength, 0xbf);
  
  if (length == 0) {
    // special case for an empty binary
    return;
  }

  uint8_t const* p = _start + _pos;

  do {
    VELOCYPACK_ASSERT(remainder > 0);

    uint8_t const b0 = (                 p[0] <= 'z') ? Base64DecodeTable[p[0]] : 0xff;
    uint8_t const b1 = (remainder > 1 && p[1] <= 'z') ? Base64DecodeTable[p[1]] : 0xff;
    uint8_t const b2 = (remainder > 2 && p[2] <= 'z') ? Base64DecodeTable[p[2]] : 0xff;
    uint8_t const b3 = (remainder > 3 && p[3] <= 'z') ? Base64DecodeTable[p[3]] : 0xff;
   
    if (b1 != 0xff) {
      _b->_start[_b->_pos++] = ((b0 & 0x3f) << 2) + ((b1 & 0x30) >> 4);
      if (b2 != 0xff) {
        _b->_start[_b->_pos++] = ((b1 & 0x0f) << 4) + ((b2 & 0x3c) >> 2);
        if (b3 != 0xff) {
          _b->_start[_b->_pos++] = ((b2 & 0x03) << 6) + ((b3 & 0x3f) >> 0);
        } else if (remainder > 3) {
          throw Exception(Exception::ParseError, "Invalid VJSON base64 value");
        }
      } else if (remainder > 2) {
        throw Exception(Exception::ParseError, "Invalid VJSON base64 value");
      }
    } else {
      // always an error
      throw Exception(Exception::ParseError, "Invalid VJSON base64 value");
    }

    if (remainder <= 4) {
      // reached the end
      break;
    }

    remainder -= 4;
    p += 4;    
  } while (true);
    
  // finally adjust read position
  _pos += length;
}

void VJsonParser::parseUTCDate() {
  uint8_t const* end = static_cast<uint8_t const*>(memchr(_start + _pos, '"', _size - _pos));

  if (end == nullptr) {
    throw Exception(Exception::ParseError, "Invalid VJSON datetime value");
  }

  // create string from date
  size_t const length = end - _start - _pos;
  char const* p = reinterpret_cast<char const*>(_start + _pos);
  std::string save(p, length);
  std::istringstream in{save};
  date::sys_time<std::chrono::milliseconds> tp;
  // parse date value
  date::parse(in, "%FT%TZ", tp);
  if (in.fail()) {
    in.clear();
    in.exceptions(std::ios::failbit);
    in.str(save);
    date::parse(in, "%FT%T%Ez", tp);
  }
  
  _b->addUTCDate(tp.time_since_epoch().count());
  // finally adjust read position
  _pos += length;
}

