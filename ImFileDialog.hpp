#ifndef IM_FILE_DIALOG_HPP
#define IM_FILE_DIALOG_HPP

#include <string>
#include <vector>

namespace ImFileDialog {

/**
 * @brief String vector.
 */
typedef std::vector<std::string> StringVec;

class FileDialog
{
public:
    /**
     * @brief Constructor a file dialog.
     * @param[in] title  The title of the dialog.
     * @param[in] filter The filter of the dialog.
     */
    FileDialog(const char* title, const char* filter);
    virtual ~FileDialog();

public:
    /**
     * @brief Show the file dialog.
     * @return True if dialog has result. In this case query result by GetResult().
     */
    bool Query(void);

    /**
     * @brief Query the result of the dialog.
     * @param[out] vec The result of the dialog.
     * @return True if success, false if not.
     */
    bool GetResult(StringVec& vec) const;

private:
    /**
     * @brief The implementation of the file dialog.
     * @{
     */
    struct Iner;
    struct Iner* m_iner;
    /**
     * @}
     */
};

}

#endif
