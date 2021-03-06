/*
 * =====================================================================================
 *
 *       Filename: zumamonster.hpp
 *        Created: 04/08/2017 16:30:48
 *  Last Modified: 04/09/2017 00:48:02
 *
 *    Description: zuma monster which can be petrified
 *                 can't apply any action when petrified, also can't attack them
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

#include "monster.hpp"
class ZumaMonster: public Monster
{
    protected:
        ZumaMonster(uint32_t nUID, uint32_t nMonsterID, ProcessRun *pRun)
            : Monster(nUID, nMonsterID, pRun)
        {}

    public:
       ~ZumaMonster() = default;

    public:
        static ZumaMonster *Create(uint32_t, uint32_t, ProcessRun *, const ActionNode &);
};
