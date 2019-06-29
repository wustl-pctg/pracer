#ifndef _PIPER_RD_H_
#define _PIPER_RD_H_

#include <cilk/cilk_api.h>
#include <internal/abi.h>
#include <stdio.h>
#include "src/omrd.h"
#include "src/list.cpp"
#include <cilk/piper_abi.h>
#include "instrumentation.h"
//#define PRINTINFO

class PiperRDControl;
struct StageFrame;

extern PiperRDControl g_rdc;
extern __thread __cilkrts_pipe_iter_num_t g_curIterNum;
extern __thread __cilkrts_pipe_stage_num_t g_curStageNum;
extern __thread uint64_t stack_low_watermark;
extern __thread uint64_t stack_high_watermark;
extern __thread bool clear_stack;
//extern __thread List<StageFrame> *pCurIter;
//extern __thread List<StageFrame> *pPreIter;
extern __thread om_node *curRight;
extern __thread om_node *curDown;
//extern __thread int indexCur;

extern volatile size_t g_relabel_id;

extern uint64_t totalIter;
extern uint64_t totalStage;
extern uint64_t stageCurIter;
extern uint64_t max;
extern uint64_t min;
extern uint64_t reader;
extern uint64_t writer;


struct Order {
    om_node *current;
    om_node *rightCont;
    om_node *downCont;
};

struct StageFrame {
    __cilkrts_pipe_iter_num_t iterNum;//for double check
    __cilkrts_pipe_stage_num_t stageNum;
    Order downFirst;
    Order rightFirst;
};

//save up to (throttle_limit + 1) iteration's stage list
class Iterations {
private:
    List<StageFrame> **p;
    __cilkrts_pipe_iter_num_t *iterNumList;
    int throttleLimit;
    int head;
    int tail;
    uint64_t *stack_low_watermark;
    uint64_t *stack_high_watermark;

    bool valid(List<StageFrame> *list, __cilkrts_pipe_iter_num_t iterNum) {
        return (list != NULL && list->length() > 0 && list->get(0).iterNum == iterNum);
    }

public:
    Iterations(int limit) {
        this->throttleLimit = limit;
        p = new List<StageFrame> *[throttleLimit + 1];
        iterNumList = new __cilkrts_pipe_iter_num_t[throttleLimit + 1];
        stack_low_watermark = new uint64_t[throttleLimit + 1];
        stack_high_watermark = new uint64_t[throttleLimit + 1];

        for (int i = 0; i < throttleLimit + 1; ++i) {
            p[i] = NULL;
            iterNumList[i] = -1;
            stack_low_watermark[i] = stack_high_watermark[i] = (uint64_t)(-1);
        }
        head = -1;
        tail = -1;
    }

    void free() {
        for (int i = 0; i < throttleLimit + 1; ++i)
            if (p[i] != NULL) delete p[i];
    }

    ~Iterations() {
        free();
        delete[] p;
        delete[] iterNumList;
        delete[] stack_low_watermark;
        delete[] stack_high_watermark;
    }

    uint64_t getLowWatermark(__cilkrts_pipe_iter_num_t iterNum) {
        /*
        for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) return stack_low_watermark[i];
        }

        return (uint64_t)(-1);*/
        int index = getIndex(iterNum);
        if (iterNumList[index] == iterNum) {
            return stack_low_watermark[index];
        } else {
            return (uint64_t)(-1);
        }
    }

    uint64_t getHighWatermark(__cilkrts_pipe_iter_num_t iterNum) {
        /*
        for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) return stack_high_watermark[i];
        }

        return (uint64_t)(-1);*/
        int index = getIndex(iterNum);
        if (iterNumList[index] == iterNum) {
            return stack_high_watermark[index];
        } else {
            return (uint64_t)(-1);
        }
    }

    uint64_t getLowWatermarkFast(int index) {
        return stack_low_watermark[index];
    }

    uint64_t getHighWatermarkFast(int index) {
        return stack_high_watermark[index];
    }

