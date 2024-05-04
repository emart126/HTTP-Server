### Multi-Threaded-HTTP-Server

# httpserver.c

This multi threaded httpserver program can do two different things for the user.
It can get text from a file and print it to standard output or
it can set and replace text to an existing or non existing file
in the local directory. All the while running on a specific port number
the user chooses. Effecient in reading and writing to different files due
to multi-threading.

Intructions:
use the command 'make' to create all deliverables
required for the program, use 'make clean' to
remove all the object files created as using make
creates them for functionality. Only after the
server is running, you can connect to it useing
netcat or any other connection method. Once connected
on a terminal the user is able to send it commands. 
The command to run
the server is
./httpserver -t [number of threads] [port number]

Options:
-   -t              The number of threads that are being used to multi-thread the server (default: 4)
-   [port number]   The port number to connect to using the server (ranged open ports are 1024 - 65534) 

Format:
Once the Server is running, it will be waiting for
user input. The input must be a command with strict
format including spaces and not include quotations or parenthesis:

Get command: "GET /(location) HTTP/1.1\r\n((header-field): (id)\r\n)*\r\n"
- gets the contents of a file named (location) and prints it to current socket connected to the port the server is running on.

Put command: "GET /(location) HTTP/1.1\r\n((header-field): (id)\r\n)*\r\n(Message Body)"
- Puts the file named (location) contents to be written as (contents) with length (contentLength)

Descriptions:
- (location)			the file name within the same directory
- (header-field)		used to specify Request Id for which thread to use and/or Content lenght of a file
- (id)				    the id or number used for the given header field 
- ()*                   The asterisks represents that any amount of header field id's can be given in input
- (Message Body)        The contents to be put into in a put command

## queue.c

Design:\
queue is a struct that creates an abstract data type that allows 
users to push and pop elements into or out of a queue of a given size 
and is fully implemented using mutual exclusion properties. This is
to allow for multi-threading using a single queue so that elements
cannot be overridden or retrieved in an unporper order.

Intructions:\
The queue.c file contains functions that requires the use of pointers.
This means that elements for the queue ADT must be inputed as pointer
addresses. This allows for the queue to work with any type of data type.
The queue also requires an initial size because the queue is created
on a buffer and does not dynamically add size to it so the queue can
only have up to the inputed size amount of elements.

Functions:\
queue\_t \*queue\_new(int size)\
void queue\_delete(queue\_t \*\*q)\
void queue\_push(queue\_t \*q, void \*elem)\
void queue\_pop(queue\_t \*q, void \*\*elem)

## rwlock.c

Design:\
rwlock is a struct that creates a data type that allows
users to lock and unlock threads for reading and writing purposes.
It does this through the use of mutual exclusion properties

Intructions:\
The rwlock.c allows users to lock and unlock readers or writers for various
different purposes. The file gives the ability to stop any writers while there
are readers or vice versa to solve the problem of overrides. The file requiers
and asks the user for a specific priority type. The priority type 'READERS'
prioritizes reader threads to go before any writer threads. The priority
type 'WRITERS' prioritizes writer threads to go before reader threads. The last
priority type 'N\_WAY' prioritizes 'n' amount of reader threads to read between
every writer thread. The value 'n' can be anything if priority type isn't 'N\_WAY'

Functions:\
rwlock\_t \*rwlock\_new(PRIORITY p, uint32\_t n)\
void reader\_delete(rwlock\_t \*\*rw)\
void reader\_lock(rwlock\_t \*rw)\
void reader\_unlock(rwlock\_t \*rw)\
void writer\_lock(rwlock\_t \*rw)\
void writer\_unlock(rwlock\_t \*rw)

