/****************************************************************************
 *
 * MODULE:             Firmware Distribution Daemon
 *
 * COMPONENT:          Threads/mutexes/queues
 *
 * REVISION:           $Revision: 51418 $
 *
 * DATED:              $Date: 2013-01-10 09:45:27 +0000 (Thu, 10 Jan 2013) $
 *
 * AUTHOR:             Matt Redfearn
 *
 ****************************************************************************
 *
 * This software is owned by NXP B.V. and/or its supplier and is protected
 * under applicable copyright laws. All rights are reserved. We grant You,
 * and any third parties, a license to use this software solely and
 * exclusively on NXP products [NXP Microcontrollers such as JN5148, JN5142, JN5139]. 
 * You, and any third parties must reproduce the copyright and warranty notice
 * and any other legend of ownership on each copy or partial copy of the 
 * software.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.

 * Copyright NXP B.V. 2012. All rights reserved
 *
 ***************************************************************************/

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <sys/time.h>

#include "Threads.h"

/************************** Threads Functionality ****************************/

/** Structure representing an OS independant thread */
typedef struct
{
#ifndef WIN32
    pthread_t thread;
#else
    int thread_pid;
    HANDLE thread_handle;
#endif /* WIN32 */
} tsThreadPrivate;



teThreadStatus eThreadStart(tprThreadFunction prThreadFunction, tsThread *psThreadInfo)
{
    tsThreadPrivate *psThreadPrivate;
    
    psThreadPrivate = malloc(sizeof(tsThreadPrivate));
    if (!psThreadPrivate)
    {
        return E_THREAD_ERROR_NO_MEM;
    }
    
    psThreadInfo->pvPriv = psThreadPrivate;
    
#ifndef WIN32
    if (pthread_create(&psThreadPrivate->thread, NULL,
        prThreadFunction, psThreadInfo))
    {
        perror("Could not start thread");
        return E_THREAD_ERROR_FAILED;
    }
    if (pthread_detach(psThreadPrivate->thread))
    {
        perror("pthread_detach()");
    }
#else
    psThreadPrivate->thread_handle = CreateThread(
        NULL,                           // default security attributes
        0,                              // use default stack size  
        prThreadFunction,               // thread function name
        psThreadInfo->pvData,           // argument to thread function 
        0,                              // use default creation flags 
        &psThreadPrivate->thread_pid);  // returns the thread identifier
    if (!psThreadPrivate->thread_handle)
    {
        perror("Could not start thread");
        return E_THREAD_ERROR_FAILED;
    }
#endif /* WIN32 */
    return  E_THREAD_OK;
}


teThreadStatus eThreadFinish(tprThreadFunction prThreadFunction, tsThread *psThreadInfo)
{
    /* Free the Private data */
    tsThreadPrivate *psThreadPrivate = (tsThreadPrivate *)psThreadInfo->pvPriv;
    free(psThreadPrivate);
    
#ifndef WIN32
    /* Cleanup function is called when pthread quits */
    pthread_exit(NULL);
#else
    ExitThread(0);
#endif /* WIN32 */
    return E_THREAD_OK; /* Control won't get here */
}

/************************** Lock Functionality *******************************/


typedef struct
{
#ifndef WIN32
     pthread_mutex_t mutex;
#else
     
#endif /* WIN32 */
} tsLockPrivate;

teLockStatus eLockCreate(tsLock *psLock)
{
    tsLockPrivate *psLockPrivate;
    
    psLockPrivate = malloc(sizeof(tsLockPrivate));
    if (!psLockPrivate)
    {
        return E_LOCK_ERROR_NO_MEM;
    }
    
    psLock->pvPriv = psLockPrivate;
    
#ifndef WIN32
    pthread_mutex_init(&psLockPrivate->mutex, NULL);
#else
    
#endif 
    return E_LOCK_OK;
}


teLockStatus eLockDestroy(tsLock *psLock)
{
    tsLockPrivate *psLockPrivate = (tsLockPrivate *)psLock->pvPriv;
#ifndef WIN32
    pthread_mutex_destroy(&psLockPrivate->mutex);
#else
    
#endif 
    free(psLockPrivate);
    return E_LOCK_OK;
}


