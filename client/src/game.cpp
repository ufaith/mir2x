/*
 * =====================================================================================
 *
 *       Filename: game.cpp
 *        Created: 08/12/2015 09:59:15
 *  Last Modified: 03/31/2017 10:12:16
 *
 *    Description: public API for class game only
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

#include "game.hpp"
#include <thread>
#include <future>
#include "log.hpp"
#include "xmlconf.hpp"
#include "pngtexdbn.hpp"
#include "fontexdbn.hpp"
#include "pngtexoffdbn.hpp"

Game::Game()
    : m_FPS(30.0)
    , m_NetPackTick(-1.0)
    , m_CurrentProcess(nullptr)
{
    // fullfil the time cq
    for(size_t nIndex = 0; nIndex < m_DelayTimeCQ.Capacity(); ++nIndex){
        m_DelayTimeCQ.PushHead(1000.0 / m_FPS);
    }

    // load PNGTexDB
    extern PNGTexDBN    *g_PNGTexDBN;
    extern PNGTexOffDBN *g_PNGTexOffDBN;
    extern FontexDBN    *g_FontexDBN;
    extern XMLConf      *g_XMLConf;
    extern Log          *g_Log;

    // texture path
    auto pNode = g_XMLConf->GetXMLNode("Root/Texture/PNGTexDBN");
    if(!pNode){
        g_Log->AddLog(LOGTYPE_WARNING, "No PNGTexDBN path found in configuration.");
        throw std::error_code();
    }
    g_Log->AddLog(LOGTYPE_INFO, "PNGTexDBN path: %s", pNode->GetText());
    g_PNGTexDBN->Load(pNode->GetText());

    // fontex load
    pNode = g_XMLConf->GetXMLNode("Root/Font/FontexDBN");
    if(!pNode){
        g_Log->AddLog(LOGTYPE_WARNING, "No FontexDBN path found in configuration.");
        throw std::error_code();
    }
    g_Log->AddLog(LOGTYPE_INFO, "FontexDBN path: %s", pNode->GetText());
    g_FontexDBN->Load(pNode->GetText());

    // texture with offset load
    pNode = g_XMLConf->GetXMLNode("Root/Texture/PNGTexOffDBN");
    if(!pNode){
        g_Log->AddLog(LOGTYPE_WARNING, "No PNGTexOffDBN path found in configuration.");
        throw std::error_code();
    }
    g_Log->AddLog(LOGTYPE_INFO, "PNGTexOffDBN path: %s", pNode->GetText());
    g_PNGTexOffDBN->Load(pNode->GetText());
}

Game::~Game()
{
}

void Game::ProcessEvent()
{
    if(m_CurrentProcess){
        SDL_Event stEvent;
        while(SDL_PollEvent(&stEvent)){
            m_CurrentProcess->ProcessEvent(stEvent);
        }
    }
}

void Game::Update(double fDeltaMS)
{
    if(m_CurrentProcess){
        m_CurrentProcess->Update(fDeltaMS);
    }
}

void Game::Draw()
{
    if(m_CurrentProcess){
        m_CurrentProcess->Draw();
    }
}

void Game::MainLoop()
{
    SwitchProcess(PROCESSID_LOGO);
    InitASIO();

    auto fLastMS = GetTimeTick();
    while(m_CurrentProcess->ID() != PROCESSID_EXIT){
        PollASIO();
        ProcessEvent();
        
        double fCurrentMS = GetTimeTick();
        // if(m_NetPackTick > 0.0){
        //     if(fCurrentMS - m_NetPackTick > 15.0 * 1000){
        //         std::exit(0);
        //     }
        // }

        Update(fCurrentMS - fLastMS);
        Draw();

        m_DelayTimeCQ.PushHead(fCurrentMS - fLastMS);
        fLastMS = fCurrentMS;

        // try to expect next delay time interval
        double fTimeSum = 0.0;
        for(m_DelayTimeCQ.Reset(); !m_DelayTimeCQ.Done(); m_DelayTimeCQ.Forward()){
            fTimeSum += m_DelayTimeCQ.Current();
        }

        double fExpectedTime = (1.0 + m_DelayTimeCQ.Size()) * 1000.0 / m_FPS - fTimeSum;
        EventDelay(fExpectedTime);
    }
}
