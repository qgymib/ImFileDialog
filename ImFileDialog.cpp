#include <stdexcept>
#include "ImFileDialog.hpp"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))
#endif

using namespace ImFileDialog;

class PalFilter
{
public:
    typedef std::vector<PalFilter> Vec;

public:
    /**
     * @brief Parse the filter string.
     * @param[in] filter The filter string.
     * @return The parsed filter.
     */
    static PalFilter Parse(const std::string &filter);

    /**
     * @brief Parse the filter string list.
     * @param[in] filters The filter string list.
     * @param[in] filter_sz The size of the filter list.
     * @return The parsed filter list.
     */
    static Vec Parse(const StringVec& filters);

public:
    std::string name;     /**< Name of the filter. */
    StringVec   patterns; /**< File patterns. */
};

static StringVec StringSplit(const std::string &orig, const std::string &delimiter)
{
    StringVec vetStr;

    /* If #orig is empty or equal to #delimiter, return empty vector */
    if (orig == "" || orig == delimiter)
    {
        return vetStr;
    }
    /* If #delimiter is empty, return original vector */
    if (delimiter == "")
    {
        vetStr.push_back(orig);
        return vetStr;
    }

    std::string::size_type startPos = 0;
    std::size_t            index = orig.find(delimiter);
    while (index != std::string::npos)
    {
        std::string str = orig.substr(startPos, index - startPos);
        if (str != "")
        {
            vetStr.push_back(str);
        }
        startPos = index + delimiter.length();
        index = orig.find(delimiter, startPos);
    }

    /* Use last substring */
    std::string str = orig.substr(startPos);
    if (str != "")
    {
        vetStr.push_back(str);
    }

    return vetStr;
}

PalFilter PalFilter::Parse(const std::string &filter)
{
    PalFilter   result;
    std::size_t pos = filter.find('\n');
    if (pos == std::string::npos)
    {
        return result;
    }

    result.name = filter.substr(0, pos);

    std::string pattern_data = filter.substr(pos + 1);
    result.patterns = StringSplit(pattern_data, ",");

    return result;
}

PalFilter::Vec PalFilter::Parse(const StringVec &filters)
{
    PalFilter::Vec result;

    StringVec::const_iterator it = filters.begin();
    for (; it != filters.end(); it++)
    {
        result.push_back(PalFilter::Parse(*it));
    }
    return result;
}

#if defined(_WIN32)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#include <Windows.h>
#include <shobjidl.h>
#include <memory>
#include <atlbase.h>

/**
 * @brief Wide string.
 * @note Windows only.
 */
typedef std::shared_ptr<WCHAR> wstring;

enum OpenDialogStatus
{
    OPENDIALOG_OPEN,
    OPENDIALOG_OK,
    OPENDIALOG_CANCEL,
};

struct EnumData
{
    DWORD dwThreadId;
    HWND  hwndFound;
};

struct OpenDialog::Iner
{
    Iner(const std::string &title, const StringVec &filters);
    ~Iner();

    std::string title;  /**< Window title. */
    PalFilter::Vec filters; /**< File filter. */

    HANDLE thread; /**< Thread handle. */
    DWORD             thread_id;

    CRITICAL_SECTION mutex;
    StringVec        result;
    OpenDialogStatus status;
};

class comdlg_filterspec
{
public:
    comdlg_filterspec();
    virtual ~comdlg_filterspec();

public:
    void append(const PalFilter &filter);
    void append(const PalFilter::Vec &filters);

public:
    COMDLG_FILTERSPEC *m_save_types;
    UINT              m_save_type_sz;
};

/**
 * @brief Convert UTF-8 string into Unicode string.
 * @param[in] str   UTF-8 string.
 * @return Unicode string.
 */
static wstring utf8_to_wide(const std::string &str)
{
    const char *src = str.c_str();
    int pathw_len = MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (pathw_len == 0)
    {
        return NULL;
    }

    size_t buf_sz = pathw_len * sizeof(WCHAR);
    WCHAR *buf = (WCHAR *)malloc(buf_sz);
    if (buf == NULL)
    {
        return NULL;
    }

    int r = MultiByteToWideChar(CP_UTF8, 0, src, -1, buf, pathw_len);
    if (r != pathw_len)
    {
        abort();
    }

    return wstring(buf, free);
}

