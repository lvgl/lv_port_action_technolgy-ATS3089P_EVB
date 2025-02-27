/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_SYS_KOBJECT_H
#define ZEPHYR_INCLUDE_SYS_KOBJECT_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_thread;
struct k_mutex;
struct z_futex_data;

/**
 * @brief Kernel Object Types
 *
 * This enumeration needs to be kept in sync with the lists of kernel objects
 * and subsystems in scripts/gen_kobject_list.py, as well as the otype_to_str()
 * function in kernel/userspace.c
 */
enum k_objects {
	K_OBJ_ANY,

	/** @cond
	 *  Doxygen should ignore this build-time generated include file
	 *  when genrating API documentation.  Enumeration values are
	 *  generated during build by gen_kobject_list.py.  It includes
	 *  basic kernel objects (e.g.  pipes and mutexes) and driver types.
	 */
#include <kobj-types-enum.h>
	/** @endcond
	 */

	K_OBJ_LAST
};
/**
 * @defgroup usermode_apis User Mode APIs
 * @ingroup kernel_apis
 * @{
 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_GEN_PRIV_STACKS
/* Metadata struct for K_OBJ_THREAD_STACK_ELEMENT */
struct z_stack_data {
	/* Size of the entire stack object, including reserved areas */
	size_t size;

	/* Stack buffer for privilege mode elevations */
	uint8_t *priv;
};
#endif /* CONFIG_GEN_PRIV_STACKS */

/* Object extra data. Only some objects use this, determined by object type */
union z_object_data {
	/* Backing mutex for K_OBJ_SYS_MUTEX */
	struct k_mutex *mutex;

	/* Numerical thread ID for K_OBJ_THREAD */
	unsigned int thread_id;

#ifdef CONFIG_GEN_PRIV_STACKS
	/* Metadata for K_OBJ_THREAD_STACK_ELEMENT */
	const struct z_stack_data *stack_data;
#else
	/* Stack buffer size for K_OBJ_THREAD_STACK_ELEMENT */
	size_t stack_size;
#endif /* CONFIG_GEN_PRIV_STACKS */

	/* Futex wait queue and spinlock for K_OBJ_FUTEX */
	struct z_futex_data *futex_data;

	/* All other objects */
	int unused;
};

/* Table generated by gperf, these objects are retrieved via
 * z_object_find() */
struct z_object {
	void *name;
	uint8_t perms[CONFIG_MAX_THREAD_BYTES];
	uint8_t type;
	uint8_t flags;
	union z_object_data data;
} __packed __aligned(4);

struct z_object_assignment {
	struct k_thread *thread;
	void * const *objects;
};

/**
 * @brief Grant a static thread access to a list of kernel objects
 *
 * For threads declared with K_THREAD_DEFINE(), grant the thread access to
 * a set of kernel objects. These objects do not need to be in an initialized
 * state. The permissions will be granted when the threads are initialized
 * in the early boot sequence.
 *
 * All arguments beyond the first must be pointers to kernel objects.
 *
 * @param name_ Name of the thread, as passed to K_THREAD_DEFINE()
 */
