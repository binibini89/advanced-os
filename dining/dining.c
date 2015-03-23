#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>

typedef enum  {left, right} direction_t;

pthread_mutex_t chopstick_mutex[5];

int phil_to_chopstick(int phil_id, direction_t d){
  return (phil_id + d) % 5;      
}

int chopstick_to_phil(int stick_id, direction_t d){
  return (stick_id + 5 - d) % 5;      
}

void pickup_one_chopstick(int stick_id, int phil_id){
  /*
    Use the line

    printf("Philosopher %d picks up chopstick %d \n", phil_id, stick_id);

    to have the philosopher pick-up a chopstick. Debugging will be easier
    if you remember to flush stdout.

    fflush(stdout)
  */
  /*Your code here*/

    pthread_mutex_lock(&chopstick_mutex[stick_id]);

	printf("Philosopher %d picks up chopstick %d \n", phil_id, stick_id);
	fflush(stdout);

}
void putdown_one_chopstick(int stick_id, int phil_id){
  /*
    Use the line

    printf("Philosopher %d puts down chopstick %d \n", phil_id, stick_id);

    to have the philosopher pick-up a chopstick. Debugging will be easier
    if you remember to flush stdout.

    fflush(stdout)
  */
  /*Your code here*/

	printf("Philosopher %d puts down chopstick %d \n", phil_id, stick_id);
	fflush(stdout);
    
     pthread_mutex_unlock(&chopstick_mutex[stick_id]);

}

void pickup_chopsticks(int phil_id){
  /*Use     
    pickup_chopstick(phil_to_chopstick(phil_id, right), phil_id);

    and

    pickup_chopstick(phil_to_chopstick(phil_id, left), phil_id);
    
    to pickup the right and left chopsticks.  Beware of deadlock!
  */
  
  /*Your code here*/
    
  if (phil_id == 0 || phil_id == 2 || phil_id == 4) {
		while (1){
			pickup_one_chopstick(phil_to_chopstick(phil_id, right), phil_id);
			if (pthread_mutex_trylock(&chopstick_mutex[(phil_to_chopstick(phil_id, left))]) == 0) {
				printf("Philosopher %d picks up chopstick %d \n", phil_id, phil_to_chopstick(phil_id, left));
				fflush(stdout);
				return;
			}
			else {
				putdown_one_chopstick(phil_to_chopstick(phil_id, right), phil_id);
                sched_yield();
			}
		}

	}
	if (phil_id == 1 || phil_id == 3) {
		while (1) {
			pickup_one_chopstick(phil_to_chopstick(phil_id, left), phil_id);
			if (pthread_mutex_trylock(&chopstick_mutex[(phil_to_chopstick(phil_id, right))]) == 0) {
				printf("Philosopher %d picks up chopstick %d \n", phil_id, phil_to_chopstick(phil_id, right));
				fflush(stdout);
				return;
			}
			else {
				putdown_one_chopstick(phil_to_chopstick(phil_id, left), phil_id);
                sched_yield();
			}
		}

	}
}

void putdown_chopsticks(int phil_id){
  /*Use putdown_chopstick to put down the chopsticks*/    
  /*Your code here*/
	
	putdown_one_chopstick(phil_to_chopstick(phil_id, right), phil_id);

	putdown_one_chopstick(phil_to_chopstick(phil_id, left), phil_id);

}