    void setLowWatermark(__cilkrts_pipe_iter_num_t iterNum, uint64_t watermark) {
        /*for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) stack_low_watermark[i] = watermark;
        }*/
        int index = getIndex(iterNum);
        stack_low_watermark[index] = watermark;
    }

    void setHighWatermark(__cilkrts_pipe_iter_num_t iterNum, uint64_t watermark) {
        /*for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) stack_high_watermark[i] = watermark;
        }*/
        int index = getIndex(iterNum);
        stack_high_watermark[index] = watermark;
    }

    void setLowWatermarkFast(int index, uint64_t watermark) {
        stack_low_watermark[index] = watermark;
    }

    void setHighWatermarkFast(int index, uint64_t watermark) {
        stack_high_watermark[index] = watermark;
    }

    int getIndex(__cilkrts_pipe_iter_num_t iterNum) {
        int m = iterNum % (throttleLimit + 1);
        if (m == 0) {
            return throttleLimit;
        } else {
            return m - 1;
        }

        /*
        for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) return i;
        }

        return -1;
        */
    }

    //get op may have race with insert, so we need to double check
    List<StageFrame> *getStageFrameList(__cilkrts_pipe_iter_num_t iterNum) {
        /*List<StageFrame> *list = NULL;
        do {
            for (int i = 0; i < throttleLimit + 1; ++i) {
                if (iterNumList[i] == iterNum) {
                    list = p[i];
                    break;
                }
            }

            if (list == NULL) break;

        } while (!valid(list, iterNum));

        return list;*/
        /*
        for (int i = 0; i < throttleLimit + 1; ++i) {
            if (iterNumList[i] == iterNum) {
                return p[i];
            }
        }

        return NULL;*/
        int index = getIndex(iterNum);
        return p[index];
    }

    List<StageFrame> *getStageFrameListFast(int index) {
        return p[index];
    }

    List<StageFrame> *insertStageFrameList(__cilkrts_pipe_iter_num_t iterNum) {
        int index;
        if (tail <= head && head + 1 < throttleLimit + 1) {
            index = ++head;
        } else if (tail < head) {
            index = head = 0;
            tail = 1;
        } else if (head < tail) {
            index = ++head;
            tail = (tail + 1) % (throttleLimit + 1);
        }

        if (p[index] != NULL) delete p[index];
        p[index] = new List<StageFrame>();
        iterNumList[index] = iterNum;
        return p[index];
    }
};


//one piperRDControl corresponds to one piper while and all piperRDControls share three static members
class PiperRDControl {
private:
    omrd_t *downFirst;
    omrd_t *rightFirst;
    Iterations *iterations;
    int throttleLimit;

    void init(int throttleLimit) {
        //if (downFirst != NULL) return;
        this->throttleLimit = throttleLimit;
        //downFirst = new omrd_t();
        //rightFirst = new omrd_t();
        iterations = new Iterations(throttleLimit);
        g_english = new omrd_t();
        g_hebrew = new omrd_t();
    }

