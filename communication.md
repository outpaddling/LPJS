# Communication flow examples

This document describes the sequence of events that occur in
response to common events in the LPJS system.  It is intended to
help with debugging problems.

Note: All communication is authenticated and encrypted using munge.

## Compute node check-in

1.  lpjs_dispatchd daemon starts on the head node and calls listen() to take
    incoming connection requests from lpjs_compd or other lpjs commands.
2.  lpjs_compd daemon starts on a compute node.
3.  lpjs_compd loads the configuration file to determine head node address.
4.  lpjs_compd collects node configuration (processors, memory, etc).
5.  lpjs_compd calls connect() to open a new socket to lpjs_dispatchd.
6.  lpjs_dispatchd checks whether the compute node is authorized to connect
    and if so, calls accept() to establish the socket, and adds the compute
    node to the list of up nodes.
7.  lpjs_compd awaits dispatch requests from lpjs_dispatchd

## Job dispatch

1.  lpjs_dispatchd receives a request from `lpjs submit`.
2.  lpjs_dispatchd loads the job script and determines job parameters
3.  lpjs_dispatchd locates resources (CPU and memory) sufficient to run
    the job(s) described by the script.  For job arrays, this need not
    be all of the jobs.
4.  lpjs_dispatchd sends dispatch requests to each selected compute node.
5.  lpjs_compd starts a chaperone process on the compute node, a very
    small process dedicated to monitoring that individual job.
6.  The chaperone process attempts to run the job script on the compute node.
7.  The chaperone process waits for the job to terminate and reports
    back to lpjs_dispatchd.  The chaperone process monitors resource use
    of all descendant processes while the script runs and terminates them
    if they exceed the job parameters.
...

## Job completion

1.  A job script on a compute node terminates, either successfully or not.
2.  The chaperone process (parent of the job script) calls connect() to
    open a new socket to lpjs_dispatchd.
3.  lpjs_dispatchd verifies that the node is authorize to connect, and
    if so, calls accept() to establish the new socket connection.
4.  The chaperone process sends a message containing job status and
    statistics to lpjs_dispatchd.
5.  lpjs_dispatchd frees the resources allocated to the job.

## Job cancellation
