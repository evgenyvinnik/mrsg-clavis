/* Modified by Evgeny Vinnik (evinnik@sfu.ca) on July 25th, 2013 for the project MRSG-Clavis.*/

#include "common.h"
#include "dfs.h"
#include "mrsg.h"
#include <getopt.h>

static const struct option long_options[] =
{
{ "platform", required_argument, NULL, 'p' },
{ "config", required_argument, NULL, 'c' },
{ "schedule", optional_argument, NULL, 's' } };

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

/**
 * User function that indicates the amount of bytes
 * that a map task will emit to a reduce task.
 *
 * @param  mid  The ID of the map task.
 * @param  rid  The ID of the reduce task.
 * @return The amount of data emitted (in bytes).
 */
unsigned long long my_map_output_function(size_t mid, size_t rid, int configuration_id)
{
	//return 4 * 1024 * 1024;
	return configs[configuration_id].map_output_bytes;
}

/**
 * User function that indicates the cost of a task.
 *
 * @param  phase  The execution phase.
 * @param  tid    The ID of the task.
 * @param  wid    The ID of the worker that received the task.
 * @return The task cost in FLOPs.
 */
double my_task_cost_function(enum phase_e phase, size_t tid, size_t wid, int configuration_id)
{
	switch (phase)
	{
	case MAP:
		return configs[configuration_id].cpu_flops_map;

	case REDUCE:
		return configs[configuration_id].cpu_flops_reduce;
	}

	return 0;
}

/**
 *  Display program usage, and exit.
 */
void display_usage(const char* application)
{
	XBT_INFO("Usage: %s -platform [platform_file, required] -config [configuration_file, required] -schedule [schedule_file, optional]", application);
	XBT_INFO("Example: %s -platform g5k_sim.xml -config confcollection.txt -schedule schedule.csv", application);
	XBT_INFO("Short form is also supported: %s -p g5k_sim.xml -c confcollection.txt  -s schedule.csv", application);

	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	int opt = 0;
	int longIndex = 0;

	//set default values for optional parameters
	char* plat = NULL;
	char* conf = NULL;
	char* sched = NULL;

	while ((opt = getopt_long_only(argc, argv, "p:c:s:h?", long_options, &longIndex)) != -1)
	{
		switch (opt)
		{
		case 'p':
			plat = optarg;
			break;
		case 'c':
			conf = optarg;
			break;
		case 's':
			sched = optarg;
			break;

		case 'h': /* fall-through is intentional */
		case '?':
		default:
			display_usage(argv[0]);
			break;
		}

	}

	//check that user specified the directory for the files
	if (plat == NULL )
	{
		XBT_INFO("Platform file was not specified.");
		display_usage(argv[0]);
	}

	if (conf == NULL )
	{
		XBT_INFO("Configuration file was not specified.");
		display_usage(argv[0]);
	}

	XBT_INFO("platform file %s", plat);
	XBT_INFO("configuration file %s", conf);
	if (sched == NULL )
	{
		XBT_INFO("scheduling file is not provided");
	}
	else
	{
		XBT_INFO("scheduling file %s", sched);
	}

	/* set the default DFS function. */
	MRSG_set_dfs_f(default_dfs_f);
	/* Set the task cost function. */
	MRSG_set_task_cost_f(my_task_cost_function);
	/* Set the map output function. */
	MRSG_set_map_output_f(my_map_output_function);

	/* Run the simulation. */
	MRSG_main(plat, conf, sched);
	return 0;
}

