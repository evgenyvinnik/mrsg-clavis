/*
 * This is the function that performs scheduling of virtual machines
 * */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "msg/msg.h"            /* Yeah! If you want to use msg, you need to include msg/msg.h */
#include "xbt/sysdep.h"         /* calloc, printf */

/* Create a log channel to have nice outputs. */
#include "xbt/log.h"
#include "xbt/asserts.h"


int scheduler(int argc, char *argv[])
{
//	int slaves_count = 10; //TODO: temporary, 1 master + 6 slaves in the future it would be dynamic
//	msg_host_t * slaves = xbt_new(msg_host_t, 10);
//
//	msg_vm_t vm;
//	unsigned int i;
//
//	/* Retrive the hostnames constituting our playground today */
//	for (i = 1; i < argc; i++)
//	{
//		slaves[i - 1] = MSG_get_host_by_name(argv[i]);
//		xbt_assert(slaves[i - 1] != NULL, "Cannot use inexistent host %s",
//		        argv[i]);
//	}
//
//
//
//
//	/* Launch the sub processes: one VM per host, with one process inside each */
//
//	for (i = 0; i < slaves_count; i++)
//	{
//		char slavename[64];
//		sprintf(slavename, "Slave %d", i);
//		char**argv = xbt_new(char*,3);
//		argv[0] = xbt_strdup(slavename);
//		argv[1] = bprintf("%d", i);
//		argv[2] = NULL;
//
//		char vmName[64];
//		snprintf(vmName, 64, "vm_%d", i);
//
//		msg_vm_t vm = MSG_vm_start(slaves[i], vmName, 2);
//		MSG_vm_bind(vm,
//		//TODO add already created processes
//		        MSG_process_create_with_arguments(slavename, slave_fun, NULL,
//		                slaves[i], 2, argv));
//	}
//
//	xbt_dynar_t vms = MSG_vms_as_dynar();
//	XBT_INFO("Launched %ld VMs", xbt_dynar_length(vms));
//
//	XBT_INFO("Now suspend all VMs, just for fun");
//
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_suspend(vm);
//	}
//
//	XBT_INFO("Wait a while");
//	MSG_process_sleep(2);
//
//	XBT_INFO("Enough. Let's resume everybody.");
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_resume(vm);
//	}
//	XBT_INFO(
//	        "Sleep long enough for everyone to be done with previous batch of work");
//	MSG_process_sleep(1000 - MSG_get_clock());
//
//	XBT_INFO("Add one more process per VM");
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		msg_vm_t vm = xbt_dynar_get_as(vms,i,msg_vm_t);
//		char slavename[64];
//		sprintf(slavename, "Slave %ld", i + xbt_dynar_length(vms));
//		char**argv = xbt_new(char*,3);
//		argv[0] = xbt_strdup(slavename);
//		argv[1] = bprintf("%ld", i + xbt_dynar_length(vms));
//		argv[2] = NULL;
//		MSG_vm_bind(vm,
//		        MSG_process_create_with_arguments(slavename, slave_fun, NULL,
//		                slaves[i], 2, argv));
//	}
//
//	XBT_INFO("Reboot all the VMs");
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_reboot(vm);
//	}
//
//	work_batch(slaves_count * 2);
//
//	XBT_INFO("Migrate everyone to the second host.");
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_migrate(vm, slaves[1]);
//	}
//	XBT_INFO("Suspend everyone, move them to the third host, and resume them.");
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_suspend(vm);
//		MSG_vm_migrate(vm, slaves[2]);
//		MSG_vm_resume(vm);
//	}
//
//	XBT_INFO(
//	        "Let's shut down the simulation. 10 first processes will be shut down cleanly while the second half will forcefully get killed");
//	for (i = 0; i < slaves_count; i++)
//	{
//		char mailbox_buffer[64];
//		sprintf(mailbox_buffer, "Slave_%d", i);
//		msg_task_t finalize = MSG_task_create("finalize", 0, 0, 0);
//		MSG_task_send(finalize, mailbox_buffer);
//	}
//
//	xbt_dynar_foreach(vms,i,vm)
//	{
//		MSG_vm_shutdown(vm);
//		MSG_vm_destroy(vm);
//	}
//
//	XBT_INFO("Goodbye now!");
//	free(slaves);
//	xbt_dynar_free(&vms);

	return 0;
}
