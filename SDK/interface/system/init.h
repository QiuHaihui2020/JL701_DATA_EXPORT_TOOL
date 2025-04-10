#ifndef SYS_INIT_H
#define SYS_INIT_H







typedef int (*initcall_t)(void);

#define __initcall(fn)  \
	const initcall_t __initcall_##fn sec(.initcall) = fn

#define early_initcall(fn)  \
	const initcall_t __initcall_##fn sec(.early.initcall) = fn


#define late_initcall(fn)  \
	const initcall_t __initcall_##fn sec(.late.initcall) = fn


#define platform_initcall(fn) \
	const initcall_t __initcall_##fn sec(.platform.initcall) = fn


#define module_initcall(fn) \
	const initcall_t __initcall_##fn sec(.module.initcall) = fn




#define __do_initcall(prefix) \
	do { \
		initcall_t *init; \
		extern initcall_t prefix##_begin[], prefix##_end[]; \
		for (init=prefix##_begin; init<prefix##_end; init++) { \
			(*init)();	\
		} \
	}while(0)


#define do_initcall()           __do_initcall(initcall)

#define do_early_initcall()     __do_initcall(early_initcall)

#define do_late_initcall()      __do_initcall(late_initcall)

#define do_platform_initcall()  __do_initcall(platform_initcall)

#define do_module_initcall()    __do_initcall(module_initcall)



typedef void (*uninitcall_t)(void);

#define platform_uninitcall(fn) \
	const uninitcall_t __uninitcall_##fn sec(.platform.uninitcall) = fn


#define __do_uninitcall(prefix) \
	do { \
		uninitcall_t *uninit; \
		extern uninitcall_t prefix##_begin[], prefix##_end[]; \
		for (uninit=prefix##_begin; uninit<prefix##_end; uninit++) { \
			(*uninit)();	\
		} \
	}while(0)

#define do_platform_uninitcall()  __do_uninitcall(platform_uninitcall)

#endif

