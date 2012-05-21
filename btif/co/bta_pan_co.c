/************************************************************************************
 *
 *  Copyright (C) 2009-2011 Broadcom Corporation
 *
 *  This program is the proprietary software of Broadcom Corporation and/or its
 *  licensors, and may only be used, duplicated, modified or distributed
 *  pursuant to the terms and conditions of a separate, written license
 *  agreement executed between you and Broadcom (an "Authorized License").
 *  Except as set forth in an Authorized License, Broadcom grants no license
 *  (express or implied), right to use, or waiver of any kind with respect to
 *  the Software, and Broadcom expressly reserves all rights in and to the
 *  Software and all intellectual property rights therein.
 *  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE NO RIGHT TO USE THIS
 *  SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY BROADCOM AND DISCONTINUE
 *  ALL USE OF THE SOFTWARE.
 *
 *  Except as expressly set forth in the Authorized License,
 *
 *  1.     This program, including its structure, sequence and organization,
 *         constitutes the valuable trade secrets of Broadcom, and you shall
 *         use all reasonable efforts to protect the confidentiality thereof,
 *         and to use this information only in connection with your use of
 *         Broadcom integrated circuit products.
 *
 *  2.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED
 *         "AS IS" AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES,
 *         REPRESENTATIONS OR WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY,
 *         OR OTHERWISE, WITH RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY
 *         DISCLAIMS ANY AND ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY,
 *         NONINFRINGEMENT, FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES,
 *         ACCURACY OR COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR
 *         CORRESPONDENCE TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT
 *         OF USE OR PERFORMANCE OF THE SOFTWARE.
 *
 *  3.     TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
 *         ITS LICENSORS BE LIABLE FOR
 *         (i)   CONSEQUENTIAL, INCIDENTAL, SPECIAL, INDIRECT, OR EXEMPLARY
 *               DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY WAY RELATING TO
 *               YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN IF BROADCOM
 *               HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES; OR
 *         (ii)  ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
 *               SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE
 *               LIMITATIONS SHALL APPLY NOTWITHSTANDING ANY FAILURE OF
 *               ESSENTIAL PURPOSE OF ANY LIMITED REMEDY.
 *
 ************************************************************************************/

/************************************************************************************
 *
 *  Filename:      bta_pan_co.c
 *
 *  Description:   PAN stack callout api
 *
 *
 ***********************************************************************************/
#include "bta_api.h"
#include "bta_pan_api.h"
#include "bta_pan_ci.h"
#include "bta_pan_co.h"
#include "pan_api.h"
#include "gki.h"
//#include "btui.h"
//#include "btui_int.h"
#include <hardware/bluetooth.h>
#include <hardware/bt_pan.h>
#include "btif_pan_internal.h"
#include "bd.h"


#include <cutils/log.h>
#define info(fmt, ...)  LOGI ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define debug(fmt, ...) LOGD ("%s: " fmt,__FUNCTION__,  ## __VA_ARGS__)
#define warn(fmt, ...) LOGW ("## WARNING : %s: " fmt "##",__FUNCTION__,  ## __VA_ARGS__)
#define error(fmt, ...) LOGE ("## ERROR : %s: " fmt "##",__FUNCTION__,  ## __VA_ARGS__)
#define asrt(s) if(!(s)) LOGE ("## %s assert %s failed at line:%d ##",__FUNCTION__, #s, __LINE__)




/*******************************************************************************
**
** Function         bta_pan_co_init
**
** Description
**
**
** Returns          Data flow mask.
**
*******************************************************************************/
UINT8 bta_pan_co_init(UINT8 *q_level)
{

    LOGD("bta_pan_co_init");

    /* set the q_level to 30 buffers */
    *q_level = 30;

    //return (BTA_PAN_RX_PULL | BTA_PAN_TX_PULL);
    return (BTA_PAN_RX_PUSH_BUF | BTA_PAN_RX_PUSH | BTA_PAN_TX_PULL);
}




