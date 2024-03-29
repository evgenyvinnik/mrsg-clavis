/* Copyright (c) 2013. Evgeny Vinnik (evinnik@sfu.ca). All rights reserved. */

/* This file is part of MRSG-Clavis.

 MRSG-Clavis is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MRSG-Clavis is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MRSG-Clavis.  If not, see <http://www.gnu.org/licenses/>. */

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
#include "csv.h"
#include <errno.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

int hosts_ordering_function(const void* a, const void* b);
int vm_ordering_function(const void* a, const void* b);

int master(int argc, char *argv[]);
int worker(int argc, char *argv[]);
static void init_job(int configuration_id);
static void init_stats(int configuration_id);
static void free_global_mem(int configuration_id);
static void cb1(void *s, size_t len, void *data);
static void cb2(int c, void *data);

#define SCHEDULE_SLEEP_TIME 500

struct schedule
{
	xbt_dynar_t vms_ids;
	xbt_dynar_t hosts_ids;
	int row_reading;
	xbt_dynar_t vms;
	xbt_dynar_t hosts_dynar;
};

/**
 * This is the function that performs scheduling of virtual machines
 * */
int scheduler(int argc, char *argv[])
{
	srand(12345);
	xbt_dynar_t hosts_dynar;
	xbt_dynar_t vms;
	unsigned long physical_machines, hosts_length;
	msg_vm_t vm;
	msg_host_t host;
	msg_process_t process;
	size_t wid;
	char vmName[64];
	unsigned long i, j, id, host_count, vm_count, vm_count_local;
	unsigned int cursor;
	int conf_count;

	if(argc < 1)
	{
		XBT_INFO("Scheduling file was not specified");
	}

	/* Retrieve the first hosts of the platform file */
	hosts_dynar = MSG_hosts_as_dynar();
	xbt_dynar_sort(hosts_dynar, hosts_ordering_function);

	//get the number hosts required by the simulation:
	physical_machines = 0;
	hosts_length = xbt_dynar_length(hosts_dynar)-1;
	for (conf_count = 0; conf_count < configs_count; conf_count++)
	{
		physical_machines += configs[conf_count].worker_hosts_number;
	}
	XBT_INFO("Number of available hosts %lu", hosts_length);
	xbt_assert(hosts_length >= physical_machines + 1, "I need at least %ld hosts in the platform file, but platform file contains only %lu hosts.",
	        physical_machines + 1, hosts_length);

	host_count = 0;
	vm_count = 0;
	for (conf_count = 0; conf_count < configs_count; conf_count++)
	{

		configs[conf_count].grid_cpu_power = 0.0;
		vm_count_local = 0;
		wid = 0;

		for (i = 0; i < configs[conf_count].worker_hosts_number; i++)
		{

			for (j = 0; j < configs[conf_count].vm_per_host; j++)
			{
				snprintf(vmName, 64, "vm_%05lu", vm_count);
				vm_count++;
				vm_count_local++;
				host = xbt_dynar_get_as(hosts_dynar, host_count+1,msg_host_t);
				vm = MSG_vm_start(host, vmName, 2);

				char**argv_process = xbt_new(char*,3);
				argv_process[0] = bprintf("%d", wid);
				argv_process[1] = bprintf("%d", conf_count);
				argv_process[2] = NULL;
				process = MSG_process_create_with_arguments("worker", worker, vm, host, 2, argv_process);
				MSG_vm_bind(vm, process);

				XBT_INFO("config %d created worker %zu in vm %s on host %s", conf_count, wid, vmName, MSG_host_get_name(host));

				wid++;
				configs[conf_count].number_of_workers++;
			}
			configs[conf_count].grid_cpu_power += MSG_get_host_speed(host);
			host_count++;
		}

		//init config
		configs[conf_count].grid_average_speed = configs[conf_count].grid_cpu_power / configs[conf_count].number_of_workers;
		configs[conf_count].heartbeat_interval = maxval(3, configs[conf_count].number_of_workers / 100);
		configs[conf_count].number_of_maps = configs[conf_count].chunk_count;
		configs[conf_count].initialized = 1;

		w_heartbeats[conf_count] = xbt_new (struct heartbeat_s, configs[conf_count].number_of_workers);
		for (id = 0; id < configs[conf_count].number_of_workers; id++)
		{
			w_heartbeats[conf_count][id].slots_av[MAP] = configs[conf_count].map_slots;
			w_heartbeats[conf_count][id].slots_av[REDUCE] = configs[conf_count].reduce_slots;
		}

		init_stats(conf_count);
		init_job(conf_count);
		distribute_data(conf_count);

		char**argv_master = xbt_new(char*,2);
		argv_master[0] = bprintf("%d", conf_count);
		argv_master[1] = NULL;
		MSG_process_create_with_arguments("master", master, NULL, master_host, 1, argv_master);
		XBT_INFO("config %d created master on host %s", conf_count, MSG_host_get_name(master_host));

		vms = MSG_vms_as_dynar();
		xbt_dynar_sort(vms, vm_ordering_function);
		XBT_INFO("Launched %ld VMs for configuration %d, %ld VMs total", vm_count_local, conf_count, vm_count);
	}

	//do scheduling here
	if(argc >= 1)
	{
		struct csv_parser p;
		FILE *fp;
		csv_init(&p, 0);
		char buf[1024];
		size_t bytes_read;
		struct schedule c;
		c.row_reading = 0;
		c.vms_ids = xbt_dynar_new(sizeof(int), NULL );
		c.hosts_ids = xbt_dynar_new(sizeof(int), NULL );
		c.hosts_dynar = hosts_dynar;
		c.vms = vms;

		if (csv_init(&p, 0) != 0)
			exit(EXIT_FAILURE);

		fp = fopen(argv[0], "rb");
		if (!fp)
			exit(EXIT_FAILURE);
		while ((bytes_read = fread(buf, 1, 1024, fp)) > 0)
			if (csv_parse(&p, buf, bytes_read, cb1, cb2, &c) != bytes_read)
			{
				XBT_INFO("Error while parsing file: %s\n", csv_strerror(csv_error(&p)));
				exit(EXIT_FAILURE);
			}
		csv_fini(&p, cb1, cb2, &c);

		fclose(fp);
		csv_free(&p);
	}

	//if we are done with scheduling, but the simulation is going on - just wait till it finish
	while (1)
	{
		int break_cycle, jb;

		MSG_process_sleep(100);

		break_cycle = 1;
		for (jb = 0; jb < configs_count; jb++)
		{
			if (!jobs[jb].finished)
				break_cycle = 0;
		}
		if (break_cycle)
			break;
	}

	xbt_dynar_foreach(vms,cursor,vm)
	{
		MSG_vm_shutdown(vm);
		MSG_vm_destroy(vm);
	}

	XBT_INFO("Goodbye now!");

	xbt_dynar_free(&vms);
	for (conf_count = 0; conf_count < configs_count; conf_count++)
	{
		free_global_mem(conf_count);
	}

	xbt_dynar_free(&hosts_dynar);
	return 0;
}

