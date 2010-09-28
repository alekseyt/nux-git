/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it 
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranties of 
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the applicable version of the GNU Lesser General Public 
 * License for more details.
 * 
 * You should have received a copy of both the GNU Lesser General Public 
 * License version 3 along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jay.taoko_AT_gmail_DOT_com>
 *
 */


#include "NKernel.h"


namespace nux { //NUX_NAMESPACE_BEGIN

#ifdef _WIN32
    #define NUX_ATOMOP_ITERLOCKED_INCREMENT      InterlockedIncrement
    #define NUX_ATOMOP_ITERLOCKED_DECREMENT      InterlockedDecrement
    #define NUX_ATOMOP_ITERLOCKED_EXCHANGED      InterlockedExchange
    #define NUX_ATOMOP_ITERLOCKED_VALUE
#elif _WIN64
    #define NUX_ATOMOP_ITERLOCKED_INCREMENT      InterlockedIncrement64
    #define NUX_ATOMOP_ITERLOCKED_DECREMENT      InterlockedDecrement64
    #define NUX_ATOMOP_ITERLOCKED_EXCHANGED      InterlockedExchange64
    #define NUX_ATOMOP_ITERLOCKED_VALUE
#endif

t_int NThreadSafeCounter::Increment()
{
    return __sync_add_and_fetch(&m_Counter, 1);
}

t_int NThreadSafeCounter::Decrement()
{
    return __sync_add_and_fetch(&m_Counter, -1);
}

t_int NThreadSafeCounter::Set(t_int i)
{
    m_Counter = i;
    return m_Counter;
}

t_int NThreadSafeCounter::GetValue() const
{
    return m_Counter;
}

t_int NThreadSafeCounter::operator ++ ()
{
    return Increment();
}

t_int NThreadSafeCounter::operator -- ()
{
    return Decrement();
}

t_bool NThreadSafeCounter::operator == (t_int i)
{
    return (m_Counter == i);
}

__thread void* NThreadLocalStorage::m_TLSIndex[NThreadLocalStorage::NbTLS];    
BOOL NThreadLocalStorage::m_TLSUsed[NThreadLocalStorage::NbTLS];
NThreadLocalStorage::TLS_ShutdownCallback  NThreadLocalStorage::m_TLSCallbacks[NThreadLocalStorage::NbTLS];

BOOL NThreadLocalStorage::RegisterTLS(t_u32 index, NThreadLocalStorage::TLS_ShutdownCallback shutdownCallback)
{
    nuxAssert(!m_TLSUsed[index]);

    if (!m_TLSUsed[index])
    {
//         m_TLSIndex[index] = TlsAlloc();
//         if(m_TLSIndex[index] == TLS_OUT_OF_INDEXES)
//         {
//             nuxAssertMsg(0, TEXT("[NThreadLocalStorage::RegisterTLS] Out of TLS index."));
//         }
        m_TLSUsed[index]  = TRUE;	
        m_TLSCallbacks[index] =  shutdownCallback;
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

void NThreadLocalStorage::Initialize()
{
    Memset(m_TLSUsed, 0, sizeof(m_TLSUsed));

    for (t_u32 i = 0; i < NThreadLocalStorage::NbTLS; i++)
    {
        // Fill the array with invalid values
        m_TLSIndex[i] = 0;
    }
}

void NThreadLocalStorage::Shutdown()
{
    ThreadShutdown();
}

void NThreadLocalStorage::ThreadInit()
{
}

void NThreadLocalStorage::ThreadShutdown()
{
    TLS_ShutdownCallback *callback = m_TLSCallbacks;
    for (t_u32 i = 0; i < NThreadLocalStorage::NbTLS; ++i, ++callback)
    {
        if (*callback)
        {
            (**callback)();
        }
    }
}

NUX_IMPLEMENT_ROOT_OBJECT_TYPE(NThread);

NThread::NThread()
:   m_ThreadState(THREADINIT)
{ 
    m_pThreadFunc = NThread::EntryPoint; // Can call Detach() also.
}

NThread::NThread(ThreadRoutineFunc lpExternalRoutine)
{
    Attach(lpExternalRoutine);
}

NThread::~NThread()
{
    if(m_ThreadCtx.m_dwTID)
        pthread_detach(m_ThreadCtx.m_dwTID);
}

ThreadState NThread::Start( void* arg )
{
    m_ThreadCtx.m_pUserData = arg;
    int ret = pthread_create(&m_ThreadCtx.m_dwTID,
        NULL,
        m_pThreadFunc,
        this);

    if(ret != 0)
    {
        nuxDebugMsg(TEXT("[NThread::Start] Cannot start thread."));
        m_ThreadState = THREAD_START_ERROR;
        return m_ThreadState;
    }
    return m_ThreadState;
}

ThreadState NThread::Stop( bool bForceKill )
{
    int ret = pthread_detach(m_ThreadCtx.m_dwTID);
    if(ret != 0)
    {
        nuxDebugMsg(TEXT("[NThread::Stop] Cannot detach thread."));
        m_ThreadState = THREAD_STOP_ERROR;
        return m_ThreadState;
    }
    m_ThreadState = THREADSTOP;
    return m_ThreadState;
}

ThreadState NThread::Suspend()
{
    m_ThreadState = THREADSUSPENDED;
    return m_ThreadState;
}

ThreadState NThread::Resume()
{
    return m_ThreadState;
}

// go from suspended to thread start
ThreadState NThread::ResumeStart()
{
    m_ThreadState = THREADINIT;
    return m_ThreadState;
}

// go from suspended to thread exit
ThreadState NThread::ResumeExit()
{
    m_ThreadState = THREADSTOP;
    return m_ThreadState;
}

void* NThread::EntryPoint(void* pArg)
{
    NThread *pParent = reinterpret_cast<NThread*>(pArg);
    if(pParent == 0)
    {
        nuxDebugMsg(TEXT("[NThread::EntryPoint] Invalid pointer. The thread will exit."));
        return 0;
    }

    if(!pParent->ThreadCtor())
    {
        // return another message saying the thread could not execute due to error in ThreadCtor;
    }

    pParent->Run( pParent->m_ThreadCtx.m_pUserData );
    
    pParent->ThreadDtor();
    return 0;
}

t_u32 NThread::GetExitCode() const 
{ 
    return m_ThreadCtx.m_dwExitCode;
}

t_u32 NThread::GetThreadId()
{
    return (t_u32)m_ThreadCtx.m_dwTID;
}

ThreadState NThread::GetThreadState() const
{
    return m_ThreadState;
}

void NThread::SetThreadState(ThreadState state)
{
    m_ThreadState = state;
}

} //NUX_NAMESPACE_END
