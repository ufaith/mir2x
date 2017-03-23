/*
 * =====================================================================================
 *
 *       Filename: reactobject.hpp
 *        Created: 04/21/2016 23:02:31
 *  Last Modified: 03/22/2017 16:23:54
 *
 *    Description: object only react to message, with an object pod
 *                 atoms of an react object:
 *                      1. before Activate(), we can call its internal method by ``this"
 *                      2. after Activate(), we can only use ReactObject::Send()
 *
 *                 In other word, after Activate(), react object can only communicate
 *                 with react object or receiver
 *
 *                 This prevent me implement MonoServer as react object. For MonoServer
 *                 it needs to manager SessionHub. However SessionHub is not an actor, so
 *                 if MonoServer is an react object, we have to launch SessionHub before
 *                 calling of MonoServer::Activate(), but, before activation of MonoServer
 *                 we don't have the address of Mo MonoServer to pass to SessionHub!
 *
 *                 In my design, SessionHub create Session's with SID and pass it to the
 *                 MonoServer, then MonoServer check info of this connection from DB and
 *                 create player object, bind Session pointer to the player and send the
 *                 player to proper RegionMonitor via ServerMap object.
 *
 *                 Another thing is for g_MonoServer->AddLog(...), if make MonoServer as
 *                 a react object, we can't use it anymore
 *
 *                 So let's make MonoServer as a receriver instead.
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
#include <queue>
#include <Theron/Theron.h>

#include "actorpod.hpp"
#include "statehook.hpp"
#include "delaycmd.hpp"
#include "messagepack.hpp"
#include "serverobject.hpp"

class ReactObject: public ServerObject
{
    protected:
        ActorPod *m_ActorPod;

    protected:
        StateHook m_StateHook;
        std::priority_queue<DelayCmd> m_DelayCmdQ;

    public:
        ReactObject(uint8_t);
       ~ReactObject();

    public:
        Theron::Address Activate();

    public:
        bool ActorPodValid() const
        {
            return GetAddress() == Theron::Address::Null();
        }

        Theron::Address GetAddress() const
        {
            return m_ActorPod ? m_ActorPod->GetAddress() : Theron::Address::Null();
        }

    public:
        virtual void Operate(const MessagePack &, const Theron::Address &) = 0;

    public:
        void Delay(uint32_t, const std::function<void()> &);

#if defined(MIR2X_DEBUG) && (MIR2X_DEBUG >= 5)
    protected:
        virtual const char *ClassName() = 0;
#endif
};
