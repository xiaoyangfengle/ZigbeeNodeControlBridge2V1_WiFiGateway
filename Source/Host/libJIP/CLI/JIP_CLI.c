/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:          JIP test program
 *
 * REVISION:           $Revision: 36357 $
 *
 * DATED:              $Date: 2011-11-02 11:52:12 +0000 (Wed, 02 Nov 2011) $
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <libtecla.h>

#include <JIP.h>


/* This is the default location for storing the JIP cache files */
#define CACHE_DEFINITIONS_FILE_NAME "/tmp/jip_cache_definitions.xml"
#define CACHE_NETWORK_FILE_NAME "/tmp/jip_cache_network.xml"


/** Delimeter symbol used for --exec command line */
#define COMMAND_DELIMETER ";"


/** JIP context structure */
static tsJIP_Context sJIP_Context;


#ifndef VERSION
#error Version is not defined!
#else
const char *Version = "0.7 (r" VERSION ")";
#endif


/** When non-zero, this global means the user is done using this program. */
static int iQuit = 0;

/** Port to conenct to */
static int iPort = JIP_DEFAULT_PORT;

/* Filter variables */
static char *filter_ipv6 = NULL;
static char *filter_device = NULL;
static char *filter_mib = NULL;
static char *filter_var = NULL;


/** Utility function to return the difference between 2 timevals in milliseconds.
 *  \param      sNewer      Newer time value
 *  \param      sOlder      Older time value
 *  \return uint32_t containing number of milliseconds elapsed between sOlder and sNewer
 */
static uint32_t u32diff_timeval(struct timeval sNewer, struct timeval sOlder)
{
    return ((sNewer.tv_sec - sOlder.tv_sec) * 1000) + ((sNewer.tv_usec - sOlder.tv_usec)/1000);
}


/* Callback function types from \ref jip_iterate */
typedef int(*tprCbNode) (tsNode *psNode, void *pvUser);
typedef int(*tprCbMib)  (tsMib *psMib, void *pvUser);
typedef int(*tprCbVar)  (tsVar *psVar, void *pvUser);


/** General purpose function to iterate over the known devices,
 *  filtering on known items and call function callbacks as
 *  required for each node / mib / variable that matches.
 *  \param prCbNode     Callback function to call per matching node
 *  \param pvNodeUser   User data to pass to node callback
 *  \param prCbMib      Callback function to call per matching Mib
 *  \param pvMibUser    User data to pass to Mib callback
 *  \param prCbVar      Callback function to call per matching node
 *  \param pvVarUser    User data to pass to Var callback
 *  \return non-zero on success.
 */
static int jip_iterate(tprCbNode prCbNode,   void *pvNodeUser, 
                        tprCbMib prCbMib,     void *pvMibUser, 
                        tprCbVar prCbVar,     void *pvVarUser)    
{
    tsNode *psNode;
    tsMib *psMib;
    tsVar *psVar;
    tsJIPAddress   *NodeAddressList = NULL;
    uint32_t        u32NumNodes = 0;
    uint32_t        NodeIndex;
    uint32_t        Device_ID;
    struct in6_addr node_addr;
    int is_multicast = 0;
    
    if (filter_ipv6)
    {
        if (inet_pton(AF_INET6, filter_ipv6, &node_addr) != 1)
        {
            fprintf(stderr, "Invalid IPv6 address '%s'\n\r", filter_ipv6);
            return 0;
        }
        
        if ((strncmp(filter_ipv6, "FF", 2) == 0) || (strncmp(filter_ipv6, "ff", 2) == 0))
        {
            is_multicast = 1;
        }
    }
    
    if (filter_device)
    {
        char *pcEnd;
        errno = 0;
        Device_ID = strtoul(filter_device, &pcEnd, 0);
        if (errno)
        {
            fprintf(stderr, "Device ID '%s' cannot be converted to 32 bit integer (%s)\n\r", filter_device, strerror(errno));
            return 0;
        }
        if (*pcEnd != '\0')
        {
            fprintf(stderr, "Device ID '%s' contains invalid characters\n\r", filter_device);
            return 0;
        }
    }
    else
    {
        Device_ID = E_JIP_DEVICEID_ALL;
    }
    
    if (eJIP_GetNodeAddressList(&sJIP_Context, Device_ID, &NodeAddressList, &u32NumNodes) != E_JIP_OK)
    {
        fprintf(stderr, "Error reading node list\n");
        return 0;
    }

    for (NodeIndex = 0; NodeIndex < u32NumNodes; NodeIndex++)
    {
        psNode = psJIP_LookupNode(&sJIP_Context, &NodeAddressList[NodeIndex]);
        if (!psNode)
        {
            fprintf(stderr, "Node has been removed\n");
            continue;
        }

        if (filter_ipv6)
        {
            if (!is_multicast)
            {
                /* Filter on IPv6 address */
                if (memcmp(&psNode->sNode_Address.sin6_addr, &node_addr, sizeof(struct in6_addr)) != 0)
                {
                    /* Not the node we want */
                    eJIP_UnlockNode(psNode);
                    continue;
                }
            }
        }

        if (prCbNode)
        {
            /* Call node callback */
            if (!prCbNode(psNode, pvNodeUser))
            {
                eJIP_UnlockNode(psNode);
                goto done;
            }
        }
        
        psMib = psNode->psMibs;
        while (psMib)
        {
            if (filter_mib)
            {
                char *pcEnd;
                uint32_t u32MibId;
                
                /* Filtering on MiB */

                errno = 0;
                u32MibId = strtoul(filter_mib, &pcEnd, 0);
                if (errno)
                {
                    fprintf(stderr, "MiB ID '%s' cannot be converted to 32 bit integer (%s)\n\r", filter_mib, strerror(errno));
                    return 0;
                }

                if ((pcEnd != filter_mib) && (*pcEnd == '\0')) /* Whole string has been converted - must be a legit number */
                {
                    if (u32MibId != psMib->u32MibId)
                    {
                        psMib = psMib->psNext;
                        continue;
                    }
                }
                else
                {   
                    if (strcmp(filter_mib, psMib->pcName) != 0)
                    {
                        psMib = psMib->psNext;
                        continue;
                    }
                }
            }
            
            if (prCbMib)
            {
                /* Call node callback */
                if (!prCbMib(psMib, pvMibUser))
                {
                    eJIP_UnlockNode(psNode);
                    goto done;
                }
            }
            
            psVar = psMib->psVars;
            while (psVar)
            {
                if (filter_var)
                {
                    char *pcEnd;
                    uint32_t u32VarIndex;
                    
                    /* Filtering on Var */

                    errno = 0;
                    u32VarIndex = strtoul(filter_var, &pcEnd, 0);
                    if ((errno) || ((u32VarIndex > 0x000000FF) ? (errno=ERANGE) : (errno=0)))
                    {
                        fprintf(stderr, "Var Index '%s' cannot be converted to 8 bit integer (%s)\n\r", filter_var, strerror(errno));
                        return 0;
                    }

                    if ((pcEnd != filter_var) && (*pcEnd == '\0')) /* Whole string has been converted - must be a legit number */
                    {
                        if (u32VarIndex != psVar->u8Index)
                        {
                            psVar = psVar->psNext;
                            continue;
                        }
                    }
                    else
                    {   
                        if (strcmp(filter_var, psVar->pcName) != 0)
                        {
                            psVar = psVar->psNext;
                            continue;
                        }
                    }
                }
                
                if (prCbVar)
                {
                    /* Call variable callback */
                    if (!prCbVar(psVar, pvVarUser))
                    {
                        eJIP_UnlockNode(psNode);
                        goto done;
                    }
                }
                
                psVar = psVar->psNext;
            }
            psMib = psMib->psNext;
        }
        eJIP_UnlockNode(psNode);
        psNode = psNode->psNext;
    }

done:
    free(NodeAddressList);
    return 1;
}

