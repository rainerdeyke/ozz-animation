//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#ifndef OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_
#define OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_

#include "ozz/base/platform.h"
#ifndef OZZ_INCLUDE_PRIVATE_HEADER
#error "This header is private, it cannot be included from public headers."
#endif  // OZZ_INCLUDE_PRIVATE_HEADER

namespace ozz {
namespace animation {

// Offset to previous keyframes are stored on uint16_t.
enum Constants { kMaxPreviousOffset = (1 << 16) - 1 };

// Define animation key frame types (translation, rotation, scale). Every type
// as the same base made of the key time ratio and it's track index. This is
// required as key frames are not sorted per track, but sorted by ratio to favor
// cache coherency. Key frame values are compressed, according on their type.
// Decompression is efficient because it's done on SoA data and cached during
// sampling.

// Defines the float3 key frame type, used for translations and scales.
// Translation values are stored as half precision floats with 16 bits per
// component.
struct Float3Key {
  uint16_t value[3];
};

// Defines the rotation key frame type.
// Rotation value is a quaternion. Quaternion are normalized, which means each
// component is in range [0:1]. This property allows to quantize the 3
// components to 3 signed integer 16 bits values. The 4th component is restored
// at runtime, using the knowledge that |w| = sqrt(1 - (a^2 + b^2 + c^2)).
// The sign of this 4th component is stored using 1 bit taken from the track
// member.
//
// In more details, compression algorithm stores the 3 smallest components of
// the quaternion and restores the largest. The 3 smallest can be pre-multiplied
// by sqrt(2) to gain some precision indeed.
//
struct QuaternionKey {
  // 2b for the largest component index of the quaternion.
  // 1b for the sign of the largest component. 1 for negative.
  // 12b for each component
  // 1b wasted
  uint8_t value[5];
};

inline void pack(int _largest, int _sign, uint64_t _a, uint64_t _b, uint64_t _c,
                 QuaternionKey* _key) {
  const uint64_t packed = (_largest & 0x3) | ((_sign & 0x1) << 2) |
                          (_a & 0xfff) << 3 | (_b & 0xfff) << 15 |
                          (_c & 0xfff) << 27;
  _key->value[0] = packed & 0xff;
  _key->value[1] = (packed >> 8) & 0xff;
  _key->value[2] = (packed >> 16) & 0xff;
  _key->value[3] = (packed >> 24) & 0xff;
  _key->value[4] = (packed >> 32) & 0xff;
}

inline void unpack(const QuaternionKey& _key, int& _biggest, int& _sign,
                   int _components[3]) {
  const uint64_t packed =
      uint64_t(_key.value[0]) | uint64_t(_key.value[1]) << 8 |
      uint64_t(_key.value[2]) << 16 | uint64_t(_key.value[3]) << 24 |
      uint64_t(_key.value[4]) << 32;
  _biggest = packed & 3;
  _sign = (packed >> 2) & 1;
  _components[0] = (packed >> 3) & 0xfff;
  _components[1] = (packed >> 15) & 0xfff;
  _components[2] = (packed >> 27) & 0xfff;
}

}  // namespace animation
}  // namespace ozz
#endif  // OZZ_ANIMATION_RUNTIME_ANIMATION_KEYFRAME_H_