/*******************************************************************************
**
** Function         bta_pan_co_open
**
** Description
**
**
**
**
**
** Returns          void
**
*******************************************************************************/

void bta_pan_co_open(UINT16 handle, UINT8 app_id, tBTA_PAN_ROLE local_role, tBTA_PAN_ROLE peer_role, BD_ADDR peer_addr)
{
    LOGD("bta_pan_co_open:app_id:%d, local_role:%d, peer_role:%d, handle:%d",
            app_id, local_role, peer_role, handle);
    btpan_conn_t* conn = btpan_find_conn_addr(peer_addr);
    if(conn == NULL)
        conn = btpan_new_conn(handle, peer_addr, local_role, peer_role);
    if(conn)
    {
        LOGD("bta_pan_co_open:tap_fd:%d, open_count:%d, conn->handle:%d should = handle:%d, local_role:%d, remote_role:%d",
             btpan_cb.tap_fd, btpan_cb.open_count, conn->handle, handle, conn->local_role, conn->remote_role);
        //refresh the role & bt address

        btpan_cb.open_count++;
        conn->handle = handle;
        //bdcpy(conn->peer, peer_addr);
        if(btpan_cb.tap_fd < 0)
        {
            btpan_cb.tap_fd = btpan_tap_open();
            if(btpan_cb.tap_fd >= 0)
                create_tap_read_thread(btpan_cb.tap_fd);
        }
        if(btpan_cb.tap_fd >= 0)
        {
            conn->state = PAN_STATE_OPEN;
            bta_pan_ci_rx_ready(handle);
        }
    }
}


/*******************************************************************************
**
** Function         bta_pan_co_close
**
** Description      This function is called by PAN when a connection to a
**                  peer is closed.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_close(UINT16 handle, UINT8 app_id)
{
    LOGD("bta_pan_co_close:app_id:%d, handle:%d", app_id, handle);
    btpan_conn_t* conn = btpan_find_conn_handle(handle);
    if(conn && conn->state == PAN_STATE_OPEN)
    {
        LOGD("bta_pan_co_close");

        // let bta close event reset this handle as it needs
        // the handle to find the connection upon CLOSE
        //conn->handle = -1;
        conn->state = PAN_STATE_CLOSE;
        btpan_cb.open_count--;

        if(btpan_cb.open_count == 0)
        {
            destroy_tap_read_thread();
            if(btpan_cb.tap_fd != -1)
            {
                btpan_tap_close(btpan_cb.tap_fd);
                btpan_cb.tap_fd = -1;
            }
        }
    }
}

/*******************************************************************************
**
** Function         bta_pan_co_tx_path
**
** Description      This function is called by PAN to transfer data on the
**                  TX path; that is, data being sent from BTA to the phone.
**                  This function is used when the TX data path is configured
**                  to use the pull interface.  The implementation of this
**                  function will typically call Bluetooth stack functions
**                  PORT_Read() or PORT_ReadData() to read data from RFCOMM
**                  and then a platform-specific function to send data that
**                  data to the phone.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_tx_path(UINT16 handle, UINT8 app_id)
{
    BT_HDR          *p_buf;
    UINT8           i;
    BD_ADDR            src;
    BD_ADDR            dst;
    UINT16            protocol;
    BOOLEAN            ext;
    BOOLEAN         forward;

    LOGD("bta_pan_co_tx_path, handle:%d, app_id:%d", handle, app_id);

    btpan_conn_t* conn = btpan_find_conn_handle(handle);
    if(conn && conn->state != PAN_STATE_OPEN)
    {
        LOGE("bta_pan_co_tx_path: cannot find pan connction or conn is not opened, conn:%p, conn->state:%d", conn, conn->state);
        return;
    }
    do
    {

        /* read next data buffer from pan */
        if ((p_buf = bta_pan_ci_readbuf(handle, src, dst, &protocol,
                                 &ext, &forward)))
        {
            LOGD("bta_pan_co_tx_path, calling btapp_tap_send, p_buf->len:%d, offset:%d", p_buf->len, p_buf->offset);
            if(is_empty_eth_addr(conn->eth_addr) && is_valid_bt_eth_addr(src))
            {
                LOGD("pan bt peer addr: %02x:%02x:%02x:%02x:%02x:%02x, update its ethernet addr: %02x:%02x:%02x:%02x:%02x:%02x",
                        conn->peer[0], conn->peer[1], conn->peer[2], conn->peer[3],conn->peer[4], conn->peer[5],
                        src[0], src[1], src[2], src[3],src[4], src[5]);
                memcpy(conn->eth_addr, src, sizeof(conn->eth_addr));

            }
            btpan_tap_send(btpan_cb.tap_fd, src, dst, protocol, (char*)(p_buf + 1) + p_buf->offset, p_buf->len, ext, forward);
            GKI_freebuf(p_buf);
        }

    } while (p_buf != NULL);

}

