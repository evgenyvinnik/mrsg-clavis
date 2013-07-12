/* Copyright (c) 2012. MRSG Team. All rights reserved. */

/* This file is part of MRSG.

 MRSG is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 MRSG is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with MRSG.  If not, see <http://www.gnu.org/licenses/>. */

#include "common.h"
#include "dfs.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

static void heartbeat(void);
static int listen(int argc, char* argv[]);
static int compute(int argc, char* argv[]);
static void update_map_output(msg_process_t worker, size_t mid);
static void get_chunk(msg_process_t worker, task_info_t ti);
static void get_map_output(msg_process_t worker, task_info_t ti);

/**
 * @brief  Main worker function.
 *
 * This is the initial function of a worker node.
 * It creates other processes and runs a heartbeat loop.
 */
int worker(int argc, char* argv[])
{
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_process_t my_process, process_create, process_data_node;
	msg_host_t my_host;
	msg_vm_t vm;
	int my_id;
	vm = (msg_vm_t) MSG_process_get_data(MSG_process_self());

	my_host = MSG_host_self();
	my_process = MSG_process_self();
	sscanf(argv[0], "%d", &my_id);
	MSG_process_set_data(my_process, (void*) my_id);
	//my_id = MSG_process_self_PID();
	/* Spawn a process that listens for tasks. */
	process_create = MSG_process_create("listen", listen, vm, my_host);
	MSG_vm_bind(vm, process_create);
	/* Spawn a process to exchange data with other workers. */
	process_data_node = MSG_process_create("data-node", data_node, vm, my_host);
	MSG_vm_bind(vm, process_data_node);
	/* Start sending heartbeat signals to the master node. */
	heartbeat();

	sprintf(mailbox, DATANODE_MAILBOX, get_worker_id(my_process));
	send_sms(SMS_FINISH, mailbox); //TODO might be dangerous
	sprintf(mailbox, TASKTRACKER_MAILBOX, get_worker_id(my_process));
	send_sms(SMS_FINISH, mailbox); //TODO might be dangerous

	return 0;
}

/**
 * @brief  The heartbeat loop.
 */
static void heartbeat(void)
{
	while (!job.finished)
	{
		send_sms(SMS_HEARTBEAT, MASTER_MAILBOX);
		MSG_process_sleep(config.heartbeat_interval);
	}
}

/**
 * @brief  Process that listens for tasks.
 */
static int listen(int argc, char* argv[])
{
	char mailbox[MAILBOX_ALIAS_SIZE];
	int parent_id;
	msg_process_t parent_process, process_compute;
	msg_host_t my_host;
	msg_task_t msg = NULL;
	msg_vm_t vm;

	vm = (msg_vm_t) MSG_process_get_data(MSG_process_self());
	//const char* process_name = NULL;

	my_host = MSG_host_self();

	parent_id = MSG_process_self_PPID();
	parent_process = MSG_process_from_PID(parent_id);
	//process_name = MSG_process_get_name(parent_process);

	sprintf(mailbox, TASKTRACKER_MAILBOX, get_worker_id(parent_process));

	while (!job.finished)
	{
		msg = NULL;
		receive(&msg, mailbox);

		if (message_is(msg, SMS_TASK))
		{
			process_compute = MSG_process_create("compute", compute, msg,
			        my_host);
			MSG_vm_bind(vm, process_compute);
		}
		else if (message_is(msg, SMS_FINISH))
		{
			MSG_task_destroy(msg);
			break;
		}
	}

	return 0;
}
#define FILENAME1 "./doc/simgrid/examples/platforms/g5k.xml"
#define FILENAME2 "./memory/mem.mem"
/**
 * @brief  Process that computes a task.
 */
