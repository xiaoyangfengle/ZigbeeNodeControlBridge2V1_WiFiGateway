/****************************************************************************
 *
 * MODULE:             libJIP
 *
 * COMPONENT:          Persist.c
 *
 * REVISION:           $Revision: 62922 $
 *
 * DATED:              $Date: 2014-07-25 11:54:41 +0100 (Fri, 25 Jul 2014) $
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h> 

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include <JIP.h>
#include <JIP_Private.h>

#define DBG_FUNCTION_CALLS 0
#define DBG_PERSIST 0

#define CACHE_FILE_VERSION 4

#define MY_ENCODING "ISO-8859-1"


teJIP_Status eJIPService_PersistXMLSaveDefinitions(tsJIP_Context *psJIP_Context, const char *pcFileName)
{
    PRIVATE_CONTEXT(psJIP_Context);
    tsMibIDCacheEntry       *psMibCacheEntry;
    tsDeviceIDCacheEntry    *psDeviceCacheEntry;
    tsNode *psNode;
    tsMib *psMib;
    tsVar *psVar;
    int rc;
    xmlTextWriterPtr writer;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    LIBXML_TEST_VERSION
    /* Don't print parser errors to stdout - that messes up cgi's etc. */
    initGenericErrorDefaultFunc(NULL);

    DBG_vPrintf(DBG_PERSIST, "Saving network state to %s\n", pcFileName);

    /* Create a new XmlWriter for uri, with no compression. */
    writer = xmlNewTextWriterFilename(pcFileName, 0);
    if (writer == NULL) {
        printf("testXmlwriterFilename: Error creating the xml writer\n");
        return E_JIP_ERROR_FAILED;
    }
    
    xmlTextWriterSetIndent(writer, 1);

    /* Start the document with the xml default for the version,
     * encoding ISO 8859-1 and the default for the standalone
     * declaration. */
    rc = xmlTextWriterStartDocument(writer, NULL, MY_ENCODING, NULL);
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartDocument\n");
        return E_JIP_ERROR_FAILED;
    }

    /* Start an element named "JIP_Cache" */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "JIP_Cache");
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return E_JIP_ERROR_FAILED;
    }
    
    /* Add an attribute with name "Version" */
    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Version",
                                            "%d", CACHE_FILE_VERSION);
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
        return E_JIP_ERROR_FAILED;
    }
    
    /* Start an element named "MibIdCache" */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "MibIdCache");
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return E_JIP_ERROR_FAILED;
    }
    
    psMibCacheEntry = psJIP_Private->sCache.psMibCacheHead;
    DBG_vPrintf(DBG_PERSIST, "Cache head: %p\n", psMibCacheEntry);
    while (psMibCacheEntry)
    {
        psMib = psMibCacheEntry->psMib;
        
        DBG_vPrintf(DBG_PERSIST, "Saving Mib id 0x%08x into Mib cache\n", psMib->u32MibId);
        
        /* Start an element named "Mib" as child of MibIDCache. */
        rc = xmlTextWriterStartElement(writer, BAD_CAST "Mib");
        if (rc < 0) {
            printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            return E_JIP_ERROR_FAILED;
        }
        
        /* Add an attribute with name "MibID" */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ID",
                                            "0x%08x", psMib->u32MibId);
        if (rc < 0) {
            printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
            return E_JIP_ERROR_FAILED;
        }
        
        psVar = psMib->psVars;
        while (psVar)
        {
            /* Start an element named "Var" as child of "Mib". */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "Var");
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
                return E_JIP_ERROR_FAILED;
            }

            /* Add an attribute with name "Index" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Index",
                                                "%02d", psVar->u8Index);
            if (rc < 0) {
                printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Add an attribute with name "Name" */
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "Name",
                                            BAD_CAST psVar->pcName);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return E_JIP_ERROR_FAILED;
            }

            /* Add an attribute with name "Type" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Type",
                                                "%02d", psVar->eVarType);
            if (rc < 0) {
                printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return E_JIP_ERROR_FAILED;
            }

            /* Add an attribute with name "Access" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Access",
                                                "%02d", psVar->eAccessType);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return E_JIP_ERROR_FAILED;
            }
        
            /* Add an attribute with name "Security" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Security",
                                                "%02d", psVar->eSecurity);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return E_JIP_ERROR_FAILED;
            }

            
            /* Don't write out the value, we dont really want to cache that do we?? */
            
            /* Close the element named "Var". */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Next variable */
            psVar = psVar->psNext;
        }
        
        /* Close the element named "Mib". */
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
            return E_JIP_ERROR_FAILED;
        }
        
        /* Next entry */
        psMibCacheEntry = psMibCacheEntry->psNext;
    }
    
    /* Close the element named "MibIdCache". */
    rc = xmlTextWriterEndElement(writer);
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
        return E_JIP_ERROR_FAILED;
    }

    /* Start an element named "DeviceIdCache" */
    rc = xmlTextWriterStartElement(writer, BAD_CAST "DeviceIdCache");
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
        return E_JIP_ERROR_FAILED;
    }
    
    psDeviceCacheEntry = psJIP_Private->sCache.psDeviceCacheHead;
    DBG_vPrintf(DBG_PERSIST, "Cache head: %p\n", psDeviceCacheEntry);
    while (psDeviceCacheEntry)
    {
        psNode = psDeviceCacheEntry->psNode;
        
        DBG_vPrintf(DBG_PERSIST, "Saving device id 0x%08x into device ID cache\n", psNode->u32DeviceId);
        
        /* Start an element named "Device" as child of DeviceIdCache. */
        rc = xmlTextWriterStartElement(writer, BAD_CAST "Device");
        if (rc < 0) {
            printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
            return E_JIP_ERROR_FAILED;
        }
        
        /* Add an attribute with name "Device_Id" */
        rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ID",
                                            "0x%08x", psNode->u32DeviceId);
        if (rc < 0) {
            printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
            return E_JIP_ERROR_FAILED;
        }
 
        psMib = psNode->psMibs;
        while (psMib)
        {
            /* Start an element named "Mib" as child of Device. */
            rc = xmlTextWriterStartElement(writer, BAD_CAST "Mib");
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Add an attribute with name "ID" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "ID",
                                                "0x%08x", psMib->u32MibId);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Add an attribute with name "Index" */
            rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Index",
                                                "%02d", psMib->u8Index);
            if (rc < 0) {
                printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Add an attribute with name "Name" */
            rc = xmlTextWriterWriteAttribute(writer, BAD_CAST "Name",
                                            BAD_CAST psMib->pcName);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterWriteAttribute\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Close the element named "MiB". */
            rc = xmlTextWriterEndElement(writer);
            if (rc < 0) {
                printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                return E_JIP_ERROR_FAILED;
            }
            
            /* Next Mib */
            psMib = psMib->psNext;
        }
        
        psMib = psNode->psMibs;
        while (psMib)
        {
            tsVar *psVar;
            psVar = psMib->psVars;
            
            while (psVar)
            {
                if (psVar->pvData)
                {
                    DBG_vPrintf(DBG_PERSIST, "Saving value of Mib 0x%08X, Var %d into cache\n", psMib->u32MibId, psVar->u8Index); 
                    /* Start an element named "VarData" as child of Device. */
                    rc = xmlTextWriterStartElement(writer, BAD_CAST "VarData");
                    if (rc < 0) {
                        printf("testXmlwriterFilename: Error at xmlTextWriterStartElement\n");
                        return E_JIP_ERROR_FAILED;
                    }
                    
                    /* Add an attribute with name "MibID" */
                    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "MibID",
                                                        "0x%08x", psMib->u32MibId);
                    if (rc < 0) {
                        printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                        return E_JIP_ERROR_FAILED;
                    }
                    
                    /* Add an attribute with name "VarIndex" */
                    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "VarIndex",
                                                        "%02d", psVar->u8Index);
                    if (rc < 0) {
                        printf ("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                        return E_JIP_ERROR_FAILED;
                    }
                    
                    /* Add an attribute with name "Size" */
                    rc = xmlTextWriterWriteFormatAttribute(writer, BAD_CAST "Size",
                                                    "%02d", psVar->u8Size);
                    if (rc < 0) {
                        printf("testXmlwriterFilename: Error at xmlTextWriterWriteFormatAttribute\n");
                        return E_JIP_ERROR_FAILED;
                    }
                    
                    /* Add an attribute with name "Data" */
                    rc = xmlTextWriterWriteBinHex(writer, psVar->pvData, 0, psVar->u8Size);
                    if (rc < 0) {
                        printf("testXmlwriterFilename: Error at xmlTextWriterWriteBinHex\n");
                        return E_JIP_ERROR_FAILED;
                    }
                    
                    /* Close the element named "VarData". */
                    rc = xmlTextWriterEndElement(writer);
                    if (rc < 0) {
                        printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
                        return E_JIP_ERROR_FAILED;
                    }
                }
            
                /* Next variable */
                psVar = psVar->psNext;
            }
            
            /* Next Mib */
            psMib = psMib->psNext;
        }

        /* Close the element named "Device". */
        rc = xmlTextWriterEndElement(writer);
        if (rc < 0) {
            printf("testXmlwriterFilename: Error at xmlTextWriterEndElement\n");
            return E_JIP_ERROR_FAILED;
        }
        
        /* Next entry */
        psDeviceCacheEntry = psDeviceCacheEntry->psNext;
    }
    
    /* Finish the document and write it out */
    rc = xmlTextWriterEndDocument(writer);
    if (rc < 0) {
        printf("testXmlwriterFilename: Error at xmlTextWriterEndDocument\n");
        return E_JIP_ERROR_FAILED;
    }

    /* Free up it's memory */
    xmlFreeTextWriter(writer);

    return E_JIP_OK;
}


