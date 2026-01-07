// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015-16  David Lamparter, for NetDEF, Inc.
 *
 * This file is part of Quagga
 */

#include <zebra.h>

#include "frrevent.h"
#include "memory.h"
#include "hash.h"
#include "log.h"
#include "qobj.h"
#include "jhash.h"
#include "network.h"

static uint32_t qobj_hash(const struct qobj_node *node)
{
	return (uint32_t)node->nid;
}

static int qobj_cmp(const struct qobj_node *na, const struct qobj_node *nb)
{
	if (na->nid < nb->nid)
		return -1;
	if (na->nid > nb->nid)
		return 1;
	return 0;
}

DECLARE_HASH(qobj_nodes, struct qobj_node, nodehash,
			qobj_cmp, qobj_hash);
/*
static inline __attribute__((unused)) void qobj_nodes_init(struct qobj_nodes_head * h) {
    memset(h, 0, sizeof( * h));
}
static inline __attribute__((unused)) void qobj_nodes_fini(struct qobj_nodes_head * h) {
    (void)((!!(h -> hh.count == 0)) || (_wassert(L "h->hh.count == 0", L "Z:\\codebase\\FRRouting\\frr-frr-10.0.1\\lib\\qobj.c", 33), 0));
    h -> hh.minshift = 0;
    typesafe_hash_shrink( & h -> hh);
    memset(h, 0, sizeof( * h));
}
static inline __attribute__((unused)) struct qobj_node * qobj_nodes_add(struct qobj_nodes_head * h, struct qobj_node * item) {
    h -> hh.count++;
    if (!h -> hh.tabshift || ((h -> hh).count >= ({
            do {
                if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
            } while (0);
            (1 U << ((h -> hh).tabshift)) >> 1;
        }))) typesafe_hash_grow( & h -> hh);
    uint32_t hval = qobj_hash(item), hbits = ({
        do {
            if (!(((h -> hh).tabshift) >= 2 && ((h -> hh).tabshift) <= 31)) __builtin_unreachable();
        } while (0);
        (hval) >> (33 - ((h -> hh).tabshift));
    });
    item -> nodehash.hi.hashval = hval;
    struct thash_item ** np = & h -> hh.entries[hbits];
    while ( * np && ( * np) -> hashval < hval) np = & ( * np) -> next;
    while ( * np && ( * np) -> hashval == hval) {
        if (qobj_cmp((__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof( * np)) || __builtin_types_compatible_p(void * , typeof( * np)), ({
                typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )( * np);
                (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }), ({
                typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = ( * np);
                (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }))), item) == 0) {
            h -> hh.count--;
            return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof( * np)) || __builtin_types_compatible_p(void * , typeof( * np)), ({
                typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )( * np);
                (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }), ({
                typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = ( * np);
                (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            })));
        }
        np = & ( * np) -> next;
    }
    item -> nodehash.hi.next = * np;* np = & item -> nodehash.hi;
    return ((void * ) 0);
}
static inline __attribute__((unused)) const struct qobj_node * qobj_nodes_const_find(const struct qobj_nodes_head * h,
    const struct qobj_node * item) {
    if (!h -> hh.tabshift) return ((void * ) 0);
    uint32_t hval = qobj_hash(item), hbits = ({
        do {
            if (!(((h -> hh).tabshift) >= 2 && ((h -> hh).tabshift) <= 31)) __builtin_unreachable();
        } while (0);
        (hval) >> (33 - ((h -> hh).tabshift));
    });
    const struct thash_item * hitem = h -> hh.entries[hbits];
    while (hitem && hitem -> hashval < hval) hitem = hitem -> next;
    while (hitem && hitem -> hashval == hval) {
        if (!qobj_cmp((__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(hitem)) || __builtin_types_compatible_p(void * , typeof(hitem)), ({
                typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(hitem);
                (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }), ({
                typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (hitem);
                (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }))), item)) return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(hitem)) || __builtin_types_compatible_p(void * , typeof(hitem)), ({
            typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(hitem);
            (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        }), ({
            typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (hitem);
            (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        })));
        hitem = hitem -> next;
    }
    return ((void * ) 0);
}
static inline __attribute__((unused)) struct qobj_node * qobj_nodes_find(struct qobj_nodes_head * h,
    const struct qobj_node * item) {
    return (struct qobj_node * ) qobj_nodes_const_find(h, item);
}
static inline __attribute__((unused)) struct qobj_node * qobj_nodes_del(struct qobj_nodes_head * h, struct qobj_node * item) {
    if (!h -> hh.tabshift) return ((void * ) 0);
    uint32_t hval = item -> nodehash.hi.hashval, hbits = ({
        do {
            if (!(((h -> hh).tabshift) >= 2 && ((h -> hh).tabshift) <= 31)) __builtin_unreachable();
        } while (0);
        (hval) >> (33 - ((h -> hh).tabshift));
    });
    struct thash_item ** np = & h -> hh.entries[hbits];
    while ( * np && ( * np) -> hashval < hval) np = & ( * np) -> next;
    while ( * np && * np != & item -> nodehash.hi && ( * np) -> hashval == hval) np = & ( * np) -> next;
    if ( * np != & item -> nodehash.hi) return ((void * ) 0);* np = item -> nodehash.hi.next;
    item -> nodehash.hi.next = ((void * ) 0);
    h -> hh.count--;
    if (((h -> hh).count <= (({
            do {
                if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
            } while (0);
            (1 U << ((h -> hh).tabshift)) >> 1;
        }) - 1) / 2)) typesafe_hash_shrink( & h -> hh);
    return item;
}
static inline __attribute__((unused)) struct qobj_node * qobj_nodes_pop(struct qobj_nodes_head * h) {
    uint32_t i;
    for (i = 0; i < ({
            do {
                if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
            } while (0);
            (1 U << ((h -> hh).tabshift)) >> 1;
        }); i++)
        if (h -> hh.entries[i]) {
            struct thash_item * hitem = h -> hh.entries[i];
            h -> hh.entries[i] = hitem -> next;
            h -> hh.count--;
            hitem -> next = ((void * ) 0);
            if (((h -> hh).count <= (({
                    do {
                        if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
                    } while (0);
                    (1 U << ((h -> hh).tabshift)) >> 1;
                }) - 1) / 2)) typesafe_hash_shrink( & h -> hh);
            return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(hitem)) || __builtin_types_compatible_p(void * , typeof(hitem)), ({
                typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(hitem);
                (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            }), ({
                typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (hitem);
                (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
            })));
        } return ((void * ) 0);
}
static inline __attribute__((unused)) void qobj_nodes_swap_all(struct qobj_nodes_head * a, struct qobj_nodes_head * b) {
    struct qobj_nodes_head tmp = * a;* a = * b;* b = tmp;
}
static inline __attribute__((unused, pure)) const struct qobj_node * qobj_nodes_const_first(const struct qobj_nodes_head * h) {
    uint32_t i;
    for (i = 0; i < ({
            do {
                if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
            } while (0);
            (1 U << ((h -> hh).tabshift)) >> 1;
        }); i++)
        if (h -> hh.entries[i]) return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(h -> hh.entries[i])) || __builtin_types_compatible_p(void * , typeof(h -> hh.entries[i])), ({
            typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(h -> hh.entries[i]);
            (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        }), ({
            typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (h -> hh.entries[i]);
            (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        })));
    return ((void * ) 0);
}
static inline __attribute__((unused, pure)) const struct qobj_node * qobj_nodes_const_next(const struct qobj_nodes_head * h,
    const struct qobj_node * item) {
    const struct thash_item * hitem = & item -> nodehash.hi;
    if (hitem -> next) return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(hitem -> next)) || __builtin_types_compatible_p(void * , typeof(hitem -> next)), ({
        typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(hitem -> next);
        (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
    }), ({
        typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (hitem -> next);
        (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
    })));
    uint32_t i = ({
        do {
            if (!(((h -> hh).tabshift) >= 2 && ((h -> hh).tabshift) <= 31)) __builtin_unreachable();
        } while (0);
        (hitem -> hashval) >> (33 - ((h -> hh).tabshift));
    }) + 1;
    for (; i < ({
            do {
                if (!(((h -> hh).tabshift) <= 31)) __builtin_unreachable();
            } while (0);
            (1 U << ((h -> hh).tabshift)) >> 1;
        }); i++)
        if (h -> hh.entries[i]) return (__builtin_choose_expr(__builtin_types_compatible_p(typeof( & ((struct qobj_node * ) 0) -> nodehash.hi), typeof(h -> hh.entries[i])) || __builtin_types_compatible_p(void * , typeof(h -> hh.entries[i])), ({
            typeof(((struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (void * )(h -> hh.entries[i]);
            (struct qobj_node * )((char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        }), ({
            typeof(((const struct qobj_node * ) 0) -> nodehash.hi) * __mptr = (h -> hh.entries[i]);
            (const struct qobj_node * )((const char * ) __mptr - __builtin_offsetof(struct qobj_node, nodehash.hi));
        })));
    return ((void * ) 0);
}
static inline __attribute__((unused, pure)) struct qobj_node * qobj_nodes_first(struct qobj_nodes_head * h) {
    return (struct qobj_node * ) qobj_nodes_const_first(h);
}
static inline __attribute__((unused, pure)) struct qobj_node * qobj_nodes_next(struct qobj_nodes_head * h, struct qobj_node * item) {
    return (struct qobj_node * ) qobj_nodes_const_next(h, item);
}
static inline __attribute__((unused, pure)) struct qobj_node * qobj_nodes_next_safe(struct qobj_nodes_head * h, struct qobj_node * item) {
    if (!item) return ((void * ) 0);
    return qobj_nodes_next(h, item);
}
static inline __attribute__((unused, pure)) size_t qobj_nodes_count(const struct qobj_nodes_head * h) {
    return h -> hh.count;
}
static inline __attribute__((unused, pure)) _Bool qobj_nodes_member(const struct qobj_nodes_head * h,
    const struct qobj_node * item) {
    if (!h -> hh.tabshift) return ((void * ) 0);
    uint32_t hval = item -> nodehash.hi.hashval, hbits = ({
        do {
            if (!(((h -> hh).tabshift) >= 2 && ((h -> hh).tabshift) <= 31)) __builtin_unreachable();
        } while (0);
        (hval) >> (33 - ((h -> hh).tabshift));
    });
    const struct thash_item * hitem = h -> hh.entries[hbits];
    while (hitem && hitem -> hashval < hval) hitem = hitem -> next;
    for (hitem = h -> hh.entries[hbits]; hitem && hitem -> hashval <= hval; hitem = hitem -> next)
        if (hitem == & item -> nodehash.hi) return 1;
    return 0;
}
_Static_assert(1, "please add a semicolon after this macro")
*/

