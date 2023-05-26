#ifndef __PSM_LIST_H__
#define __PSM_LIST_H__

#include "sys_assert.h"

/*!
 * @addtogroup psm
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/**
 * @brief Get PSM list object structure pointer.
 */
#define PSM_LIST_OBJ(type, field, list) (type)((uint32_t)list - (uint32_t)(&((type)0)->field))

/**
 * @brief PSM list fields
 */
typedef struct _psm_list
{
    struct _psm_list *prev; /*!< previous list node */
    struct _psm_list *next; /*!< next list node */
} psm_list_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/*!
 * @brief Initialize PSM list head.
 *
 * @param list PSM list head pointer.
 */
static inline void PSM_List_Init(psm_list_t *list)
{
    ASSERT(list);

    list->prev = list;
    list->next = list;
}

/*!
 * @brief Check whether PSM list is empty.
 *
 * @param list PSM list head pointer.
 * @return TRUE when list is empty, FALSE otherwise.
 */
static inline bool PSM_List_IsEmpty(psm_list_t *list)
{
    ASSERT(list);

    return list->next == list;
}

/*!
 * @brief Add list node at list head.
 *
 * @param list PSM list head pointer.
 * @param node PSM list node pointer to add.
 */
static inline void PSM_List_AddHead(psm_list_t *list, psm_list_t *node)
{
    ASSERT(list);
    ASSERT(node);

    node->next       = list->next;
    node->prev       = list;
    list->next->prev = node;
    list->next       = node;
}

/*!
 * @brief Add list node at list tail.
 *
 * @param list PSM list head pointer.
 * @param node PSM list node pointer to add.
 */
static inline void PSM_List_AddTail(psm_list_t *list, psm_list_t *node)
{
    ASSERT(list);
    ASSERT(node);

    node->prev       = list->prev;
    node->next       = list;
    list->prev->next = node;
    list->prev       = node;
}

/*!
 * @brief Insert list node before another.
 *
 * @param anchor PSM list anchor node pointer.
 * @param node PSM list node pointer to insert.
 */
static inline void PSM_List_InsertBefore(psm_list_t *anchor, psm_list_t *node)
{
    PSM_List_AddTail(anchor, node);
}

/*!
 * @brief Insert list node after another.
 *
 * @param anchor PSM list anchor node pointer.
 * @param node PSM list node pointer to insert.
 */
static inline void PSM_List_InsertAfter(psm_list_t *anchor, psm_list_t *node)
{
    PSM_List_AddHead(anchor, node);
}

/*!
 * @brief Remove list node from list.
 *
 * @param node PSM list node pointer to remove.
 */
static inline void PSM_List_Remove(psm_list_t *node)
{
    ASSERT(node);

    node->prev->next = node->next;
    node->next->prev = node->prev;
    /* clear node */
    PSM_List_Init(node);
}

#ifdef __cplusplus
}
#endif

/*! @} */

#endif /* __PSM_LIST_H__ */
