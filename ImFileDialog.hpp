#ifndef IM_FILE_DIALOG_HPP
#define IM_FILE_DIALOG_HPP

#include <string>
#include <vector>

namespace ImFileDialog
{

/**
 * @brief String vector.
 */
typedef std::vector<std::string> StringVec;

class FileDialog
{
public:
    /**
     * @brief Constructor a file dialog.
     * @param[in] title  The title of the dialog. Encoding in UTF-8.
     * @param[in] filters The filter list of the dialog, Encoding in UTF-8. The
     *   filter must have syntax like: `NAME\nPATTERN1,PATTERN12`s.
     * @param[in] filter_sz The size of the filter list.
     */
    FileDialog(const std::string &title, const std::string &filter);
    FileDialog(const std::string &title, const StringVec &filters);
    virtual ~FileDialog();

public:
    /**
     * @brief Show the file dialog.
     * @return True if dialog has result. In this case query result by GetResult().
     */
    bool Query(void);

    /**
     * @brief Query the result of the dialog.
     * @param[out] vec The result of the paths, encoding in UTF-8.
     * @return True if success, false if not.
     */
    bool GetResult(StringVec &vec) const;

public:
    /**
     * @brief The implementation of the file dialog.
     * @{
     */
    struct Iner;
    struct Iner *m_iner;
    /**
     * @}
     */
};

} // namespace ImFileDialog

#endif
