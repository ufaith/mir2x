/*
 * =====================================================================================
 *
 *       Filename: myhero.hpp
 *        Created: 04/07/2016 03:48:41 AM
 *  Last Modified: 04/07/2017 13:10:40
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
#include "hero.hpp"

class MyHero: public Hero
{
    public:
        MyHero(uint32_t, uint32_t, bool, uint32_t, ProcessRun *);
       ~MyHero() = default;

    public:
        bool Update();
};
