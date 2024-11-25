#ifndef IM_FILE_DIALOG_HPP
#define IM_FILE_DIALOG_HPP

#include <string>
#include <vector>

#define IMFILEDIALOG_VERSION_MAJOR  0
#define IMFILEDIALOG_VERSION_MINOR  0
#define IMFILEDIALOG_VERSION_PATCH  1

namespace ImFileDialog
{

/**
 * @brief String vector.
 */
typedef std::vector<std::string> StringVec;

class OpenDialog
{
public:
    enum Option
    {
        /**
         * @brief Present an Open dialog that offers a choice of folders rather
         *   than files.
         */
        PICK_FOLDERS = 0x01,

        /**
         * @brief Enables the user to select multiple items in the open dialog.
         */
        ALLOW_MULTISELECT = 0x02,
    };

public:
    /**
     * @brief Constructor a file dialog.
     * @param[in] filter The filter list of the dialog, Encoding in UTF-8. The
     *   filter must have syntax like: `NAME:PATTERN1,PATTERN12\nNAME:PATTERN3`.
     * @param[in] flags Options. See #OpenDialog::Option.
     */
    OpenDialog(const char *filter, unsigned flags = 0);

    /**
     * @brief Constructor a file dialog.
     * @param[in] title  The title of the dialog. Encoding in UTF-8.
     * @param[in] filter The filter list of the dialog, Encoding in UTF-8. 
     * @param[in] flags Options. See #OpenDialog::Option.
     */
    OpenDialog(const char* title, const char* filter, unsigned flags = 0);

    /**
     * @brief Destructor.
     */
    virtual ~OpenDialog();

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
