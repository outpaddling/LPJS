# Communication flow examples

## Compute node checkin

-   Note: All communication is authenticated and encrypted using munge.

1.  lpjs_dispatchd daemon starts on head node and calls listen() to take
    incoming connection requests from lpjs_compd or other lpjs commands.
2.  lpjs_compd daemon starts on a compute node.
3.  lpjs_compd collects node configuration (processors, memory, ...).
4.  lpjs_compd calls connect() to open a socket to lpjs_dispatchd.
5.  lpjs_dispatchd checks whether compute node is authorized to connect
    and if so, calls accept() to establish the socket.
6.  lpjs_compd awaits dispatch requests from lpjs_dispatchd

## Job dispatch

1.  lpjs_dispatchd receives a request from `lpjs submit`.
2.  lpjs_dispatchd loads the job script and determines job parameters
3.  
...

## Job completion

## Job cancelation
