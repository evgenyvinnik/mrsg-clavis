/*
 * This is the function that performs scheduling of virtual machines
 * */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "msg/msg.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "common.h"
#include "dfs.h"
#include "mrsg.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);
int master(int argc, char *argv[]);
int worker(int argc, char *argv[]);
static void init_job(void);
static void init_stats(void);

#define VM_PER_HOST 1

int scheduler(int argc, char *argv[])
{
	int worker_hosts_count = WORKERS_NUMBER; //TODO: temporary, 1 master + 6 slaves in the future it would be dynamic
	msg_host_t * worker_hosts = xbt_new(msg_host_t, 10);
	int i, j;
	msg_vm_t vm;
	xbt_dynar_t vms;
	msg_process_t process;
	int wid;
	char vmName[64];

	srand(12345);

	master_host = MSG_get_host_by_name(argv[0]);
	xbt_assert(master_host, "UNABLE TO IDENTIFY THE MASTER NODE");

	/* Retrive the hostnames constituting our playground today */
	for (i = 1; i < argc; i++)
	{
		worker_hosts[i - 1] = MSG_get_host_by_name(argv[i]);
		xbt_assert(worker_hosts[i - 1] != NULL, "Cannot use inexistent host %s",
		        argv[i]);
	}

	config.grid_cpu_power = 0.0;
	wid = 0;
	//Launch 10 virtual machines: one VM per host
	//Launch the sub processes: two processes inside each virtual machine
	for (i = 0; i < worker_hosts_count; i++)
	{

		for (j = 0; j < VM_PER_HOST; j++)
		{
			snprintf(vmName, 64, "vm_%d", i);

			vm = MSG_vm_start(worker_hosts[i], vmName, 2);

			char**argv = xbt_new(char*,2);
			argv[0] = bprintf("%d", wid);
			argv[1] = NULL;
			process = MSG_process_create_with_arguments("worker", worker, vm,
			        worker_hosts[i], 1, argv);
			MSG_vm_bind(vm, process);
			//
			wid++;
			config.number_of_workers++;
			config.grid_cpu_power += MSG_get_host_speed(worker_hosts[i]);
		}
	}

	//init config
	config.grid_average_speed = config.grid_cpu_power
	        / config.number_of_workers;
	config.heartbeat_interval = maxval(3, config.number_of_workers / 100);
	config.number_of_maps = config.chunk_count;
	config.initialized = 1;

	w_heartbeat = xbt_new (struct heartbeat_s, config.number_of_workers);
	for (wid = 0; wid < config.number_of_workers; wid++)
	{
		w_heartbeat[wid].slots_av[MAP] = config.map_slots;
		w_heartbeat[wid].slots_av[REDUCE] = config.reduce_slots;
	}

	init_stats();
	init_job();
	distribute_data();
	MSG_process_create("master", master, NULL, master_host);

	vms = MSG_vms_as_dynar();
	XBT_INFO("Launched %ld VMs", xbt_dynar_length(vms));

	/*XBT_INFO("Now suspend all VMs, just for fun");

	xbt_dynar_foreach(vms,i,vm)
	{
		MSG_vm_suspend(vm);
	}

	XBT_INFO("Wait a while");
	MSG_process_sleep(1000);
	XBT_INFO("Enough. Let's resume everybody.");
	xbt_dynar_foreach(vms,i,vm)
	{
		MSG_vm_resume(vm);
	}

	XBT_INFO("Migrate everyone to the first host.");
	xbt_dynar_foreach(vms,i,vm)
	{
		MSG_vm_migrate(vm, worker_hosts[0]);
	}

	MSG_process_sleep(1000);
	XBT_INFO( "Suspend everyone, move them to the second host, and resume them.");
	xbt_dynar_foreach(vms,i,vm)
	{
		MSG_vm_suspend(vm);
		MSG_vm_migrate(vm, worker_hosts[1]);
		MSG_vm_resume(vm);
	}*/

	while (!job.finished)
	{
		MSG_process_sleep(config.heartbeat_interval);
	}

	xbt_dynar_foreach(vms,i,vm)
	{
		MSG_vm_shutdown(vm);
		MSG_vm_destroy(vm);
	}

	XBT_INFO("Goodbye now!");
	free(worker_hosts);
	xbt_dynar_free(&vms);

	return 0;
}

/**
 * @brief  Initialize the job structure.
 */
static void init_job(void)
{
	int i;

	xbt_assert(config.initialized,
	        "init_config has to be called before init_job");

	job.finished = 0;

	/* Initialize map information. */
	job.tasks_pending[MAP] = config.number_of_maps;
	job.task_status[MAP] = xbt_new0 (int, config.number_of_maps);
	job.task_has_spec_copy[MAP] = xbt_new0 (int, config.number_of_maps);
	job.task_list[MAP] = xbt_new0 (msg_task_t*, MAX_SPECULATIVE_COPIES);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		job.task_list[MAP][i] = xbt_new0 (msg_task_t, config.number_of_maps);

	job.map_output = xbt_new (size_t*, config.number_of_workers);
	for (i = 0; i < config.number_of_workers; i++)
		job.map_output[i] = xbt_new0 (size_t, config.number_of_reduces);

	/* Initialize reduce information. */
	job.tasks_pending[REDUCE] = config.number_of_reduces;
	job.task_status[REDUCE] = xbt_new0 (int, config.number_of_reduces);
	job.task_has_spec_copy[REDUCE] = xbt_new0 (int, config.number_of_reduces);
	job.task_list[REDUCE] = xbt_new0 (msg_task_t*, MAX_SPECULATIVE_COPIES);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		job.task_list[REDUCE][i] =
		        xbt_new0 (msg_task_t, config.number_of_reduces);
}

/**
 * @brief  Initialize the stats structure.
 */
static void init_stats(void)
{
	xbt_assert(config.initialized,
	        "init_config has to be called before init_stats");

	stats.map_local = 0;
	stats.map_remote = 0;
	stats.map_spec_l = 0;
	stats.map_spec_r = 0;
	stats.reduce_normal = 0;
	stats.reduce_spec = 0;
	stats.maps_processed = xbt_new0 (int, config.number_of_workers);
	stats.reduces_processed = xbt_new0 (int, config.number_of_workers);
}
