/*
 * =====================================================================================
 *
 *       Filename: hexstring.hpp
 *        Created: 02/06/2016 13:35:51
 *  Last Modified: 04/04/2016 00:05:21
 *
 *    Description: 
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */


#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <type_traits>


// 1. invocation should prepared enough buffer to szString
// 2. no '\0' at the end, be careful
template<typename T, size_t ByteN = 0>
const char *HexString(T nKey, char *szString)
{
    static_assert(std::is_unsigned<T>::value, "unsigned intergal please");
    const size_t nByteN = (ByteN) ? std::min(ByteN, sizeof(T)) : sizeof(T);
    const uint16_t knvHexStringChunk[] = {
        12336, 12592, 12848, 13104, 13360, 13616, 13872, 14128,
        14384, 14640, 16688, 16944, 17200, 17456, 17712, 17968,
        12337, 12593, 12849, 13105, 13361, 13617, 13873, 14129,
        14385, 14641, 16689, 16945, 17201, 17457, 17713, 17969,
        12338, 12594, 12850, 13106, 13362, 13618, 13874, 14130,
        14386, 14642, 16690, 16946, 17202, 17458, 17714, 17970,
        12339, 12595, 12851, 13107, 13363, 13619, 13875, 14131,
        14387, 14643, 16691, 16947, 17203, 17459, 17715, 17971,
        12340, 12596, 12852, 13108, 13364, 13620, 13876, 14132,
        14388, 14644, 16692, 16948, 17204, 17460, 17716, 17972,
        12341, 12597, 12853, 13109, 13365, 13621, 13877, 14133,
        14389, 14645, 16693, 16949, 17205, 17461, 17717, 17973,
        12342, 12598, 12854, 13110, 13366, 13622, 13878, 14134,
        14390, 14646, 16694, 16950, 17206, 17462, 17718, 17974,
        12343, 12599, 12855, 13111, 13367, 13623, 13879, 14135,
        14391, 14647, 16695, 16951, 17207, 17463, 17719, 17975,
        12344, 12600, 12856, 13112, 13368, 13624, 13880, 14136,
        14392, 14648, 16696, 16952, 17208, 17464, 17720, 17976,
        12345, 12601, 12857, 13113, 13369, 13625, 13881, 14137,
        14393, 14649, 16697, 16953, 17209, 17465, 17721, 17977,
        12353, 12609, 12865, 13121, 13377, 13633, 13889, 14145,
        14401, 14657, 16705, 16961, 17217, 17473, 17729, 17985,
        12354, 12610, 12866, 13122, 13378, 13634, 13890, 14146,
        14402, 14658, 16706, 16962, 17218, 17474, 17730, 17986,
        12355, 12611, 12867, 13123, 13379, 13635, 13891, 14147,
        14403, 14659, 16707, 16963, 17219, 17475, 17731, 17987,
        12356, 12612, 12868, 13124, 13380, 13636, 13892, 14148,
        14404, 14660, 16708, 16964, 17220, 17476, 17732, 17988,
        12357, 12613, 12869, 13125, 13381, 13637, 13893, 14149,
        14405, 14661, 16709, 16965, 17221, 17477, 17733, 17989,
        12358, 12614, 12870, 13126, 13382, 13638, 13894, 14150,
        14406, 14662, 16710, 16966, 17222, 17478, 17734, 17990};

    for(size_t nIndex = 0; nIndex < nByteN; ++nIndex, (nKey >>= 8)){
        *(uint16_t *)(szString + 2 * (nByteN - nIndex - 1)) = knvHexStringChunk[(nKey & 0XFF)];
    }

    return szString;
}

template<typename T, size_t ByteN = 0> T StringHex(const char *szString)
{
    static_assert(std::is_unsigned<T>::value, "unsigned intergal please");
    const size_t nByteN = (ByteN) ? std::min(ByteN, sizeof(T)) : sizeof(T);
    const uint8_t knvStringHexChunk[] = {
        0X00,  // "0" - "0"
        0X01,  // "1" - "0"
        0X02,  // "2" - "0"
        0X03,  // "3" - "0"
        0X04,  // "4" - "0"
        0X05,  // "5" - "0"
        0X06,  // "6" - "0"
        0X07,  // "7" - "0"
        0X08,  // "8" - "0"
        0X09,  // "9" - "0"
        0XFF,  // ":" - "0" invalid
        0XFF,  // ";" - "0" invalid
        0XFF,  // "<" - "0" invalid
        0XFF,  // "=" - "0" invalid
        0XFF,  // ">" - "0" invalid
        0XFF,  // "?" - "0" invalid
        0XFF,  // "@" - "0" invalid
        0X0A,  // "A" - "0"
        0X0B,  // "B" - "0"
        0X0C,  // "C" - "0"
        0X0D,  // "D" - "0"
        0X0E,  // "E" - "0"
        0X0F,  // "F" - "0"
    };

    T nRes = 0;
    for(size_t nIndex = 0; szString[nIndex] != '\0' && nIndex < nByteN * 2; ++nIndex){
        nRes = (nRes << 4) + knvStringHexChunk[szString[nIndex] - '0'];
    }
    return nRes;
}