int hosts_ordering_function(const void* a, const void* b)
{
	msg_host_t* system_host1 = (msg_host_t*) a;
	msg_host_t* system_host2 = (msg_host_t*) b;
	const char* name1 = MSG_host_get_name(*system_host1);
	const char* name2 = MSG_host_get_name(*system_host2);

	return strcmp(name1, name2);
}

int vm_ordering_function(const void* a, const void* b)
{
	msg_vm_t* vm1 = (msg_vm_t*) a;
	msg_vm_t* vm2 = (msg_vm_t*) b;
	const char* name1 = (*vm1)->name;
	const char* name2 = (*vm2)->name;

	return strcmp(name1, name2);
}
/**
 * @brief  Initialize the job structure.
 */
static void init_job(int configuration_id)
{
	unsigned int i;

	xbt_assert(configs[configuration_id].initialized, "init_config has to be called before init_job");

	jobs[configuration_id].finished = 0;

	/* Initialize map information. */
	jobs[configuration_id].tasks_pending[MAP] = configs[configuration_id].number_of_maps;
	jobs[configuration_id].task_status[MAP] = xbt_new0 (int, configs[configuration_id].number_of_maps);
	jobs[configuration_id].task_has_spec_copy[MAP] = xbt_new0 (int, configs[configuration_id].number_of_maps);
	jobs[configuration_id].task_list[MAP] = xbt_new0 (msg_task_t*, MAX_SPECULATIVE_COPIES);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		jobs[configuration_id].task_list[MAP][i] = xbt_new0 (msg_task_t, configs[configuration_id].number_of_maps);

	jobs[configuration_id].map_output = xbt_new (unsigned long long*, configs[configuration_id].number_of_workers);
	for (i = 0; i < configs[configuration_id].number_of_workers; i++)
		jobs[configuration_id].map_output[i] = xbt_new0 (unsigned long long, configs[configuration_id].number_of_reduces);

	/* Initialize reduce information. */
	jobs[configuration_id].tasks_pending[REDUCE] = configs[configuration_id].number_of_reduces;
	jobs[configuration_id].task_status[REDUCE] = xbt_new0 (int, configs[configuration_id].number_of_reduces);
	jobs[configuration_id].task_has_spec_copy[REDUCE] = xbt_new0 (int, configs[configuration_id].number_of_reduces);
	jobs[configuration_id].task_list[REDUCE] = xbt_new0 (msg_task_t*, MAX_SPECULATIVE_COPIES);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		jobs[configuration_id].task_list[REDUCE][i] = xbt_new0 (msg_task_t, configs[configuration_id].number_of_reduces);
}

