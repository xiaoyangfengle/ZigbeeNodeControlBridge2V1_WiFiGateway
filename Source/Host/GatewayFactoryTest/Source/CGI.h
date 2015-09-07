/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:          CGI Driver
 *
 * REVISION:           $Revision: 40016 $
 *
 * DATED:              $Date: 2012-03-27 09:01:19 +0100 (Tue, 27 Mar 2012) $
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

#ifndef __CGI_H_
#define __CGI_H_


/** Enumerated type of status codes from cgi driver */
typedef enum
{
    E_CGI_OK,               /**< All ok */
    E_CGI_ERROR,            /**< Generic error */
    E_CGI_INVALID_PARAMS,   /**< Invalid parameters were passed */
    E_CGI_MEM_ERROR,        /**< Error allocating memory */
} teCGIStatus;


typedef struct
{
    char *pcValue;          /**< Value of the variable as a string */
} tsCGIVar;                 /**< Variable */

typedef struct
{
    char *pcFileName;       /**< Filename */
    char *pcTmpFile;        /**< Temporary filename of upload */
    char *pcMimeType;       /**< Mime type of upload */
} tsCGIFile;                /**< File upload */


/** Structure representing a cgi parameter */
typedef struct
{
    enum
    {
        E_CGI_VARIABLE,         /**< Variable */
        E_CGI_FILE,             /**< Uploaded file */
    } eType;                    /**< Parameter type */
    
    char *pcName;               /**< Name of the parameter */
    
    union
    {
        tsCGIVar    sVar;       /**< Variable */
        tsCGIFile   sFile;      /**< File upload */
    } uParam;
} tsCGIParam;


/** Structure for cgi driver */
typedef struct
{
    int         iNumParams;     /**< Number of parameters passed to the cgi program */
    tsCGIParam* asParams;       /**< Array of \ref tsCGIParam structures containing the parameters */
} tsCGI;


/** Read parameters that have been passed to the cgi, either as environment variables 
 *  or on stdinput.
 *  \param psCGI            Pointer to CGI structure to populate with parameters.
 *  \return E_CGI_OK on success
 */
teCGIStatus eCGIReadParameters(tsCGI *psCGI);


/** Get a pointer to a parameter that was passed.
 *  \param psCGI            Pointer to CGI structure populated with parameters.
 *  \param pcParamName      String containing the parameter name
 *  \return NULL if parameter name not found. Pointer to parameter structure if found.
 */
tsCGIParam* psCGIGetParameter(tsCGI *psCGI, const char *pcParamName);


/** Get the string value of a variable parameter
 *  \param psCGI            Pointer to CGI structure populated with parameters.
 *  \param pcParamName      String containing the parameter name
 *  \return NULL if parameter name not found. Pointer to string value if found.
 */
static inline char *pcCGIGetVariable(tsCGI *psCGI, const char *pcParamName)
{
    tsCGIParam* psParam = psCGIGetParameter(psCGI, pcParamName);
    if (psParam)
    {
        if (psParam->eType == E_CGI_VARIABLE)
        {
            return psParam->uParam.sVar.pcValue;
        }
    }
    return NULL;
}



/** URL Encode the given string.
 *  \param ppcOutput        Pointer to location to store newly mallocd string output
 *  \param pcInput          String containging input string
 *  \return E_CGI_OK on success
 */
teCGIStatus eCGIURLEncode(char **ppcOutput, const char *pcInput);


/** Replaces escaped values in the input string with the real characters.
 *  \param pcString         Input String. Modified in place.
 *  \return E_CGI_OK on success
 */
teCGIStatus eCGIURLDecode (char *pcInput);


/** Return string representing this script
 *  \param psCGI            Pointer to CGI structure.
 *  \return String or NULL
 */
char *pcCGISelf(tsCGI *psCGI);


/** Debug dump of cgi parameters
 *  \param psCGI            Pointer to CGI structure.
 *  \param pfStream         IO stream to dump to
 *  \return void
 */
void vCGIDumpParameters(tsCGI *psCGI, FILE *pfStream);

#endif /* __CGI_H_ */
