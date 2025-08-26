#include "challenge.h"
#include "player_api.h"

static const int A1[]={1,2,3,4,5};
static const int A2[]={-2,7,-1,0};
static const int A4[]={1000000000,1000000000,-1000000000};

static const SumArrayCase SUM_CASES[]={
    {A1,5,15,"accumulate all elements"},
    {A2,4,4 ,"handle negatives"},
    {NULL,0,0,"empty arrays sum to 0"},
    {A4,3,1000000000,"watch intermediate width"},
};

void register_content_pack_arrays(void){
    static Challenge sumc = {
        .slug="arrays.sum",
        .name="Sum of Array",
        .description="Implement sum_array(const int*, size_t).",
        .sig=SIG_SUM_ARRAY,
        .solution_fn=(void*)sum_array,
        .cases=SUM_CASES,
        .case_count=sizeof(SUM_CASES)/sizeof(SUM_CASES[0]),
        .xp_reward=100,
        .visibility=1,
    };
    challenges_register(&sumc);
}