/**
 * @brief Convert Unicode string into UTF-8 string.
 * @param[in] str   Unicode string.
 * @return UTF-8 string.
 */
std::string wide_to_utf8(const WCHAR *str)
{
    std::string ret;

    int target_len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (target_len == 0)
    {
        return ret;
    }

    char *buf = (char *)malloc(target_len);
    if (buf == NULL)
    {
        return ret;
    }

    int r = WideCharToMultiByte(CP_UTF8, 0, str, -1, buf, target_len, NULL, NULL);
    if (r != target_len)
    {
        abort();
    }

    ret = buf;
    free(buf);

    return ret;
}

comdlg_filterspec::comdlg_filterspec()
{
    m_save_types = NULL;
    m_save_type_sz = 0;
}

comdlg_filterspec::~comdlg_filterspec()
{
    for (size_t i = 0; i < m_save_type_sz; i++)
    {
        free((void *)m_save_types[i].pszName);
        free((void *)m_save_types[i].pszSpec);
    }
    free(m_save_types);
}

void comdlg_filterspec::append(const PalFilter& filter)
{
    std::string str;
    for (size_t i = 0; i < filter.patterns.size(); i++)
    {
        const std::string &item = filter.patterns[i];
        if (i != 0)
        {
            str.append(";");
        }
        str.append(item);
    }

    size_t             new_sz = sizeof(COMDLG_FILTERSPEC) * (m_save_type_sz + 1);
    COMDLG_FILTERSPEC *new_save_types = (COMDLG_FILTERSPEC *)realloc(m_save_types, new_sz);
    if (new_save_types == nullptr)
    {
        throw std::runtime_error("Out of memory");
    }
    m_save_types = new_save_types;

    m_save_types[m_save_type_sz].pszName = _wcsdup(utf8_to_wide(filter.name).get());
    m_save_types[m_save_type_sz].pszSpec = _wcsdup(utf8_to_wide(str).get());
    m_save_type_sz++;
}

void comdlg_filterspec::append(const PalFilter::Vec &filters)
{
    PalFilter::Vec::const_iterator it = filters.begin();
    for (; it != filters.end(); it++)
    {
        append(*it);
    }
}

static DWORD CALLBACK _file_dialog_task(LPVOID lpThreadParameter)
{
    OpenDialog::Iner *iner = static_cast<OpenDialog::Iner *>(lpThreadParameter);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("CoInitializeEx failed");
    }

    CComPtr<IFileOpenDialog> pfd = nullptr;
    hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("CoCreateInstance failed");
    }

    DWORD dwFlags = 0;
    hr = pfd->GetOptions(&dwFlags);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("GetOptions failed");
    }

    hr = pfd->SetOptions(dwFlags | FOS_FORCEFILESYSTEM | FOS_ALLOWMULTISELECT);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("SetOptions failed");
    }

    if (!iner->title.empty())
    {
        wstring title = utf8_to_wide(iner->title);
        hr = pfd->SetTitle(title.get());
        if (!SUCCEEDED(hr))
        {
            throw std::runtime_error("SetTitle failed");
        }
    }

    comdlg_filterspec spec;
    spec.append(iner->filters);
    hr = pfd->SetFileTypes(spec.m_save_type_sz, spec.m_save_types);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("SetFileTypes failed");
    }

    hr = pfd->SetFileTypeIndex(1);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("SetFileTypeIndex failed");
    }

    /*
     * This is a blocking operation.
     */
    hr = pfd->Show(nullptr);

    /* The user closed the window by cancelling the operation. */
    if (hr == HRESULT_FROM_WIN32(ERROR_CANCELLED))
    {
        EnterCriticalSection(&iner->mutex);
        iner->status = OPENDIALOG_CANCEL;
        LeaveCriticalSection(&iner->mutex);
        return 0;
    }

    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("Show failed");
    }

    CComPtr<IShellItemArray> psiaResults;
    hr = pfd->GetResults(&psiaResults);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("GetResults failed");
    }

    DWORD dwNumItems = 0;
    hr = psiaResults->GetCount(&dwNumItems);
    if (!SUCCEEDED(hr))
    {
        throw std::runtime_error("GetCount failed");
    }

    StringVec result;
    for (DWORD i = 0; i < dwNumItems; i++)
    {
        CComPtr<IShellItem> psiResult;
        hr = psiaResults->GetItemAt(i, &psiResult);
        if (!SUCCEEDED(hr))
        {
            throw std::runtime_error("GetItemAt failed");
        }

        PWSTR pszFilePath = NULL;
        hr = psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
        if (!SUCCEEDED(hr))
        {
            throw std::runtime_error("GetDisplayName failed");
        }

        std::string path = wide_to_utf8(pszFilePath);
        CoTaskMemFree(pszFilePath);

        result.push_back(path);
    }

    EnterCriticalSection(&iner->mutex);
    {
        iner->status = OPENDIALOG_OK;
        iner->result = result;
    }
    LeaveCriticalSection(&iner->mutex);

    CoUninitialize();
    return 0;
}

