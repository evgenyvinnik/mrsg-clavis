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

#include <msg/msg.h>
#include <xbt/sysdep.h>
#include <xbt/log.h>
#include <xbt/asserts.h>
#include "common.h"
#include "dfs.h"
#include "mrsg.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(msg_test, "MRSG");

#define MAX_LINE_SIZE 256

int scheduler(int argc, char *argv[]);

static void check_config(void);
static msg_error_t run_simulation(const char* platform_file);

static void read_mr_config_file(const char* file_name);

int MRSG_main(const char* plat, const char* conf)
{
	int argc = 8;
	char* argv[] =
	{ "mrsg", "--cfg=tracing:1", "--cfg=tracing/buffer:1", "--cfg=tracing/filename:tracefile.trace", "--cfg=tracing/categorized:1",
	        "--cfg=tracing/uncategorized:1", "--cfg=viva/categorized:cat.plist", "--cfg=viva/uncategorized:uncat.plist" };

	msg_error_t res = MSG_OK;

	config.initialized = 0;

	check_config();

	MSG_init(&argc, argv);

	read_mr_config_file(conf);

	MSG_create_environment(plat);

	res = run_simulation(plat);

	if (res == MSG_OK)
		return 0;
	else
		return 1;
}

/**
 * @brief Check if the user configuration is sound.
 */
static void check_config(void)
{
	xbt_assert(user.task_cost_f != NULL, "Task cost function not specified.");
	xbt_assert(user.map_output_f != NULL, "Map output function not specified.");
}

/**
 * @param  platform_file   The path/name of the platform file.
 * @param  deploy_file     The path/name of the deploy file.
 * @param  mr_config_file  The path/name of the configuration file.
 */
static msg_error_t run_simulation(const char* platform_file)
{
	msg_error_t res = MSG_OK;
	xbt_dynar_t hosts_dynar;
	msg_host_t* system_hosts = xbt_new(msg_host_t,config.worker_hosts_number+1);
	char** system_hosts_names = xbt_new(char*,config.worker_hosts_number+1);
	char** scheduler_argv = xbt_new(char*,config.worker_hosts_number+2);
	long unsigned int i;


	// for tracing purposes..
	TRACE_category_with_color("MAP", "1 0 0");
	TRACE_category_with_color("REDUCE", "0 0 1");

	/* Retrieve the 11 first hosts of the platform file */
	hosts_dynar = MSG_hosts_as_dynar();
	XBT_INFO("Number of available hosts %lu", xbt_dynar_length(hosts_dynar));
	xbt_assert(xbt_dynar_length(hosts_dynar) >= config.worker_hosts_number + 1, "I need at least %lu hosts in the platform file, but %s contains only %ld hosts_dynar.",
			config.worker_hosts_number + 1, platform_file, xbt_dynar_length(hosts_dynar));

	for (i = 0; i < config.worker_hosts_number + 1; i++)
	{
		system_hosts[i] = xbt_dynar_get_as(hosts_dynar,i,msg_host_t);
		system_hosts_names[i] = xbt_strdup(MSG_host_get_name(system_hosts[i]));
		scheduler_argv[i] = xbt_strdup(MSG_host_get_name(system_hosts[i]));
#ifdef VERBOSE
		XBT_INFO("added %lu host %s", i, MSG_host_get_name(system_hosts[i]));
#endif
	}

	scheduler_argv[config.worker_hosts_number + 1] = NULL;

	srand(12345);
	//we start scheduler and a Map-Reduce job tracker (master) on host 0
	MSG_process_create_with_arguments("scheduler", scheduler, NULL, system_hosts[0], (int) config.worker_hosts_number + 1, scheduler_argv);
	res = MSG_main();

	xbt_free(system_hosts);
	for (i = 0; i < config.worker_hosts_number + 1; i++)
		xbt_free(system_hosts_names[i]);
	xbt_free_ref(system_hosts_names);
	xbt_dynar_free(&hosts_dynar);

	return res;
}

/**
 * @brief  Read the MapReduce configuration file.
 * @param  file_name  The path/name of the configuration file.
 */
static void read_mr_config_file(const char* file_name)
{
	char property[256];
	FILE* file;

	/* Set the default configuration. */
	config.chunk_size = 67108864;
	config.chunk_count = 0;
	config.chunk_replicas = 3;
	config.map_slots = 2;
	config.number_of_reduces = 1;
	config.reduce_slots = 2;
	config.worker_hosts_number = 40;
	config.vm_per_host = 2;
	/* Read the user configuration file. */

	file = fopen(file_name, "r");

	xbt_assert(file != NULL, "Error reading cofiguration file: %s", file_name);

	while (fscanf(file, "%256s", property) != EOF)
	{
		if (strcmp(property, "chunk_size") == 0)
		{
			fscanf(file, "%lg", &config.chunk_size);
			config.chunk_size *= 1024 * 1024; /* MB -> bytes */
		}
		else if (strcmp(property, "input_chunks") == 0)
		{
			fscanf(file, "%u", &config.chunk_count);
		}
		else if (strcmp(property, "dfs_replicas") == 0)
		{
			fscanf(file, "%u", &config.chunk_replicas);
		}
		else if (strcmp(property, "map_slots") == 0)
		{
			fscanf(file, "%u", &config.map_slots);
		}
		else if (strcmp(property, "reduces") == 0)
		{
			fscanf(file, "%u", &config.number_of_reduces);
		}
		else if (strcmp(property, "reduce_slots") == 0)
		{
			fscanf(file, "%u", &config.reduce_slots);
		}
		else if (strcmp(property, "worker_hosts_number") == 0)
		{
			fscanf(file, "%ld", &config.worker_hosts_number);
		}
		else if (strcmp(property, "vm_per_host") == 0)
		{
			fscanf(file, "%ld", &config.vm_per_host);
		}
		else
		{
			printf("Error: Property %s is not valid. (in %s)", property, file_name);
			exit(1);
		}
	}

	fclose(file);

	/* Assert the configuration values. */

	xbt_assert(config.chunk_size > 0, "Chunk size must be greater than zero");
	xbt_assert(config.chunk_count > 0, "The amount of input chunks must be greater than zero");
	xbt_assert(config.chunk_replicas > 0, "The amount of chunk replicas must be greater than zero");
	xbt_assert(config.map_slots > 0, "Map slots must be greater than zero");
	xbt_assert(config.reduce_slots > 0, "Reduce slots must be greater than zero");
	xbt_assert(config.worker_hosts_number > 0, "Number of worker hosts must be greater than zero");
	xbt_assert(config.vm_per_host > 0, "Number of virtual machines per host must be greater than zero");
}