static int compute(int argc, char* argv[])
{
	msg_error_t error;
	msg_task_t task;
	task_info_t ti;
	xbt_ex_t e;
	int grand_parent_id;
	msg_process_t grand_parent_process_worker;
	int parent_id;
	msg_process_t parent_process;
	size_t wid;
	msg_host_t dest_host;

	parent_id = MSG_process_self_PPID();
	parent_process = MSG_process_from_PID(parent_id);
	grand_parent_id = MSG_process_get_PPID(parent_process);
	grand_parent_process_worker = MSG_process_from_PID(grand_parent_id);
	wid = get_worker_id(grand_parent_process_worker);
	dest_host = MSG_process_get_host(grand_parent_process_worker);

	task = (msg_task_t) MSG_process_get_data(MSG_process_self());
	ti = (task_info_t) MSG_task_get_data(task);
	ti->pid = MSG_process_self_PID();
	ti->worker_process = grand_parent_process_worker;

	switch (ti->phase)
	{
	case MAP:
		get_chunk(grand_parent_process_worker, ti);
		XBT_INFO("\t\tmap %zu received by task tracker\t wid %d\t on %s", ti->id, wid,
		        MSG_host_get_name(dest_host));

		break;

	case REDUCE:
		get_map_output(grand_parent_process_worker, ti);
		XBT_INFO("\t\treduce %zu received by task tracker\t wid %d\t on %s", ti->id,
		        wid, MSG_host_get_name(dest_host));
		break;
	}

	if (job.task_status[ti->phase][ti->id] != T_STATUS_DONE)
	{
		TRY
				{
					int times = 1;

					if (wid == 0 || wid == 2)
						times = 2000;

					//perform memory operations
					{
						msg_file_t file = NULL;
						s_msg_stat_t stat;
						void *ptr = NULL;
						char* mount = bprintf("/slot");
						double read, write;
						int j;

						for (j = 0; j < times; j++)
						{
							file = MSG_file_open(mount, FILENAME2, "rw");
							//XBT_INFO("\tOpen file '%s'", file->name);

							read = MSG_file_read(ptr, 100000000, sizeof(char*),
							        file);     // Read for 10Mo
							//XBT_INFO("\tHave read    %8.1f on %s", read,
							//        file->name);

							//for(j = 0; j < 5;j++)

							//write = MSG_file_write(ptr, 60000000,
							//        sizeof(char*), file);  // Write for 10Mo
							//XBT_INFO("\tHave written %8.1f on %s", write,
							//        file->name);

							//read = MSG_file_read(ptr, 1000000000, sizeof(char*),
							//        file);     // Read for 10Mo
							//XBT_INFO("\tHave read    %8.1f on %s", read,
							//        file->name);

							MSG_file_stat(file, &stat);
							//XBT_INFO("\tFile stat %s Size %.1f", file->name,
							//        stat.size);
							MSG_file_free_stat(&stat);

							//XBT_INFO("\tClose file '%s'", file->name);
							MSG_file_close(file);
						}

						free(mount);
					}
					//perform IO operations

					{
						msg_file_t file = NULL;
						s_msg_stat_t stat;
						void *ptr = NULL;
						char* mount = bprintf("/home");
						double read, write;
						int j;

						for (j = 0; j < times; j++)
						{
							file = MSG_file_open(mount, FILENAME1, "rw");
							//XBT_INFO("\tOpen file '%s'", file->name);

							read = MSG_file_read(ptr, 10000000, sizeof(char*),
							        file);     // Read for 10Mo
							//XBT_INFO("\tHave read    %8.1f on %s", read,
							//        file->name);

							//write = MSG_file_write(ptr, 10000000, sizeof(char*),
							//       file);  // Write for 10Mo
							//XBT_INFO("\tHave written %8.1f on %s", write,
							//        file->name);

							//read = MSG_file_read(ptr, 10000000, sizeof(char*),
							//        file);      // Read for 10Mo
							//XBT_INFO("\tHave read    %8.1f on %s", read,
							//        file->name);

							MSG_file_stat(file, &stat);
							//XBT_INFO("\tFile stat %s Size %.1f", file->name,
							//        stat.size);
							MSG_file_free_stat(&stat);

							//XBT_INFO("\tClose file '%s'", file->name);
							MSG_file_close(file);
						}
						free(mount);
					}
					//Perform memory operations (treated as very fast harddrive);
					//perform execution (CPU)
					error = MSG_task_execute(task);

					if (ti->phase == MAP && error == MSG_OK)
						update_map_output(grand_parent_process_worker, ti->id);
				}
					CATCH(e)
		{
			xbt_assert(e.category == cancel_error, "%s", e.msg);
			xbt_ex_free(e);
		}
	}

	w_heartbeat[ti->wid].slots_av[ti->phase]++;

	if (!job.finished)
		send(SMS_TASK_DONE, 0.0, 0.0, ti, MASTER_MAILBOX);

	switch (ti->phase)
	{
	case MAP:
		XBT_INFO("\t\tmap %zu complete by task tracker\t wid %d\t on %s", ti->id, wid,
		        MSG_host_get_name(dest_host));

		break;

	case REDUCE:
		XBT_INFO("\t\treduce %zu complete by task tracker\t wid %d\t on %s", ti->id,
		        wid, MSG_host_get_name(dest_host));
		break;
	}

	return 0;
}

