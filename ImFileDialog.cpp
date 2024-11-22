#include "ImFileDialog.hpp"

typedef ImFileDialog::StringVec StringVec;

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
    static Vec Parse(const char *filters[], size_t filter_sz);

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

PalFilter::Vec PalFilter::Parse(const char *filters[], size_t filter_sz)
{
    PalFilter::Vec result;
    for (size_t i = 0; i < filter_sz; i++)
    {
        result.push_back(PalFilter::Parse(filters[i]));
    }
    return result;
}

#if defined(_WIN32)

struct ImFileDialog::FileDialog::Iner
{
    Iner(const char *title, const char *filter);
    ~Iner();

    std::string title;  /**< Window title. */
    std::string filter; /**< File filter. */

    HANDLE thread; /**< Thread handle. */
};

static DWORD CALLBACK _file_dialog_thread_win32(LPVOID lpThreadParameter)
{
}

ImFileDialog::FileDialog::Iner::Iner(const char *title, const char *filter)
{
    this->title = title;
    this->filter = filter;

    this->thread = CreateThread(NULL, 0, _file_dialog_thread_win32, this, 0, NULL);
    if (this->thread == NULL)
    {
        throw std::runtime_error("CreateThread failed");
    }
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