typedef int(*command_fn)(char *);

/* Prototypes for command functions */
int jip_help       (char *);
int jip_quit       (char *);

int jip_discover   (char *);
int jip_load       (char *);
int jip_save       (char *);
int jip_print      (char *);
int jip_ipv6       (char *);
int jip_device     (char *);
int jip_mib        (char *);
int jip_var        (char *);

int jip_get        (char *);
int jip_set        (char *);


/** A structure which contains information on the commands this program
    can understand. */
typedef struct {
    char *name;           /**< User printable name of the function. */
    command_fn func;      /**< Function to call to do the job. */
    char *arg;            /**< Description of argument to function */
    char *doc;            /**< Documentation for this function.  */
} COMMAND;


/** Array of supported commands */
COMMAND commands[] = {
    { "help",       jip_help,          NULL,                       "Show help" },
    { "?",          jip_help,          NULL,                       "Show help" },
    { "quit",       jip_quit,          NULL,                       "Quit program" },
    
    { "discover",   jip_discover,      NULL,                       "Discover network contents" },
    { "load",       jip_load,          "<File name>",              "Load network contents from file (name optional)" },
    { "save",       jip_save,          "<File name>",              "Save network contents to file (name optional)" },
    
    { "print",      jip_print,         NULL,                       "Print active section of discovered network" },
    
    { "ipv6",       jip_ipv6,          "<IPv6 address>",           "Change active IPv6 address" },
    { "device",     jip_device,        "<Device ID>",              "Change active device ID" },
    { "mib",        jip_mib,           "<MiB name / ID>",          "Change active MiB" },
    { "var",        jip_var,           "<Variable name / Index>",  "Change active Variable" },
    
    { "get",        jip_get,           NULL,                       "Read active variable" },
    { "set",        jip_set,           "<Value>",                  "Write active variable with new data" },
    { (char *)NULL, (command_fn)NULL,   (char *)NULL }
};


/** Look up NAME as the name of a command, and return a pointer to that
 *  command.  Return a NULL pointer if NAME isn't a command name. 
 *  \param pcName   Command to look for
 *  \return pointer to COMMAND structure appropriate to the command
 */
COMMAND * find_command (char *pcName)
{
    int i;
    
    for (i = 0; commands[i].name; i++)
    {
        if (strcmp (pcName, commands[i].name) == 0)
        {
            return (&commands[i]);
        }
    }

    return ((COMMAND *)NULL);
}

/** Check if a character is whitespace */
int whitespace(char ch)
{
    return ((ch == ' ') || (ch == '\t'));
}

/** Check if a character is newline */
int newline(char ch)
{
    return ((ch == '\n') || (ch == '\r'));
}


/** Execute a command line. 
 *  \param line     String containing the command line
 *  \return 0 on success
 */
int execute_line (char *line)
{
    int i;
    COMMAND *command;
    char *word;

    /* Isolate the command word. */
    i = 0;
    while (line[i] && whitespace (line[i]))
    {
        i++;
    }
    word = line + i;

    while (line[i] && !whitespace (line[i]))
    {
        i++;
    }

    if (line[i])
    {
        line[i++] = '\0';
    }

    command = find_command (word);

    if (!command)
    {
        fprintf (stderr, "%s: Unknown command.\n\r", word);
        return (-1);
    }

    /* Get argument to command, if any. */
    while (whitespace (line[i]))
    {
        i++;
    }

    word = line + i;

    /* Call the function. */
    return ((*(command->func)) (word));
}


/** Strip whitespace from the start and end of STRING.  
 *  \param      String to remove whitespace from
 *  \return a pointer into STRING without the whitespace. 
 */
char *stripwhite (char *string)
{
    char *s, *t;

    for (s = string; whitespace (*s); s++)
    {
    }
        
    if (*s == 0)
    {
        return (s);
    }

    t = s + strlen (s) - 1;
    while (t >= s && (whitespace (*t) || newline(*t)))
    {
        t--;
    }
    *++t = '\0';

    return s;
}


/** Custom string comparison to return the index of the first character
 *  in s1 and s2, up to maxlen, that differs.
 *  \param s1       String 1
 *  \param s2       String 2
 *  \param maxlen   Maximum length to check
 *  \return Index of first differing character
 */
