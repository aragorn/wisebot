/* $Id$ */
/////////////////////////////////
//   multi-processing module   //
/////////////////////////////////

#if 0
1. thread or process?
2. scoreboard

standard status
required element
1. allocated slot number
2. occupied slot number - thread
3. start time - epoch time
4. parent generation

3. active slot number - currently being processing - no need ?
4. idle slot number                                - no need ?

extended status - no need?
7. total access
8. total traffic
9. cpu usage

slot
1. child server number - generation
2. pid, thread_id
3. mode of operation - required element

4. access count this connection/this child/this slot
5. traffic count this connection/this child/this slot
6. cpu usage - times(2) system call for each request
7. recent request time - time(2) system call for each request
8. required time for recent request - derived from 6
9. query string

---------------------------------
create_scoreboard(key); /* key : pid of parent server? */
destroy_scoreboard(key); /* debugging purpose */
print_scoreboard()
spawn_child_process()

bind()
listen()
select() - accept()



update_scoreboard(scoreboard);
update_slot(int slot_number); /* if not shared memory */


#endif

typedef struct {

} standard_slot_t;

typedef struct {
	int slot_size;
	int parent_generation;
	
} standard_scoreboard_t;

typedef struct {
	int slot_size;
} extended_slot_t;


