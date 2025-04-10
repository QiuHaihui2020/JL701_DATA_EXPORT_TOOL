#ifndef BTSTACK_TASK_H
#define BTSTACK_TASK_H

int btstack_init();
int btstack_exit();
int btstack_exit_edr();//关闭底层edr相关使用，不释放线程
int btstack_int_edr();

void ble_bqb_test_thread_init(void);

#endif