#define K_THREAD_ACCESS_GRANT(name_, ...) \
	static void * const _CONCAT(_object_list_, name_)[] = \
		{ __VA_ARGS__, NULL }; \
	static const STRUCT_SECTION_ITERABLE(z_object_assignment, \
					_CONCAT(_object_access_, name_)) = \
			{ (&_k_thread_obj_ ## name_), \
			  (_CONCAT(_object_list_, name_)) }

/** Object initialized */
#define K_OBJ_FLAG_INITIALIZED	BIT(0)
/** Object is Public */
#define K_OBJ_FLAG_PUBLIC	BIT(1)
/** Object allocated */
#define K_OBJ_FLAG_ALLOC	BIT(2)
/** Driver Object */
#define K_OBJ_FLAG_DRIVER	BIT(3)

/**
 * Lookup a kernel object and init its metadata if it exists
 *
 * Calling this on an object will make it usable from userspace.
 * Intended to be called as the last statement in kernel object init
 * functions.
 *
 * @param obj Address of the kernel object
 */
void z_object_init(const void *obj);
#else
/* LCOV_EXCL_START */
#define K_THREAD_ACCESS_GRANT(thread, ...)

/**
 * @internal
 */
static inline void z_object_init(const void *obj)
{
	ARG_UNUSED(obj);
}

/**
 * @internal
 */
static inline void z_impl_k_object_access_grant(const void *object,
						struct k_thread *thread)
{
	ARG_UNUSED(object);
	ARG_UNUSED(thread);
}

/**
 * @internal
 */
static inline void k_object_access_revoke(const void *object,
					  struct k_thread *thread)
{
	ARG_UNUSED(object);
	ARG_UNUSED(thread);
}

/**
 * @internal
 */
static inline void z_impl_k_object_release(const void *object)
{
	ARG_UNUSED(object);
}

static inline void k_object_access_all_grant(const void *object)
{
	ARG_UNUSED(object);
}
/* LCOV_EXCL_STOP */
#endif /* !CONFIG_USERSPACE */

/**
 * Grant a thread access to a kernel object
 *
 * The thread will be granted access to the object if the caller is from
 * supervisor mode, or the caller is from user mode AND has permissions
 * on both the object and the thread whose access is being granted.
 *
 * @param object Address of kernel object
 * @param thread Thread to grant access to the object
 */
__syscall void k_object_access_grant(const void *object,
				     struct k_thread *thread);

/**
 * Revoke a thread's access to a kernel object
 *
 * The thread will lose access to the object if the caller is from
 * supervisor mode, or the caller is from user mode AND has permissions
 * on both the object and the thread whose access is being revoked.
 *
 * @param object Address of kernel object
 * @param thread Thread to remove access to the object
 */
void k_object_access_revoke(const void *object, struct k_thread *thread);

/**
 * @brief Release an object
 *
 * Allows user threads to drop their own permission on an object
 * Their permissions are automatically cleared when a thread terminates.
 *
 * @param object The object to be released
 *
 */
__syscall void k_object_release(const void *object);

/**
 * Grant all present and future threads access to an object
 *
 * If the caller is from supervisor mode, or the caller is from user mode and
 * have sufficient permissions on the object, then that object will have
 * permissions granted to it for *all* current and future threads running in
 * the system, effectively becoming a public kernel object.
 *
 * Use of this API should be avoided on systems that are running untrusted code
 * as it is possible for such code to derive the addresses of kernel objects
 * and perform unwanted operations on them.
 *
 * It is not possible to revoke permissions on public objects; once public,
 * any thread may use it.
 *
 * @param object Address of kernel object
 */
void k_object_access_all_grant(const void *object);

/**
 * Allocate a kernel object of a designated type
 *
 * This will instantiate at runtime a kernel object of the specified type,
 * returning a pointer to it. The object will be returned in an uninitialized
 * state, with the calling thread being granted permission on it. The memory
 * for the object will be allocated out of the calling thread's resource pool.
 *
 * Currently, allocation of thread stacks is not supported.
 *
 * @param otype Requested kernel object type
 * @return A pointer to the allocated kernel object, or NULL if memory wasn't
 * available
 */
__syscall void *k_object_alloc(enum k_objects otype);

#ifdef CONFIG_DYNAMIC_OBJECTS
/**
 * Allocate memory and install as a generic kernel object
 *
 * This is a low-level function to allocate some memory, and register that
 * allocated memory in the kernel object lookup tables with type K_OBJ_ANY.
 * Initialization state and thread permissions will be cleared. The
 * returned z_object's data value will be uninitialized.
 *
 * Most users will want to use k_object_alloc() instead.
 *
 * Memory allocated will be drawn from the calling thread's reasource pool
 * and may be freed later by passing the actual object pointer (found
 * in the returned z_object's 'name' member) to k_object_free().
 *
 * @param align Required memory alignment for the allocated object
 * @param size Size of the allocated object
 * @return NULL on insufficient memory
 * @return A pointer to the associated z_object that is installed in the
 *	kernel object tables
 */
struct z_object *z_dynamic_object_aligned_create(size_t align, size_t size);

/**
 * Allocate memory and install as a generic kernel object
 *
 * This is a low-level function to allocate some memory, and register that
 * allocated memory in the kernel object lookup tables with type K_OBJ_ANY.
 * Initialization state and thread permissions will be cleared. The
 * returned z_object's data value will be uninitialized.
 *
 * Most users will want to use k_object_alloc() instead.
 *
 * Memory allocated will be drawn from the calling thread's reasource pool
 * and may be freed later by passing the actual object pointer (found
 * in the returned z_object's 'name' member) to k_object_free().
 *
 * @param size Size of the allocated object
 * @return NULL on insufficient memory
 * @return A pointer to the associated z_object that is installed in the
 *	kernel object tables
 */
static inline struct z_object *z_dynamic_object_create(size_t size)
{
	return z_dynamic_object_aligned_create(0, size);
}

/**
 * Free a kernel object previously allocated with k_object_alloc()
 *
 * This will return memory for a kernel object back to resource pool it was
 * allocated from.  Care must be exercised that the object will not be used
 * during or after when this call is made.
 *
 * @param obj Pointer to the kernel object memory address.
 */
void k_object_free(void *obj);
#else
/* LCOV_EXCL_START */
static inline void *z_impl_k_object_alloc(enum k_objects otype)
{
	ARG_UNUSED(otype);

	return NULL;
}

static inline struct z_object *z_dynamic_object_aligned_create(size_t align,
							       size_t size)
{
	ARG_UNUSED(align);
	ARG_UNUSED(size);

	return NULL;
}

static inline struct z_object *z_dynamic_object_create(size_t size)
{
	ARG_UNUSED(size);

	return NULL;
}

/**
 * @brief Free an object
 *
 * @param obj
 */
static inline void k_object_free(void *obj)
{
	ARG_UNUSED(obj);
}
/* LCOV_EXCL_STOP */
#endif /* CONFIG_DYNAMIC_OBJECTS */

/** @} */

#include <syscalls/kobject.h>
#ifdef __cplusplus
}
#endif

#endif
