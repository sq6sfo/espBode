#include <ESP8266WiFi.h>
#include "esp_network.h"
#include "esp_config.h"
#include "esp_parser.h"

/* Packet length must be four-bytes aligned. This function generates remaining
    bytes to fill the gap:

     unpadded   | padding    | packet length
    ------------+------------+-------------------
        0       |   0        |  0
        1       |   3        |  4
        2       |   2        |  4
        3       |   1        |  4
        4       |   0        |  4
        5       |   3        |  8
        6       |   2        |  8
        7       |   1        |  8
        ...     |   ...      |  ...
        n       | 3-(n+3)mod4|  unpadded+padding
    */
uint8_t getPadding(uint8_t unpadded)
{
    return 3-(unpadded+3)%4;
}

/* Procedure to swap the endianness from big-endian (network) to
    little-endian (ESP). We use fast-xor swap to omit the usage of temporary
    accumulator when swapping the variables:

    a <-> b:
    a = a xor b
    b = b xor a
    b = a xor b

    We swap every four bytes: ABCD EFGH -> DCBA HGFE
    Procedure is symmetric.
    */
void swapEndianess(uint8_t *data, uint8_t len)
{
    uint8_t i;
    len += getPadding(len);

    for(i=0;i<len;i+=4)
    {
            data[i]   ^= data[i+3];
            data[i+3] ^= data[i];
            data[i]   ^= data[i+3];

            data[i+1] ^= data[i+2];
            data[i+2] ^= data[i+1];
            data[i+1] ^= data[i+2];
    }
}

/* Function fills the RPC answer header. We use single-fragment communication,
    so bit #31 in the 'frag' field is always set. Also the length of the packet
    does not include the 'frag' field, so we need to subtract four bytes. */
void fillResponseHeader(uint8_t *hdr, uint32_t xid, uint32_t length)
{
    /* NOTE: for some mysterious reasons current compiler refuses to use defined
        struct pointer as a function arguments. To be fixed someday.
        Currently we just pass void* then cast it to a more useful type. */
    rpcresp_header *header = (rpcresp_header*)hdr;

    header->frag = RPC_SINGLE_FRAG | (length - 4);
    header->xid = xid;
    header->msg_type = RPC_REPLY;
    header->reply_state = 0x00;
    header->verifier_l = 0x00;
    header->verifier_h = 0x00;
    header->rpc_status = 0x00;
}

/* Receives the RPC packet. User is responsible for destroying the pointer.
    Function is blocking.
    - Arguments:
        uint8_t**   -   output pointer to data
        WiFiClient  -   pointer to the connection
    - Returns:
        uint8_t     -   number of received bytes */
uint32_t receiveRpcPacket(uint8_t **data, WiFiClient client)
{
    rpcreq_header   header;
    uint32_t        length;

    /* Wait till some data is available */
    while(!client.available());

    /* Read the RPC header */
    client.readBytes((uint8_t*)&header, sizeof(rpcreq_header));
    swapEndianess((uint8_t*)&header, sizeof(rpcreq_header));

    /* We only handle single-fragment VXI_11 packets */
    if((header.frag&RPC_SINGLE_FRAG) &&
       ((header.program == PORTMAP) || header.program == VXI_11_CORE))
    {
        /* Length of the whole packet is stored in the RPC header */
        length = (header.frag & 0x7FFFFFFF) + 4;
        *data = (uint8_t*)malloc(length);

        /* Copy the header */
        memcpy((rpcreq_header*)*data, &header, sizeof(rpcreq_header));

        /* Receive remaining data (length - header) */
        while(!client.available());
        client.readBytes(*data + sizeof(rpcreq_header),
                          length - sizeof(rpcreq_header));
        swapEndianess((uint8_t*)(*data + sizeof(rpcreq_header)),
                      length - sizeof(rpcreq_header));

        return length;
    }
    else
    {
        *data = NULL;
        return 0;
    }
}

