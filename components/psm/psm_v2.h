#ifndef __PSM_V2_H__
#define __PSM_V2_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "partition.h"
#include "sys_errno.h"

#define PSM_MODULE_VER "2.0"

/** PSM Handle */
typedef void *psm_hnd_t;

/** PSM Object handle */
typedef void *psm_object_handle_t;

/** PSM opening mode */
typedef enum
{
    /** Read mode */
    PSM_MODE_READ = 1,
    /** Write mode */
    PSM_MODE_WRITE = 2,
} psm_mode_t;

/** PSM Flags
 *
 * Required in \ref psm_objattrib_t. Please refer \ref psm-v2_index_caching
 * for details
 */
typedef enum
{
    /** Indexing enabled */
    PSM_INDEXING_ENABLED = 0x01,
    /** Caching enabled */
    PSM_CACHING_ENABLED = 0x02,
} psm_flags_t;

/** PSM Object attributes
 *
 * This structure is accepted by psm_object_open()
 */
typedef struct
{
    /** OR of the flags defined in \ref psm_flags_t */
    uint32_t flags;
} psm_objattrib_t;

/** PSM Configuration
 *
 * This structure is passed to psm_module_init(). PSMv2 won't need the user
 * copy of this structure after call to psm_module_init() returns.
 */
typedef struct
{
    /** If read-only is enabled no writes will be possible to the
    partition */
    bool read_only;
    /**
     * If 'secure' is set, the object will be encrypted before
     * writing, and object will be decrypted before reading. If secure
     * PSM is enabled KEY_PSM_ENCRYPT_KEY and KEY_PSM_NONCE needs to
     * be present in the keystore or must be provided through following
     * members. Without these parameters the PSM initialization will fail.
     * This variable should not be set in case of mc200.
     */
    bool secure;
} psm_cfg_t;

/** Part Info types */
enum part_info_type
{
    /** part_info.name is valid */
    PART_INFO_NAME = 0,
    /** part_info.fl is valid */
    PART_INFO_FL_DESC = 1,
};

/** PSM init partition info
 *
 * This structure is passed to psm_module_init_ex() to specify partition
 * details. It allows app to specify an arbitrary flash space
 * or an existing partition name in addition to the PSM configuration for
 * this partition. PSMv2 won't need the user copy of this structure or
 * any pointer within this structure after call to psm_module_init_ex()
 * returns.
 */
typedef struct
{
    /** part info type below  */
    enum part_info_type pitype;
    union
    {
        /** 'name' is valid if 'pitype'= PART_INFO_NAME */
        const char *name;
        /** 'fl' is valid if 'pitype'= PART_INFO_FL_DESC */
        const flash_desc_t *fl;
    } part_info;

    /** PSM Configuration settings for this init partition */
    psm_cfg_t *psm_cfg;
} psm_init_part_t;

