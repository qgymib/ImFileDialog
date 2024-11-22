#include "ImFileDialog.hpp"

class PalThread
{
public:
    typedef void (*Fn)(void *);

public:
    PalThread(Fn fn, void *arg);
    virtual ~PalThread();

private:
    struct Iner;
    struct Iner *m_iner;
};

#if __cplusplus >= 201103L

#include <thread>

struct PalThread::Iner
{
    Iner(PalThread::Fn fn, void *arg);
    ~Iner();

    PalThread::Fn fn;
    void         *arg;
    std::thread  *thread;
};

PalThread::Iner::Iner(PalThread::Fn fn, void *arg)
{
    this->fn = fn;
    this->arg = arg;
    this->thread = new std::thread(fn, arg);
}

PalThread::Iner::~Iner()
{
    thread->join();
}

PalThread::PalThread(PalThread::Fn fn, void *arg)
{
    m_iner = new Iner(fn, arg);
}

PalThread::~PalThread()
{
    delete m_iner;
}

#elif defined(_WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <Windows.h>

struct PalThread::Iner
{
    Iner(PalThread::Fn fn, void *arg);
    ~Iner();

    PalThread::Fn fn;
    void         *arg;
    HANDLE        thread;
};

static DWORD CALLBACK _thread_proxy(LPVOID lpThreadParameter)
{
    PalThread::Iner *iner = (PalThread::Iner *)lpThreadParameter;
    iner->fn(iner->arg);
    return 0;
}

PalThread::Iner::Iner(PalThread::Fn fn, void *arg)
{
    this->fn = fn;
    this->arg = arg;
    this->thread = CreateThread(NULL, 0, _thread_proxy, this, 0, NULL);
    if (this->thread == NULL)
    {
        abort();
    }
}

PalThread::Iner::~Iner()
{
    int ret = WaitForSingleObject(thread, INFINITE);
    if (ret != WAIT_OBJECT_0)
    {
        abort();
    }
    CloseHandle(thread);
}

PalThread::PalThread(PalThread::Fn fn, void *arg)
{
    m_iner = new Iner(fn, arg);
}

PalThread::~PalThread()
{
    delete m_iner;
}

#elif defined(__linux__)

#include <pthread.h>

struct PalThread::Iner
{
    Iner(PalThread::Fn fn, void *arg);
    ~Iner();

    PalThread::Fn fn;
    void         *arg;
    pthread_t     thread;
};

PalThread::Iner::Iner(PalThread::Fn fn, void *arg)
{
    this->fn = fn;
    this->arg = arg;
    if (pthread_create(&thread, NULL, fn, arg) != 0)
    {
        abort();
    }
}

PalThread::Iner::~Iner()
{
    void *ret = NULL;
    if (pthread_join(thread, &ret) != 0)
    {
        abort();
    }
}

#else

#error "Unsupported platform"

#endif

struct ImFileDialog::FileDialog::Iner
{
    Iner(const char *title, const char *filter);
    ~Iner();

    std::string title;
    std::string filter;
};

ImFileDialog::FileDialog::Iner::Iner(const char *title, const char *filter)
{
    this->title = title;
    this->filter = filter;
}

ImFileDialog::FileDialog::Iner::~Iner()
{
}

ImFileDialog::FileDialog::FileDialog(const char *title, const char *filter)
{
    m_iner = new Iner(title, filter);
}

ImFileDialog::FileDialog::~FileDialog()
{
    delete m_iner;
}
