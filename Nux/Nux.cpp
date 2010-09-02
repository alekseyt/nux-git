#include "Nux.h"

NAMESPACE_BEGIN_GUI

static NCriticalSection ThreadArrayLock;
std::vector<NThread*> ThreadArray;

void NuxInitialize(const TCHAR* CommandLine)
{
    nux::NuxCoreInitialize(0);

    // Register a thread local store for the WindowThreads. Each window thread will be able to access its own WindowThread pointer.
    inlRegisterThreadLocalIndex(0, ThreadLocal_InalogicAppImpl, NULL);
}

static WindowThread* _CreateModalWindowThread(const TCHAR* WindowTitle,
                                              UINT width,
                                              UINT height,
                                              WindowThread *Parent,
                                              ThreadUserInitFunc UserInitFunc,
                                              void* InitData,
                                              bool Modal)
{
    // check that Parent exist
    WindowThread* w = new WindowThread(WindowTitle, width, height, Parent, Modal);
    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[_CreateModalWindowThread] WindowThread creation failed."));
        return 0;
    }
    return w;
}

WindowThread* CreateGUIThread(const TCHAR* WindowTitle,
                                             UINT width,
                                             UINT height,
                                             WindowThread *Parent,
                                             ThreadUserInitFunc UserInitFunc,
                                             void* InitData)
{
    if(GetThreadApplication())
    {   
        // An WindowThread already exist for this thread.
        nuxAssertMsg(0, "[CreateGUIThread] Only one WindowThread per thread is allowed");
        return 0;
    }
    inlSetThreadLocalStorage(ThreadLocal_InalogicAppImpl, 0);

    WindowThread* w = new WindowThread(WindowTitle, width, height, 0, true);

    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[CreateGUIThread] WindowThread creation failed."));
        return 0;
    }

    w->m_UserInitFunc = UserInitFunc;
    w->m_UserExitFunc = 0;
    w->m_InitData = InitData;
    w->m_ExitData = 0;
    w->SetWindowStyle(WINDOWSTYLE_NORMAL);
    w->ThreadCtor();
    return w;
}

#if defined(INL_OS_WINDOWS)
WindowThread* CreateFromForeignWindow(HWND WindowHandle, HDC WindowDCHandle, HGLRC OpenGLRenderingContext,
    ThreadUserInitFunc UserInitFunc,
    void* InitData
)
{
    if(GetThreadApplication())
    {   
        // An WindowThread already exist for this thread.
        nuxAssertMsg(0, "[CreateGUIThread] Only one WindowThread per thread is allowed");
        return 0;
    }
    inlSetThreadLocalStorage(ThreadLocal_InalogicAppImpl, 0);

    WindowThread* w = new WindowThread("WindowTitle", 400, 300, 0, true);

    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[CreateGUIThread] WindowThread creation failed."));
        return 0;
    }

    w->m_UserInitFunc = UserInitFunc;
    w->m_UserExitFunc = 0;
    w->m_InitData = InitData;
    w->m_ExitData = 0;
    w->SetWindowStyle(WINDOWSTYLE_NORMAL);
    w->ThreadCtor(WindowHandle, WindowDCHandle, OpenGLRenderingContext);
    return w;
}

#elif defined(INL_OS_LINUX)
WindowThread* CreateFromForeignWindow(Display *X11Display, Window X11Window, GLXContext OpenGLContext,
    ThreadUserInitFunc UserInitFunc,
    void* InitData)
{
    if(GetThreadApplication())
    {   
        // An WindowThread already exist for this thread.
        nuxAssertMsg(0, "[CreateGUIThread] Only one WindowThread per thread is allowed");
        return 0;
    }
    inlSetThreadLocalStorage(ThreadLocal_InalogicAppImpl, 0);

    WindowThread* w = new WindowThread("WindowTitle", 400, 300, 0, true);

    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[CreateGUIThread] WindowThread creation failed."));
        return 0;
    }

    w->m_UserInitFunc = UserInitFunc;
    w->m_UserExitFunc = 0;
    w->m_InitData = InitData;
    w->m_ExitData = 0;
    w->SetWindowStyle(WINDOWSTYLE_NORMAL);
    w->m_embedded_window = true;
    w->ThreadCtor(X11Display, X11Window, OpenGLContext);
    return w;
}
#endif
    
// Create a window thread that is a child of the Parent. This thread has a window.
WindowThread* CreateWindowThread(WindowStyle WndStyle,
                                                const TCHAR* WindowTitle,
                                                UINT width,
                                                UINT height,
                                                WindowThread *Parent,
                                                ThreadUserInitFunc UserInitFunc,
                                                void* InitData)
{
    WindowThread *w = _CreateModalWindowThread(WindowTitle, width, height, Parent, UserInitFunc, InitData, false);
    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[CreateWindowThread] WindowThread creation failed."));
        return 0;
    }
    w->m_UserInitFunc = UserInitFunc;
    w->m_UserExitFunc = 0;
    w->m_InitData = InitData;
    w->m_ExitData = 0;
    w->SetWindowStyle(WndStyle);

    return w;
}