/**
 * Initialize the PSM module
 *
 * This API initializes the PSM module. The given flash space will
 * be accessed and initialized if necessary. Since this function is the
 * first one called after a power cycle, if a flash operation was in
 * progress recovery actions will be performed.
 *
 * @note If secure PSM is enabled KEY_PSM_ENCRYPT_KEY and KEY_PSM_NONCE
 * needs to be present in the keystore or can optionally be provided
 * through \t ref psm_cfg_t structure.
 *
 * @param[in] fdesc Populated flash descriptor \ref flash_desc_t having
 * partition details. Please refer to \ref partition.h for relevant APIs.
 * @param[out] phandle Pointer to the handle \ref psm_hnd_t. Populated by PSM.
 * @param[in] psm_cfg \ref psm_cfg_t PSM configuration structure. If NULL
 * is passed default configuration i.e. read/write mode is used.
 *
 * @return SYS_OK Init success.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_NOMEM Memory allocation failure.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_module_init(flash_desc_t *fdesc, psm_hnd_t *phandle, psm_cfg_t *psm_cfg);

/**
 * Initialize the PSM module with specified partition name or flash space.
 *
 * This API initializes the PSM module with the enhancement that
 * partition details can be specified either as a partition name
 * from partition table or as flash descriptor itself.
 *
 * The given flash spaces will be accessed and initialized if necessary.
 * Since this function is the first one called after a power cycle,
 * if a flash operation was in progress recovery actions will be performed.
 *
 * @note If secure PSM is enabled KEY_PSM_ENCRYPT_KEY and KEY_PSM_NONCE
 * needs to be present in the keystore or can optionally be provided
 * through \t ref psm_cfg_t structure that is part of \ref
 * psm_init_part_t structure.
 *
 * @note there is no corresponding psm_module_deinit_ex() API required.
 * Use existing \ref psm_module_deinit() instead.
 *
 * @param[in] psm_init_part Pointer to \ref psm_init_part_t structure having
 * partition details. PSMv2 won't need the user copy of any field in
 * psm_init_part after call returns.
 * @param[out] phandle Pointer to the handle \ref psm_hnd_t. Populated by PSM.
 *
 * @return SYS_OK Init success.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_NOMEM Memory allocation failure.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_module_init_ex(const psm_init_part_t *psm_init_part, psm_hnd_t *phandle);

/**
 * Open an existing or a new PSM object
 *
 * This API allows the caller to open a new or existing PSM object.
 * This call is essential before called psm_object_read() or
 * psm_object_write().
 *
 * \note If write mode is specified and if an object with the same
 * name is already present, the existing object will be deleted
 * and replaced with the new one.
 *
 * @param[in] phandle Handle returned in earlier call to psm_module_init()
 * @param[in] name NULL terminated ASCII name of PSM object.
 * @param[in] mode Specify read mode (\ref PSM_MODE_READ) or a write
 * mode(\ref PSM_MODE_WRITE).
 * @param[in] max_data_len Valid only for write mode. This specifies the
 * maximum length of the data the caller expect to write across single or
 * multiple calls of psm_object_write(). It is okay if the caller later
 * actually writes less number of bytes to be object than max_data_len
 * given here. For e.g. Giving 'max_data_len' 20 but writing only 4 bytes
 * is perfectly alright. The remaining  16 bytes will be released to the
 * free space pool once this object is closed.
 * @param[in] attribs Additional attributes as per \ref psm_objattrib_t.
 * Pass NULL if not required.
 * @param[out] ohandle Pointer to Object handle. Will be populated by
 * psm_object_open() if open operation was successful. This needs to be
 * passed to other psm read/write API's.
 *
 * @return For read mode: If open is successful size of the object is returned.
 *         For write mode: If open is successful SYS_OK is returned.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_NOMEM Memory allocation failure.
 * @return -WM_E_NOSPC If enough space is unavailable for the new
 * object. This error can only be returned in write mode.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_object_open(psm_hnd_t phandle,
                    const char *name,
                    psm_mode_t mode,
                    uint32_t max_data_len,
                    psm_objattrib_t *attribs,
                    psm_object_handle_t *ohandle);

/**
 * Read data of an existing and opened object
 *
 * This API allows the caller to read data from an object. This
 * call can be invoked as many times as the caller wants till the data read
 * finishes. Offset from where data is read is automatically incremented
 * depending on the size read. When data read is over, zero is returned.
 *
 * @pre psm_object_open()
 *
 * @note To avoid double pass over flash data area, CRC calculations are done
 * on-the-go as data is being read progressively by the caller. CRC
 * verification is done only in the last call to psm_object_read(). The
 * downside to this is that whole object needs to be read by caller before
 * object integrity can be confirmed.
 *
 * @note This API will block if other writers are currently using
 * the PSM in write mode.
 *
 * @param[in] ohandle Handle returned from call to psm_object_open().
 * @param[out] buf Destination buffer of data read from flash. Ensure that
 * it is atleast equal to max_len
 * @param[in] max_len Length of the buffer passed as second parameter. Data
 * read can be less than this value is end of object is reached.
 *
 * @return Non-zero positive value equal to number of bytes written to user
 * given buffer
 * @return 0 No more data available.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_CRC If CRC check failed.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_object_read(psm_object_handle_t ohandle, void *buf, uint32_t max_len);

/**
 * Write data to an opened PSM object.
 *
 * This API allows the caller to write data to an object. This
 * call can be invoked as many times as the caller wants as long as total
 * size written is less that 'max_len' specified during
 * psm_object_open(). Data passed in every invocation is appended to the
 * existing data already written.
 *
 * @pre psm_object_open()
 *
 * @note This API will block if other readers/writers are currently using
 * the PSM.
 *
 * @param[in] ohandle Handle returned from call to psm_object_open().
 * @param[in] buf Pointer to buffer containing data to be written to flash.
 * @param[in] len Length of the data in the buffer passed.
 *
 * @return SYS_OK Data was written successfully.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_PERM If object was opened in read mode.
 * @return -WM_E_NOSPC If caller tries to write more data than specified in
 * call to psm_object_open().
 * @return SYS_FAIL If any other error occurs.
 */