teJIP_Status eJIPService_PersistXMLSaveNetwork(tsJIP_Context *psJIP_Context, const char *pcFileName)
{
    PRIVATE_CONTEXT(psJIP_Context);
    tsNode             *psNode;
    char                acNodeAddressBuffer[INET6_ADDRSTRLEN] = "Could not determine address\n";
    xmlDocPtr           psXMLDocument   = NULL;
    xmlNodePtr          psXMLNodeCache  = NULL;
    xmlNodePtr          psXMLNetwork    = NULL;
    xmlNodePtr          psXMLNode       = NULL;
    teJIP_Status        eStatus         = E_JIP_ERROR_FAILED;
    static int          iInitXML        = 1;
    
    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    if (iInitXML)
    {
        // Initialise XML Parser once per application otherwise BAD THINGS(tm) happen.
        xmlInitParser();
        iInitXML = 0;
    }
    
    xmlThrDefIndentTreeOutput(1);
    LIBXML_TEST_VERSION
    /* Don't print parser errors to stdout - that messes up cgi's etc. */
    initGenericErrorDefaultFunc(NULL);

    DBG_vPrintf(DBG_PERSIST, "\n\nSaving network state to %s\n", pcFileName);

    psXMLDocument = xmlParseFile(pcFileName);
    if (psXMLDocument)
    {
        DBG_vPrintf(DBG_PERSIST, "Parsed existing file successfully\n");
        psXMLNodeCache = xmlDocGetRootElement(psXMLDocument);
        
        if (psXMLNodeCache)
        {
            if (strcmp((const char *)psXMLNodeCache->name, "JIP_Cache"))
            {
                // This doesn't look like a JIP_Cache file :-s
                xmlFreeDoc(psXMLDocument);
                psXMLDocument = NULL;
                psXMLNodeCache = NULL;
            }
        }
    }
    
    if (psXMLNodeCache == NULL)
    {
        DBG_vPrintf(DBG_PERSIST, "Error parsing network file %s, creating from scratch.\n", pcFileName);
        
        if ((psXMLDocument = xmlNewDoc(BAD_CAST "1.0")) == NULL) // New XML version 1.0 document
        {
            DBG_vPrintf(DBG_PERSIST, "Error creating new document\n");
            goto done;
        }
        
        if ((psXMLNodeCache = xmlNewNode(NULL, BAD_CAST "JIP_Cache")) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "Error creating JIP_Cache tag\n");
            goto done;
        }
        xmlDocSetRootElement(psXMLDocument, psXMLNodeCache);
        {
            char acBuffer[5];
            snprintf(acBuffer, sizeof(acBuffer), "%d", CACHE_FILE_VERSION);
            if ((xmlNewProp(psXMLNodeCache, BAD_CAST "Version", BAD_CAST acBuffer)) == NULL)
            {
                DBG_vPrintf(DBG_PERSIST, "Error adding version\n");
                goto done;
            }
        }
    }
    
    {
        xmlXPathContextPtr xpathContext; 
        xmlXPathObjectPtr xpathObject; 
        char acXpathExpression[1024];
        
        // Create an xpath expresssion to find the network with the border router we want within the xml document.
        inet_ntop(AF_INET6, &psJIP_Private->sNetworkContext.sBorder_Router_IPv6_Address.sin6_addr, acNodeAddressBuffer, INET6_ADDRSTRLEN);
        snprintf(acXpathExpression, sizeof(acXpathExpression), "//Network[@BorderRouter=\"%s\"]", acNodeAddressBuffer);
        DBG_vPrintf(DBG_PERSIST, "Using XPATH %s\n", acXpathExpression);
        
        /* Create xpath evaluation context */
        if((xpathContext = xmlXPathNewContext(psXMLDocument)) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST,"Error: unable to create new XPath context\n");
            goto done;
        }
        
        /* Evaluate xpath expression */
        if((xpathObject = xmlXPathEvalExpression(BAD_CAST acXpathExpression, xpathContext)) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "Error: unable to evaluate xpath expression \"%s\"\n", acXpathExpression);
            xmlXPathFreeContext(xpathContext); 
            goto done;
        }
        
        if (xpathObject->nodesetval)
        {
            DBG_vPrintf(DBG_PERSIST, "Number of matching nodes: %d\n", xpathObject->nodesetval->nodeNr);
            if (xpathObject->nodesetval->nodeNr)
            {
                int i;
                for (i = 0; i < xpathObject->nodesetval->nodeNr; i++)
                {
                    DBG_vPrintf(DBG_PERSIST, "Network %d for border router found.\n", i);
                
                    psXMLNetwork = xpathObject->nodesetval->nodeTab[i];

                    /* Empty the existing node */
                    while (psXMLNetwork->children)
                    {
                        xmlUnlinkNode(psXMLNetwork->children);
                        xmlFreeNode(psXMLNetwork->children);
                    }
                }
            }
        }
        
        xmlXPathFreeObject(xpathObject);
        xmlXPathFreeContext(xpathContext); 
        
        if (!psXMLNetwork)
        {
            DBG_vPrintf(DBG_PERSIST, "Matching network not found, creating.\n");
            
            if ((psXMLNetwork = xmlNewChild(psXMLNodeCache, NULL, BAD_CAST "Network", NULL)) == NULL)
            {
                DBG_vPrintf(DBG_PERSIST, "Error creating Network tag\n");
                goto done;
            }
            if ((xmlNewProp(psXMLNetwork, BAD_CAST "BorderRouter", BAD_CAST acNodeAddressBuffer)) == NULL)
            {
                DBG_vPrintf(DBG_PERSIST, "Error creating BorderRouter attribute\n");
                goto done;
            }
            
            DBG_vPrintf(DBG_PERSIST, "Created new network at %p\n", psXMLNetwork);
        }
    }
    
    psNode = psJIP_Context->sNetwork.psNodes;
    while (psNode)
    {
        /* Now add all of the nodes to the network object */
        char acBuffer[64];
        snprintf(acBuffer, sizeof(acBuffer), "0x%08x", psNode->u32DeviceId);
        inet_ntop(AF_INET6, &psNode->sNode_Address.sin6_addr, acNodeAddressBuffer, INET6_ADDRSTRLEN);
        
        DBG_vPrintf(DBG_PERSIST, "Adding device %s, address %s\n", acBuffer, acNodeAddressBuffer);
        
        if ((psXMLNode = xmlNewChild(psXMLNetwork, NULL, BAD_CAST "Node", NULL)) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "Error adding node tag\n");
            goto done;
        }
        if ((xmlNewProp(psXMLNode, BAD_CAST "DeviceID", BAD_CAST acBuffer)) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "Error creating DeviceID attribute\n");
            goto done;
        }
        if ((xmlNewProp(psXMLNode, BAD_CAST "Address", BAD_CAST acNodeAddressBuffer)) == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "Error creating Address attribute\n");
            goto done;
        }

        /* Next node */
        psNode = psNode->psNext;
    }

    DBG_vPrintf(DBG_PERSIST, "Saving DOM to %s\n", pcFileName);
    /* Save the output file */
    if (xmlSaveFileEnc (pcFileName, psXMLDocument, MY_ENCODING) < 0)
    {
        DBG_vPrintf(DBG_PERSIST, "Error saving file\n");
        goto done;
    }
        
    DBG_vPrintf(DBG_PERSIST, "Network saved sucessfully.\n");
    eStatus = E_JIP_OK;
    