    void destroy() {
        //delete downFirst;
        //delete rightFirst;
        delete iterations;
        //downFirst = rightFirst = NULL;
        iterations = NULL;
        delete g_english;
        delete g_hebrew;
        g_english = g_hebrew = NULL;
    }
/*
    void createDummyStage(List<StageFrame> *list, __cilkrts_pipe_stage_num_t userStage) {
        if (userStage <= list->length()) return;

        StageFrame parent = list->get(list->length() - 1);
        StageFrame dummyFrame;
        dummyFrame.dummy = true;
        dummyFrame.iterNum = parent.iterNum;
        dummyFrame.stageNum = parent.stageNum + 1;
        dummyFrame.

        int length = list->length();
        for (int i = 0; i < userStage - length; ++i) {
            dummyFrame.stageNum++;
            dummyFrame.rightFirst.current = parent.rightFirst.downCont;
            dummyFrame.downFirst.current = parent.downFirst.downCont;
            createOrder(dummyFrame);
            list->append(dummyFrame);
            parent = dummyFrame;
        }
    }
*/
    void createOrder(StageFrame &sf) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        sf.rightFirst.rightCont = g_hebrew->insert(w, sf.rightFirst.current);
        sf.rightFirst.downCont = g_hebrew->insert(w, sf.rightFirst.rightCont);
        sf.downFirst.downCont = g_english->insert(w, sf.downFirst.current);
        sf.downFirst.rightCont = g_english->insert(w, sf.downFirst.downCont);
    }

    void createOrderFast(StageFrame &sf) {
        __cilkrts_worker *w = __cilkrts_get_tls_worker();
        sf.rightFirst.rightCont = g_hebrew->insert(w, sf.rightFirst.current);
        sf.rightFirst.downCont = g_hebrew->insert(w, sf.rightFirst.rightCont);
        sf.downFirst.downCont = g_english->insert(w, sf.downFirst.current);
        sf.downFirst.rightCont = NULL;
    }

    void printInfo(int phase, __cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t stage) {
        switch (phase) {
            case 1:
                fprintf(stderr, "pipe while begin\n");
                break;
            case 2:
                fprintf(stderr, "stage first:\n");
                break;
            case 3:
                fprintf(stderr, "stage wait:\n");
                break;
            case 4:
                fprintf(stderr, "stage:\n");
                break;
            case 5:
                fprintf(stderr, "pipe while end:\n");
                break;
        }

        fprintf(stderr, "iterNum: %lld stage: %lld\n", iterNum, stage);
    }

