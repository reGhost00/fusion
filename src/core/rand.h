#ifndef _FU_RAND_H_
#define _FU_RAND_H_

#include "object.h"

typedef enum _EFURandAlgorithm {
    EFU_RAND_ALGORITHM_DEFAULT,
    EFU_RAND_ALGORITHM_LCG,
    EFU_RAND_ALGORITHM_MT,
    EFU_RAND_ALGORITHM_CNT
} EFURandAlgorithm;

FU_DECLARE_TYPE(FURand, fu_rand)
#define FU_TYPE_RAND (fu_rand_get_type())

FURand* fu_rand_new(EFURandAlgorithm type);
int fu_rand_int_range(FURand* self, int min, int max);

#endif