static pthread_rwlock_t nodes_lock;
static struct qobj_nodes_head nodes = { };


void qobj_reg(struct qobj_node *node, const struct qobj_nodetype *type)
{
	node->type = type;
	pthread_rwlock_wrlock(&nodes_lock);
	do {
		node->nid = (uint64_t)frr_weak_random();
		node->nid ^= (uint64_t)frr_weak_random() << 32;
	} while (!node->nid || qobj_nodes_find(&nodes, node));
	qobj_nodes_add(&nodes, node);
	pthread_rwlock_unlock(&nodes_lock);
}

void qobj_unreg(struct qobj_node *node)
{
	pthread_rwlock_wrlock(&nodes_lock);
	qobj_nodes_del(&nodes, node);
	pthread_rwlock_unlock(&nodes_lock);
}

struct qobj_node *qobj_get(uint64_t id)
{
	struct qobj_node dummy = {.nid = id}, *rv;
	pthread_rwlock_rdlock(&nodes_lock);
	rv = qobj_nodes_find(&nodes, &dummy);
	pthread_rwlock_unlock(&nodes_lock);
	return rv;
}

void *qobj_get_typed(uint64_t id, const struct qobj_nodetype *type)
{
	struct qobj_node dummy = {.nid = id};
	struct qobj_node *node;
	void *rv;

	pthread_rwlock_rdlock(&nodes_lock);
	node = qobj_nodes_find(&nodes, &dummy);

	/* note: we explicitly hold the lock until after we have checked the
	 * type.
	 * if the caller holds a lock that for example prevents the deletion of
	 * route-maps, we can still race against a delete of something that
	 * isn't
	 * a route-map. */
	if (!node || node->type != type)
		rv = NULL;
	else
		rv = (char *)node - node->type->node_member_offset;

	pthread_rwlock_unlock(&nodes_lock);
	return rv;
}

void qobj_init(void)
{
	pthread_rwlock_init(&nodes_lock, NULL);
	qobj_nodes_init(&nodes);
}

void qobj_finish(void)
{
	struct qobj_node *node;
	while ((node = qobj_nodes_pop(&nodes)))
		qobj_nodes_del(&nodes, node);
	pthread_rwlock_destroy(&nodes_lock);
}
