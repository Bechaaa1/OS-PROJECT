// include/policy.h
#ifndef POLICY_H
#define POLICY_H

void fifo_scheduler(void);
void priority_scheduler(void);
void round_robin_scheduler(int quantum);
void aging_scheduler(int quantum);   // NEW

#endif