#include <iostream>

OpenDialog::Iner::Iner(const std::string &title, const StringVec &filters)
{
    this->title = title;
    this->filters = PalFilter::Parse(filters);
    status = OPENDIALOG_OPEN;
    InitializeCriticalSection(&mutex);

    this->thread = CreateThread(nullptr, 0, _file_dialog_task, this, 0, &thread_id);
    if (this->thread == nullptr)
    {
        throw std::runtime_error("CreateThread failed");
    }

    DWORD ctid = GetCurrentThreadId();
    std::cout << "tid:" << thread_id << ",ctid:" << ctid << std::endl;
}

static BOOL CALLBACK EnumThreadWndProc(HWND hwnd, LPARAM lParam)
{
    EnumData *data = (EnumData*)lParam;
    DWORD     dwThreadId = GetWindowThreadProcessId(hwnd, NULL);

    if (data->dwThreadId != dwThreadId)
    {
        return TRUE;
    }

    /* Check if this window is a file dialog. */
    wchar_t className[256];
    if (!GetClassNameW(hwnd, className, ARRAY_SIZE(className)))
    {
        return TRUE;
    }
    if (wcscmp(className, L"#32770") != 0)
    { /* Not a file dialog. */
        return TRUE;
    }

    data->hwndFound = hwnd;
    return FALSE;
}

OpenDialog::Iner::~Iner()
{
    /* If dialog not closed, close it. */
    {
        EnumData data = { thread_id, NULL };
        EnumWindows(EnumThreadWndProc, (LPARAM)&data);

        if (data.hwndFound != NULL)
        {
            PostMessage(data.hwndFound, WM_CLOSE, 0, 0);
        }
    }

    /* Wait for thread exit. */
    int ret = WaitForSingleObject(thread, INFINITE);
    if (ret != WAIT_OBJECT_0)
    {
        abort();
    }
    CloseHandle(thread);
    thread = nullptr;

    DeleteCriticalSection(&mutex);
}

OpenDialog::OpenDialog(const std::string &title, const StringVec &filters)
{
    m_iner = new Iner(title, filters);
}

OpenDialog::OpenDialog(const std::string &title, const std::string &filter)
{
    StringVec filters;
    filters.push_back(filter);
    m_iner = new Iner(title, filters);
}

OpenDialog::~OpenDialog()
{
    delete m_iner;
}

bool OpenDialog::Query()
{
    OpenDialogStatus ret;
    EnterCriticalSection(&m_iner->mutex);
    {
        ret = m_iner->status;
    }
    LeaveCriticalSection(&m_iner->mutex);

    return ret != OPENDIALOG_OPEN;
}

bool OpenDialog::GetResult(StringVec &vec) const
{
    bool ret;
    EnterCriticalSection(&m_iner->mutex);
    do
    {
        if (m_iner->status != OPENDIALOG_OK)
        {
            ret = false;
            break;
        }

        vec = m_iner->result;
        ret = true;
    } while (false);
    LeaveCriticalSection(&m_iner->mutex);

    return ret;
}

#else

#include <stdio.h>
#include <pthread.h>
#include <memory>

typedef std::shared_ptr<StringVec> StringVecPtr;

struct ImFileDialog::FileDialog::Iner
{
    Iner(const char *title, const char *filters[], size_t filter_sz);
    ~Iner();

    std::string    title;     /**< Window title. */
    PalFilter::Vec filters;   /**< File filters. */
    bool           draw_mode; /**< Draw mode. */

