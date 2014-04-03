/* *
 * Modified Problem Master Judge for Sum-Of-Points scoring for the SPOJ platform.
 * This program returns AC for a submission if it passed all tests. Otherwise
 * it returns the appropriate error code. 
 * In all cases, it returns along the percentage (0 - 100) of passed tests.
 *
 *
 * Modification by Roberto Abreu 
 */

#include <spoj.h>
#include <cstdio>

int main()
{
    spoj_init();
    int test, sig, mem, memMax=0, tc=0;
    double score, scoreAll=0, time, timeAll=0;
    char status[4], statusBad[4];
    bool allOK = true;

    while (fscanf(spoj_p_in, "%d %3s %lf %d %lf %d\n", &test, status, &score, &sig, &time, &mem)==6)
    {
        fprintf(spoj_p_info, "test %d - %s (score=%lf, sig=%d, time=%lf, mem=%d)\n", test, status, score, sig, time, mem);
        if (status[0]=='A')
        {
            if (mem > memMax) memMax = mem;
            timeAll += time;
            scoreAll++;
        }
        else {
            allOK = false;
            strcpy(statusBad, status);
        }
        tc++;
    }
    if (allOK) {
        fprintf(spoj_score, "AC %.2lf 0 %lf %d\n", 100.*scoreAll/tc, timeAll, memMax);
    }
    else {
        fprintf(spoj_score, "%s %.2lf 0 %lf %d\n", statusBad, 100.*scoreAll/tc, timeAll, memMax);   
    }
}

