#include <stdio.h>
#include <stdlib.h>

struct area {
    unsigned long begin_addr;
    unsigned long length;
    unsigned long end_addr;
};

struct page {
    char dirty;
    long addr;
    double dirty_time;     //
//    double last_scan_time;
    char actual_dirty;          //
};

struct page pages[16777216] = {{0, 0} };
unsigned int global_dirty = 0;

unsigned long hash(unsigned long x) //from page start address to hashed page number
{
    unsigned long xx;
    xx = ((x >> 12) & 16777215);    //hash function.
    while ((pages[xx].addr != x) && (pages[xx].addr != 0)) {  //check validation
        xx += (xx >> 1) + 1;
        xx = xx & 16777215;
    }
    pages[xx].addr = x;
    return xx;
}

int find_victims(int *scaned, int *saved, int scan_max, int save_max, struct area areas[], int areas_number, unsigned long *i, unsigned long *j, double time_now, double time_last)
{
    static double threshold;
    double orig_threshold;



    if (threshold < (time_now - time_last))
        threshold = time_now - time_last;
    orig_threshold = threshold;
//    printf("%lf\n", threshold);
    *scaned = 1;
    *saved = 1;
    (*j)++;
    while (1) {
        for (; (*i) < areas_number; (*i)++) {
            if (*j < areas[*i].begin_addr)      //if the j round just begin
                *j = areas[*i].begin_addr;
            for (; *j < (areas[*i].end_addr); (*j)+=4096) {
                //printf("%d", *j);
                if (((*scaned) >= scan_max) || ((*saved) >= save_max)) {    //exit and adjust threshold
                    //assume the apprixmate interval is even distribute.
                    printf("%ld\n", ((*saved)));
//                    threshold = threshold * ((double)(*saved) * scan_max / save_max / (*scaned)) / 2;
                //    printf("%lf\n", threshold);

                    return 1;
                }

                threshold = orig_threshold * ((double)(*saved) * scan_max / save_max / (*scaned)) / 2;

                (*scaned)++;

                if (pages[hash(*j)].dirty) {
                    pages[hash(*j)].actual_dirty = 1;
                    pages[hash(*j)].dirty = 0;
                    pages[hash(*j)].dirty_time = time_last;
                }
                if (pages[hash(*j)].actual_dirty && ((time_now - pages[hash(*j)].dirty_time) >= threshold))
                {
                    pages[hash(*j)].actual_dirty = 0;
                    (*saved)++;
                    global_dirty--;
                }

            }
            *j = 0;     //clean for next j round
        }
        *i = 0;     // clean for next i round
    }
}



int time_tick(FILE *fp, double time_intetval, double *time_now)    //make dirty. If return 0, the input ends.
{
    static double time;
    static unsigned long addr;
    double endtime;


    if (addr == 0) {        //if the first time
        fscanf(fp, "%lf %lX\n", &time, &addr);

//        printf("%lf %lX\n", time, addr);

        endtime = time + time_intetval;

        (*time_now) = endtime;
//        printf("%lf %lX\n", endtime, addr);
        pages[hash(addr)].dirty = 1;
    } else {                // not the first time
        endtime = time + time_intetval;
        (*time_now) = endtime;
        if (!(pages[hash(addr)].dirty)) {
            pages[hash(addr)].dirty = 1;
            if (!(pages[hash(addr)].actual_dirty))
                global_dirty++;
        }
    }
    while (fscanf(fp, "%lf %lX\n", &time, &addr) == 2) {

//        printf("~");
//        printf("%lf %lX\n", time, addr);

        if (time > endtime) return 1;
        if (pages[hash(addr)].dirty) continue;
        pages[hash(addr)].dirty = 1;
        if (pages[hash(addr)].actual_dirty) continue;
        global_dirty++;
    }
//    if (fscanf(fp, "%lf %lX\n", &time, &addr) != 2) return 0;
    return 0;
}

int init(char *filename, struct area areas[], int *areas_number)    //read area info
{
//    *areas_number = 2;
//    areas[0].end_addr =
    char s[255];
    int i = 0;
    char *c;
    FILE *fp;
//    int x;

    printf("!!");

    fp = fopen(filename, "r");
//    if (fp == NULL) printf("!!");
    fgets(s, 255, fp);

    while (fgets(s, 255, fp) != NULL){
//    printf("%s\n", s);
        if (sscanf(s, "%lx", &areas[i].begin_addr) != 1) break;
//        printf("%d ",x);
        c = s;
        while (*c != ' ') c++;
//        printf("%s\n", c);
        sscanf(c, " %ld", &areas[i].length);
        while (*c == ' ') c++;
        while (*c != ' ') c++;
        while (*c == ' ') c++;
        c++;
//        printf("%s\n", c);
        if (*c == 'w') {
            areas[i].end_addr = areas[i].begin_addr + areas[i].length * 1024;
            i++;
        }

    }
    (*areas_number) = i;
    return i;

}


int main(int argc, char *argv[])        //pmap file, log file, time interval (frame length), scan max, save max.
{
    struct area areas[10000];
    int areas_number;
    unsigned long i,j;
    int scaned, saved;
    double time_interval;
    double time_now, time_last;
    int scan_max, save_max;
    FILE *fp, *fout;

//    double time;
//    unsigned long addr;

//    for (i = 0; i < argc; i++) {
//        printf("%s\n", argv[i]);
//    }
    printf("!!");

    init(argv[1], areas, &areas_number);    //read area info

    printf("!!");

    fp = fopen(argv[2], "r");
    fout = fopen("hehe", "w");
    time_interval = atof(argv[3]);
    scan_max = atof(argv[4]);
    save_max = atof(argv[5]);

    save_max++;
    scan_max++;

    printf("!!");
    if (fp == NULL) printf("@@@");
//    fscanf(fp, "%lf %lX\n", &time, &addr);

    while (time_tick(fp, time_interval, &time_now)) {      //make dirty

        find_victims(&scaned, &saved, scan_max, save_max, areas, areas_number, &i, &j, time_now, time_last);     //backup
        time_last = time_now;
        fprintf(fout, "%d\n", global_dirty);
    }


//    printf("%d", i);
//    for (; i>=0; i--) {
//        printf("%lX %lX\n", areas[i].begin_addr, areas[i].end_addr);
//    }

    printf("\n\nHello world!\n");
    return 0;
}
