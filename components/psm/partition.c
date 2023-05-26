#include "psm_crc32.h"
#include "partition.h"
#include "sys_os.h"

#define prt_e(...)	log_e("partition", ##__VA_ARGS__)
#define prt_w(...)	log_w("partition", ##__VA_ARGS__)
#ifdef DBG_ENABLE_PART
	#define prt_d(...)	log("partition", ##__VA_ARGS__)
#else
	#define prt_d(...)	
#endif 

#define PART_ENTRY_SIZE		sizeof(struct partition_entry)
#define PART_TABLE_SIZE		sizeof(struct partition_table)
	
struct partition_table g_flash_table;
struct partition_entry g_flash_parts[MAX_FL_COMP];
static uint32_t g_active_part_addr;

static uint32_t entries;


struct partition_entry part_data[5] = 
{
	{1,0,"boot2", 	0x0, 0x6000,1,},
	{1,0,"psm", 	0x6000, 0x100000,1,},
	{1,0,"mcufw", 	0x106000, 0x100000,1,},
	{1,0,"ftfs", 	0x206000, 0x200000,1,},
	{1,0,"mfg", 	0x406000, 0x200000,1,},
};

const char component[5][20] = 
{
	{"FC_COMP_BOOT2",},
	{"FC_COMP_PSM",},
	{"FC_COMP_FW",},
	{"FC_COMP_FTFS",},
	{"FC_COMP_USER_APP",},
};

int flash_get_comp(const char *comp)
{
	if (!strcmp(comp, "FC_COMP_FW"))
	{
		return FC_COMP_FW;
	}
	else if (!strcmp(comp, "FC_COMP_WLAN_FW"))
	{
		return FC_COMP_WLAN_FW;
	}
	else if (!strcmp(comp, "FC_COMP_FTFS"))
	{
		return FC_COMP_FTFS;
	}
	else if (!strcmp(comp, "FC_COMP_BOOT2"))
	{
		return FC_COMP_BOOT2;
	}
	else if (!strcmp(comp, "FC_COMP_PSM"))
	{
		return FC_COMP_PSM;
	}
	else if (!strcmp(comp, "FC_COMP_USER_APP"))
	{
		return FC_COMP_USER_APP;
	}
	else
	{
		prt_e("Error: Invalid flash component %s", comp);
		return SYS_FAIL;
	}
}

int create_layout_binary(void)
{
	int comp_ctr = 0, ret = -1, parts_no = 0;
	struct partition_entry *part_entry_ptr = &part_data[0];
	int i;
	int write_size;
	uint32_t part_addr =FL_PART1_START;
	
	memset(&g_flash_table, 0, PART_TABLE_SIZE);

	for(i=0;i<5;i++)
	{
		if (comp_ctr >= MAX_FL_COMP)
		{
			prt_e("Only %d partition entries are supported, Truncating...",MAX_FL_COMP);
			break;
		}
		if ((ret = flash_get_comp(component[i])) != -1)
		{
			part_entry_ptr[comp_ctr].type = ret;
		}
		else
		{
			return ret;
		}
		part_entry_ptr[comp_ctr].gen_level = 1;/* Default generation level for all partitions */
		parts_no++;
		comp_ctr++;
	}

	g_flash_table.magic = PARTITION_TABLE_MAGIC;
	g_flash_table.version = PARTITION_TABLE_VERSION;
	g_flash_table.partition_entries_no = parts_no;
	g_flash_table.gen_level = 0;

	/*
	* Write crc for partition table
	*
	* 32-bit crc is calculated for partition table
	* structure except last entity in that structure
	*/
	g_flash_table.crc = soft_crc32(&g_flash_table,PART_TABLE_SIZE - sizeof(g_flash_table.crc),0);

	/* Variable defined to improve readability of code */
	entries = g_flash_table.partition_entries_no;

	/* Get crc for partition entries */
	uint32_t part_entry_crc = soft_crc32(&part_entry_ptr[0],PART_ENTRY_SIZE * entries,0);
	
	flash_drv_erase(part_addr, FL_PART_SIZE);
	flash_drv_write(part_addr, (uint32_t *)&g_flash_table,PART_TABLE_SIZE);
	
	part_addr += PART_TABLE_SIZE;
	flash_drv_write(part_addr, (uint32_t *)&part_entry_ptr[0],PART_ENTRY_SIZE * entries);
	
	part_addr += PART_ENTRY_SIZE * entries;
	flash_drv_write(part_addr, (uint32_t *)&part_entry_crc,sizeof(part_entry_crc));

	os_thread_sleep(os_msec_to_ticks(100));	
	return 1;
}