/*******************************************************************************
**
** Function         bta_pan_co_rx_path
**
** Description
**
**
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_rx_path(UINT16 handle, UINT8 app_id)
{


    UINT8           i;

    LOGD("bta_pan_co_rx_path not used");


}

/*******************************************************************************
**
** Function         bta_pan_co_tx_write
**
** Description      This function is called by PAN to send data to the phone
**                  when the TX path is configured to use a push interface.
**                  The implementation of this function must copy the data to
**                  the phone's memory.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_tx_write(UINT16 handle, UINT8 app_id, BD_ADDR src, BD_ADDR dst, UINT16 protocol, UINT8 *p_data,
                                UINT16 len, BOOLEAN ext, BOOLEAN forward)
{
     LOGD("bta_pan_co_tx_write not used");

}

/*******************************************************************************
**
** Function         bta_pan_co_tx_writebuf
**
** Description      This function is called by PAN to send data to the phone
**                  when the TX path is configured to use a push interface with
**                  zero copy.  The phone must free the buffer using function
**                  GKI_freebuf() when it is through processing the buffer.
**
**
** Returns          TRUE if flow enabled
**
*******************************************************************************/
void  bta_pan_co_tx_writebuf(UINT16 handle, UINT8 app_id, BD_ADDR src, BD_ADDR dst, UINT16 protocol, BT_HDR *p_buf,
                                   BOOLEAN ext, BOOLEAN forward)
{

    LOGD("bta_pan_co_tx_writebuf not used");


}

/*******************************************************************************
**
** Function         bta_pan_co_rx_flow
**
** Description      This function is called by PAN to enable or disable
**                  data flow on the RX path when it is configured to use
**                  a push interface.  If data flow is disabled the phone must
**                  not call bta_pan_ci_rx_write() or bta_pan_ci_rx_writebuf()
**                  until data flow is enabled again.
**
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_rx_flow(UINT16 handle, UINT8 app_id, BOOLEAN enable)
{

    LOGD("bta_pan_co_rx_flow, enabled:%d, not used", enable);

}


/*******************************************************************************
**
** Function         bta_pan_co_filt_ind
**
** Description      protocol filter indication from peer device
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_pfilt_ind(UINT16 handle, BOOLEAN indication, tBTA_PAN_STATUS result,
                                    UINT16 len, UINT8 *p_filters)
{
    LOGD("bta_pan_co_pfilt_ind");

}
/*******************************************************************************
**
** Function         bta_pan_co_mfilt_ind
**
** Description      multicast filter indication from peer device
**
** Returns          void
**
*******************************************************************************/
void bta_pan_co_mfilt_ind(UINT16 handle, BOOLEAN indication, tBTA_PAN_STATUS result,
                                    UINT16 len, UINT8 *p_filters)
{

    LOGD("bta_pan_co_mfilt_ind");
}

