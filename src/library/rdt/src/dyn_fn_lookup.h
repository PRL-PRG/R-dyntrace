//
// Created by nohajc on 3.4.17.
//

#ifndef R_3_3_1_DYN_FN_LOOKUP_H
#define R_3_3_1_DYN_FN_LOOKUP_H

#ifdef __cplusplus
extern "C" {
#endif

void init_dll_handle(void *handle);
void *find_fn_by_name(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif //R_3_3_1_DYN_FN_LOOKUP_H