static inline uint32_t part_get_table_addr_from_id(bool part_id)
{
    return FL_PART1_START;
}

static inline uint32_t part_get_passive_table_addr(void)
{
    return FL_PART1_START;
}

static int part_read_layout(void)
{
    int read_size, ret;
    uint32_t crc;
    uint32_t part_addr;

    part_addr = g_active_part_addr;
    crc32_init();

    ret = flash_drv_read(part_addr, (uint32_t *)&g_flash_table, sizeof(struct partition_table));
    if (ret != SYS_OK)
    {
		prt_e("Unable flash_drv_read 1");
        return SYS_E_IO;
    }

    crc = soft_crc32(&g_flash_table, sizeof(g_flash_table), 0);
    if (crc != 0)
    {
        prt_e("CRC mismatch");
        return SYS_E_CRC;
    }

    if (g_flash_table.magic != PARTITION_TABLE_MAGIC)
    {
        prt_e("magic number mismatch");
        return SYS_FAIL;
    }
    if (g_flash_table.version != PARTITION_TABLE_VERSION)
    {
        prt_e("Partition table version mismatch");
        return SYS_FAIL;
    }
    part_addr += sizeof(struct partition_table);

    if (g_flash_table.partition_entries_no > MAX_FL_COMP)
    {
        prt_w("Only %d partition entries are supported. Current = %d. Truncating ...",MAX_FL_COMP, g_flash_table.partition_entries_no);
        g_flash_table.partition_entries_no = MAX_FL_COMP;
    }

    read_size = g_flash_table.partition_entries_no * sizeof(struct partition_entry);
    ret = flash_drv_read(part_addr, (uint32_t *)g_flash_parts, read_size);
    if (ret != SYS_OK)
    {
		prt_e("Unable flash_drv_read 2");
        return SYS_E_IO;
    }
    return SYS_OK;
}

int part_init(void)
{
    static bool part_init_done;
	int ret;
	
	if (part_init_done == true)
	{
		return SYS_OK;
	}

    g_active_part_addr = part_get_table_addr_from_id(0);
    part_init_done     = true;
	ret=part_read_layout();
	if(ret!=SYS_OK)
	{
		create_layout_binary();
		return part_read_layout();
	}
	return ret;
}

int part_write_layout(void)
{
    /* Get and update passive partition table */
    uint32_t part_addr = part_get_passive_table_addr();
    uint32_t crc;
    int write_size;

    flash_drv_erase(part_addr, FL_PART_SIZE);

    /* Increment active partition table generation level this will get
     * written to passive address */
    ++g_flash_table.gen_level;

    /* Update checksum with updated generation level */
    crc               = soft_crc32((uint8_t *)&g_flash_table, sizeof(struct partition_table) - 4, 0);
    g_flash_table.crc = crc;

    flash_drv_write(part_addr, (uint32_t *)&g_flash_table, sizeof(struct partition_table));

    part_addr += sizeof(struct partition_table);

    if (g_flash_table.partition_entries_no > MAX_FL_COMP)
    {
        prt_w("Only %d partition entries are supported.Truncating ...",MAX_FL_COMP);
        g_flash_table.partition_entries_no = MAX_FL_COMP;
    }

    write_size = g_flash_table.partition_entries_no * sizeof(struct partition_entry);
    flash_drv_write(part_addr, (uint32_t *)g_flash_parts, write_size);
    part_addr += write_size;

    crc = soft_crc32((uint8_t *)g_flash_parts, write_size, 0);
    flash_drv_write(part_addr, (uint32_t *)&crc, sizeof(crc));

    return 0;
}