    pthread_t   *thread; /**< Thread handle. */
    StringVecPtr result; /**< Result of the dialog. */

    std::string zenity_cmd;
};

/**
 * @brief True if zenity is available.
 */
static bool s_is_zenity_available = true;

/**
 * @brief Build the zenity filter string.
 * @param[in] filter The filter.
 * @return The zenity filter string.
 */
static std::string _build_zenity_filter(const PalFilter::Vec &filters)
{
    std::string result;

    PalFilter::Vec::const_iterator it = filters.begin();
    for (size_t cnt = 0; it != filters.end(); it++, cnt++)
    {
        const PalFilter &filter = *it;

        if (cnt != 0)
        {
            result += " ";
        }

        result += "--file-filter=\"" + filter.name + " |";
        for (size_t i = 0; i < filter.patterns.size(); i++)
        {
            result += " " + filter.patterns[i];
        }
        result += "\" ";
    }

    return result;
}

static void *_file_dialog_zenity_thread(void *data)
{
    ImFileDialog::FileDialog::Iner *iner = static_cast<ImFileDialog::FileDialog::Iner *>(data);
    FILE                           *cmd = popen(iner->zenity_cmd.c_str(), "r");
    if (cmd == nullptr)
    {
        iner->draw_mode = true;
        s_is_zenity_available = false;
        return nullptr;
    }

    std::string result;
    char        buff[1024];

    while (!feof(cmd))
    {
        size_t read_sz = fread(buff, 1, sizeof(buff), cmd);
        result.append(buff, read_sz);
    }

    if (pclose(cmd))
    {
        iner->draw_mode = true;
        s_is_zenity_available = false;
        return nullptr;
    }

    StringVec vec = StringSplit(result, "\n");
    iner->result = std::make_shared<StringVec>(vec);

    return nullptr;
}

static void _file_dialog_run_zenity(ImFileDialog::FileDialog::Iner *iner)
{
    iner->thread = (pthread_t *)malloc(sizeof(pthread_t));
    int ret = pthread_create(iner->thread, nullptr, _file_dialog_zenity_thread, iner);
    if (ret != 0)
    {
        free(iner->thread);
        iner->thread = nullptr;
        return;
    }
}

ImFileDialog::FileDialog::Iner::Iner(const char *title, const char *filters[], size_t filter_sz)
{
    this->title = title;
    this->filters = PalFilter::Parse(filters, filter_sz);
    this->thread = nullptr;
    this->draw_mode = false;

    if (s_is_zenity_available)
    {
        zenity_cmd = "zenity --file-selection --multiple --title=\"" + this->title + "\" --file-filter=\"" +
                     _build_zenity_filter(this->filters) + "\"";
        _file_dialog_run_zenity(this);
    }
}

ImFileDialog::FileDialog::Iner::~Iner()
{
    if (thread != nullptr)
    {
        if (pthread_join(*thread, nullptr) != 0)
        {
            abort();
        }
        free(thread);
        thread = nullptr;
    }
}

ImFileDialog::FileDialog::FileDialog(const char *title, const char *filters[], size_t filter_sz)
{
    m_iner = new Iner(title, filters, filter_sz);
}

ImFileDialog::FileDialog::~FileDialog()
{
    delete m_iner;
}

static bool _dialog_query_draw(ImFileDialog::FileDialog::Iner *iner)
{
    (void)iner;
    return false;
}

static bool _dialog_query_thread(ImFileDialog::FileDialog::Iner *iner)
{
    if (iner->thread == nullptr)
    {
        return true;
    }

    int ret = pthread_tryjoin_np(*iner->thread, nullptr);
    if (ret == 0)
    {
        free(iner->thread);
        iner->thread = nullptr;
        return true;
    }

    return false;
}

bool ImFileDialog::FileDialog::Query(void)
{
    if (m_iner->draw_mode)
    {
        return _dialog_query_draw(m_iner);
    }

    return _dialog_query_thread(m_iner);
}

bool ImFileDialog::FileDialog::GetResult(StringVec &vec) const
{
    StringVecPtr reuslt = m_iner->result;
    if (reuslt == nullptr)
    {
        return false;
    }

    vec = *reuslt;
    return true;
}

#endif