teLockStatus eLockLock(tsLock *psLock)
{
    tsLockPrivate *psLockPrivate = (tsLockPrivate *)psLock->pvPriv;
#ifndef WIN32
    pthread_mutex_lock(&psLockPrivate->mutex);
#else
    
    
#endif /* WIN32 */
    return E_LOCK_OK;
}


teLockStatus eLockLockTimed(tsLock *psLock, uint32_t u32WaitTimeoutMs)
{
    tsLockPrivate *psLockPrivate = (tsLockPrivate *)psLock->pvPriv;
    
#ifndef WIN32
    {
        struct timeval sNow;
        struct timespec sTimeout;
        
        gettimeofday(&sNow, NULL);
        sTimeout.tv_sec = sNow.tv_sec + (u32WaitTimeoutMs/1000);
        sTimeout.tv_nsec = (sNow.tv_usec + ((u32WaitTimeoutMs % 1000) * 1000)) * 1000;
        if (sTimeout.tv_nsec > 1000000000)
        {
            sTimeout.tv_sec++;
            sTimeout.tv_nsec -= 1000000000;
        }

        switch (pthread_mutex_timedlock(&psLockPrivate->mutex, &sTimeout))
        {
            case (0):
                return E_LOCK_OK;
                break;
                
            case (ETIMEDOUT):
                return E_LOCK_ERROR_TIMEOUT;
                break;

            default:
                return E_LOCK_ERROR_FAILED;
                break;
        }
    }
    
#else
    
    
#endif /* WIN32 */
    
}


teLockStatus eLockUnlock(tsLock *psLock)
{
    tsLockPrivate *psLockPrivate = (tsLockPrivate *)psLock->pvPriv;
    
#ifndef WIN32
    pthread_mutex_unlock(&psLockPrivate->mutex);
#else
    
    
#endif /* WIN32 */
    return E_LOCK_OK;
}


/************************** Queue Functionality ******************************/


typedef struct
{
    void **apvBuffer;
    uint32_t u32Capacity;
    uint32_t u32Size;
    uint32_t u32In;
    uint32_t u32Out;

#ifndef WIN32
    pthread_mutex_t mutex;    
    pthread_cond_t cond_space_available;
    pthread_cond_t cond_data_available;
#else
     
#endif /* WIN32 */
} tsQueuePrivate;


teQueueStatus eQueueCreate(tsQueue *psQueue, uint32_t u32Capacity)
{
    tsQueuePrivate *psQueuePrivate;
    
    psQueuePrivate = malloc(sizeof(tsQueuePrivate));
    if (!psQueuePrivate)
    {
        return E_QUEUE_ERROR_NO_MEM;
    }
    
    psQueue->pvPriv = psQueuePrivate;
    
    psQueuePrivate->apvBuffer = malloc(sizeof(void *) * u32Capacity);
    
    if (!psQueuePrivate->apvBuffer)
    {
        free(psQueue->pvPriv);
        return E_QUEUE_ERROR_NO_MEM;
    }
    
    psQueuePrivate->u32Capacity = u32Capacity;
    psQueuePrivate->u32Size = 0;
    psQueuePrivate->u32In = 0;
    psQueuePrivate->u32Out = 0;
    
#ifndef WIN32
    pthread_mutex_init(&psQueuePrivate->mutex, NULL);
    pthread_cond_init(&psQueuePrivate->cond_space_available, NULL);
    pthread_cond_init(&psQueuePrivate->cond_data_available, NULL);
#else
    
#endif 
    return E_LOCK_OK;
    
}


teQueueStatus eQueueDestroy(tsQueue *psQueue)
{
    tsQueuePrivate *psQueuePrivate = (tsQueuePrivate*)psQueue->pvPriv;
    
    free(psQueuePrivate->apvBuffer);
    
#ifndef WIN32
    pthread_mutex_destroy(&psQueuePrivate->mutex);
    pthread_cond_destroy(&psQueuePrivate->cond_space_available);
    pthread_cond_destroy(&psQueuePrivate->cond_data_available);
#else
    
#endif 
    free(psQueuePrivate);
    return E_QUEUE_OK;
}