void part_to_flash_desc(struct partition_entry *p, flash_desc_t *f)
{
    f->fl_dev   = p->device;
    f->fl_start = p->start;
    f->fl_size  = p->size;
}

struct partition_entry *part_get_layout_by_id(enum flash_comp comp, short *start_index)
{
    int i = 0;

    if (start_index)
	{
		i = *start_index;
	}
    for (; i < g_flash_table.partition_entries_no; i++)
    {
        if (g_flash_parts[i].type == comp)
        {
            if (start_index)
			{
				*start_index = i + 1;
			}
            return &g_flash_parts[i];
        }
    }
    return NULL;
}

struct partition_entry *part_get_layout_by_name(const char *name, short *start_index)
{
    int i = 0;

    if (start_index)
	{
		i = *start_index;
	}
    for (; i < g_flash_table.partition_entries_no; i++)
    {
        if (!strcmp(g_flash_parts[i].name, name))
        {
            if (start_index)
			{
				*start_index = i + 1;
			}
            return &g_flash_parts[i];
        }
    }
    return NULL;
}

struct partition_entry *part_get_active_partition(struct partition_entry *p1, struct partition_entry *p2)
{
    /* Somehow there aren't two partition defined for this */
    if (!p1 || !p2)
	{
		return NULL;
	}
    return p1->gen_level >= p2->gen_level ? p1 : p2;
}

struct partition_entry *part_get_active_partition_by_name(const char *name)
{
    short index                = 0;
    struct partition_entry *p1 = part_get_layout_by_name(name, &index);
    struct partition_entry *p2 = part_get_layout_by_name(name, &index);

    return part_get_active_partition(p1, p2);
}

struct partition_entry *part_get_passive_partition(struct partition_entry *p1, struct partition_entry *p2)
{
    /* Somehow there aren't two partition defined for this */
    if (!p1 || !p2)
	{
		return NULL;
	}

    return p1->gen_level < p2->gen_level ? p1 : p2;
}

struct partition_entry *part_get_passive_partition_by_name(const char *name)
{
    short index                = 0;
    struct partition_entry *p1 = part_get_layout_by_name(name, &index);
    struct partition_entry *p2 = part_get_layout_by_name(name, &index);

    return part_get_passive_partition(p1, p2);
}

static bool part_match(struct partition_entry *p1, struct partition_entry *p2)
{
    if (p1->type == p2->type && p1->device == p2->device && p1->start == p2->start && p1->size == p2->size)
	{
		return true;
	}
    return false;
}

int part_set_active_partition(struct partition_entry *p)
{
    short index                = 0;
    struct partition_entry *p1 = part_get_layout_by_name(p->name, &index);
    struct partition_entry *p2 = part_get_layout_by_name(p->name, &index);

    /* Somehow there aren't two partition defined for this */
    if (!p1 || !p2)
	{
		return SYS_E_INVAL;
	}
    if (part_match(p1, p) == true)
	{
		p1->gen_level = p2->gen_level + 1;
	}       
    else if (part_match(p2, p) == true)
	{
		p2->gen_level = p1->gen_level + 1;
	}   
    else
	{
		return SYS_E_INVAL;
	}
    part_write_layout();
    return SYS_OK;
}

bool part_is_flash_desc_within_one_partition(flash_desc_t *f)
{
    int i;
    uint32_t start = f->fl_start;
    uint32_t end   = f->fl_start + f->fl_size;

    if (start >= end)
    {
        return false;
    }
    for (i = 0; i < g_flash_table.partition_entries_no; i++)
    {
        if ((start >= g_flash_parts[i].start) && (end <= (g_flash_parts[i].start + g_flash_parts[i].size)))
        {
            return true;
        }
    }
    return false;
}
