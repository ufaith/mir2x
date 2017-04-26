/*
 * =====================================================================================
 *
 *       Filename: messagebase.hpp
 *        Created: 04/24/2017 00:46:48
 *  Last Modified: 04/24/2017 14:42:36
 *
 *    Description: length encoding for mode 1:
 *
 *                  [0 - 254]          : length in 0 ~ 254
 *                  [    255][0 ~ 255] : length as 0 ~ 255 + 255
 *              
 *                  1. most likely we are using 0 ~ 254
 *                  2. if compressed length more than 254 we need to bytes
 *                  3. we support range in [0, 255 + 255]
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
#include <cstdint>
#include <cstddef>
#include "messageattribute.hpp"

class MessageBase
{
    private:
        const uint8_t m_HC;

    protected:
        MessageBase(uint8_t nHC)
            : m_HC(nHC)
        {}

    public:
        int Type() const
        {
            return GetAttribute(m_HC).Type;
        }

        size_t DataLen() const
        {
            return GetAttribute(m_HC).DataLen;
        }

        size_t MaskLen() const
        {
            switch(Type()){
                case 1:
                    {
                        return (DataLen() + 7) / 8;
                    }
                case 0:
                case 2:
                case 3:
                default:
                    {
                        // for invalid type I also return 0
                        // should I return (size_t)(-1) or make return type as int?
                        return 0;
                    }
            }
        }

        const std::string &Name() const
        {
            return GetAttribute(m_HC).Name;
        }

    private:
        virtual const MessageAttribute &GetAttribute(uint8_t nHC) const = 0;
};
