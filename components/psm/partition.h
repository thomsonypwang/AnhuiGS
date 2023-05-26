#ifndef _PARTITION_H_
#define _PARTITION_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "sys_errno.h"
#include "sys_log.h"
#include "qspi_flash.h"
#include "flash_drv.h"

#define MAX_FL_COMP 16
#define MAX_NAME    8

 /** Flash base address */
#define QSPI_FLASH_BASE	0    
/** Section: Secondary stage boot-loader with bootrom header
 *  Start:   0x0
 *  Length:  0x5000(16KiB)
 *  Device:  Internal Flash
 */
#define FL_BOOT_START 		(QSPI_FLASH_BASE + 0x0)
#define FL_BOOT_BLOCK_SIZE 	0x5000
#define FL_BOOT_BLOCK_END 	(FL_BOOT_START + FL_BOOT_BLOCK_SIZE)
#define FL_BOOT_DEV			FL_INT

/** Section: Partition table 1
 *  Start:   0x5000
 *  Length:  0x1000(4KiB)
 *  Device:  Internal Flash
 */
#define FL_PART_SIZE 		W25Q64_SECTOR_SIZE
#define FL_PART1_START 		FL_BOOT_BLOCK_END
#define FL_PART1_TABLE_END (FL_PART1_START + FL_PART_SIZE)
#define FL_PART_DEV			FL_INT

/** Partition Table */
struct partition_table
{
	#define PARTITION_TABLE_MAGIC (('W' << 0) | ('M' << 8) | ('P' << 16) | ('T' << 24))
    /** The magic identifying the start of the partition table */
    uint32_t magic;
	#define PARTITION_TABLE_VERSION 1
    /** The version number of this partition table */
    uint16_t version;
    /** The number of partition entries that follow this */
    uint16_t partition_entries_no;
    /** Generation level */
    uint32_t gen_level;
    /** The CRC of all the above components */
    uint32_t crc;
};

/** Partition Entry */
struct partition_entry
{
    /** The type of the flash component */
    uint8_t type;
    /** The device id, internal flash is always id 0 */
    uint8_t device;
    /** A descriptive component name */
    char name[MAX_NAME];
    /** Start address on the given device */
    uint32_t start;
    /** Size on the given device */
    uint32_t size;
    /** Generation level */
    uint32_t gen_level;
};

/** The various components in a flash layout */
enum flash_comp
{
    /** The secondary stage boot loader to assist firmware bootup */
    FC_COMP_BOOT2 = 0,
    /** The firmware image. There can be a maximum of two firmware
     * components available in a flash layout. These will be used in an
     * active-passive mode if rfget module is enabled.
     */
    FC_COMP_FW,
    /** The wlan firmware image. There can be one wlan firmware image in the
     * system. The contents of this location would be downloaded to the WLAN
     * chip.
     */
    FC_COMP_WLAN_FW,
    /** The FTFS image. */
    FC_COMP_FTFS,
    /** The PSM data */
    FC_COMP_PSM,
    /** Application Specific component */
    FC_COMP_USER_APP,
    /** The BT firmware image if external BT/BLE chip is used */
    FC_COMP_BT_FW,
};

/** The flash descriptor
 *
 * All components that work with flash refer to the flash descriptor. The flash
 * descriptor captures the details of the component resident in flash.
 */
typedef struct flash_desc
{
    /** The flash device */
    uint8_t fl_dev;
    /** The start address on flash */
    uint32_t fl_start;
    /** The size on flash  */
    uint32_t fl_size;
} flash_desc_t;

/** Initialize the partitioning module
 *
 * \return SYS_OK on success or error code
 */
int part_init(void);

/** Update the partitioning table
 *
 * \return SYS_OK on success or error code
 */
int part_write_layout(void);

/** Convert from partition to flash_descriptor
 *
 * All the components that own a portion of flash in the SDK use the flash
 * descriptor function to identify this portion. This function converts a
 * partition_entry into a corresponding flash descriptor that can be passed to
 * these modules.
 *
 * \param[in] p a pointer to a partition entry
 * \param[out] f a pointer to a flash descriptor
 */
void part_to_flash_desc(struct partition_entry *p, flash_desc_t *f);

/** Get partition entry by ID
 *
 * Retrieve the pointer to a partition entry using an ID. This function can be
 * called to repetitively to retrieve more entries of the same type. The value
 * of start_index is modified by the function so that search starts from this
 * index in the next call.
 *
 * \param [in] comp the flash component to search for
 * \param [in,out] start_index the start index for the search. The first call to
 * this function should always have a start index of 0.
 *
 * \return a pointer to a partition entry on success, null otherwise.
 */
struct partition_entry *part_get_layout_by_id(enum flash_comp comp, short *start_index);

/** Get partition entry by name
 *
 * Retrieve the pointer to a partition entry using a name. This function can be
 * called to repetitively to retrieve more entries of the same type. The value
 * of start_index is modified by the function so that search starts from this
 * index in the next call.
 *
 * \param [in] name the flash component's name as mentioned in the layout.
 * \param [in,out] start_index the start index for the search. The first call to
 * this function should always have a start index of 0.
 *
 * \return a pointer to a partition entry on success, null otherwise.
 */
struct partition_entry *part_get_layout_by_name(const char *name, short *start_index);

struct partition_entry *part_get_active_partition_by_name(const char *name);
struct partition_entry *part_get_passive_partition_by_name(const char *name);

/** Find the active partition
 *
 * Given two partition entries that are upgrade buddies (that is, have the same
 * friendly name), this function returns the partition that is currently marked
 * as active.
 * \param[in] p1 Pointer to a partition entry
 * \param[in] p2 Pointer to the upgrade buddy for the first partition entry
 *
 * \return a pointer to the active partition entry on success, null otherwise.
 */
struct partition_entry *part_get_active_partition(struct partition_entry *p1, struct partition_entry *p2);

/** Set active partition
 *
 * Given a pointer to a currently passive partition entry, this function marks
 * this partition entry as active.
 *
 * \param[in] p Pointer to partition entry object
 *
 * \return SYS_E_INVAL if invalid parameter.
 * \return SYS_OK if operation successful.
 */
int part_set_active_partition(struct partition_entry *p);

static inline int part_get_desc_from_id(enum flash_comp comp, flash_desc_t *fl)
{
    if (!fl)
	{
		return SYS_E_INVAL;
	}
    int rv = part_init();
    if (rv != 0)
	{
		return rv;
	}

    struct partition_entry *p = part_get_layout_by_id(comp, NULL);
    if (!p)
	{
		return SYS_FAIL;
	}

    part_to_flash_desc(p, fl);

    return SYS_OK;
}

/* Check if the flash descriptor is within a partition in partition table.
 *
 * When return true, the input flash_desc_t 'f' can be either a subset of
 * a partition's flash space or exactly the same as this partition flash
 * space itself.
 *
 * \param[in] f Pointer to a flash descriptor for a flash space range.
 *
 * \return true if 'f' is part of an existing partition flash space.
 * \return false if not.
 */
bool part_is_flash_desc_within_one_partition(flash_desc_t *f);

#endif /* _PARTITION_H_ */
