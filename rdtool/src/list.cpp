#include <stdio.h>
#include <cilk/piper_abi.h>

extern __thread __cilkrts_pipe_iter_num_t g_curIterNum; 
extern __thread __cilkrts_pipe_stage_num_t g_curStageNum; 

class ListException
{
};

template <typename T>
class List
{
private:
    T *p;
    volatile int top;
    int size;

    void ExtendList()
    {
        T *pTemp = new T[size + 100];
        for (int i = 0; i < size; i++)
            pTemp[i] = p[i];
        delete[] p;
        p = pTemp;
        size += 100;
    }
public:
    List()
    {
        p = new T[100];
        size = 100;
        top = -1;
    }
    ~List()
    {
        delete[] p;
    }
    
    __cilkrts_pipe_stage_num_t cachedLeftStage;

    __cilkrts_pipe_stage_num_t latestWaitStage;

    __cilkrts_pipe_iter_num_t iterNum;

    __cilkrts_pipe_stage_num_t stageNum;

    int index;

    void append(const T &element) {
        if ((top + 1) == size)
        {
            ExtendList();
        }
        p[++top] = element;
    }

    T get(int index)
    {
        if (index >= 0 && index < length())
        {
            return p[index];
        }
        else
        {
            fprintf(stderr, "list exception: index=%d iter=%lld stage=%lld length=%d \n", index, g_curIterNum, g_curStageNum, length());
            throw ListException();
            assert(0);
        }
    }

    int length() {
        return top + 1;
    }
};