done:
    xmlFreeDoc(psXMLDocument);
    return eStatus;
}


static     tsJIP_Context *psDecodeJipContext;
static     int iValidCacheFile          = 0;
static     int iPopulateNetworkNodes    = 1; // Populate the network from file if possible
static     int iPendingMib              = 0;
static     int iPendingDeviceIdNode     = 0;
static     tsNode *psNode = NULL;
static     tsMib *psMib;
static     tsVar *psVar;


/**
 * processNode:
 * @reader: the xmlReader
 *
 * Dump information about the current node
 */
static void
processNode(xmlTextReaderPtr reader) {

    PRIVATE_CONTEXT(psDecodeJipContext);

    static enum
    {
        E_STATE_NONE,
        E_STATE_MIBS,
        E_STATE_DEVICES,
        E_STATE_NETWORK,
    } eState = E_STATE_NONE;
    
    char *pcBorderRouterAddress = NULL;
    char *NodeName              = NULL;
    char *pcVersion             = NULL;
    char *pcMibId               = NULL;
    char *pcName                = NULL;
    char *pcIndex               = NULL;
    char *pcType                = NULL;
    char *pcAccess              = NULL;
    char *pcSecurity            = NULL;
    char *pcDeviceId            = NULL;
    char *pcAddress             = NULL;
    char *pcSize                = NULL;
    char *pcData                = NULL;
    uint8_t *pu8DataBuffer      = NULL;


    NodeName = (char *)xmlTextReaderName(reader);
    
    if (strcmp(NodeName, "JIP_Cache") == 0)
    {
        if (xmlTextReaderAttributeCount(reader) == 1)
        {
            long int u32Version;
            
            pcVersion = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Version");
            if (pcVersion == NULL)
            {
                DBG_vPrintf(DBG_PERSIST, "No Version attribute for Cache file\n");
                u32Version = 0;
                goto done;
            }
            else
            {
                errno = 0;
                u32Version = strtoll(pcVersion, NULL, 16);
                if (errno)
                {
                    u32Version = 0;
                    perror("strtol");
                goto done;
                }
            }
            
            if (u32Version == CACHE_FILE_VERSION)
            {
                DBG_vPrintf(DBG_PERSIST, "Valid cache file\n");
                iValidCacheFile = 1;
                goto done;
            }
            else
            {
                DBG_vPrintf(DBG_PERSIST, "Incorrect version of cache file (%ld) for this version of libJIP(%d)\n", u32Version, CACHE_FILE_VERSION);
                goto done;
            }
        }
    }
    
    if (!iValidCacheFile)
    {
        DBG_vPrintf(DBG_PERSIST, "Not a valid cache file\n");
        /* Not a valid cache file */
        goto done;
    }

    if (strcmp(NodeName, "MibIdCache") == 0)
    {
        eState = E_STATE_MIBS;
        goto done;
    }
    else if (strcmp(NodeName, "DeviceIdCache") == 0)
    {
        eState = E_STATE_DEVICES;
        if (iPendingMib && psMib)
        {
            DBG_vPrintf(DBG_PERSIST, "Adding Mib ID 0x%08x to cache\n", psMib->u32MibId);
            (void)Cache_Add_Mib(&psJIP_Private->sCache, psMib);
            eJIP_FreeMib(psDecodeJipContext, psMib);
            psMib = NULL;
            iPendingMib = 0;
        }
        goto done;
    }
    else if (strcmp(NodeName, "Network") == 0)
    {
        tsJIPAddress sBorder_Router_IPv6_Address;
    
        pcBorderRouterAddress = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"BorderRouter");
        if (pcBorderRouterAddress == NULL)
        {
            DBG_vPrintf(DBG_PERSIST, "No border router attribute for Cache file\n");
            goto done;
        }
        
        /* Get border router IPv6 address */
        if (inet_pton(AF_INET6, pcBorderRouterAddress, &sBorder_Router_IPv6_Address.sin6_addr) <= 0)
        {
            DBG_vPrintf(DBG_PERSIST, "Could not determine network address from [%s]\n", pcBorderRouterAddress);
            goto done;
        }
        
        if (memcmp(&sBorder_Router_IPv6_Address.sin6_addr, &psJIP_Private->sNetworkContext.sBorder_Router_IPv6_Address.sin6_addr, sizeof(struct in6_addr)) == 0)
        {
            DBG_vPrintf(DBG_PERSIST, "Found network for my border router\n");
            eState = E_STATE_NETWORK;
        }
        else
        {
            DBG_vPrintf(DBG_PERSIST, "Found network for different border router\n");
            eState = E_STATE_NONE;
        }
        goto done;
    }
    
    switch (eState)
    {
        case(E_STATE_MIBS):
        {
            if (strcmp(NodeName, "Mib") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 1)
                {
                    uint32_t u32MibId;
                    
                    /* Add previous Mib */
                    if (iPendingMib && psMib)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Adding Mib ID 0x%08x to cache\n", psMib->u32MibId);
                        (void)Cache_Add_Mib(&psJIP_Private->sCache, psMib);
                        eJIP_FreeMib(psDecodeJipContext, psMib);
                        psMib = NULL;
                        iPendingMib = 0;
                    }

                    pcMibId = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"ID");
                    if (pcMibId == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No ID attribute for Mib\n");
                        goto done;
                    }
                    errno = 0;
                    u32MibId = strtoll(pcMibId, NULL, 16);
                    if (errno)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Error in strtoll (%s)\n", strerror(errno));
                        goto done;
                    }
                    
                    DBG_vPrintf(DBG_PERSIST, "Got Mib ID 0x%08x\n", u32MibId);

                    psMib = malloc(sizeof(tsMib));
    
                    if (!psMib)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Error allocating space for Mib\n");
                        goto done;
                    }
                    
                    iPendingMib = 1;
                    memset(psMib, 0, sizeof(tsMib));
                    psMib->u32MibId = u32MibId;
                }
            }
            else if (strcmp(NodeName, "Var") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 5)
                {
                    uint8_t u8Index;
                    pcName      = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Name");
                    if (pcName == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Name attribute for Var\n");
                        goto done;
                    }
                    pcIndex     = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Index");
                    if (pcIndex == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Index attribute for Var\n");
                        goto done;
                    }
                    pcType      = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Type");
                    if (pcType == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Type attribute for Var\n");
                        goto done;
                    }
                    pcAccess    = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Access");
                    if (pcAccess == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Access Type attribute for Var\n");
                        goto done;
                    }
                    pcSecurity  = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Security");
                    if (pcSecurity == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Security attribute for Var\n");
                        goto done;
                    }
                    u8Index = atoi(pcIndex);
                    DBG_vPrintf(DBG_PERSIST, "Got Var index %d with name %s, Type %s, Access %s, Security %s\n", u8Index, pcName, pcType, pcAccess, pcSecurity);

                    psVar = psJIP_MibAddVar(psMib, u8Index, pcName, atoi(pcType), atoi(pcAccess), atoi(pcSecurity));
                }
            }
            break;
        }
        
        case(E_STATE_DEVICES):
        {
            if (strcmp(NodeName, "Device") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 1)
                {
                    long int u32DeviceId;
                    
                    /* Add previous entry */
                    if (iPendingDeviceIdNode && psNode)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Adding Device ID 0x%08x to cache\n", psNode->u32DeviceId);
                        Cache_Add_Node(&psJIP_Private->sCache, psNode);
                        eJIP_NetFreeNode(psDecodeJipContext, psNode);
                        psNode = NULL;
                        iPendingDeviceIdNode = 0;
                    }
                    
                    pcDeviceId = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"ID");
                    if (pcDeviceId == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No ID attribute for Device\n");
                        goto done;
                    }
                    else
                    {
                        errno = 0;
                        u32DeviceId = strtoll(pcDeviceId, NULL, 16);
                        if (errno)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Error in strtoll (%s)\n", strerror(errno));
                            goto done;
                        }
                    }

                    DBG_vPrintf(DBG_PERSIST, "Got device ID 0x%08x\n", (unsigned int)u32DeviceId);

                    DBG_vPrintf(DBG_PERSIST, "Creating new node for device cache\n");
                    psNode = malloc(sizeof(tsNode));

                    if (!psNode)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Error allocating space for Node\n");
                        goto done;
                    }
                    
                    iPendingDeviceIdNode = 1;

                    memset(psNode, 0, sizeof(tsNode));

                    psNode->u32DeviceId    = u32DeviceId;
                    psNode->u32NumMibs     = 0;
                    
                    eUtils_LockCreate(&psNode->sLock);
                }
            }
            else if (strcmp(NodeName, "Mib") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 3)
                {
                    uint32_t u32MibId;
                    uint8_t u8Index;
                    
                    pcMibId = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"ID");
                    if (pcMibId == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No ID attribute for Mib\n");
                        goto done;
                    }
                    else
                    {
                        errno = 0;
                        u32MibId = strtoll(pcMibId, NULL, 16);
                        if (errno)
                        {
                            perror("strtol");
                            goto done;
                        }
                    }
                    
                    pcIndex = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Index");
                    if (pcIndex == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Index attribute for Mib\n");
                        goto done;
                    }
                    u8Index = atoi(pcIndex);
                    
                    pcName = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Name");
                    if (pcName == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Name attribute for Mib\n");
                        goto done;
                    }

                    DBG_vPrintf(DBG_PERSIST, "Got Mib ID 0x%0x Index %d with name %s\n", u32MibId, u8Index, pcName);
                    
                    /* Add the Mib to the device cache */
                    psMib = psJIP_NodeAddMib(psNode, u32MibId, u8Index, pcName);
                    
                    /* And populate it from the vars we already loaded */
                    if (Cache_Populate_Mib(&psJIP_Private->sCache, psMib) != E_JIP_OK)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Failed to populate Mib ID 0x%0x from cache\n", u32MibId);
                    }
                }
            }
            else if (strcmp(NodeName, "VarData") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 3)
                {
                    uint32_t u32MibId;
                    uint8_t  u8Index;
                    uint8_t  u8Size;
                    int i;
            
                    DBG_vPrintf(DBG_PERSIST, "Got VarData\n");
                    
                    pcMibId = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"MibID");
                    if (pcMibId == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Mib ID attribute for VarData\n");
                        goto done;
                    }
                    else
                    {
                        errno = 0;
                        u32MibId = strtoll(pcMibId, NULL, 16);
                        if (errno)
                        {
                            perror("strtol");
                            goto done;
                        }
                    }
                    
                    pcIndex = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"VarIndex");
                    if (pcIndex == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No VarIndex attribute for VarData\n");
                        goto done;
                    }
                    u8Index = atoi(pcIndex);
                    
                    pcSize = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Size");
                    if (pcSize == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Size attribute for VarData\n");
                        goto done;
                    }
                    u8Size = atoi(pcSize);
                    
                    DBG_vPrintf(DBG_PERSIST, "Got VarData for Mib ID 0x%08x, var index %d, size %d\n", u32MibId, u8Index, u8Size);
                    
                    pcData = xmlTextReaderReadString(reader);
                    
                    if (!pcData)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Error parsing data value\n");
                        goto done;
                    }
                    
                    if (strlen(pcData) != u8Size *2)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Reported size (%d) does not match buffer length(%d)\n", u8Size*2, strlen(pcData));
                        goto done;
                    }
                    
                    pu8DataBuffer = malloc(u8Size);
                    if (!pu8DataBuffer)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Memory allocation failure\n");
                        goto done;
                    }
                    
                    for (i = u8Size; i > 0; i--)
                    {
                        if (sscanf(&pcData[(i-1)*2], "%hhx", &pu8DataBuffer[i-1]) != 1)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Error converting buffer\n");
                            goto done;
                        }
                        pcData[(i-1)*2] = '\0';
                        DBG_vPrintf(DBG_PERSIST, "Byte %d: 0x%02X\n", i-1, pu8DataBuffer[i-1]);
                    }
                    
                    psMib = psJIP_LookupMibId(psNode, NULL, u32MibId);
                    if (psMib)
                    {
                        DBG_vPrintf(DBG_PERSIST, "Found mib\n");
                        psVar = psJIP_LookupVarIndex(psMib, u8Index);
                        if (psVar)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Found var to set\n");
                            eJIP_SetVarValue(psVar, pu8DataBuffer, u8Size);
                        }
                    }
                }
            }
            break;
        }
        
        case(E_STATE_NETWORK):
        {
            if (strcmp(NodeName, "Node") == 0)
            {
                if (xmlTextReaderAttributeCount(reader) == 2)
                {
                    struct sockaddr_in6 sNodeAddress;

                    uint32_t u32DeviceId;
                    int s;
                    
                    pcDeviceId = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"DeviceID");
                    if (pcDeviceId == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No DeviceID attribute for Node\n");
                        goto done;
                    }
                    else
                    {
                        errno = 0;
                        u32DeviceId = strtoll(pcDeviceId, NULL, 16);
                        if (errno)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Error in strtoll (%s)\n", strerror(errno));
                            goto done;
                        }
                    }

                    pcAddress = (char *)xmlTextReaderGetAttribute(reader, (unsigned char *)"Address");
                    if (pcAddress == NULL)
                    {
                        DBG_vPrintf(DBG_PERSIST, "No Address attribute for Node\n");
                        goto done;
                    }

                    DBG_vPrintf(DBG_PERSIST, "Got node device type 0x%08x with address %s\n", u32DeviceId, pcAddress);

                    memset (&sNodeAddress, 0, sizeof(struct sockaddr_in6));
                    sNodeAddress.sin6_family  = AF_INET6;
                    sNodeAddress.sin6_port    = htons(JIP_DEFAULT_PORT);

                    s = inet_pton(AF_INET6, pcAddress, &sNodeAddress.sin6_addr);
                    if (s <= 0)
                    {
                        if (s == 0)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Unknown host: %s\n", pcAddress);
                        }
                        else if (s < 0)
                        {
                            DBG_vPrintf(DBG_PERSIST, "Error in inet_pton (%s)\n", strerror(errno));
                        }
                        goto done;
                    }

                    if (iPopulateNetworkNodes)
                    {

                        DBG_vPrintf(DBG_PERSIST, "Adding node device type 0x%08x with address %s to network\n", u32DeviceId, pcAddress);
                        eJIP_NetAddNode(psDecodeJipContext, &sNodeAddress, u32DeviceId, NULL);
                    }
                }
            }
            break;
        }
        default:
            DBG_vPrintf(DBG_PERSIST, "In unknown state\n");
            break;
    }

