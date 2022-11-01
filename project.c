#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>

int rw_head[2]; // head[0] = current track number of read write head,  head[1] = current sector number of read write head

int rotation_speed;          // Rotational speed of disk
double average_seek_time;    // Average time for the disk arm to switch between any two tracks
double sector_rotation_time; // Time take by the disk to rotate area equal to one sector
double traverse_time_delay;  // Time taken to read/write from the sector
int sector_size;             // Size of sector in bytes
int policy_no;               // Policy number for disk scheduling algorithms
                             // Policy number = 1 => random disc scheduling
                             // Policy number = 2 => first in first out scheduling
                             // Policy number = 3 => shortest service time first scheduling
                             // Policy number = 4 => SCAN scheduling
                             // Policy number = 5 => C-SCAN scheduling

int throught_put; // Throughput for given simulation

int request[1000][3]; // Request queue
int pos = 0,          // Position of current process in process queue(FIFO)
    pos1 = 0,         // Position of process whos track number which will be considered first(SCAN, C-SCAN)
    start = 0,        // Starting position of the process in process queue having current track number(SCAN, C-SCAN)
    end = 0;          // Ending position of the process in process queue having current track number(SCAN, C-SCAN)

bool visited[1000] = {false}; // visited[i] = true if the ith process is alreay considered in the process queue, otherwise visited[i] = false
bool pos_switch = false;      // Flag to indicate the direction of traverse in the process queue(SCAN, C-SCAN)

double data[1000]; // data[i] = response time of ith process
double sum = 0;    // Total time taken by all processes

double calc_time(int platter_no, int track_no, int sector_no)
{ // To calculate the reponse time for the current process

    double t = 0;
    int num_sec_rotate;         // Number of sector through which disk have to rotate for the current request
    if (rw_head[0] != track_no) // If track number of current request != track number corresponding to current read write head position
        t += average_seek_time; // Then add average_seek_time for calulating response time

    num_sec_rotate = (20 + rw_head[1] - sector_no) % 20; // Calculating rotaional delay

    printf("NUM_SEC_ROTATE: %d\n", num_sec_rotate);

    t += ((num_sec_rotate) * (sector_rotation_time));
    t += traverse_time_delay; // Adding read_write delay for the current request

    rw_head[0] = track_no;             // Updating track number of read-write head
    rw_head[1] = (sector_no + 1) % 20; // Updating sector number of read-write head

    return t;
}

void sort_request()
{ // To sort reuqests with respect to 1)Track number 2)Sector number

    int temp1 = 0, temp2 = 0, temp3 = 0;

    for (int i = 0; i < 999; i++)
    {

        for (int j = 0; j < 999 - i; j++)
        {

            if (request[j][1] > request[j + 1][1])
            {

                temp1 = request[j][0];
                temp2 = request[j][1];
                temp3 = request[j][2];

                request[j][0] = request[j + 1][0];
                request[j][1] = request[j + 1][1];
                request[j][2] = request[j + 1][2];

                request[j + 1][0] = temp1;
                request[j + 1][1] = temp2;
                request[j + 1][2] = temp3;
            }
        }
    }
}

void find_pos()
{ // To find the position of process in process queue having having track number closest to the track number of read-write head in begining

    pos1 = 0;

    for (int i = 0; i < 1000; i++)
    {

        if (request[i][1] >= rw_head[0])
        {

            pos1 = i;
            break;
        }
    }

    start = pos1;
    int i = pos1;

    while (i < 1000 && request[i][1] == request[start][1])
    {
        i++;
    }

    end = i - 1;
}

void update_start_end_4()
{ // Update start and end if all request from current track are considered in case of SCAN

    int i;

    if (!pos_switch)
    { // If search is  not complete one direction

        start = end + 1;
        if (start == 1000)
        { // If the current track number become equal to the last track number

            end = pos1 - 1;
            pos_switch = true;
            i = end;

            while (i >= 0 && request[i][1] == request[end][1])
            {
                i--;
            }

            start = i + 1;
        }

        else
        { // If the current track number is not equal to the last trak number
            i = start;

            while (i < 1000 && request[i][1] == request[start][1])
            {
                i++;
            }

            end = i - 1;
        }
    }

    else
    { // If search is complete in one direction

        end = start - 1;
        i = end;

        while (i >= 0 && request[i][1] == request[end][1])
        {
            i--;
        }

        start = i + 1;
    }
}

