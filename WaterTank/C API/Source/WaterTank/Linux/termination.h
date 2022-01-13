#ifndef _TERMINATION_H_
#define _TERMINATION_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void catch_termination();
bool is_terminated();

#ifdef __cplusplus
}
#endif

#endif//_TERMINATION_H_
