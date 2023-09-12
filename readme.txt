To run the C++ simulation script, you need to build the NS3 simulator.

To build the simulator, you can follow the following:

https://www.nsnam.org/docs/release/3.39/installation/singlehtml/index.html.

Once built, you will then need to navigate to the home directory of ns3. For me this is, ns-allinone-3.39/ns-3.39, as i
chose to build the full package, which comes with an animator.

Afterwards, you copy the files in "C++ code" to the scratch directory. Once done, you can now run the files.

Each script has a blurb at the beginning which says what output files they produce and what command line options 
available.

// ===================================================================== /

To run the python visualisers, you need to hard code the raw path to the output files in the visualiser scripts.

// ===================================================================== /

The mobility trace of the SUMO model is provided in the cardiff_edited.tcl file.

If you would like to run the simulation of the mobility of the vehicles, first install SUMO by following the steps as in
the remote repository https://github.com/eclipse/sumo. In particular one can run the command:

    "git clone --recursive https://github.com/eclipse/sumo"

Once SUMO is built the various tools should be available. You can then cd into SUMO_CardiffMobility and write the following in the command prompt:

    "sumo-gui osm.sumocfg"

in order to get a visualisation of the vehicle traffic simulated by the configuration.

To generate the trace for the mobility, you can write the following:

    "sumo -c osm.sumocfg --fcd-output sumoTrace.xml"

This generates the trace file in .xml format.

You then need to invoke the traceExporter tool like so:

    "./traceExporter.py --fcd-input sumoTrace.xml --ns2mobility-output cardiff.tcl"

which converts the sumoTrace.xml file to the mobility trace cardiff.tcl file