void update_start_end_5()
{ // Update start and end if all request from current track are considered in case of C-SCAN

    int i;

    if (!pos_switch)
    { // If search is  not complete one direction

        start = end + 1;

        if (start == 1000)
        { // If the current track number become equal to the last track number

            start = 0;
            pos_switch = true;
            i = start;

            while (i < pos1 && request[i][1] == request[start][1])
            {
                i++;
            }

            end = i - 1;
        }

        else
        { // If the current track number is not equal to the last track number

            i = start;

            while (i < 1000 && request[i][1] == request[start][1])
            {
                i++;
            }

            end = i - 1;
        }
    }

    else
    { // If search is complete in one direction

        start = end + 1;
        i = start;

        while (i < pos1 && request[i][1] == request[start][1])
        {
            i++;
        }

        end = i - 1;
    }
}

int find_process_number()
{ // Find position of the next process in the process queue according to the disck scheduling algorithm and current read write head positoin
    int idx;

    if (policy_no == 1)
    { // In case of Random disk scheduling

        do
        {

            idx = rand() % 1000; // generating random process number

        } while (visited[idx]);

        visited[idx] = true;

        return idx;
    }

    else if (policy_no == 2)
    { // In case of FIFO disk scheduling

        idx = pos;
        pos += 1;
        return idx;
    }

    else if (policy_no == 3)
    { // In case of SSFT disk scheduling

        int diff1 = 0, diff2 = 0;

        for (int i = 0; i < 1000; i++)
        {

            if (!visited[i])
            {

                idx = i;
                diff1 = abs(request[i][1] - rw_head[0]);
                diff2 = (20 + rw_head[1] - request[i][2]) % 20;
            }
        }

        for (int i = 0; i < 1000; i++)
        {

            if (!visited[i])
            {

                if (abs(request[i][1] - rw_head[0]) < diff1)
                {

                    diff1 = abs(request[i][1] - rw_head[0]);
                    diff2 = (20 + rw_head[1] - request[i][2]) % 20;
                    idx = i;
                }

                else if (abs(request[i][1] - rw_head[0]) == diff1)
                {

                    diff1 = abs(request[i][1] - rw_head[0]);
                    int temp = (20 + rw_head[1] - request[i][2]) % 20;

                    if (temp <= diff2)
                    {

                        diff2 = temp;
                        idx = i;
                    }
                }
            }
        }

        visited[idx] = true;
        return idx;
    }

    else if (policy_no == 4)
    { // In case of SCAN disk scheduling

        while (true)
        {

            int diff = 20;
            int idx = 0;

            for (int i = start; i <= end; i++)
            {

                if (!visited[i] && ((20 + rw_head[1] - request[i][2]) % 20) < diff)
                {

                    diff = (20 + rw_head[1] - request[i][2]) % 20;
                    idx = i;
                }
            }

            if (diff == 20)
                update_start_end_4();

            else
            {

                visited[idx] = true;
                return idx;
            }
        }
    }

    else if (policy_no == 5)
    { // In case of C-SCAN disk scheduling

        while (true)
        {

            int diff = 20;
            int idx = 0;

            for (int i = start; i <= end; i++)
            {

                if (!visited[i] && ((20 + rw_head[1] - request[i][2]) % 20) < diff)
                {

                    diff = (20 + rw_head[1] - request[i][2]) % 20;
                    idx = i;
                }
            }

            if (diff == 20)
                update_start_end_5();

            else
            {

                visited[idx] = true;
                return idx;
            }
        }
    }
}

double find_min_resp_time()
{ // To find minimum reponse time

    double small = data[0];

    for (int i = 0; i < 1000; i++)
    {
        if (data[i] < small)
            small = data[i];
    }

    return small;
}

