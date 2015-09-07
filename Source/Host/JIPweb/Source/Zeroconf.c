/****************************************************************************
 *
 * MODULE:             JIP Web Apps
 *
 * COMPONENT:          Zeroconf
 *
 * REVISION:           $Revision: 43426 $
 *
 * DATED:              $Date: 2012-06-18 15:24:02 +0100 (Mon, 18 Jun 2012) $
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



#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <arpa/inet.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-client/lookup.h>

#include <avahi-common/alternative.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#define LOG(stream, fmt, ...) //fprintf(stream, fmt, __VA_ARGS__)

static AvahiEntryGroup *group = NULL;
static AvahiSimplePoll *simple_poll = NULL;

static char *name       = NULL;

static void create_services(AvahiClient *c);

static void entry_group_callback(AvahiEntryGroup *g, AvahiEntryGroupState state, AVAHI_GCC_UNUSED void *userdata) {
    assert(g == group || group == NULL);
    group = g;

    /* Called whenever the entry group state changes */

    switch (state) {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
            LOG(stderr, "Service '%s' successfully established.\n", name);
            break;

        case AVAHI_ENTRY_GROUP_COLLISION : {
            char *n;

            /* A service name collision with a remote service
             * happened. Let's pick a new name */
            n = avahi_alternative_service_name(name);
            avahi_free(name);
            name = n;

            LOG(stderr, "Service name collision, renaming service to '%s'\n", name);

            /* And recreate the services */
            create_services(avahi_entry_group_get_client(g));
            break;
        }

        case AVAHI_ENTRY_GROUP_FAILURE :

            LOG(stderr, "Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));

            /* Some kind of failure happened while we were registering our services */
            avahi_simple_poll_quit(simple_poll);
            break;

        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
            ;
    }
}

static void create_services(AvahiClient *c) {
    char *n;
    int ret;
    assert(c);

    /* If this is the first time we're called, let's create a new
     * entry group if necessary */

    if (!group)
        if (!(group = avahi_entry_group_new(c, entry_group_callback, NULL))) {
            LOG(stderr, "avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(c)));
            goto fail;
        }

    /* If the group is empty (either because it was just created, or
     * because it was reset previously, add our entries.  */

    if (avahi_entry_group_is_empty(group)) {
        LOG(stderr, "Adding UDP service '%s'\n", name);
        
        /* Add the service for JIP on the module */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0, name, "_jip4._udp", NULL, NULL, 1873, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            LOG(stderr, "Failed to add _jip._udp service: %s\n", avahi_strerror(ret));
            goto fail;
        }
        
        LOG(stderr, "Adding TCP service '%s'\n", name);
        
        /* Add the service for JIP on the module */
        if ((ret = avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0, name, "_jip4._tcp", NULL, NULL, 1873, NULL)) < 0) {

            if (ret == AVAHI_ERR_COLLISION)
                goto collision;

            LOG(stderr, "Failed to add _jip._udp service: %s\n", avahi_strerror(ret));
            goto fail;
        }
        /* Tell the server to register the service */
        if ((ret = avahi_entry_group_commit(group)) < 0) {
            LOG(stderr, "Failed to commit entry group: %s\n", avahi_strerror(ret));
            goto fail;
        }
    }

    return;

collision:

    /* A service name collision with a local service happened. Let's
     * pick a new name */
    n = avahi_alternative_service_name(name);
    avahi_free(name);
    name = n;

    LOG(stderr, "Service name collision, renaming service to '%s'\n", name);

    avahi_entry_group_reset(group);

    create_services(c);
    return;

fail:
    avahi_simple_poll_quit(simple_poll);
}

static void client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    switch (state) {
        case AVAHI_CLIENT_S_RUNNING:

            /* The server has startup successfully and registered its host
             * name on the network, so it's time to create our services */
            create_services(c);
            break;

        case AVAHI_CLIENT_FAILURE:

            LOG(stderr, "Client failure: %s\n", avahi_strerror(avahi_client_errno(c)));
            avahi_simple_poll_quit(simple_poll);

            break;

        case AVAHI_CLIENT_S_COLLISION:

            /* Let's drop our registered services. When the server is back
             * in AVAHI_SERVER_RUNNING state we will register them
             * again with the new host name. */

        case AVAHI_CLIENT_S_REGISTERING:

            /* The server records are now being established. This
             * might be caused by a host name change. We need to wait
             * for our own records to register until the host name is
             * properly esatblished. */

            if (group)
                avahi_entry_group_reset(group);

            break;

        case AVAHI_CLIENT_CONNECTING:
            ;
    }
}