int jip_strcmp(const char *s1, const char *s2, const int maxlen)
{
    int iPos = 0;
    
    while (s1[iPos] && s2[iPos] && (iPos < maxlen))
    {
        if (s1[iPos] != s2[iPos])
        {
            break;
        }
        iPos++;
    }
    return iPos;
}


/** Completor function for commands.  
 *  \param cpl      Completion state structure
 *  \param data     User data (not used)
 *  \param line     Line to match
 *  \param word_end Index of character at which matching should start/end
 *  \return 0 on success.
 */
static int jip_command_completor(WordCompletion *cpl, void *data,
                                 const char *line, int word_start, int word_end)
{
    int list_index;
    char *name;

    list_index = 0;

    /* Return the next name which partially matches from the command list. */
    while ((name = commands[list_index].name) != 0)
    {
        int iPos;
        list_index++;

        if (((iPos = jip_strcmp(name, line, word_end)) != 0)
            || (word_start == word_end))
        {
            if (iPos >= word_end)
            {
                cpl_add_completion( cpl, line, 0,
                                    word_end, &name[iPos],
                                    " ", " ");
            }
        }
    }

    /* Return 0 on success */
    return 0;
}


/** Structure containing parameters to be passed to argument completors */
typedef struct
{
    WordCompletion *cpl;    /**< Completion structure */
    void *data;             /**< Application data */
    const char *line;       /**< Pointer to start of line */
    int word_start;         /**< Index of completion word on line */
    int word_end;           /**< Index of end of line */
} tsCompletorParameters;


/** Callback function for matching IPv6 address
 *  Check if IPv6 matches the request
 *  \param psMib        Pointer to matching node
 *  \param pvUser       Pointer to completor parameters
 *  \return 1 on success.
 */
int jip_argument_completor_ipv6(tsNode *psNode, void *pvUser)
{
    tsCompletorParameters *psParams = (tsCompletorParameters*)pvUser;
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    int iPos;
    
    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);
    if (((iPos = jip_strcmp(buffer, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &buffer[iPos], " ", " ");
        }
    }

    return 1;
}



/** Callback function for matching Device ID
 *  Check if device ID matches the request
 *  \param psMib        Pointer to matching node
 *  \param pvUser       Pointer to completor parameters
 *  \return 1 on success.
 */