// Create modal graphics thread that is a child of the Parent. This thread has a window.
WindowThread* CreateModalWindowThread(WindowStyle WndStyle,
                                                     const TCHAR* WindowTitle,
                                                     UINT width,
                                                     UINT height,
                                                     WindowThread *Parent,
                                                     ThreadUserInitFunc UserInitFunc,
                                                     void* InitData)
{
    WindowThread *w = _CreateModalWindowThread(WindowTitle, width, height, Parent, UserInitFunc, InitData, true);
    if(w == 0)
    {
        nuxAssertMsg(0, TEXT("[CreateWindowThread] WindowThread creation failed."));
        return 0;
    }

    w->m_UserInitFunc = UserInitFunc;
    w->m_UserExitFunc = 0;
    w->m_InitData = InitData;
    w->m_ExitData = 0;
    w->SetWindowStyle(WndStyle);

    return w;
}

SystemThread* CreateSimpleThread(AbstractThread *Parent, ThreadUserInitFunc UserInitFunc, void* InitData)
{
    SystemThread* simpleThread = new SystemThread(Parent);
    return static_cast<SystemThread*>(simpleThread);
}

bool RegisterNuxThread(NThread* ThreadPtr)
{
    nuxAssert(ThreadPtr);
    INL_RETURN_VALUE_IF_NULL(ThreadPtr, false);

    NScopeLock Scope(&ThreadArrayLock);
    std::vector<NThread*>::iterator it = find(ThreadArray.begin(), ThreadArray.end(), ThreadPtr);
    if(it == ThreadArray.end())
    {
        ThreadArray.push_back(ThreadPtr);
    }
    return true;
}

void UnregisterNuxThread(NThread* ThreadPtr)
{
    nuxAssert(ThreadPtr);
    INL_RETURN_IF_NULL(ThreadPtr);
    
    NScopeLock Scope(&ThreadArrayLock);
    std::vector<NThread*>::iterator it = find(ThreadArray.begin(), ThreadArray.end(), ThreadPtr);
    if(it != ThreadArray.end())
    {
        ThreadArray.erase(it);
    }
}

ThreadState GetThreadState(unsigned int ThreadID)
{
    NScopeLock Scope(&ThreadArrayLock);
    std::vector<NThread*>::iterator it;
    for(it = ThreadArray.begin(); it != ThreadArray.end(); it++)
    {
        if((*it)->GetThreadId() == ThreadID)
        {
            return (*it)->GetThreadState();
            break;
        }
    }
    return THREADSTOP;
}

GLWindowImpl& GetWindow()
{
    NThread* thread = GetThreadApplication();
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetWindow] You can't call GetWindow on this type of thread: s"), thread->Type().GetName());
    }
    return (static_cast<WindowThread*> (thread))->GetWindow();
}

GraphicsContext& GetGraphicsContext()
{
    NThread* thread = GetThreadApplication();
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetGraphicsContext] You can't call GetGraphicsContext on this type of thread: s"), thread->Type().GetName());
    }
    return (static_cast<WindowThread*> (thread))->GetGraphicsContext();
}

WindowCompositor& GetThreadWindowCompositor()
{
    NThread* thread = GetThreadApplication();
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetThreadWindowCompositor] You can't call GetThreadWindowCompositor on this type of thread: s"), thread->Type().GetName());
        inlOutputDebugString(TEXT("[GetThreadWindowCompositor] You can't call GetThreadWindowCompositor on this type of thread: s"), thread->Type().GetName());
        INL_HARDWARE_BREAK;
    }
    return (static_cast<WindowThread*> (thread))->GetStackManager();
}

NThread* GetThreadApplication()
{
    NThread* thread = static_cast<NThread*>(inlGetThreadLocalStorage(ThreadLocal_InalogicAppImpl));
    if(thread == 0)
    {
    }
    return thread;
}

WindowThread* GetGraphicsThread()
{
    NThread* thread = GetThreadApplication();
    nuxAssertMsg(thread, TEXT("[GetGraphicsThread] The current thread is not of the type WindowThread."));
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetGraphicsThread] You can't call GetGraphicsThread on this type of thread: s"), thread->Type().GetName());
        return 0;
    }
    return static_cast<WindowThread*>(inlGetThreadLocalStorage(ThreadLocal_InalogicAppImpl));
}

BasePainter& GetThreadPainter()
{
    NThread* thread = GetThreadApplication();
    nuxAssertMsg(thread, TEXT("[GetGraphicsThread] The current thread is not of the type WindowThread."));
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetThreadPainter] You can't call GetThreadPainter on this type of thread: s"), thread->Type().GetName());
    }
    return (static_cast<WindowThread*> (thread))->GetPainter();
}

UXTheme& GetThreadTheme()
{
    NThread* thread = GetThreadApplication();
    nuxAssertMsg(thread, TEXT("[GetThreadTheme] The current thread is not of the type WindowThread."));
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetThreadTheme] You can't call GetThreadTheme on this type of thread: s"), thread->Type().GetName());
    }
    return (static_cast<WindowThread*> (thread))->GetTheme();
}

TimerHandler& GetThreadTimer()
{
    NThread* thread = GetThreadApplication();
    nuxAssertMsg(thread, TEXT("[GetGraphicsThread] The current thread is not of the type WindowThread."));
    if(!thread->Type().IsObjectType(WindowThread::StaticObjectType))
    {
        nuxAssertMsg(0, TEXT("[GetThreadTimer] You can't call GetThreadTimer on this type of thread: s"), thread->Type().GetName());
        inlOutputDebugString(TEXT("[GetThreadTimer] You can't call GetThreadTimer on this type of thread: s"), thread->Type().GetName());
        INL_HARDWARE_BREAK;
    }
    return (static_cast<WindowThread*> (thread))->GetTimerHandler();
}

NAMESPACE_END_GUI
