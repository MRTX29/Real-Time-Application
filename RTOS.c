/*



*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define TASK_STK_SIZE 512 /* Size of each task's stacks (# of WORDs)            */
#define N_TASKS 15        /* Number of  tasks                          */
#define MyQueueDataSize 100
#define NUL ((void *)0)
/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/

OS_STK TaskStk[N_TASKS][TASK_STK_SIZE]; /* Tasks stacks                                  */
OS_STK TaskStartStk[TASK_STK_SIZE];
OS_STK KeyboardReadTaskStack[TASK_STK_SIZE];
OS_STK KeyProcessTaskStack[TASK_STK_SIZE];
OS_STK SendTaskStack[TASK_STK_SIZE];
OS_STK DisplayTaskStack[TASK_STK_SIZE];
OS_EVENT *FirstQueue;      // Queue
OS_EVENT *KeyboardMailBox; // MailBox KeyboardReadTask -> ProcessTask
OS_EVENT *ProcessMailBox;  // MailBox  ProcessTask -> SendTask
OS_EVENT *Semaphore;       // Semaphore
OS_EVENT *MBox[5];         // Matrix 5 MailBoxes to SendTask -> MailBoxTask
OS_EVENT *SendQueue;       // Matrix 5 Mailboxes to SendTask -> MailBoxTask
OS_MEM *MemForMessages;    // Dynamic memory used for the messages

INT32U TaskData[N_TASKS]; /* Parameters to pass to each task               */
void *MyQueueData[MyQueueDataSize];

INT32U semaphore_load; // Global variable used for the sempahores

struct Message
{
    INT32U load;
    INT32U TaskNumber;
    INT32U iterator;
    INT32U serial_number;
    char status[32];
    INT32U status_code;
};

struct Message MemForMessagesBuf[32];

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

void Task(void *data);              /* Function prototypes of tasks                  */
void TaskStart(void *data);         /* Function prototypes of Startup task           */
void KeyboardReadTask(void *pdata); // Function prototype of KeyboardRead
void KeyProcessTask(void *pdata);   // Function prototype of KeyProcess
void DisplayTask(void *pdata);      // Function prototype of KeyDisplay
void SendTask(void *pdata);
void SemaphoreTask(void *pdata);
void QueueTask(void *pdata);
void MailBoxTask(void *pdata);
static void TaskStartCreateTasks(void);
static void TaskStartDispInit(void);
static void TaskStartDisp(void);

/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void main(void)
{
    PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK); /* Clear the screen                         */

    OSInit(); /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();        /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw); /* Install uC/OS-II's context switch vector */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);
    OSStart(); /* Start multitasking                       */
}

/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void TaskStart(void *pdata)
{
#if OS_CRITICAL_METHOD == 3 /* Allocate storage for CPU status register */
    OS_CPU_SR cpu_sr;
#endif
    char s[100];
    INT16S key;
    INT8U err;
    INT32U i;

    pdata = pdata; /* Prevent compiler warning                 */

    TaskStartDispInit(); /* Initialize the display                   */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);      /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC); /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit(); /* Initialize uC/OS-II's statistics         */

    TaskStartCreateTasks(); /* Create all the application tasks         */

    MemForMessages = OSMemCreate(MemForMessagesBuf, 10, sizeof(struct Message), &err); // Create memory partition
    KeyboardMailBox = OSMboxCreate((void *)0);                                         // Initialise mailboxes
    ProcessMailBox = OSMboxCreate((void *)0);
    FirstQueue = OSQCreate(&MyQueueData[0], MyQueueDataSize); // Initialise the queue to handle messages
    Semaphore = OSSemCreate(1);                               // Global variable semaphore
    SendQueue = OSQCreate(&MyQueueData[0], MyQueueDataSize);  // Initialise 5 mailboxes for tasks

    for (i = 0; i < 5; i++)
    {
        MBox[i] = OSMboxCreate((void *)0);
    }

    s

        for (;;)
    {
        TaskStartDisp(); /* Update the display                       */

        if (PC_GetKey(&key) == TRUE)
        { /* See if key has been pressed              */
            if (key == 0x1B)
            {                   /* Yes, see if it's the ESCAPE key          */
                PC_DOSReturn(); /* Return to DOS                            */
            }
        }

        OSCtxSwCtr = 0;            /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0); /* Wait one second                          */
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                        INITIALIZE THE DISPLAY
*********************************************************************************************************
*/

