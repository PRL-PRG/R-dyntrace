//
// Created by nohajc on 3.4.17.
//

#ifndef R_3_3_1_DYN_FN_LOOKUP_H
#define R_3_3_1_DYN_FN_LOOKUP_H

void init_dll_handle(void * handle);
void * find_fn_by_name(const char * format, ...);

#endif //R_3_3_1_DYN_FN_LOOKUP_H