/**
 * @brief  Initialize the stats structure.
 */
static void init_stats(int configuration_id)
{
	xbt_assert(configs[configuration_id].initialized, "init_config has to be called before init_stats");

	statistics[configuration_id].map_local = 0;
	statistics[configuration_id].map_remote = 0;
	statistics[configuration_id].map_spec_l = 0;
	statistics[configuration_id].map_spec_r = 0;
	statistics[configuration_id].reduce_normal = 0;
	statistics[configuration_id].reduce_spec = 0;
	statistics[configuration_id].maps_processed = xbt_new0 (int, configs[configuration_id].number_of_workers);
	statistics[configuration_id].reduces_processed = xbt_new0 (int, configs[configuration_id].number_of_workers);
}

/**
 * @brief  Free allocated memory for global variables.
 */
static void free_global_mem(int configuration_id)
{
	unsigned int i;

	for (i = 0; i < configs[configuration_id].chunk_count; i++)
		xbt_free_ref(&chunk_owners[configuration_id][i]);
	xbt_free_ref(&chunk_owners[configuration_id]);

	xbt_free_ref(&statistics[configuration_id].maps_processed);

	//xbt_free_ref(&worker_hosts);
	xbt_free_ref(&jobs[configuration_id].task_status[MAP]);
	xbt_free_ref(&jobs[configuration_id].task_has_spec_copy[MAP]);
	xbt_free_ref(&jobs[configuration_id].task_status[REDUCE]);
	xbt_free_ref(&jobs[configuration_id].task_has_spec_copy[REDUCE]);
	xbt_free_ref(&w_heartbeats[configuration_id]);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		xbt_free_ref(&jobs[configuration_id].task_list[MAP][i]);
	xbt_free_ref(&jobs[configuration_id].task_list[MAP]);
	for (i = 0; i < MAX_SPECULATIVE_COPIES; i++)
		xbt_free_ref(&jobs[configuration_id].task_list[REDUCE][i]);
	xbt_free_ref(&jobs[configuration_id].task_list[REDUCE]);
	xbt_free_ref(&statistics[configuration_id].reduces_processed);
}

static void cb1(void *s, size_t len, void *data)
{
	struct schedule* sched = ((struct schedule *) data);
	if (sched->row_reading == 0)
	{
		xbt_dynar_push_as(sched->vms_ids, int, atoi(s));
	}
	else
	{
		xbt_dynar_push_as(sched->hosts_ids, int, atoi(s));
	}
}

static void cb2(int c, void *data)
{
	struct schedule* sched = ((struct schedule *) data);
	if (sched->row_reading == 0)
	{
		sched->row_reading = 1;
	}
	else
	{
		//do scheduling
		//verify the length
		if (xbt_dynar_length(sched->vms_ids) == xbt_dynar_length(sched->hosts_ids))
		{
			while (!(xbt_dynar_is_empty(sched->vms_ids)))
			{
				int host_id = xbt_dynar_pop_as(sched->hosts_ids, int);
				int vm_id = xbt_dynar_pop_as(sched->vms_ids, int);
				msg_host_t host = xbt_dynar_get_as(sched->hosts_dynar, (long unsigned int)host_id+1,msg_host_t);
				msg_vm_t vm = xbt_dynar_get_as(sched->vms,(long unsigned int)vm_id,msg_vm_t);

				if(vm->location == host)
				{
					XBT_INFO("VM %s is already running on host %s.", vm->name, MSG_host_get_name(host));
				}
				else
				{
					XBT_INFO("Migrate VM %s to %s.", vm->name, MSG_host_get_name(host));
					MSG_vm_migrate(vm, host);
				}
			}
			//NOTE: this is a work around to solve some weird stuff with the sleep function
			MSG_process_sleep(SCHEDULE_SLEEP_TIME);
		}

		sched->row_reading = 0;
	}
}
