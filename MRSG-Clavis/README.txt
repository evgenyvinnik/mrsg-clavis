MRSG-Clavis is a cloud data centre simulator based on MRSG (https://github.com/MRSG/MRSG) project and SimGrid (http://simgrid.gforge.inria.fr/) distributed systems simulator.
It allows to simulate behaviour of multiple MapReduce applications concurrently running in the large data centre setting.

The distributed source code itself is an Eclipse CDT (C/C++ Development Tools) project so it is highly recommended to open and work with the sources by using this IDE.
Yet for the distribution purposes we also provide a separate makefile.

To run the program, follow there steps:

1) Make sure you have installed SimGrid (3.9 recommended). (http://simgrid.gforge.inria.fr/)


2.a) Open the MRSG-Clavis project in the Eclipse IDE and compile it (recommended)

2.b) Compile MRSG-Clavis using make in the command line.


3) Use the platform, config and schedule files distributed with the MRSG-Clavis sources or create your own. 
   To learn more on this topic refer to document called HOWTO.txt

 
4) Execute the MRSG-Clavis example:
	./MRSG-Clavis -platform g5k_sim.xml -config confcollection.txt -schedule schedule.csv