int psm_object_write(psm_object_handle_t ohandle, const void *buf, uint32_t len);

/**
 * Close the PSM object
 *
 * This API will close the earlier opened PSM object. The object
 * is finalized in the flash only after this call.
 *
 * @pre psm_object_open()
 *
 * @param[in] ohandle Handle returned from earlier call to
 * psm_object_open()
 *
 * @return SYS_OK Object was closed successfully.
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_object_close(psm_object_handle_t *ohandle);

/**
 * Delete a PSM object
 *
 * This API deletes an object from the PSM.
 *
 * @param[in] phandle Handle returned from earlier call to
 * psm_module_init()
 * @param[in] name NULL terminated ASCII name of the PSM object to be
 * deleted.
 *
 * @note This API will block if other readers/writers are currently using
 * the PSM.
 *
 * @return SYS_OK Object was deleted successfully.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_NOSPC If caller tries to write more data than specified in
 * call to psm_object_open().
 * @return SYS_FAIL If any other error occurs.
 */
int psm_object_delete(psm_hnd_t phandle, const char *name);

/**
 * Format the PSM partition
 *
 * This API allows the caller to erase and initialize the PSM
 * partition. All the existing objects will be erased.
 *
 * @note This API will block if other readers/writers are currently using
 * the PSM.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 *
 * @return SYS_OK Format operation was successful.
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_format(psm_hnd_t phandle);

/**
 * This callback will be invoked 'n' times if there are 'n' total variables
 * in the PSM. The user can call any API to read the PSM variables in the
 * callback. Any write operation to PSM will cause a deadlock if done in
 * the callback implementation.
 *
 * If the callback implementation returns a non-zero value no futher
 * callbacks will be given. This can be useful when the callback has found
 * what it was searching for and does not need remaining variables. The
 * callback is invoked till all variables are done or callback returns
 * SYS_OK, whichever comes first.
 */
typedef int (*psm_list_cb)(const uint8_t *name);

/**
 * Print all the objects and their data to console.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 * @param[in] list_cb Callback invoked by this API to pass on variable
 * names one by one to the user.
 *
 * @return void
 */
void psm_objects_list(psm_hnd_t phandle, psm_list_cb list_cb);

/*
 * Check if PSM module is encrypted.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 *
 * @return 'true' if PSM is encrypted or 'false' otherwise.
 */
bool is_psm_encrypted(psm_hnd_t phandle);

/**
 * Set a variable value pair
 *
 * This API is built on top of existing
 * psm_object_open(), psm_object_write() and psm_object_close()
 * APIs. This API is preferable when small sized objects
 * are being written.
 *
 * @note psm_object_open() and psm_object_close() are not required to be
 * called if this API is used.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 * @param[in] variable Name of the object.
 * @param[in] value Pointer to buffer having the data to be written
 * @param[in] len Length of the data to be written.
 *
 * @return SYS_OK Variable-value pair was set successfully.
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_set_variable(psm_hnd_t phandle, const char *variable, const void *value, uint32_t len);

/**
 * Helper function to simplify set of NULL terminated value.
 *
 * psm_set_variable_str() will self calculate the length of the NULL
 * terminated string sent by caller.
 *
 * @param[in] phandle See documentation of psm_set_variable()
 * @param[in] variable See documentation of psm_set_variable()
 * @param[in] value See documentation of psm_set_variable()
 *
 * @return See documentation of psm_set_variable()
 */
static inline int psm_set_variable_str(psm_hnd_t phandle, const char *variable, const void *value)
{
    return psm_set_variable(phandle, variable, value, strlen((const char *)value));
}

/**
 * Helper function to set integer value to PSM
 *
 * This function will convert given int to string representation and store
 * it to PSM. psm_get_variable_int() can be used to read the value back
 * again in int format.
 *
 * for e.g. 1234 given by the user will be stored as "1234"
 *
 * @param[in] phandle See documentation of psm_get_variable()
 * @param[in] variable See documentation of psm_get_variable()
 * @param[out] value The value to be stored to PSM
 *
 * @return See documentation of psm_set_variable()
 */