void sendReadResponse(uint32_t xid, WiFiClient client)
{
    uint8_t length = sizeof(rcpresp_devReadWrite) + strlen(ID) + getPadding(strlen(ID));
    rcpresp_devReadWrite *response = (rcpresp_devReadWrite*)malloc(length);
    memset(response, 0, length);

    fillResponseHeader((uint8_t*)(&(response->header)), xid, length);

    response->errorCode = 0x00;
    response->reason = 0x04;
    response->dataLen = strlen((char*)gReadBuffer) + getPadding(strlen((char*)gReadBuffer));
    strcpy((char*)(response->data), (char*)gReadBuffer);

    /* Note: we do not swap endianness of the data payload */
    swapEndianess((uint8_t*)response, sizeof(rcpresp_devReadWrite));

    while(!client.availableForWrite());
    client.write((uint8_t*)response, length);

    free(response);
}

/* Handles RPC portmap GETPORT request */
uint8_t handlePortmap(uint8_t *packet, WiFiClient client)
{
    rpcreq_getport *getport = (rpcreq_getport*)packet;

    if(getport->header.procedure != PORTMAP_GETPORT)
    {
        DEBUG("ERROR: UNKNOWN RPC PACKET");
    }
    else
    {
        rpcresp_getport response;
        fillResponseHeader((uint8_t*)&(response.header), getport->header.xid, sizeof(rpcresp_getport));
        response.vxi_port = LXI_PORT;
        swapEndianess((uint8_t*)&response, sizeof(rpcresp_getport));
        while(!client.availableForWrite());
        client.write((uint8_t*)&response, sizeof(rpcresp_getport));
    }
    return 0;
}

void parseVxiWrite(uint8_t *packet)
{
    rcpreq_devReadWrite *request = (rcpreq_devReadWrite*)packet;

    swapEndianess((uint8_t*)&(request->data), request->dataLen);
    handleWriteMsg((char*)&(request->data), request->dataLen);
}

uint8_t handleVxi11(uint8_t *packet, WiFiClient client)
{
    rpcreq_header *header = (rpcreq_header*)packet;
    switch(header->procedure)
    {
    case VXI_11_CREATE_LINK:
        DEBUG("CREATE_LINK");
        /* We confirm that the device is ready */
        rpcresp_createLink create_response;
        fillResponseHeader((uint8_t*)&(create_response.header), header->xid, sizeof(rpcresp_createLink));

        create_response.errorCode = 0x00;
        create_response.linkId = 0x00;
        create_response.abortPort = 0x00;
        create_response.maxReceiveSize = 0x00800000;

        swapEndianess((uint8_t*)&create_response, sizeof(rpcresp_createLink));
        while(!client.availableForWrite());
        client.write((uint8_t*)&create_response, sizeof(rpcresp_createLink));
        return 0;

        break;
    case VXI_11_DESTROY_LINK:
        DEBUG("DESTROY_LINK");
        /* Received at the end of the communication. We reset and wait for PORTMAP */
        return 1;
        break;
    case VXI_11_DEV_READ:
        DEBUG("DEV_READ");
        /* Answer with rcpresp_devReadWrite packet for DEV_READ */
        sendReadResponse(header->xid, client);
        break;

    case VXI_11_DEV_WRITE:
        DEBUG("DEV_WRITE ");
        parseVxiWrite((uint8_t*)header);

        rcpresp_devReadWrite write_response;
        fillResponseHeader((uint8_t*)&(write_response), header->xid, sizeof(rcpresp_devReadWrite));

        write_response.errorCode = 0x00;
        write_response.reason = 0x04;
        write_response.dataLen = 0x00;

        swapEndianess((uint8_t*)&write_response, sizeof(rcpresp_devReadWrite));
        while(!client.availableForWrite());
        client.write((uint8_t*)&write_response, sizeof(rcpresp_devReadWrite));
        return 0;
        break;
    }
}

uint8_t handlePacket(WiFiClient client)
{
    uint8_t retVal = 0;
    rpcreq_header *header;
    if(0 == receiveRpcPacket((uint8_t**)&header, client))
    {
        /* ERROR: no packed received */
        return 1;
    }

    switch(header->program)
    {
    case PORTMAP:
        retVal = handlePortmap((uint8_t*)header, client);
        break;

    case VXI_11_CORE:
        retVal = handleVxi11((uint8_t*)header, client);
        break;
    }

    /* We are the user responsible for destroying the incoming packet */
    free(header);

    return retVal;
}