static pthread_t sZC_ThreadInfo;
static     AvahiClient *client = NULL;

void *pvZC_Thread(void *args)
{
    /* Run the main loop */
    avahi_simple_poll_loop(simple_poll);
}


int ZC_RegisterServices(const char *pcServiceName)
{
    int error;
    int ret = 1;
    struct timeval tv;

    /* Allocate main loop object */
    if (!(simple_poll = avahi_simple_poll_new())) {
        LOG(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    name = avahi_strdup(pcServiceName);

    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(simple_poll), 0, client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        LOG(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Create thread to run the main loop */
    if (pthread_create(&sZC_ThreadInfo, NULL, pvZC_Thread, NULL) != 0)
    {
        perror("Error starting ZC thread");
        goto fail;
    }

    ret = 0;
    return ret;

fail:
    /* Cleanup things */
    if (client)
        avahi_client_free(client);

    if (simple_poll)
        avahi_simple_poll_free(simple_poll);

    avahi_free(name);

    return ret;
}


static AvahiSimplePoll *browse_simple_poll = NULL;

struct
{
    int                 iNumServices;
    int                 iNumAddresses;
    struct in6_addr     *asAddresses;
} sAddressResults;

static void resolve_callback(
    AvahiServiceResolver *r,
    AVAHI_GCC_UNUSED AvahiIfIndex interface,
    AVAHI_GCC_UNUSED AvahiProtocol protocol,
    AvahiResolverEvent event,
    const char *name,
    const char *type,
    const char *domain,
    const char *host_name,
    const AvahiAddress *address,
    uint16_t port,
    AvahiStringList *txt,
    AvahiLookupResultFlags flags,
    AVAHI_GCC_UNUSED void* userdata) {

    assert(r);

    /* Called whenever a service has been resolved successfully or timed out */

    switch (event) {
        case AVAHI_RESOLVER_FAILURE:
            LOG(stderr, "(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
            sAddressResults.iNumServices--;
            break;

        case AVAHI_RESOLVER_FOUND: {
            char a[AVAHI_ADDRESS_STR_MAX], *t;

            LOG(stderr, "Service '%s' of type '%s' in domain '%s':\n", name, type, domain);

            avahi_address_snprint(a, sizeof(a), address);
            t = avahi_string_list_to_string(txt);
            LOG(stderr,
                    "\t%s:%u (%s)\n"
                    "\tTXT=%s\n"
                    "\tcookie is %u\n"
                    "\tis_local: %i\n"
                    "\tour_own: %i\n"
                    "\twide_area: %i\n"
                    "\tmulticast: %i\n"
                    "\tcached: %i\n",
                    host_name, port, a,
                    t,
                    avahi_string_list_get_service_cookie(txt),
                    !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
                    !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
                    !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
                    !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
                    !!(flags & AVAHI_LOOKUP_RESULT_CACHED));
            
            if (address->proto == AVAHI_PROTO_INET6)
            {
                int i, duplicate = 0;
                for (i = 0; i < sAddressResults.iNumAddresses; i++)
                {
                    if (memcmp(&sAddressResults.asAddresses[i], &address->data.ipv6, sizeof(struct in6_addr)) == 0)
                    {
                        duplicate = 1;
                        break;
                    }
                }
                if (!duplicate)
                {
                    sAddressResults.asAddresses = realloc(sAddressResults.asAddresses, sizeof(struct in6_addr) * (sAddressResults.iNumAddresses + 1));
                    memcpy(&sAddressResults.asAddresses[sAddressResults.iNumAddresses], &address->data.ipv6, sizeof(struct in6_addr));
                    sAddressResults.iNumAddresses++;
                }
                else
                {
                    sAddressResults.iNumServices--;
                }   
            }
            else
            {
                sAddressResults.iNumServices--;
            }
            
            avahi_free(t);
            
            if (sAddressResults.iNumServices == sAddressResults.iNumAddresses)
            {
                avahi_simple_poll_quit(browse_simple_poll);
            }
        }
    }

    avahi_service_resolver_free(r);
}

static void browse_callback(
    AvahiServiceBrowser *b,
    AvahiIfIndex interface,
    AvahiProtocol protocol,
    AvahiBrowserEvent event,
    const char *name,
    const char *type,
    const char *domain,
    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
    void* userdata) {

    AvahiClient *c = userdata;
    assert(b);

    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */

    switch (event) {
        case AVAHI_BROWSER_FAILURE:

            LOG(stderr, "(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
            avahi_simple_poll_quit(browse_simple_poll);
            return;

        case AVAHI_BROWSER_NEW:
            LOG(stderr, "(Browser) NEW: service '%s' of type '%s' in domain '%s'\n", name, type, domain);

            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */

            sAddressResults.iNumServices++;
            if (!(avahi_service_resolver_new(c, interface, protocol, name, type, domain, AVAHI_PROTO_UNSPEC, 0, resolve_callback, c)))
                LOG(stderr, "Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(c)));

            break;

        case AVAHI_BROWSER_REMOVE:
            LOG(stderr, "(Browser) REMOVE: service '%s' of type '%s' in domain '%s'\n", name, type, domain);
            break;

        case AVAHI_BROWSER_ALL_FOR_NOW:
        case AVAHI_BROWSER_CACHE_EXHAUSTED:
            LOG(stderr, "(Browser) %s\n", event == AVAHI_BROWSER_CACHE_EXHAUSTED ? "CACHE_EXHAUSTED" : "ALL_FOR_NOW");
            if (event == AVAHI_BROWSER_ALL_FOR_NOW)
            {
                //
                if (sAddressResults.iNumServices == sAddressResults.iNumAddresses)
                {
                    avahi_simple_poll_quit(browse_simple_poll);
                }
            }
            break;
    }
}

static void browse_client_callback(AvahiClient *c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata) {
    assert(c);

    /* Called whenever the client or server state changes */

    if (state == AVAHI_CLIENT_FAILURE) {
        LOG(stderr, "Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
        avahi_simple_poll_quit(browse_simple_poll);
    }
}

int ZC_Get_Module_Addresses(struct in6_addr **pasAddress, int *piNumAddresses)
{
    AvahiClient *client = NULL;
    AvahiServiceBrowser *sb = NULL;
    int error;
    int ret = 1;

    *piNumAddresses = 0;
    *pasAddress = 0;
    
    sAddressResults.iNumAddresses = 0;
    sAddressResults.iNumServices = 0;
    sAddressResults.asAddresses = malloc(0);

    /* Allocate main loop object */
    if (!(browse_simple_poll = avahi_simple_poll_new())) {
        LOG(stderr, "Failed to create simple poll object.\n");
        goto fail;
    }

    /* Allocate a new client */
    client = avahi_client_new(avahi_simple_poll_get(browse_simple_poll), 0, browse_client_callback, NULL, &error);

    /* Check wether creating the client object succeeded */
    if (!client) {
        LOG(stderr, "Failed to create client: %s\n", avahi_strerror(error));
        goto fail;
    }

    /* Create the service browser */
    if (!(sb = avahi_service_browser_new(client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_jip._udp", NULL, 0, browse_callback, client))) {
        LOG(stderr, "Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(client)));
        goto fail;
    }

    /* Run the main loop */
    avahi_simple_poll_loop(browse_simple_poll);

    
    //printf("Got %d addresses\n", sAddressResults.iNumAddresses);
    
    *piNumAddresses = sAddressResults.iNumAddresses;
    *pasAddress = sAddressResults.asAddresses;
    
    ret = 0;

fail:

    /* Cleanup things */
    if (sb)
        avahi_service_browser_free(sb);

    if (client)
        avahi_client_free(client);

    if (browse_simple_poll)
        avahi_simple_poll_free(browse_simple_poll);
    
    return ret;
}

