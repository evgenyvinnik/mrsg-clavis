#include "common.h"
#include "mrsg.h"
XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(msg_test);

/**
 * User function that indicates the amount of bytes
 * that a map task will emit to a reduce task.
 *
 * @param  mid  The ID of the map task.
 * @param  rid  The ID of the reduce task.
 * @return The amount of data emitted (in bytes).
 */
size_t my_map_output_function(size_t mid, size_t rid)
{
	return 4 * 1024 * 1024;
}

/**
 * User function that indicates the cost of a task.
 *
 * @param  phase  The execution phase.
 * @param  tid    The ID of the task.
 * @param  wid    The ID of the worker that received the task.
 * @return The task cost in FLOPs.
 */
double my_task_cost_function(enum phase_e phase, size_t tid, size_t wid)
{
	switch (phase)
	{
	case MAP:
		return 1e+11;

	case REDUCE:
		return 5e+11;
	}

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		XBT_INFO("Usage: %s platform_file MRSG_config_file\n", argv[0]);
		XBT_INFO("example: %s g5k_sim.xml realistic.conf\n", argv[0]);
		exit(1);
	}


	/* MRSG_init must be called before setting the user functions. */
	MRSG_init();
	/* Set the task cost function. */
	MRSG_set_task_cost_f(my_task_cost_function);
	/* Set the map output function. */
	MRSG_set_map_output_f(my_map_output_function);
	/* Run the simulation. */
	//MRSG_main ("g5k.xml", "msg_platform.xml", "hello.conf");
	//MRSG_main ("g5k.xml", "hello.deploy.xml", "hello.conf");
	//MRSG_main ("g5k.xml", "hello.deploy.xml", "hello.conf");
	//MRSG_main("g5k_sim.xml", "hello.deploy.xml", "realistic.conf");
	//MRSG_main("g5k_sim.xml", "realistic.conf");
	MRSG_main(argv[1], argv[2]);
	//MRSG_main ("g5k.xml", "hello.deploy.xml", "realistic.conf");
	return 0;
}