double find_max_resp_time()
{ // To find maximum reponse time
    double large = data[0];

    for (int i = 0; i < 1000; i++)
    {
        sum += data[i];

        if (data[i] > large)
        {
            large = data[i];
        }
    }

    return large;
}

double find_std_dev_time()
{ // To find the standard deviation of response time

    throught_put = 1000 / sum;

    double mean = sum / 1000;
    double sqr_sum = 0;

    for (int i = 0; i < 1000; i++)
    {

        double temp = mean - data[i];
        sqr_sum += (temp * temp);
    }

    sqr_sum /= 1000;

    return sqrt(sqr_sum);
}
int main(int argc, char *argv[])
{

    if (argc < 4)
        printf("INVALID NUMBER OF COMMAND LINE ARGUMENTS\n");

    else
    {

        rotation_speed = atoi(argv[1]);
        sector_size = atoi(argv[2]);
        average_seek_time = atoi(argv[3]);
        sector_rotation_time = (3.0 / rotation_speed);
        average_seek_time = average_seek_time / 1000;
        traverse_time_delay = 60.0 / rotation_speed;

        rw_head[0] = 12; // hardcoding intial track number of read write head
        rw_head[1] = 10; // hardcoding intial secotr number of read write head

        printf("ENTER THE SHCEDULING POLICY NUMBER:\n");
        printf("PRESS 1 FOR RANDOM SCHEDULING POLICY.\n");
        printf("PRESS 2 FOR FIFO SCHEDULING POLICY.\n");
        printf("PRESS 3 FOR SSTF SCHEDULING POLICY.\n");
        printf("PRESS 4 FOR SCAN SCHEDULING POLICY.\n");
        printf("PRESS 5 FOR C-SCAN SCHEDULING POLICY.\n");
        scanf("%d", &policy_no);

        srand(time(0));

        for (int i = 0; i < 1000; i++)
        { // Generating random requests

            for (int j = 0; j < 3; j++)
            {

                if (j == 0) // Generating random platter number
                    request[i][j] = rand() % 4;

                else if (j == 1) // Generating random track number
                    request[i][j] = rand() % 25;

                else if (j == 2) // Generating random sector number
                    request[i][j] = rand() % 20;
            }
        }

        printf("\nREQUEST\n");

        for (int i = 0; i < 1000; i++)
            printf("REQUEST%d: PLATTER_NO: %d, TRACK_NO: %d, SECTOR_NO: %d\n", i + 1, request[i][0], request[i][1], request[i][2]);

        int k = 0;

        if (policy_no == 4 || policy_no == 5)
        { // If current disck scheduling algorithm is SCAN or C-SCAN then sort the processes first

            sort_request();

            printf("\nAFTER REQUEST SORTING\n");

            for (int i = 0; i < 10; i++)
                printf("REQUEST%d: PLATTER_NO: %d, TRACK_NO: %d, SECTOR_NO: %d\n", i + 1, request[i][0], request[i][1], request[i][2]);
            printf("\n");

            find_pos();
        }

        printf("\n");

        while (k < 1000)
        {

            int idx = find_process_number(); // Find process number in the process queue acording to disck scheduling algorithm and current read write haed position
            printf("REQUEST_%d: REQUEST_INDEX: %d\n", k + 1, idx);

            data[k] = calc_time(request[idx][0], request[idx][1], request[idx][2]); // Storing the respone time of the current process
            printf("RESPONSE_TIME FOR REQUEST_%d: %lfms\n\n", k + 1, data[k] * 1000);

            k++;
        }

        double min_resp_time = find_min_resp_time() * 1000;
        double max_resp_time = find_max_resp_time() * 1000;
        double std_dev = find_std_dev_time() * 1000;
        double avg_resp_time = sum;

        printf("\nMINIMUM_RESPONSE_TIME: %lfms\nMAXIMUM_RESPONSE_TIME: %lfms\nAVERAGE RESPONSE TIME: %lfms\nSTANDARD_DEVIATION_IN_RESPONSE_TIME: %lfms\nTHROUGH_PUT: %d\n", min_resp_time, max_resp_time, avg_resp_time, std_dev, throught_put);
    }

    return 0;
}