int jip_argument_completor_device(tsNode *psNode, void *pvUser)
{
    tsCompletorParameters *psParams = (tsCompletorParameters*)pvUser;
    char buffer[20];
    int iPos;

    sprintf(buffer, "0x%08x", psNode->u32DeviceId);
    if (((iPos = jip_strcmp(buffer, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &buffer[iPos], " ", " ");
        }
    }

    return 1;
}


/** Callback function for matching MiB
 *  Check if Mib or Mib ID match the request
 *  \param psMib        Pointer to matching MiB
 *  \param pvUser       Pointer to completor parameters
 *  \return 1 on success.
 */
int jip_argument_completor_mib(tsMib *psMib, void *pvUser)
{
    tsCompletorParameters *psParams = (tsCompletorParameters*)pvUser;
    char buffer[20];
    int iPos;

    if (((iPos = jip_strcmp(psMib->pcName, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &psMib->pcName[iPos], " ", " ");
        }
    }
    
    sprintf(buffer, "0x%08x", psMib->u32MibId);
    if (((iPos = jip_strcmp(buffer, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &buffer[iPos], " ", " ");
        }
    }

    return 1;
}


/** Callback function for matching variable
 *  Check if variable name or index match the request
 *  \param psVar        Pointer to matching variable
 *  \param pvUser       Pointer to completor parameters
 *  \return 1 on success.
 */
int jip_argument_completor_var(tsVar *psVar, void *pvUser)
{
    tsCompletorParameters *psParams = (tsCompletorParameters*)pvUser;
    char buffer[20];
    int iPos;

    if (((iPos = jip_strcmp(psVar->pcName, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &psVar->pcName[iPos], " ", " ");
        }
    }
    
    sprintf(buffer, "%d", psVar->u8Index & 0xFF);
    if (((iPos = jip_strcmp(buffer, &psParams->line[psParams->word_start], psParams->word_end - psParams->word_start)) != 0)
        || (psParams->word_end == psParams->word_start))
    {
        if (psParams->word_start + iPos >= psParams->word_end)
        {
            cpl_add_completion( psParams->cpl, psParams->line, psParams->word_start,
                                psParams->word_end, &buffer[iPos], " ", " ");
        }
    }

    return 1;
}


/** Completor function for arguments.  
 *  \param cpl      Completion state structure
 *  \param data     User data (not used)
 *  \param line     Line to match
 *  \param word_start Index of character at which current word starts
 *  \param word_end Index of character at which matching should start/end
 *  \return 0 on success.
 */
static int jip_argument_completor(WordCompletion *cpl, void *data,
                                  const char *line, int word_start, int word_end)
{
    tsCompletorParameters sParams;
    sParams.cpl         = cpl;
    sParams.data        = data;
    sParams.line        = line;
    sParams.word_start  = word_start;
    sParams.word_end    = word_end;

    if (strncmp(line, "ipv6", strlen("ipv6")) == 0)
    {
        if (filter_ipv6)
        {
            free(filter_ipv6);
        }
        filter_ipv6 = NULL;
        
        /* Populate possibilites with ipv6 addresses */
        if (!jip_iterate(jip_argument_completor_ipv6, &sParams, NULL, NULL, NULL, NULL))
        {
            fprintf(stderr, "Error generating list of IPv6 addresses\n");
            return 0;
        }
    }
    else if (strncmp(line, "device", strlen("device")) == 0)
    {
        if (filter_device)
        {
            free(filter_device);
        }
        filter_device = NULL;
        
        /* Check for matching device IDs */
        if (!jip_iterate(jip_argument_completor_device, &sParams, NULL, NULL, NULL, NULL))
        {
            fprintf(stderr, "Error generating list of device IDs\n");
            return 0;
        }
    }
    else if (strncmp(line, "mib", strlen("mib")) == 0)
    {
        if (filter_mib)
        {
            free(filter_mib);
        }
        filter_mib = NULL;
        
        /* Check for matching mib names or IDs */
        if (!jip_iterate(NULL, NULL, jip_argument_completor_mib, &sParams, NULL, NULL))
        {
            fprintf(stderr, "Error generating list of MiBs\n");
            return 0;
        }
    }
    else if (strncmp(line, "var", strlen("var")) == 0)
    {
        if (filter_var)
        {
            free(filter_var);
        }
        filter_var = NULL;
        
        /* Check for matching variable names or indexes */
        if (!jip_iterate(NULL, NULL, NULL, NULL, jip_argument_completor_var, &sParams))
        {
            fprintf(stderr, "Error generating list of Variables\n");
            return 0;
        }
    }
    else
    {
        /* No completion for this command */
    }
    return 0;
}


/** Attempt to complete on the contents of TEXT.  START and END bound the
 *  region of rl_line_buffer that contains the word to complete.  TEXT is
 *  the word to complete.  We can use the entire contents of rl_line_buffer
 *  in case we want to do some simple parsing.  Return the array of matches,
 *  or NULL if there aren't any. 
 *  If start is 0, match commands.
 *  If there is a single space in the command line, match argument as appropriate.
 *  Otherwise, stop completing the line.
 *  \param      text        Command/Argument to complete
 *  \param      start       Index of start in readline buffer.
 *  \param      end         Index of end in readline buffer.
 *  \return List of matches, or NULL if there aren't any.
 */
int jip_completion(WordCompletion *cpl, void *data,
                   const char *text, int word_end)
{
    int text_pos, sp_cnt = 0;
    int word_start = 0;
    
    for (text_pos = 0; text_pos < word_end; text_pos++)
    {
        if (text[text_pos] == ' ')
        {
            /* Detect spaces in the command line. 
             * This might not work if we have MiB / variable
             * names that contain spaces.
             */
            sp_cnt++;
            word_start = text_pos+1;
        }
    }

    /* If there are 0 spaces in the line, assume we are being asked to complete a command.
     * If there is one space, assume it's a command.
     */
    if (sp_cnt == 0)
    {
        /* Complete command */
        return jip_command_completor(cpl, data, text, word_start, word_end);
    }
    else if (sp_cnt == 1)
    {
        /* Complete argument */
        return jip_argument_completor(cpl, data, text, word_start, word_end);
    }
    else
    {
        /* No more completions! */
    }

    return 0;
}


/** Print help and exit
 *  \param argv     Command line arguments
 *  \return Does not return - program exits
 */
void print_usage_exit(char *argv[])
{
    printf("JIP CLI version %s using libJIP version %s\n\r", Version, JIP_Version);
    printf("Usage: %s\n\r", argv[0]);
    printf("  Options:\n\r");
    printf("    -h   --help            Print this help.\n\r");
    printf("    -6   --ipv6 <IP>       Connect to border router using IPv6 address.\n\r");
    printf("    -4   --ipv4 <IP>       Connect to border router using IPv4 address.\n\r");
    printf("    -p   --port <port>     Connect to border router on supplied port number.\n\r");
    printf("    -t   --tcp             Connect to border router using TCP. Only available via IPv4\n\r");
    printf("    -e   --exec 'commands' Run a sequence of commands. Multiple commands are seperated by a '%s' character.\n\r", COMMAND_DELIMETER);
    printf("    -m   --mcasts <number> Set the number of times each multicast set is sent. The default is 2.\n\r");
    exit(EXIT_FAILURE);
}


/** Signal handler function for SIGINT
 *  save XML definitions, clean up JIP context and exit.
 */
void sigint_handler(int sig)
{
    eJIPService_PersistXMLSaveDefinitions(&sJIP_Context, CACHE_DEFINITIONS_FILE_NAME);
    eJIP_Destroy(&sJIP_Context);
    exit(0);
}


/** Main */
int main(int argc, char *argv[])
{
    /* Connection details */
    char *pcBorderRouterIPv6Address = NULL;
    char *pcGatewayIPv4Address = NULL;

    int iUse_TCP = 0; /* UDP by default */
    
    int iMcastCount = 2;
    
    char *pcExec = NULL;
    {
        static struct option long_options[] =
        {
            
            {"help",                    no_argument,        NULL, 'h'},
            {"ipv6",                    required_argument,  NULL, '6'},
            {"ipv4",                    required_argument,  NULL, '4'},
            {"port",                    required_argument,  NULL, 'p'},
            {"tcp",                     no_argument,        NULL, 't'},
            {"exec",                    required_argument,  NULL, 'e'},
            {"mcasts",                  required_argument,  NULL, 'm'},
            { NULL, 0, NULL, 0}
        };
        signed char opt;
        int option_index;
        
        while ((opt = getopt_long(argc, argv, "ht6:4:p:e:m:", long_options, &option_index)) != -1) 
        {
            switch (opt) 
            {
                case 'h':
                    print_usage_exit(argv);
                    break;
                case '6':
                    pcBorderRouterIPv6Address = optarg;
                    break;
                case '4':
                    pcGatewayIPv4Address = optarg;
                    break;
                case 'p':
                {
                    char *pcEnd;
                    errno = 0;
                    iPort = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("Port '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("Port '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                }
                case 't':
                    iUse_TCP = 1;
                    break;
                case 'e':
                    pcExec = strdup(optarg);
                    break;
                case 'm':
                {
                    char *pcEnd;
                    errno = 0;
                    iMcastCount = strtoul(optarg, &pcEnd, 0);
                    if (errno)
                    {
                        printf("Multicast count '%s' cannot be converted to 32 bit integer (%s)\n", optarg, strerror(errno));
                        print_usage_exit(argv);
                    }
                    if (*pcEnd != '\0')
                    {
                        printf("Multicast count '%s' contains invalid characters\n", optarg);
                        print_usage_exit(argv);
                    }
                    break;
                }

                default: /* '?' */
                    print_usage_exit(argv);
            }
        }
    }
    
    if ((!pcBorderRouterIPv6Address) && (!pcGatewayIPv4Address))
    {
        printf("Please specify gateway IPv4 or IPv6 address\n\r");
        print_usage_exit(argv);
    }
    
    if (eJIP_Init(&sJIP_Context, E_JIP_CONTEXT_CLIENT) != E_JIP_OK)
    {
        printf("JIP startup failed\n\r");
        return -1;
    }
    
    // Set how many times each multicast is sent.
    sJIP_Context.iMulticastSendCount = iMcastCount;

    /* Set up signal handler */
    {
        struct sigaction sa;

        sa.sa_handler = sigint_handler;
        sa.sa_flags = 0; // or SA_RESTART
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == -1) 
        {
            perror("sigaction");
            exit(1);
        }
    }
    
    if (pcGatewayIPv4Address)
    {
        char *pcDefaultIPv6Address = "::0";
            
        if (!pcBorderRouterIPv6Address)
        {
            printf("Using default coordinator address\n\r");
            pcBorderRouterIPv6Address = pcDefaultIPv6Address;
        }
        
        if (iUse_TCP)
        {
            printf("Connecting via IPv4 Gateway '%s:%d' over TCP\n\r", pcGatewayIPv4Address, iPort);
        }
        else
        {
            printf("Connecting via IPv4 Gateway '%s:%d' over UDP\n\r", pcGatewayIPv4Address, iPort);
        }
        if (eJIP_Connect4(&sJIP_Context, pcGatewayIPv4Address, iPort, pcBorderRouterIPv6Address, iPort, iUse_TCP) != E_JIP_OK)
        {
            printf("JIP connect failed\n\r");
            return -1;
        }
    }
    else
    {
        if (eJIP_Connect(&sJIP_Context, pcBorderRouterIPv6Address, iPort) != E_JIP_OK)
        {
            printf("JIP connect failed\n\r");
            return -1;
        }
    }

    if (eJIPService_PersistXMLLoadDefinitions(&sJIP_Context, CACHE_DEFINITIONS_FILE_NAME) != E_JIP_OK)
    {
        printf("JIP load network state failed\n\r");
    }
    
    /* Check if we have a command line to run */
    if (pcExec)
    {
        char *pcCommand;
        printf("Executing command line: %s\n\r", pcExec);
        
        pcCommand = strtok(pcExec, COMMAND_DELIMETER);
        while (pcCommand)
        {
            char *s;
            /* Remove leading and trailing whitespace from the line.
                Then, if there is anything left, add it to the history list
                and execute it. */
            s = stripwhite (pcCommand);
            execute_line (s);
            pcCommand = strtok(NULL, COMMAND_DELIMETER);
        }
    }
    else
    {
        char *line;    /* The line that the user typed */
        GetLine *gl;
    
        gl = new_GetLine(1024, 2048);
        if(!gl)
        {
            return 1;
        }
        
        /* Use our completer */
        gl_customize_completion(gl, NULL, jip_completion);
        
        /* Enter main input loop */
        while (iQuit == 0)
        {
            char *s;
            char prompt[255];
            uint32_t prompt_pos = 0;
            
            prompt_pos += sprintf(&prompt[prompt_pos], "JIP : ");
            
            if (filter_ipv6 != NULL)
            {
                prompt_pos += sprintf(&prompt[prompt_pos], "Node '%s' ", filter_ipv6);
            }
            
            if (filter_device != NULL)
            {
                prompt_pos += sprintf(&prompt[prompt_pos], "Device ID '%s' ", filter_device);
            }
            
            if (filter_mib != NULL)
            {
                prompt_pos += sprintf(&prompt[prompt_pos], "MiB '%s' ", filter_mib);
            }
            
            if (filter_var != NULL)
            {
                prompt_pos += sprintf(&prompt[prompt_pos], "Var '%s' ", filter_var);
            }
            
            prompt_pos += sprintf(&prompt[prompt_pos], "> ");

            line = gl_get_line(gl, prompt, NULL, -1);
            
            if (!line)
            {
                /* Error or Exit */
                break;
            }

            s = stripwhite (line);
            if (strlen(s) > 0)
            {
                execute_line (s);
            }
        }
        
        /* Free resources */
        del_GetLine(gl);
    }
    
    eJIPService_PersistXMLSaveDefinitions(&sJIP_Context, CACHE_DEFINITIONS_FILE_NAME);
    eJIP_Destroy(&sJIP_Context);
    
    printf("\n\r");
    return 0;
}


/** Print out help for ARG, or for all of the commands if ARG is not present. */
int jip_help (char *arg)
{
    int i;
    int printed = 0;

    for (i = 0; commands[i].name; i++)
    {
        if (!*arg || (strcmp (arg, commands[i].name) == 0))
        {
            int command_len, position = 0;
            command_len = printf ("%s", commands[i].name);
            for (position = command_len; position < 15; position++)
            {
                printf(" ");
            }
            
            command_len = 0;
            if (commands[i].arg)
            {
                command_len = printf ("%s", commands[i].arg);
            }
            for (position += command_len; position < 35; position++)
            {
                printf(" ");
            }
            
            printf ("%s.\n\r", commands[i].doc);
            printed++;
        }
    }

    if (!printed)
    {
        printf ("No commands match '%s'.  Supported commands are:\n\r", arg);

        for (i = 0; commands[i].name; i++)
        {
            /* Print in six columns. */
            if (printed == 6)
            {
                printed = 0;
                printf ("\n\r");
            }

            printf ("%s\t", commands[i].name);
            printed++;
        }

        if (printed)
        {
            printf ("\n\r");
        }
    }
    return (0);
}


/** Quit */
int jip_quit (char *arg)
{
    return iQuit = 1;
}


/** Run JIP discovery */
int jip_discover (char *arg)
{
    struct timeval tvBefore, tvAfter;
    teJIP_Status eStatus;
    
    printf("Running discovery...\n\r");
    
    if(gettimeofday(&tvBefore, NULL))
    {
        fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
    }
    
    if ((eStatus = eJIPService_DiscoverNetwork(&sJIP_Context)) != E_JIP_OK)
    {
        printf("JIP discover network failed (%s)\n\r", pcJIP_strerror(eStatus));
    }
    else
    {
        uint32_t u32NumAddresses;
        tsJIPAddress *psAddresses;
        
        if(gettimeofday(&tvAfter, NULL))
        {
            fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
        }
        
        if (eJIP_GetNodeAddressList(&sJIP_Context, E_JIP_DEVICEID_ALL, &psAddresses, &u32NumAddresses) == E_JIP_OK)
        {
            printf("Discovered %d devices (Time: %ums)\n\r", u32NumAddresses, u32diff_timeval(tvAfter, tvBefore));
            free(psAddresses);
        }
        else
        {
            printf("Error getting node list\n\r");
        }
    }
    return 0;
}


/** Load jip network file 
 *  \param arg  Filename if given
 */
int jip_load (char *arg)
{
    char *filename = CACHE_NETWORK_FILE_NAME;
    teJIP_Status eStatus;
    if (strcmp(arg, ""))
    {
        filename = arg;
    }
    
    printf("Loading network contents from '%s'\n\r", filename);
    
    if ((eStatus = eJIPService_PersistXMLLoadNetwork(&sJIP_Context, filename)) != E_JIP_OK)
    {
        printf("JIP load network failed (%s)\n\r", pcJIP_strerror(eStatus));
    }
    return 0;
}


/** Save jip network file 
 *  \param arg  Filename if given
 */
int jip_save (char *arg)
{
    char *filename = CACHE_NETWORK_FILE_NAME;
    teJIP_Status eStatus;
    if (strcmp(arg, ""))
    {
        filename = arg;
    }
    
    printf("Saving network contents to '%s'\n\r", filename);
    
    if ((eStatus = eJIPService_PersistXMLSaveNetwork(&sJIP_Context, filename)) != E_JIP_OK)
    {
        printf("JIP save network failed (%s)\n\r", pcJIP_strerror(eStatus));
    }
    return 0;
}


/* Print */


/** Callback funtion to print node details */
int jip_print_node (tsNode *psNode, void *pvUser)
{
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";

    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);

    printf("  Node: %s", buffer);
    printf("    Device ID: 0x%08x\n\r", psNode->u32DeviceId);

    return 1;
}


/** Callback funtion to print MiB details */
int jip_print_mib (tsMib *psMib, void *pvUser)
{
    printf("    Mib: '%s', ID 0x%08x\n\r", psMib->pcName, psMib->u32MibId);
    return 1;
}


/** Callback funtion to print variable details */
int jip_print_var (tsVar *psVar, void *pvUser)
{
    printf("      Var: '%s', Index %d%s\n\r", psVar->pcName, psVar->u8Index & 0xFF, psVar->eEnable == E_JIP_VAR_DISABLED ? " (disabled)": "");
    printf("        ");
    switch (psVar->eVarType)
    {
#define TEST(a) case  (a): printf(#a); break
        TEST(E_JIP_VAR_TYPE_INT8);
        TEST(E_JIP_VAR_TYPE_UINT8);
        TEST(E_JIP_VAR_TYPE_INT16);
        TEST(E_JIP_VAR_TYPE_UINT16);
        TEST(E_JIP_VAR_TYPE_INT32);
        TEST(E_JIP_VAR_TYPE_UINT32);
        TEST(E_JIP_VAR_TYPE_INT64);
        TEST(E_JIP_VAR_TYPE_UINT64);
        TEST(E_JIP_VAR_TYPE_FLT);
        TEST(E_JIP_VAR_TYPE_DBL);
        TEST(E_JIP_VAR_TYPE_STR);
        TEST(E_JIP_VAR_TYPE_BLOB);
        TEST(E_JIP_VAR_TYPE_TABLE_BLOB);
        default: printf("Unknown Type");
#undef TEST
    }
    
    printf(", ");
    switch (psVar->eAccessType)
    {
#define TEST(a) case  (a): printf(#a); break
        TEST(E_JIP_ACCESS_TYPE_CONST);
        TEST(E_JIP_ACCESS_TYPE_READ_ONLY);
        TEST(E_JIP_ACCESS_TYPE_READ_WRITE);
        default: printf("Unknown Access Type");
#undef TEST
    }             

    printf(", ");
    switch (psVar->eSecurity)
    {
#define TEST(a) case  (a): printf(#a); break
        TEST(E_JIP_SECURITY_NONE);
        default: printf("Unknown Security Type");
#undef TEST
    }   
    printf("\n\r");
    
    printf("          Value: ");
    if (psVar->pvData)
    {
        switch (psVar->eVarType)
        {
#define TEST(a, b, c) case  (a): printf(b, *psVar->c); break
            TEST(E_JIP_VAR_TYPE_INT8,   "%d\n\r",   pi8Data);
            TEST(E_JIP_VAR_TYPE_UINT8,  "%u\n\r",   pu8Data);
            TEST(E_JIP_VAR_TYPE_INT16,  "%d\n\r",   pi16Data);
            TEST(E_JIP_VAR_TYPE_UINT16, "%u\n\r",   pu16Data);
            TEST(E_JIP_VAR_TYPE_INT32,  "%d\n\r",   pi32Data);
            TEST(E_JIP_VAR_TYPE_UINT32, "%u\n\r",   pu32Data);
            TEST(E_JIP_VAR_TYPE_INT64,  "%lld\n\r", pi64Data);
            TEST(E_JIP_VAR_TYPE_UINT64, "%llu\n\r", pu64Data);
            TEST(E_JIP_VAR_TYPE_FLT,    "%f\n\r",   pfData);
            TEST(E_JIP_VAR_TYPE_DBL,    "%f\n\r",   pdData);
            case  (E_JIP_VAR_TYPE_STR): 
                printf("%s\n\r", psVar->pcData); 
                break;
            case (E_JIP_VAR_TYPE_BLOB):
            {
                uint32_t i;
                printf("{");
                for (i = 0; i < psVar->u8Size; i++)
                {
                    printf(" 0x%02x", psVar->pbData[i]);
                }
                printf(" }\n\r");
                break;
            }
            
            case (E_JIP_VAR_TYPE_TABLE_BLOB):
            {
                tsTableRow *psTableRow;
                int i;
                printf("\n");
                for (i = 0; i < psVar->ptData->u32NumRows; i++)
                {
                    psTableRow = &psVar->ptData->psRows[i];
                    if (psTableRow->pvData)
                    {
                        uint32_t j;
                        printf("            %03d {", i);
                        for (j = 0; j < psTableRow->u32Length; j++)
                        {
                            printf(" 0x%02x", psTableRow->pbData[j]);
                        }
                        printf(" }\n");
                    }
                }
                break;
            }
            default: printf("Unknown Type\n\r");
#undef TEST
        }
    }
    else
    {
        printf("?\n\r");
    }
    printf("\n\r");
    return 1;
}


/** Print filtered network */
int jip_print(char *arg)
{
    return jip_iterate(jip_print_node, NULL, jip_print_mib, NULL, jip_print_var, NULL);
}


/** Set IPv6 filter */
int jip_ipv6 (char *arg)
{
    if (arg == NULL) return 0;
    
    if (filter_ipv6)
    {
        free(filter_ipv6);
    }
    
    if (strcmp(arg, ""))
    {
        filter_ipv6 = strdup(arg);
    }
    else
    {
        filter_ipv6 = NULL;
    }
    
    return 0;
}


/** Set device ID filter */
int jip_device (char *arg)
{
    if (arg == NULL) return 0;
    
    if (filter_device)
    {
        free(filter_device);
    }
    
    if (strcmp(arg, ""))
    {
        filter_device = strdup(arg);
    }
    else
    {
        filter_device = NULL;
    }
    
    return 0;   
}


/** Set MiB filter */
int jip_mib (char *arg)
{
    
    if (arg == NULL) return 0;
    
    if (filter_mib)
    {
        free(filter_mib);
    }
    
    if (strcmp(arg, ""))
    {
        filter_mib = strdup(arg);
    }
    else
    {
        filter_mib = NULL;
    }
    return 0;
}


/** Set variable filter */
int jip_var (char *arg)
{
    if (arg == NULL) return 0;
    
    if (filter_var)
    {
        free(filter_var);
    }
    
    if (strcmp(arg, ""))
    {
        filter_var = strdup(arg);
    }
    else
    {
        filter_var = NULL;
    }
    
    return 0;
}


/* Get var */

/** Callback to get a variable */
int jip_get_var (tsVar *psVar, void *pvUser)
{
    tsNode *psNode;
    tsMib *psMib;
    struct timeval tvBefore, tvAfter;
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    teJIP_Status eStatus;
    uint32_t u32Flags = E_JIP_FLAG_NONE;
    
    psMib = psVar->psOwnerMib;
    psNode = psMib->psOwnerNode;
    
    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);
                
    printf("Reading Node '%s' MiB '%s' Var '%s': ", buffer, psMib->pcName, psVar->pcName);
    
    if(gettimeofday(&tvBefore, NULL))
    {
        fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
    }
    
    if (filter_var == NULL) /* If no filter active */
    {
        // We will be reading more than one variable, so use the stay awake flag.
        u32Flags |= E_JIP_FLAG_STAY_AWAKE;
    }
    
    eStatus = eJIP_GetVar(&sJIP_Context, psVar, u32Flags);
    
    if(gettimeofday(&tvAfter, NULL))
    {
        fprintf(stderr, "Error getting system time (Time: %s)\n\r", strerror(errno));
    }
    
    if (eStatus != E_JIP_OK)
    {
        printf("Error reading (%s)\n\r", pcJIP_strerror(eStatus));
    }
    else
    {
        printf("Success (Time: %ums)\n\r", u32diff_timeval(tvAfter, tvBefore));
    }
                
    return 1;   
}


/** Get command */
int jip_get(char *arg)
{
    return jip_iterate(NULL, NULL, NULL, NULL,jip_get_var, NULL);
}


/* Set Var */

/** Callback to set a variable */
int jip_set_var (tsVar *psVar, void *pvUser)
{
    tsNode *psNode;
    tsMib *psMib;
    struct timeval tvBefore, tvAfter;
    char buffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    char *arg = (char *)pvUser;
    int supported   = 1;
    int freeable    = 1;
    int is_multicast = 0;
    struct in6_addr node_addr;
    uint32_t u32Size = 0;
    char *buf = NULL;
    int status = 1;
    uint32_t u32Flags = E_JIP_FLAG_NONE;
    
    psMib = psVar->psOwnerMib;
    psNode = psMib->psOwnerNode;
    
    inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, buffer, INET6_ADDRSTRLEN);

    if (filter_ipv6)
    {
        if (inet_pton(AF_INET6, filter_ipv6, &node_addr) != 1)
        {
            fprintf(stderr, "Invalid IPv6 address '%s'\n\r", filter_ipv6);
            return 0;
        }
        
        if ((strncmp(filter_ipv6, "FF", 2) == 0) || (strncmp(filter_ipv6, "ff", 2) == 0))
        {
            is_multicast = 1;
        }
    }
    
    if (arg == NULL) return 0;
    
    if (filter_var == NULL) /* If no filter active */
    {
        // We will be reading more than one variable, so use the stay awake flag.
        u32Flags |= E_JIP_FLAG_STAY_AWAKE;
    }
                
    switch (psVar->eVarType)
    {
        case (E_JIP_VAR_TYPE_INT8):
        case (E_JIP_VAR_TYPE_UINT8):
            buf = malloc(sizeof(uint8_t));
            errno = 0;
            buf[0] = strtoul(arg, NULL, 0);
            if (errno)
            {
                printf("Invalid value: '%s'\n\r", arg);
                return 0;
            }
            break;
        
        case (E_JIP_VAR_TYPE_INT16):
        case (E_JIP_VAR_TYPE_UINT16):
        {
            uint16_t u16Var;
            errno = 0;
            u16Var = strtoul(arg, NULL, 0);
            if (errno)
            {
                printf("Invalid value: '%s'\n\r", arg);
                return 0;
            }
            buf = malloc(sizeof(uint16_t));
            memcpy(buf, &u16Var, sizeof(uint16_t));
            break;
        }
            
        case (E_JIP_VAR_TYPE_INT32):
        case (E_JIP_VAR_TYPE_UINT32):
        {
            uint32_t u32Var;
            errno = 0;
            u32Var = strtoul(arg, NULL, 0);
            if (errno)
            {
                printf("Invalid value: '%s'\n\r", arg);
                return 0;
            }
            buf = malloc(sizeof(uint32_t));
            memcpy(buf, &u32Var, sizeof(uint32_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_INT64):
        case (E_JIP_VAR_TYPE_UINT64):
        {
            uint64_t u64Var;
            errno = 0;
            u64Var = strtoull(arg, NULL, 0);
            if (errno)
            {
                printf("Invalid value: '%s'\n\r", arg);
                return 0;
            }
            buf = malloc(sizeof(uint64_t));
            memcpy(buf, &u64Var, sizeof(uint64_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_FLT):
        {
            float f32Var = strtof(arg, NULL);
            u32Size = sizeof(uint32_t);
            buf = malloc(u32Size);
            if (!buf) return 0;
            memcpy(buf, &f32Var, sizeof(uint32_t));
            break;
        }
        
        case (E_JIP_VAR_TYPE_DBL):
        {
            double d64Var = strtod(arg, NULL);
            u32Size = sizeof(uint64_t);
            buf = malloc(u32Size);
            if (!buf) return 0;
            memcpy(buf, &d64Var, sizeof(uint64_t));
            break;
        }
            
        case(E_JIP_VAR_TYPE_STR):
        {
            buf = arg;
            //printf("Update to \"%s\"\n", buf);
            u32Size = strlen(arg);
            freeable = 0;
            break;
        }
            
        case(E_JIP_VAR_TYPE_BLOB):
        {
            int i, j;
            buf = malloc(strlen(arg));
            if (!buf) return 0;
            memset(buf, 0, strlen(arg));
            if (strncmp(arg, "0x", 2) == 0)
            {
                arg += 2;
            }
            u32Size = 0;
            for (i = 0, j = 0; (i < strlen(arg)) && supported; i++)
            {
                uint8_t u8Nibble = 0;
                if ((arg[i] >= '0') && (arg[i] <= '9'))
                {
                    u8Nibble = arg[i]-'0';
                    //printf("Got 0-9 0x%02x\n", );
                }
                else if ((arg[i] >= 'a') && (arg[i] <= 'f'))
                {
                    u8Nibble = arg[i]-'a' + 0x0A;
                }
                else if ((arg[i] >= 'A') && (arg[i] <= 'F'))
                {
                    u8Nibble = arg[i]-'A' + 0x0A;
                }
                else
                {
                    printf("String contains non-hex character\n\r");
                    supported = 0;
                    break;
                }
                    
                if ((u32Size & 0x01) == 0)
                {
                    // Even numbered byte
                    buf[j] = u8Nibble << 4;
                }
                else
                {
                    buf[j] |= u8Nibble & 0x0F;
                    j++;
                }
                u32Size++;
            }
            if (u32Size & 0x01)
            {
                // Odd length string
                u32Size = (u32Size >> 1) + 1;
            }
            else
            {
                u32Size = u32Size >> 1;
            }
            
            break;
        }
        
        default:
            supported = 0;
    }
    
    if (filter_ipv6)
    {
        printf("Setting Node '%s' MiB '%s' Var '%s' to '%s': ", filter_ipv6, psMib->pcName, psVar->pcName, arg);
    }
    else
    {
        printf("Setting Node '%s' MiB '%s' Var '%s' to '%s': ", buffer, psMib->pcName, psVar->pcName, arg);
    }
    
    if (supported)
    {
        teJIP_Status eStatus;
        
        if(gettimeofday(&tvBefore, NULL))
        {
            fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
        }
        
        if (is_multicast)
        {
            tsJIPAddress MCastAddress;
            
            memset (&MCastAddress, 0, sizeof(struct sockaddr_in6));
            MCastAddress.sin6_family    = AF_INET6;
            MCastAddress.sin6_port      = htons(iPort);
            MCastAddress.sin6_addr      = node_addr;
            
            if ((eStatus = eJIP_MulticastSetVar(&sJIP_Context, psVar, buf, u32Size, &MCastAddress, 2, u32Flags)) != E_JIP_OK)
            {
                printf("Error setting new value (%s)\n\r", pcJIP_strerror(eStatus));
            }
            else
            {
                if(gettimeofday(&tvAfter, NULL))
                {
                    fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
                }
                
                printf("Success (Time: %ums)\n\r", u32diff_timeval(tvAfter, tvBefore));
            }
            
            /* Return an error so we don't get called again for the same multicast */
            status = 0;
        }
        else
        {
            if ((eStatus = eJIP_SetVar(&sJIP_Context, psVar, buf, u32Size, u32Flags)) != E_JIP_OK)
            {
                printf("Error setting new value (%s)\n\r", pcJIP_strerror(eStatus));
            }
            else
            {
                if(gettimeofday(&tvAfter, NULL))
                {
                    fprintf(stderr, "Error getting system time (%s)\n\r", strerror(errno));
                }
                
                printf("Success (Time: %ums)\n\r", u32diff_timeval(tvAfter, tvBefore));
            }
        }
    }
    else
    {
        printf("Variable type not supported\n\r");
    }
    
    if (freeable)
    {
        free(buf);
    }

    return status;   
}


/** Set command */
int jip_set (char *arg)
{   
    if (arg == NULL) return 0;
    
    return jip_iterate(NULL, NULL, NULL, NULL, jip_set_var, arg);
}