done:
    free(pcBorderRouterAddress);
    free(NodeName);
    free(pcVersion);
    free(pcMibId);
    free(pcName);
    free(pcIndex);
    free(pcType);
    free(pcAccess);
    free(pcSecurity);
    free(pcDeviceId);
    free(pcAddress);
    free(pcSize);
    free(pcData);
    free(pu8DataBuffer);
    
    return;
}


static teJIP_Status eJIP_PersistLoad(tsJIP_Context *psJIP_Context, const char *pcFileName, const int iPopulateNetwork)
{
    PRIVATE_CONTEXT(psJIP_Context);
    int ret;
    xmlTextReaderPtr reader;

    DBG_vPrintf(DBG_FUNCTION_CALLS, "%s\n", __FUNCTION__);
    
    LIBXML_TEST_VERSION
    /* Don't print parser errors to stdout - that messes up cgi's etc. */
    initGenericErrorDefaultFunc(NULL);

    DBG_vPrintf(DBG_PERSIST, "Loading network state from %s\n", pcFileName);
    
    iPopulateNetworkNodes = iPopulateNetwork;
    
    DBG_vPrintf(DBG_PERSIST, "Populating Device Cache\n"); // Always done
    if (iPopulateNetworkNodes)
    {
        DBG_vPrintf(DBG_PERSIST, "Populating Nodes\n");
    }
    
    psDecodeJipContext = psJIP_Context;
    
    reader = xmlReaderForFile(pcFileName, NULL, 0);
    if (reader != NULL)
    {
        ret = xmlTextReaderRead(reader);
        while (ret == 1) {
            processNode(reader);
            ret = xmlTextReaderRead(reader);
        }
        
        /* Finish up */
        if (iPendingDeviceIdNode && psNode)
        {
            DBG_vPrintf(DBG_PERSIST, "Adding Device ID 0x%08x to cache\n", psNode->u32DeviceId);
            Cache_Add_Node(&psJIP_Private->sCache, psNode);
            eJIP_NetFreeNode(psDecodeJipContext, psNode);
            psNode = NULL;
            iPendingDeviceIdNode = 0;
        }
        
        xmlTextReaderClose(reader);
        xmlFreeTextReader(reader);
        
        if (ret != 0)
        {
            DBG_vPrintf(DBG_PERSIST, "Failed to parse %s\n", pcFileName);
            return E_JIP_ERROR_FAILED;
        }
    }
    else
    {
        DBG_vPrintf(DBG_PERSIST, "Unable to open %s\n", pcFileName);
        return E_JIP_ERROR_FAILED;
    }
    
    if (!iValidCacheFile)
    {
        /* Wasn't a valid cache file */
        return E_JIP_ERROR_FAILED;
    }  

    return E_JIP_OK;
}


teJIP_Status eJIPService_PersistXMLLoadDefinitions(tsJIP_Context *psJIP_Context, const char *pcFileName)
{
    return eJIP_PersistLoad(psJIP_Context, pcFileName, 0);
}

teJIP_Status eJIPService_PersistXMLLoadNetwork(tsJIP_Context *psJIP_Context, const char *pcFileName)
{
    return eJIP_PersistLoad(psJIP_Context, pcFileName, 1);
}

