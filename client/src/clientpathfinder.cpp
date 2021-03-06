/*
 * =====================================================================================
 *
 *       Filename: clientpathfinder.cpp
 *        Created: 03/28/2017 21:15:25
 *  Last Modified: 05/18/2017 17:21:14
 *
 *    Description: For logic check servermap.cpp
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

#include "log.hpp"
#include "game.hpp"
#include "processrun.hpp"
#include "clientpathfinder.hpp"

ClientPathFinder::ClientPathFinder(bool bCheckGround, bool bCheckCreature)
    : AStarPathFinder(
            [bCheckGround](int nSrcX, int nSrcY, int nDstX, int nDstY) -> bool {
                // different to server path find class
                // for client we always return true for CanMove() part
                // but assign a very high value if actually it goes to an invalid grid
                // then stop heros if they're trying to cross the invalid grids
                //
                // this helps if player clicks on invalid grid
                // we should still give an path to make it move to the right direction
                //
                // this will spend much more time
                // but ok for client part
                if(!bCheckGround){ return true; }

                // OK we need to check ground
                // rest logic is same with server path find class
                switch(LDistance2(nSrcX, nSrcY, nDstX, nDstY)){
                    case 0:
                        {
                            extern Log *g_Log;
                            g_Log->AddLog(LOGTYPE_FATAL, "Invalid argument: (%d, %d, %d, %d)", nSrcX, nSrcY, nDstX, nDstY);
                            return false;
                        }
                    case 1:
                    case 2:
                        {
                            extern Game *g_Game;
                            if(auto pRun = g_Game->ProcessValid(PROCESSID_RUN)){
                                return ((ProcessRun *)(pRun))->CanMove(false, nSrcX, nSrcY, nDstX, nDstY);
                            }
                            extern Log *g_Log;
                            g_Log->AddLog(LOGTYPE_FATAL, "Current process is not ProcessRun");
                            return false;
                        }
                    default:
                        {
                            return false;
                        }
                }
            },

            // no matter we check ground or not
            // we assign very high value to invalid grids
            [bCheckCreature](int nSrcX, int nSrcY, int nDstX, int nDstY) -> double {
                switch(LDistance2(nSrcX, nSrcY, nDstX, nDstY)){
                    case 0:
                        {
                            extern Log *g_Log;
                            g_Log->AddLog(LOGTYPE_FATAL, "Invalid argument: (%d, %d, %d, %d)", nSrcX, nSrcY, nDstX, nDstY);
                            return 10000.0;
                        }
                    case 1:
                        {
                            extern Game *g_Game;
                            if(auto pRun = (ProcessRun *)(g_Game->ProcessValid(PROCESSID_RUN))){
                                if(pRun->CanMove(false, nDstX, nDstY)){
                                    if(bCheckCreature){
                                        // if there is no co on the way we take it
                                        // however if there is, we can still take it but with very high cost
                                        return pRun->CanMove(true, nDstX, nDstY) ? 1.0 : 100.0;
                                    }else{
                                        // won't check creature
                                        // then all walk-able step get cost 1.0
                                        return 1.0;
                                    }
                                }else{
                                    // can't go through, return the infinite
                                    return 10000.0;
                                }
                            }else{
                                extern Log *g_Log;
                                g_Log->AddLog(LOGTYPE_FATAL, "Current process is not ProcessRun");
                                return false;
                            }
                        }
                    case 2:
                        {
                            // same logic for case-1
                            // but we put higher cost (1.1) to prefer go straight
                            extern Game *g_Game;
                            if(auto pRun = (ProcessRun *)(g_Game->ProcessValid(PROCESSID_RUN))){
                                if(pRun->CanMove(false, nDstX, nDstY)){
                                    if(bCheckCreature){
                                        return pRun->CanMove(true, nDstX, nDstY) ? 1.1 : 100.1;
                                    }else{ return 1.1; }
                                }else{
                                    return 10000.0;
                                }
                            }else{
                                extern Log *g_Log;
                                g_Log->AddLog(LOGTYPE_FATAL, "Current process is not ProcessRun");
                                return false;
                            }
                        }
                    default:
                        {
                            return 10000.0;
                        }
                }
            }
      )
{}