static void TaskStartDispInit(void)
{
    /*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
    /*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
    PC_DispStr(0, 0, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 1, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 2, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 3, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 4, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 5, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 6, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 7, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 8, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 9, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 10, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 11, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 12, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 13, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 14, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 15, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 16, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 17, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 18, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 19, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 20, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 21, "                                                                                ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 22, "#Tasks          :        CPU Usage:     %                                       ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 23, "#Task switch/sec:                                                               ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
    PC_DispStr(0, 24, "                            <-PRESS 'ESC' TO QUIT->                             ", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY + DISP_BLINK);
    /*                                1111111111222222222233333333334444444444555555555566666666667777777777 */
    /*                      01234567890123456789012345678901234567890123456789012345678901234567890123456789 */
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                           UPDATE THE DISPLAY
*********************************************************************************************************
*/

static void TaskStartDisp(void)
{
    char s[80];

    sprintf(s, "%5d", OSTaskCtr); /* Display #tasks running               */
    PC_DispStr(18, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

#if OS_TASK_STAT_EN > 0
    sprintf(s, "%3d", OSCPUUsage); /* Display CPU usage in %               */
    PC_DispStr(36, 22, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);
#endif

    sprintf(s, "%5d", OSCtxSwCtr); /* Display #context switches per second */
    PC_DispStr(18, 23, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    sprintf(s, "V%1d.%02d", OSVersion() / 100, OSVersion() % 100); /* Display uC/OS-II's version number    */
    PC_DispStr(75, 24, s, DISP_FGND_YELLOW + DISP_BGND_BLUE);

    switch (_8087)
    { /* Display whether FPU present          */
    case 0:
        PC_DispStr(71, 22, " NO  FPU ", DISP_FGND_YELLOW + DISP_BGND_BLUE);
        break;

    case 1:
        PC_DispStr(71, 22, " 8087 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
        break;

    case 2:
        PC_DispStr(71, 22, "80287 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
        break;

    case 3:
        PC_DispStr(71, 22, "80387 FPU", DISP_FGND_YELLOW + DISP_BGND_BLUE);
        break;
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static void TaskStartCreateTasks(void)
{
    int i;
    INT8U err;

    // Create the task responsible for reading keyboard input
    err = OSTaskCreate(KeyboardReadTask, (void *)0, &KeyboardReadTaskStack[TASK_STK_SIZE - 1], 1);
    if (err != OS_NO_ERR)
    {
        PC_DispStr(0, 0, "ERROR WHILE CREATING KEYBOARDREAD TASK:", DISP_FGND_YELLOW);
        if (err == OS_PRIO_EXIST)
            PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
        if (err == OS_PRIO_INVALID)
            PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
        if (err == OS_NO_MORE_TCB)
            PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
    }

    // Create the task responsible for keyboard input analysis
    err = OSTaskCreate(KeyProcessTask, (void *)0, &KeyProcessTaskStack[TASK_STK_SIZE - 1], 2);
    if (err != OS_NO_ERR)
    {
        PC_DispStr(0, 0, "ERROR WHILE CREATING KEYPROCESS TASK:", DISP_FGND_YELLOW);
        if (err == OS_PRIO_EXIST)
            PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
        if (err == OS_PRIO_INVALID)
            PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
        if (err == OS_NO_MORE_TCB)
            PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
    }

    // Create the task responsible for sending the initial load
    err = OSTaskCreate(SendTask, (void *)0, &SendTaskStack[TASK_STK_SIZE - 1], 4);
    if (err != OS_NO_ERR)
    {
        PC_DispStr(0, 0, "ERROR WHILE CREATING SEND TASK:", DISP_FGND_YELLOW);
        if (err == OS_PRIO_EXIST)
            PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
        if (err == OS_PRIO_INVALID)
            PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
        if (err == OS_NO_MORE_TCB)
            PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
    }

    // Create the task responsible for displaying
    err = OSTaskCreate(DisplayTask, (void *)0, &DisplayTaskStack[TASK_STK_SIZE - 1], 3);
    if (err != OS_NO_ERR)
    {
        PC_DispStr(0, 0, "ERROR WHILE CREATING DISPLAY TASK:", DISP_FGND_YELLOW);
        if (err == OS_PRIO_EXIST)
            PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
        if (err == OS_PRIO_INVALID)
            PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
        if (err == OS_NO_MORE_TCB)
            PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
    }

    // Create the queue tasks with priorities 5-9
    for (i = 0; i < 5; i++)
    {
        TaskData[i] = i + 1;
        err = OSTaskCreate(QueueTask, &TaskData[i], &TaskStk[i][TASK_STK_SIZE - 1], i + 5);
        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "ERROR WHILE CREATING QUEUE TASK:", DISP_FGND_YELLOW);
            if (err == OS_PRIO_EXIST)
                PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
            if (err == OS_PRIO_INVALID)
                PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
            if (err == OS_NO_MORE_TCB)
                PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
        }
    }

    // Create the mailboxes tasks with priorities 10-14
    for (i = 0; i < 5; i++)
    {
        TaskData[i + 5] = i + 6;
        err = OSTaskCreate(MailBoxTask, &TaskData[i + 5], &TaskStk[i + 5][TASK_STK_SIZE - 1], i + 10);
        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "ERROR WHILE CREATING MAILBOX TASK:", DISP_FGND_YELLOW);
            if (err == OS_PRIO_EXIST)
                PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
            if (err == OS_PRIO_INVALID)
                PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
            if (err == OS_NO_MORE_TCB)
                PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
        }
    }

    // Create the semaphore tasks with priorities 15-19
    for (i = 0; i < 5; i++)
    {
        TaskData[i + 10] = i + 11;
        err = OSTaskCreate(SemaphoreTask, &TaskData[i + 10], &TaskStk[i + 10][TASK_STK_SIZE - 1], i + 15);
        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "ERROR WHILE CREATING SEMAPHORE TASK:", DISP_FGND_YELLOW);
            if (err == OS_PRIO_EXIST)
                PC_DispStr(0, 1, "OS_PRIO_EXIST", DISP_FGND_YELLOW);
            if (err == OS_PRIO_INVALID)
                PC_DispStr(0, 1, "OS_PRIO_INVALID", DISP_FGND_YELLOW);
            if (err == OS_NO_MORE_TCB)
                PC_DispStr(0, 1, "OS_NO_MORE_TCB", DISP_FGND_YELLOW);
        }
    }
}

// Read the keyboard input. This tasks does not proceed input analysis
void KeyboardReadTask(void *pdata)
{
    INT8U err;
    INT16S new_key;
    pdata = pdata;

    for (;;)
    {
        if (PC_GetKey(&new_key)) // Checks whether user pushed any key
        {

            err = OSMboxPost(KeyboardMailBox, &new_key); // Send input to the mailbox

            if (err != OS_NO_ERR) // Error handler
            {
                PC_DispStr(0, 0, "MBOX ERROR IN READ TASK:", DISP_FGND_YELLOW);
                if (err == OS_MBOX_FULL)
                    PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                if (err == OS_ERR_EVENT_TYPE)
                    PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                if (err == OS_ERR_PEVENT_NULL)
                    PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
                if (err == OS_ERR_POST_NULL_PTR)
                    PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
            }
        }

        OSTimeDly(6);
    }
}

// Analysis of the user input
void KeyProcessTask(void *pdata)
{
    INT8U err, err2;
    INT16S new_key;
    INT32U local_load = 1;   // Load set by the user with the default value of 1
    char *key_pointer;       // Pointer to the received load
    char LocalBuf[32] = {0}; // Buffer to handle user keyboard input
    char ClearBuf[80] = {0};
    char ClearBuf2[20] = {0};
    struct Message message; // Message containing all the information about the load
    INT32U i = 0;
    int e;
    pdata = pdata;

    for (;;)
    {
        key_pointer = OSMboxPend(KeyboardMailBox, 0, &err); // Read the user keyboard input

        if (err == OS_NO_ERR)
        {
            new_key = *key_pointer; // Conversion (char)pointer to INT16S

            // If the key is ESC?
            if (new_key == 0x1B)
            {
                PC_DOSReturn();
            }

            // If the key is the number between 0-9?
            if (new_key >= 48 && new_key <= 57)
            {
                LocalBuf[i] = *key_pointer;
                i++;
            }

            // If the key is backspace?
            if (new_key == 0x08)
            {
                i--;
                ;
                LocalBuf[i] = ' ';
            }
            PC_DispStr(0, 0, LocalBuf, DISP_FGND_YELLOW);

            // If the key is enter?
            if (new_key == 0x0D)
            {
                // Conversion buffer to INT32U
                local_load = strtoul(LocalBuf, (void *)0, 0);

                // Clearing the buffer
                memset(LocalBuf, ' ', i * sizeof(INT16S));

                // Set the counter
                i = 0;

                // Clear the screen
                memset(ClearBuf, ' ', 80);
                for (e = 0; e < 2; e++)
                {
                    PC_DispStr(0, e, ClearBuf, DISP_FGND_YELLOW + DISP_BGND_LIGHT_GRAY);
                }

                for (e = 4; e < 21; e++)
                {
                    PC_DispStr(65, e, ClearBuf2, DISP_FGND_YELLOW + DISP_BGND_LIGHT_GRAY);
                }

                // Send the load to the next buffer
                err = OSMboxPost(ProcessMailBox, &local_load);

                // Save the task number and current load
                message.load = local_load;

                // Unique Task ID
                message.TaskNumber = 0;

                // Error handler
                if (err != OS_NO_ERR)
                {
                    PC_DispStr(0, 0, "MBOX ERROR WHILE SENDING VALUE IN PROCESS TASK:", DISP_FGND_YELLOW);
                    if (err == OS_MBOX_FULL)
                        PC_DispStr(0, 1, "OS_MBOX_FULL", DISP_FGND_YELLOW);
                    if (err == OS_ERR_EVENT_TYPE)
                        PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                    if (err == OS_ERR_PEVENT_NULL)
                        PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
                    if (err == OS_ERR_POST_NULL_PTR)
                        PC_DispStr(0, 1, "OS_ERR_POST_NULL_PTR", DISP_FGND_YELLOW);
                }

                // Send the message containing load information
                err2 = OSQPost(FirstQueue, &message);

                // Error handler
                if (err2 != OS_NO_ERR)
                {
                    PC_DispStr(0, 0, "QUEUE ERROR WHILE SENDING VALUE IN PROCESS TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
                    if (err2 == OS_Q_FULL)
                        PC_DispStr(0, 1, "OS_Q_FULL", DISP_FGND_YELLOW);
                    if (err2 == OS_ERR_EVENT_TYPE)
                        PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                    if (err2 == OS_ERR_PEVENT_NULL)
                        PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
                }
            }
        }

        // Error handler
        else
        {
            PC_DispStr(0, 0, "MBOX ERROR WHILE DOWNLOADING VALUE IN PROCESS TASK:", DISP_FGND_YELLOW);
            if (err == OS_MBOX_FULL)
                PC_DispStr(0, 1, "OS_MBOX_FULL", DISP_FGND_YELLOW);
            if (err == OS_ERR_EVENT_TYPE)
                PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
            if (err == OS_ERR_PEVENT_NULL)
                PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
            if (err == OS_ERR_POST_NULL_PTR)
                PC_DispStr(0, 1, "OS_ERR_POST_NULL_PTR", DISP_FGND_YELLOW);
        }
    }
}

void DisplayTask(void *pdata)
{
    struct Message *message_pointer;
    char string_to_display[80];
    char first_message[32];
    char DeltaBuf[80];
    char DeltaBuf2[80];
    char DeltaBuf3[80];
    char HelpBuf[80];
    INT32U current_counters[15] = {0};
    INT32U previous_counters[15] = {0};
    INT32U delta[15] = {0};
    INT32U i;
    INT32U ticks;
    INT32U previous_ticks = 0;
    INT8U err;
    pdata = pdata;

    for (;;)
    {
        // Get the clock ticks
        ticks = OSTimeGet();

        // Display and calculate the differences
        if (ticks - previous_ticks >= 222)
        {
            previous_ticks = ticks;

            for (i = 0; i < 15; i++)
            {
                delta[i] = (current_counters[i] - previous_counters[i]);
                previous_counters[i] = current_counters[i];

                sprintf(DeltaBuf, "%5lu", delta[i]);
                PC_DispStr(36, i + 5, DeltaBuf, DISP_FGND_YELLOW);
            }
        }

        // Get the messages from the queues
        message_pointer = OSQPend(FirstQueue, 0, &err);

        if (err == OS_NO_ERR)
        {
            // Message send from the keyboard task
            if (message_pointer->TaskNumber == 0)
            {
                sprintf(first_message, "%lu", message_pointer->load);
                PC_DispStr(0, 1, "User set the load:", DISP_FGND_YELLOW);
                PC_DispStr(0, 2, first_message, DISP_FGND_YELLOW);
            }
            else
            {
                PC_DispStr(0, 4, "Load: Task Number: Counter: Delta: Status:", DISP_FGND_YELLOW);

                if (message_pointer->status_code == 0)
                {
                    // Get the current task counter
                    current_counters[(message_pointer->TaskNumber) - 1] = message_pointer->iterator;
                }

                // Check whether the message has come from the semaphore task
                if (message_pointer->TaskNumber >= 11)
                {
                    sprintf(string_to_display, "%10lu %10luS %10lu", message_pointer->load, message_pointer->TaskNumber,
                            message_pointer->iterator);

                    PC_DispStr(0, (message_pointer->TaskNumber) + 4, string_to_display, DISP_FGND_YELLOW);
                }

                // Check whether the message has come from the queue task
                if (message_pointer->TaskNumber >= 1 && message_pointer->TaskNumber <= 5)
                {
                    if (message_pointer->status_code == 0)
                    {
                        sprintf(string_to_display, "%10lu %10luQ %10lu", message_pointer->load, message_pointer->TaskNumber,
                                message_pointer->iterator);
                        PC_DispStr(0, (message_pointer->TaskNumber) + 4, string_to_display, DISP_FGND_YELLOW);
                    }
                    if (message_pointer->status_code == 1)
                    {
                        PC_DispStr(42, message_pointer->TaskNumber + 4, message_pointer->status, DISP_FGND_YELLOW);
                    }
                }

                // Check whether the message has come from the mailbox task
                if (message_pointer->TaskNumber >= 6 && message_pointer->TaskNumber <= 10)
                {
                    if (message_pointer->status_code == 0)
                    {
                        sprintf(string_to_display, "%10lu %10luM %10lu", message_pointer->load, message_pointer->TaskNumber,
                                message_pointer->iterator);
                        PC_DispStr(0, message_pointer->TaskNumber + 4, string_to_display, DISP_FGND_YELLOW);
                    }
                    if (message_pointer->status_code == 1)
                    {
                        PC_DispStr(42, message_pointer->TaskNumber + 4, message_pointer->status, DISP_FGND_YELLOW);
                    }
                }
            }
        }
    }
}

void SendTask(void *pdata)
{
    INT32U *load_pointer;
    INT8U err;
    INT32U i;
    INT32U local_serial_number = 1;
    struct Message *message_pointer;
    struct Message queue_error_message;
    pdata = pdata;

    for (;;)
    {
        // Get the load from mailbox
        load_pointer = OSMboxPend(ProcessMailBox, 0, &err);

        if (err == OS_NO_ERR)
        {
            // Send the load to the queue
            for (i = 0; i < 5; i++)
            {
                // Dynamic memory allocation
                message_pointer = OSMemGet(MemForMessages, &err);

                // Error handler
                if (err == OS_NO_ERR)
                {
                    // Set the task ID and load to the message
                    message_pointer->load = *load_pointer;
                    message_pointer->TaskNumber = i + 1;
                    message_pointer->status_code = 0;
                    message_pointer->serial_number = local_serial_number;

                    local_serial_number++;

                    // Send the message to the queue
                    err = OSQPost(SendQueue, message_pointer);

                    if (err != OS_NO_ERR)
                    {
                        PC_DispStr(0, 0, "QUEUE ERROR WHILE SENDING VALUE IN SEND TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
                        if (err == OS_Q_FULL)
                            PC_DispStr(0, 4, "OS_Q_FULL", DISP_FGND_YELLOW);
                        if (err == OS_ERR_EVENT_TYPE)
                            PC_DispStr(0, 4, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                        if (err == OS_ERR_PEVENT_NULL)
                            PC_DispStr(0, 4, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
                    }
                }
                else if (err == OS_MEM_NO_FREE_BLKS)
                {
                    // Error handler
                    queue_error_message.TaskNumber = i + 1;
                    queue_error_message.status_code = 1;
                    sprintf(queue_error_message.status, "Utracono: %lu", *load_pointer);
                    err = OSQPostFront(FirstQueue, &queue_error_message);
                }
            }

            // Send the load to the mailboxes
            for (i = 0; i < 5; i++)
            {
                // Dynamic memory allocation
                message_pointer = OSMemGet(MemForMessages, &err);

                if (err == OS_NO_ERR)
                {
                    // Set the task ID and load to the message
                    message_pointer->load = *load_pointer;
                    message_pointer->TaskNumber = i + 6;

                    // Send the message to the mailbox
                    err = OSMboxPost(MBox[i], message_pointer);

                    if (err != OS_NO_ERR)
                    {

                        if (err == OS_ERR_EVENT_TYPE)
                            PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                        if (err == OS_ERR_PEVENT_NULL)
                            PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
                        if (err == OS_ERR_POST_NULL_PTR)
                            PC_DispStr(0, 1, "OS_ERR_POST_NULL_PTR", DISP_FGND_YELLOW);
                    }
                }

                else if (err == OS_MEM_NO_FREE_BLKS)
                {
                    // Error handler
                    queue_error_message.TaskNumber = i + 6;
                    queue_error_message.status_code = 1;
                    sprintf(queue_error_message.status, "Utracono: %lu", *load_pointer);
                    err = OSQPostFront(FirstQueue, &queue_error_message);
                }
            }

            // Set the load for the semaphore task
            OSSemPend(Semaphore, 0, &err);

            if (err != OS_NO_ERR)
            {
                PC_DispStr(0, 0, "SEMAPHORE ERROR IN SEND TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
                if (err == OS_TIMEOUT)
                    PC_DispStr(0, 1, "OS_TIMEOUT", DISP_FGND_YELLOW);
                if (err == OS_ERR_EVENT_TYPE)
                    PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
                if (err == OS_ERR_PEND_ISR)
                    PC_DispStr(0, 1, "OS_ERR_PEND_ISR", DISP_FGND_YELLOW);
            }

            semaphore_load = *load_pointer;
            OSSemPost(Semaphore);
        }
    }
}

void SemaphoreTask(void *pdata)
{
    INT32U local_load;
    INT8U err;
    INT32U i;
    INT32U e;
    INT32U *number_pointer;
    INT32U task_counter = 0;
    struct Message message;

    // Set the task ID
    number_pointer = pdata;
    message.TaskNumber = *number_pointer;

    for (;;)
    {

        // Get the load from global variable
        OSSemPend(Semaphore, 0, &err);

        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "SEMAPHORE ERROR IN SEND TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
            if (err == OS_TIMEOUT)
                PC_DispStr(0, 1, "OS_TIMEOUT", DISP_FGND_YELLOW);
            if (err == OS_ERR_EVENT_TYPE)
                PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
            if (err == OS_ERR_PEND_ISR)
                PC_DispStr(0, 1, "OS_ERR_PEND_ISR", DISP_FGND_YELLOW);
        }

        local_load = semaphore_load;
        OSSemPost(Semaphore);

        // Prepare the message
        message.load = local_load;
        message.iterator = task_counter;
        message.status_code = 0;
        err = OSQPost(FirstQueue, &message);

        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "QUEUE ERROR WHILE SENDING MESSAGE IN SEMAPHORE TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
            if (err == OS_Q_FULL)
                PC_DispStr(0, 1, "OS_Q_FULL", DISP_FGND_YELLOW);
            if (err == OS_ERR_EVENT_TYPE)
                PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
            if (err == OS_ERR_PEVENT_NULL)
                PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
        }

        // Do the task
        for (i = 0; i <= local_load; i++)
        {
            e++;
        }

        task_counter++;
        OSTimeDly(1);
    }
}

void QueueTask(void *pdata)
{
    INT32U local_load = 0;
    INT8U err, err2;
    INT32U i;
    INT32U e;
    INT32U task_counter = 0;
    INT32U local_serial_number = 0;
    INT32U *number_pointer;
    OS_Q_DATA QueueData;
    struct Message *local_message_pointer;
    struct Message message;

    // Set the unique task ID
    number_pointer = pdata;
    message.TaskNumber = *number_pointer;

    for (;;)
    {
        // Scan the queue
        err = OSQQuery(SendQueue, &QueueData);
        local_message_pointer = QueueData.OSMsg;

        // Check if the next message in the queue is destined for this task
        if (local_message_pointer->TaskNumber == *number_pointer && local_serial_number < local_message_pointer->serial_number)
        {
            // Get the load from the queue
            local_message_pointer = OSQAccept(SendQueue);

            // Set the new serial number
            local_serial_number = local_message_pointer->serial_number;
        }
        else
        {
            local_message_pointer = NUL;
        }

        if (local_message_pointer != NUL)
        {
            // Prepare the message
            local_load = local_message_pointer->load;

            // Release the memory
            OSMemPut(MemForMessages, local_message_pointer);
        }

        // Prepare the message
        message.load = local_load;
        message.iterator = task_counter;

        // Send the message
        err2 = OSQPost(FirstQueue, &message);

        if (err2 != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "QUEUE ERROR WHILE SENDING MESSAGE IN QUEUE TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
            if (err2 == OS_Q_FULL)
                PC_DispStr(0, 1, "OS_Q_FULL", DISP_FGND_YELLOW);
            if (err2 == OS_ERR_EVENT_TYPE)
                PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
            if (err2 == OS_ERR_PEVENT_NULL)
                PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
        }

        // Do the task
        for (i = 0; i <= message.load; i++)
        {
            e++;
        }

        task_counter++;
        OSTimeDly(1);
    }
}

void MailBoxTask(void *pdata)
{
    INT32U local_load = 0;
    INT32U *local_load_pointer;
    INT8U err;
    INT32U i;
    INT32U e;
    INT32U *number_pointer;
    INT32U task_counter = 0;
    struct Message *local_message_pointer;
    struct Message message;

    // Set the unique task ID
    number_pointer = pdata;
    message.TaskNumber = *number_pointer;

    for (;;)
    {

        // Get the load from the Mailbox
        local_message_pointer = OSMboxAccept(MBox[*number_pointer - 6]);

        if (local_message_pointer != NUL)
        {
            local_load = local_message_pointer->load;

            // Release the memory
            OSMemPut(MemForMessages, local_message_pointer);
        }

        // Prepare the message
        message.load = local_load;
        message.iterator = task_counter;
        err = OSQPost(FirstQueue, &message);

        if (err != OS_NO_ERR)
        {
            PC_DispStr(0, 0, "QUEUE ERROR WHILE SENDING MESSAGE IN MAILBOX TASK", DISP_FGND_BLACK + DISP_BGND_LIGHT_GRAY);
            if (err == OS_Q_FULL)
                PC_DispStr(0, 1, "OS_Q_FULL", DISP_FGND_YELLOW);
            if (err == OS_ERR_EVENT_TYPE)
                PC_DispStr(0, 1, "OS_ERR_EVENT_TYPE", DISP_FGND_YELLOW);
            if (err == OS_ERR_PEVENT_NULL)
                PC_DispStr(0, 1, "OS_ERR_PEVENT_NULL", DISP_FGND_YELLOW);
        }

        // Do the task
        for (i = 0; i <= message.load; i++)
        {
            e++;
        }

        task_counter++;
        OSTimeDly(1);
    }
}