static inline int psm_set_variable_int(psm_hnd_t phandle, const char *variable, int value)
{
    char int_s[14];

    snprintf(int_s, sizeof(int_s), "%d", value);
    return psm_set_variable_str(phandle, variable, int_s);
}

/**
 * Get a variable value pair
 *
 * This API is built on top of existing
 * psm_object_open(), psm_object_read() and psm_object_close()
 * APIs. This API is preferable when small sized objects
 * are being read.
 *
 * @note psm_object_open() and psm_object_close() are not required to be
 * called if this API is used.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 * @param[in] variable Name of the object.
 * @param[out] value Pointer to buffer where value will be written by PSM.
 * @param[in] max_len Length of the buffer passed as third parameter. If
 * the buffer is not enough to fit the whole object error will be
 * returned. Partial read will not be done.
 *
 * @note Note that this function returns the length of the value. It will
 * \b not< NULL terminate the returned data.
 *
 * @return If operation is a success then length of the value field will be
 * returned.
 * @return SYS_E_INVAL Invalid arguments.
 * @return -WM_E_NOSPC If variable value exceeds buffer length given.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_get_variable(psm_hnd_t phandle, const char *variable, void *value, uint32_t max_len);

/**
 * Get size of a PSM variable
 *
 * No changes are done to the PSM.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 * @param[in] variable Name of the variable.
 *
 * @return Zero or positive value of size of variable if variable is present.
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL if object not found or any error occurred.
 */
int psm_get_variable_size(psm_hnd_t phandle, const char *variable);

/**
 * Check if a variable is present
 *
 * No changes are done to the PSM.
 *
 * @param[in] phandle Handle to PSM returned earlier from the call to
 * psm_module_init()
 * @param[in] variable Name of the variable.
 *
 * @return 'true' if variable is present.
 * @return 'false' if variable is absent or some other error.
 */
static inline bool psm_is_variable_present(psm_hnd_t phandle, const char *variable)
{
    return (psm_get_variable_size(phandle, variable) >= 0);
}

/**
 * Helper function to simplify get of NULL terminated value.
 *
 * This function will automatically NULL terminate the byte string
 * retrieved from PSM. One additional byte should be kept reserved by the
 * caller for this NULL terminator.
 *
 * @param[in] phandle See documentation of psm_get_variable()
 * @param[in] variable See documentation of psm_get_variable()
 * @param[out] value See documentation of psm_set_gariable()
 * @param[in] max_len See documentation of psm_get_variable()
 *
 * @return See documentation of psm_get_variable()
 */
static inline int psm_get_variable_str(psm_hnd_t phandle, const char *variable, void *value, uint32_t max_len)
{
    if (!max_len)
        return SYS_E_INVAL;

    uint8_t *valp = (uint8_t *)value;

    int ret = psm_get_variable(phandle, variable, valp, max_len - 1);
    if (ret >= 0) /* NULL terminate */
        valp[ret] = 0;

    return ret;
}

/**
 * Helper function to get integer value from PSM
 *
 * This function will automatically read string representation of a number
 * from PSM and convert it to int format before returning.
 *
 * for e.g. "1234" stored in PSM will be returned as 1234 in the user given
 * int variable.
 *
 * @param[in] phandle See documentation of psm_get_variable()
 * @param[in] variable See documentation of psm_get_variable()
 * @param[out] value The value returned.
 *
 * @return SYS_OK if retrieval was successful
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL If any other error occurs.
 */
static inline int psm_get_variable_int(psm_hnd_t phandle, const char *variable, int *value)
{
    char int_s[14];

    int ret = psm_get_variable_str(phandle, variable, int_s, sizeof(int_s));
    if (ret < 0)
        return ret;

    *value = strtol(int_s, NULL, 0);
    return SYS_OK;
}

/**
 * De-initialize the PSM module
 *
 * This API unloads the PSM module from handling the flash partition.
 *
 * @note This API will block till other readers/writers are currently using
 * the PSM.
 *
 * @param[in] phandle Handle to the PSM obtained earlier through
 * psm_module_init()
 *
 * @return SYS_OK Module de-init success.
 * @return SYS_E_INVAL Invalid arguments.
 * @return SYS_FAIL If any other error occurs.
 */
int psm_module_deinit(psm_hnd_t *phandle);

#endif /* __PSMV2_H__ */
