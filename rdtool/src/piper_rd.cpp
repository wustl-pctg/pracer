#include "piper_rd.h"

PiperRDControl g_rdc;
__thread __cilkrts_pipe_iter_num_t g_curIterNum;
__thread __cilkrts_pipe_stage_num_t g_curStageNum;
__thread uint64_t stack_low_watermark = (uint64_t)(-1);
__thread uint64_t stack_high_watermark = (uint64_t)(-1);
__thread bool clear_stack = false;
//__thread List<StageFrame> *pCurIter = NULL;
//__thread List<StageFrame> *pPreIter = NULL;
__thread om_node *curRight = NULL;
__thread om_node *curDown = NULL;
//__thread int indexCur = -1;
//
uint64_t totalIter = 0;
uint64_t totalStage = 0;
uint64_t stageCurIter = 0;
uint64_t max = 0;
uint64_t min = 0;
uint64_t reader = 0;
uint64_t writer = 0;