public:
    PiperRDControl() {
        //downFirst = rightFirst = NULL;
        g_english = g_hebrew = NULL;
        iterations = NULL;
        init_lock();
    }

    ~PiperRDControl() {
        //if (downFirst != NULL) destroy();
    }

    uint64_t getLowWatermark(__cilkrts_pipe_iter_num_t iterNum) {
        return iterations->getLowWatermark(iterNum);
    }

    uint64_t getHighWatermark(__cilkrts_pipe_iter_num_t iterNum) {
        return iterations->getHighWatermark(iterNum);
    }

    uint64_t getLowWatermarkFast(int index) {
        return iterations->getLowWatermarkFast(index);
    }

    uint64_t getHighWatermarkFast(int index) {
        return iterations->getHighWatermarkFast(index);
    }

    void setLowWatermark(__cilkrts_pipe_iter_num_t iterNum, uint64_t watermark) {
        iterations->setLowWatermark(iterNum, watermark);
    }

    void setHighWatermark(__cilkrts_pipe_iter_num_t iterNum, uint64_t watermark) {
        iterations->setHighWatermark(iterNum, watermark);
    }

    void setLowWatermarkFast(int index, uint64_t watermark) {
        iterations->setLowWatermarkFast(index, watermark);
    }

    void setHighWatermarkFast(int index, uint64_t watermark) {
        iterations->setHighWatermarkFast(index, watermark);
    }

    void cilkPipeWhileBegin(int throttleLimit) {
#ifdef PRINTINFO
        printInfo(1, 0, 0);
#endif
        init(throttleLimit);
        do_tool_init();
        //init(128);
        //fprintf(stderr, "workers: %d\n", __cilkrts_get_nworkers());
        //fprintf(stderr, "limit: %d\n", throttleLimit);
    }

    void cilkStageFirst(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t userStage, __cilkrts_pipe_stage_num_t stage) {
#ifdef PRINTINFO
        printInfo(2, iterNum, userStage);
#endif
        //if (stageCurIter > max) max = stageCurIter;
        //if (stageCurIter < min || min == 0) min = stageCurIter;
        //stageCurIter = 1;
        //totalStage++;
        //totalIter++;


        StageFrame sf;
        sf.iterNum = iterNum;
        sf.stageNum = 0;

        if (iterNum > 1) {
            //indexPre = iterations->getIndex(iterNum - 1);
            List<StageFrame> *pPreIter = iterations->getStageFrameList(iterNum - 1);
            StageFrame sibling = pPreIter->get(0);
            curRight = sf.rightFirst.current = sibling.rightFirst.rightCont;
            curDown  = sf.downFirst.current = sibling.downFirst.rightCont;
        } else {
            curRight = sf.rightFirst.current = g_hebrew->get_base();
            curDown  = sf.downFirst.current = g_english->get_base();
        }


        createOrder(sf);

        List<StageFrame> *list = iterations->insertStageFrameList(iterNum);
        list->append(sf);
        list->cachedLeftStage = list->latestWaitStage = list->stageNum = 0;
        list->iterNum = iterNum;
        list->index = 1;
        //pCurIter = list;
        //indexCur = iterations->getIndex(iterNum);

        g_curIterNum = iterNum;
        g_curStageNum = 0;

        uint64_t res = (uint64_t)__builtin_frame_address(1);
        setHighWatermark(iterNum, res);
        //setLowWatermark(iterNum, res);
        //fprintf(stderr, "end1\n");
        setLowWatermark(iterNum, (uint64_t)(-1));
        //fprintf(stderr, "end2\n");
    }

    int getLeftDependency(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t stage) {
        List<StageFrame> *cur = iterations->getStageFrameList(iterNum);
        List<StageFrame> *pre = iterations->getStageFrameList(iterNum - 1);

        int length = pre->length();
        int ret = 0;
        int index = cur->index;
        int iPre = 0;

        //fprintf(stderr, "index: %d\n", index);

        while (index < length) {
            if (pre->get(index).stageNum == stage) {
                ret = index++;
                break;
            } else if (pre->get(index).stageNum < stage) {
                iPre = index++;
            } else if (pre->get(index).stageNum > stage) {
                ret = iPre;
                break;
            }
        }

        cur->index = index;
        return ret > iPre ? ret : iPre;
/*
        while (index < length) {
            if (pre->get(index).stageNum == stage) {
                ret = index;
                cur->latestWaitStage = index;
                break;
            } else if() {
                index--;
                break;
            } else if (index + 1 == length) {
                if (pre->get(index).stageNum < stage) {
                    ret = index;
                } else {
                    index--;
                }
                break;
            } else if (pre->get(index).stageNum < stage 
                        && pre->get(index + 1).stageNum > stage) {
                if (pre->get(index).stageNum <= cur->latestWaitStage) {
                    break;
                    
                } else {
                    ret = index;
                    cur->latestWaitStage = index;
                    break;
                }
            }

            index++;
        }

        cur->index = index + 1;
        return ret;
*/
    }

    void cilkStageWait(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t userStage, __cilkrts_pipe_stage_num_t stage) {
#ifdef PRINTINFO
        printInfo(3, iterNum, userStage);
#endif
        //totalStage++;
        //stageCurIter++;
        
        StageFrame sf;
        sf.iterNum = iterNum;
        sf.stageNum = userStage;

        List<StageFrame> *pCurIter = iterations->getStageFrameList(iterNum);
        StageFrame parent = pCurIter->get(pCurIter->length() - 1);
        curRight = sf.rightFirst.current = parent.rightFirst.downCont;
        curDown  = sf.downFirst.current = parent.downFirst.downCont;

        if (iterNum > 1) {
            int leftIndex = getLeftDependency(iterNum, userStage);
            //fprintf(stderr, "leftIndex %d\n", leftIndex);
            if (leftIndex != 0) {
                StageFrame sibling = iterations->getStageFrameList(iterNum - 1)->get(leftIndex);
                //fprintf(stderr, "leftStage %d\n", sibling.stageNum);
                curRight = sf.rightFirst.current = sibling.rightFirst.rightCont;
            }
        }

        //createOrderFast(sf);
        createOrder(sf);
        pCurIter->append(sf);
        pCurIter->stageNum = userStage;

        g_curIterNum = iterNum;
        g_curStageNum = userStage;
    }

    void cilkStage(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t userStage, __cilkrts_pipe_stage_num_t stage) {
#ifdef PRINTINFO
        printInfo(4, iterNum, userStage);
#endif
        //totalStage++;
        //stageCurIter++;

        StageFrame sf;
        sf.iterNum = iterNum;
        sf.stageNum = userStage;

        List<StageFrame> *pCurIter = iterations->getStageFrameList(iterNum);

        StageFrame parent = pCurIter->get(pCurIter->length() - 1);
        curRight = sf.rightFirst.current = parent.rightFirst.downCont;
        curDown  = sf.downFirst.current = parent.downFirst.downCont;

        //createOrderFast(sf);
        createOrder(sf);
        pCurIter->append(sf);

        pCurIter->stageNum = userStage;


        g_curIterNum = iterNum;
        g_curStageNum = userStage;
    }

    void cilkPipeWhileEnd() {
#ifdef PRINTINFO
        printInfo(5, 0, 0);
#endif
        //do_tool_print();
        //isSerial(31, 118, 32, 121);
        //isSerial(31, 113, 32, 112);
        //isSerial(31, 118, 32, 117);
        //isSerial(31, 112, 32, 117);
        //printNodesCount(g_english);
        //printNodesCount(g_hebrew);
        //fprintf(stderr, "heavy group count1: %zu\n", g_english->heavy_count);
        //fprintf(stderr, "heavy group count2: %zu\n", g_hebrew->heavy_count);
        destroy();
        //fprintf(stderr, "pipeline done, trigger relabel: %zu\n", g_relabel_id);
        //fprintf(stderr, "iter: %lld stage: %lld\n", g_curIterNum, g_curStageNum);
        //totalIter--;
        //totalStage--;
        //fprintf(stderr, "total iters: %zu\n", totalIter);
        //fprintf(stderr, "total stages: %zu\n", totalStage);
        //fprintf(stderr, "max: %zu\n", max);
        //fprintf(stderr, "min: %zu\n", min);
        //fprintf(stderr, "reader: %zu\n", reader);
        //fprintf(stderr, "writer: %zu\n", writer);

    }

    om_node* getDownFirstNode(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t userStage) {
        return iterations->getStageFrameList(iterNum)->get(userStage).downFirst.current;
    }

    om_node* getRightFirstNode(__cilkrts_pipe_iter_num_t iterNum, __cilkrts_pipe_stage_num_t userStage) {
        return iterations->getStageFrameList(iterNum)->get(userStage).rightFirst.current;
    }

    void printNodesCount(omrd_t *order) {
        om* ds = order->get_ds();
        size_t count = om_count(ds->root);
        fprintf(stderr, "om nodes count: %zu\n", count);
    }

    size_t om_count(tl_node *n) {
        if (n == NULL) return 0;
        if (n->level < MAX_LEVEL)
            return om_count(n->left) + om_count(n->right);
        return count(n->below);
    }

    size_t count(blist *b) {
        bl_node* head = b->head;
        size_t ret = 0;
        while (head) {
            ret++;
            head = head->next;    
        }

        return ret;
    }

    bool isSerial(__cilkrts_pipe_iter_num_t iterNumA, __cilkrts_pipe_stage_num_t stageA, __cilkrts_pipe_iter_num_t iterNumB, __cilkrts_pipe_stage_num_t stageB) {
        StageFrame sfA, sfB;
        List<StageFrame>* p;

        p = iterations->getStageFrameList(iterNumA);
        int index = 0;
        while (true) {
            sfA = p->get(index++);
            if (sfA.stageNum == stageA) {
                fprintf(stderr, "found it\n");
                break;
            }
        }

        p = iterations->getStageFrameList(iterNumB);
        index = 0;
        while (true) {
            sfB = p->get(index++);
            if (sfB.stageNum == stageB) {
                fprintf(stderr, "found it\n");
                break;
            }
        }

        if ((om_precedes(sfA.rightFirst.current, sfB.rightFirst.current)
            && om_precedes(sfA.downFirst.current, sfB.downFirst.current))
            || (om_precedes(sfB.rightFirst.current, sfA.rightFirst.current)
            && om_precedes(sfB.downFirst.current, sfA.downFirst.current))) {
            fprintf(stderr, "true\n");
            return true;
        } else {
            fprintf(stderr, "false\n");
            return false;
        }
    }

};

#endif