/**
 * @brief  Update the amount of data produced by a mapper.
 * @param  worker  The worker that finished a map task.
 * @param  mid     The ID of map task.
 */
static void update_map_output(msg_process_t worker, size_t mid)
{
	int rid;
	size_t wid;

	wid = get_worker_id(worker);

	for (rid = 0; rid < config.number_of_reduces; rid++)
		job.map_output[wid][rid] += user.map_output_f(mid, rid);
}

/**
 * @brief  Get the chunk associated to a map task.
 * @param  ti  The task information.
 */
static void get_chunk(msg_process_t worker, task_info_t ti)
{
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_task_t data = NULL;
	size_t my_id;

	my_id = get_worker_id(worker);

	/* Request the chunk to the source node. */
	if (ti->src != my_id)
	{
		sprintf(mailbox, DATANODE_MAILBOX, ti->src);
		send(SMS_GET_CHUNK, 0.0, 0.0, ti, mailbox);

		sprintf(mailbox, TASK_MAILBOX, my_id, MSG_process_self_PID());
		receive(&data, mailbox);

		MSG_task_destroy(data);
	}
}

/**
 * @brief  Copy the itermediary pairs for a reduce task.
 * @param  ti  The task information.
 */
static void get_map_output(msg_process_t worker, task_info_t ti)
{
	char mailbox[MAILBOX_ALIAS_SIZE];
	msg_task_t data = NULL;
	size_t total_copied, must_copy;
	int mid;
	size_t my_id,wid_wk;
	int wid;
	size_t* data_copied;
	msg_host_t dest_host;
	dest_host = MSG_process_get_host(worker);

	wid_wk = get_worker_id(worker);

	my_id = get_worker_id(worker);
	data_copied = xbt_new0 (size_t, config.number_of_workers);
	ti->map_output_copied = data_copied;
	total_copied = 0;
	must_copy = 0;
	for (mid = 0; mid < config.number_of_maps; mid++)
		must_copy += user.map_output_f(mid, ti->id);

//#ifdef VERBOSE
	XBT_INFO("INFO: start copy must_copy %d, reduce %zu, task tracker\t wid %d\t on %s", must_copy, ti->id,
			wid_wk, MSG_host_get_name(dest_host)) ;
//#endif

	while (total_copied < must_copy)
	{
		for (wid = 0; wid < config.number_of_workers; wid++)
		{
			if (job.task_status[REDUCE][ti->id] == T_STATUS_DONE)
			{
				xbt_free_ref(&data_copied);
				return;
			}

			if (job.map_output[wid][ti->id] > data_copied[wid])
			{
				sprintf(mailbox, DATANODE_MAILBOX, wid);
				send(SMS_GET_INTER_PAIRS, 0.0, 0.0, ti, mailbox);

				sprintf(mailbox, TASK_MAILBOX, my_id, MSG_process_self_PID());
				data = NULL;
				receive(&data, mailbox);
				data_copied[wid] += MSG_task_get_data_size(data);
				total_copied += MSG_task_get_data_size(data);
				MSG_task_destroy(data);
			}
		}
		/* (Hadoop 0.20.2) mapred/ReduceTask.java:1979 */
		MSG_process_sleep(5);
	}

//#ifdef VERBOSE
	XBT_INFO("INFO: copy finished. received %d, reduce %zu, task tracker\t wid %d\t on %s", total_copied, ti->id,
			wid_wk, MSG_host_get_name(dest_host)) ;
//#endif
	ti->shuffle_end = MSG_get_clock();

	xbt_free_ref(&data_copied);
}