teQueueStatus eQueueQueue(tsQueue *psQueue, void *pvData)
{
    tsQueuePrivate *psQueuePrivate = (tsQueuePrivate*)psQueue->pvPriv;
#ifndef WIN32
    pthread_mutex_lock(&psQueuePrivate->mutex);
#endif /* WIN32 */
    while (psQueuePrivate->u32Size == psQueuePrivate->u32Capacity)
#ifndef WIN32
        pthread_cond_wait(&psQueuePrivate->cond_space_available, &psQueuePrivate->mutex);
#endif /* WIN32 */
    psQueuePrivate->apvBuffer[psQueuePrivate->u32In] = pvData;
    ++psQueuePrivate->u32Size;
    
    psQueuePrivate->u32In = (psQueuePrivate->u32In+1) % psQueuePrivate->u32Capacity;
    
#ifndef WIN32
    pthread_mutex_unlock(&psQueuePrivate->mutex);
    pthread_cond_broadcast(&psQueuePrivate->cond_data_available);
#endif /* WIN32 */
    return E_QUEUE_OK;
}


teQueueStatus eQueueDequeue(tsQueue *psQueue, void **ppvData)
{
    tsQueuePrivate *psQueuePrivate = (tsQueuePrivate*)psQueue->pvPriv;
#ifndef WIN32
    pthread_mutex_lock(&psQueuePrivate->mutex);
#endif /* WIN32 */
    while (psQueuePrivate->u32Size == 0)
#ifndef WIN32
        pthread_cond_wait(&psQueuePrivate->cond_data_available, &psQueuePrivate->mutex);
#endif /* WIN32 */
    
    *ppvData = psQueuePrivate->apvBuffer[psQueuePrivate->u32Out];
    --psQueuePrivate->u32Size;
    
    psQueuePrivate->u32Out = (psQueuePrivate->u32Out + 1) % psQueuePrivate->u32Capacity;
#ifndef WIN32
    pthread_mutex_unlock(&psQueuePrivate->mutex);
    pthread_cond_broadcast(&psQueuePrivate->cond_space_available);
#endif /* WIN32 */
    return E_QUEUE_OK;
}


teQueueStatus eQueueDequeueTimed(tsQueue *psQueue, uint32_t u32WaitTimeout, void **ppvData)
{
    tsQueuePrivate *psQueuePrivate = (tsQueuePrivate*)psQueue->pvPriv;
#ifndef WIN32
    pthread_mutex_lock(&psQueuePrivate->mutex);
#endif /* WIN32 */
    while (psQueuePrivate->u32Size == 0)
#ifndef WIN32
    {
        struct timeval sNow;
        struct timespec sTimeout;
        
        memset(&sNow, 0, sizeof(struct timeval));
        gettimeofday(&sNow, NULL);
        sTimeout.tv_sec = sNow.tv_sec + u32WaitTimeout;
        sTimeout.tv_nsec = sNow.tv_usec * 1000;

        switch (pthread_cond_timedwait(&psQueuePrivate->cond_data_available, &psQueuePrivate->mutex, &sTimeout))
        {
            case (0):
                break;
            
            case (ETIMEDOUT):
                pthread_mutex_unlock(&psQueuePrivate->mutex);
                return E_QUEUE_ERROR_TIMEOUT;
                break;
            
            default:
                pthread_mutex_unlock(&psQueuePrivate->mutex);
                return E_QUEUE_ERROR_FAILED;
        }
    }
#endif /* WIN32 */
    
    *ppvData = psQueuePrivate->apvBuffer[psQueuePrivate->u32Out];
    --psQueuePrivate->u32Size;
    
    psQueuePrivate->u32Out = (psQueuePrivate->u32Out + 1) % psQueuePrivate->u32Capacity;
#ifndef WIN32
    pthread_mutex_unlock(&psQueuePrivate->mutex);
    pthread_cond_broadcast(&psQueuePrivate->cond_space_available);
#endif /* WIN32 */
    return E_QUEUE_OK;
}




