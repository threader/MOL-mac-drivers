/*
	File:		dlpiether.h

	Contains:	Header file for the DLPI template
			 ** dlpiether.h 5.12, last change 30 Jan 1996

	Copyright:	� 1995, 1996 by Mentat Inc. and Apple Computer, Inc., all rights reserved.

*/

#ifndef __DLPIETHER__
#define __DLPIETHER__

#ifndef __OPENTPTMODULE__
#include <OpenTptModule.h>
#endif
#ifndef __OPENTPTDEVLINKS__
#include <OpenTptDevLinks.h>
#endif
#ifndef __DLPIUSER__
#include "dlpiuser.h"
#endif
#ifndef __DLPI__
#include <dlpi.h>
#endif

/* Address list structure */
struct dle_addr_s {
	struct dle_addr_s	* dlea_next;
	UInt8				dlea_addr[6];
};

typedef struct dle_addr_s	dle_addr_t;
/*
 * Board-specific routines.  This portion of the dle_t structure must be
 * initialized by the board-specific code before passing the combined
 * structure into dle_install.
 *
 * The dlehw_send_error routine needs to be supplied only if the driver is
 * prepared to send corrupted packets.  If this pointer is NULL in the
 * structure, then the common code will return EINVAL to all kOTSendErrorPacket
 * ioctls.
 *
 * dlehw_recv_error_flags should be initialized by the board code to the
 * errors which can be received by the underlying hardware, such as
 * DL_CRC_ERROR | DL_RUNT_ERROR.  If the board supports receiving corrupted
 * packets, then it should call dle_inbound_error for every bad packet
 * received after the dlehw_address_filter_reset routine has been called with
 * accept_errors > 0.
 */
typedef void	(*DLEHwStartProcPtr)(void *);
typedef void	(*DLEHwStopProcPtr)(void *);
typedef void	(*DLEHwAddressFilterResetProcPtr)
					(void * hw, dle_addr_t * addr_list, UInt32 addr_count,
					 UInt32 promisc_count, UInt32 multi_promisc_count,
					 UInt32 accept_broadcast, UInt32 accept_errors);
typedef int		(*DLEHWSendErrorProcPtr)(void*, mblk_t*, UInt32);
					 
struct dlehw_s {
	DLEHwStartProcPtr				dlehw_start;
	DLEHwStopProcPtr				dlehw_stop;
	DLEHwAddressFilterResetProcPtr	dlehw_address_filter_reset;
	DLEHWSendErrorProcPtr			dlehw_send_error;
	unsigned long					dlehw_recv_error_flags;
};

typedef struct dlehw_s	dlehw_t;
/* 
 * There is one dle_t structure allocated for each controlled device
 * being managed by the common Ethernet code in dlpiether.c.  The 
 * board-specific code should allocate its own control structure for
 * each board, with a dle_t structure at the top.  For example:
 *
 *	struct board_s {
 *		dle_t	board_dle;
 *		int		board_field1;
 *		void	* board_field2;
 *	};
 *
 * Typically, the board-specific code only needs to know about the dle_hw,
 * dle_addr, and dle_status fields of this internal structure.  The rest of
 * the fields are used by the common Ethernet code in dlpiether.c.
 */
struct dle_s {
	dlehw_t					dle_hw;							/* Board-specific routines */
	UInt8					dle_factory_addr[6];			/* Factory physical address */
	UInt8					dle_current_addr[6];			/* Current physical Ethernet address */
	volatile UInt32			dle_intr_active;
	dle_interface_status_t	dle_istatus;					/* Interface MIB statistics */
	dle_ethernet_status_t	dle_estatus;					/* Ethernet MIB statistics */
	void					* dle_sap_hash_tbl[64];			/* Hash table of sap binds */
	void					* dle_match_any;				/* List of promiscuous binds */
	void					* dle_match_any_multicast;		/* List of promiscuous multicast binds */
	void					* dle_match_matched;			/* List of binds for packets that have */
															/* matched at least one other binding (!) */
	void					* dle_match_any_802;			/* List of promiscuous 802 binds */
	void					* dle_bind_list;				/* Linked list of all binds */
	UInt32					dle_bound_count;				/* Number of binds */
	UInt32					dle_match_any_count;			/* Number of promiscuous binds */
	UInt32					dle_match_any_multicast_count;	/* Number in dle_match_any_multicast */
	void					* dle_hw_addr_list;				/* List of all physical and multicast addresses */
															/* that should be received by the hardware. */
	UInt32					dle_refcnt;
	UInt32					dle_xtra_hdr_len;				/* Any extra space the board code */
															/* needs at the top of outbound */
															/* M_DATA messages */
	UInt32					dle_min_sdu;					/* Minimum transmit size from clients */
	char					* dle_instance_head;
	UInt32					dle_reserved[6];				/* Set to 0 */
};

typedef struct dle_s	dle_t;
/*
 * DLPI client structure; one created for each open stream (held in q->q_ptr).
 * If a board driver needs more space than given here, it should declare its
 * own structure with this dcl_t as the first element.  The size of the new
 * structure should be passed to dle_open so that the proper amount of space
 * is allocated.
 */
struct dcl_s {
	UInt32		dcl_state;
	void		* dcl_hw;
	queue_t		* dcl_rq;
	UInt32		dcl_flags;
	UInt32		dcl_sap;
	UInt8		dcl_snap[5];
	char		dcl_pad[3];
	UInt32		dcl_mac_type;
	UInt32		dcl_framing_type;
	void		* dcl_addr_list;
	UInt32		dcl_truncation_length;
	UInt32		dcl_reserved[1];
};

typedef struct dcl_s	dcl_t;

/*
 * if dle_xtra_hdr_len != 0, then the following check needed for M_DATA
 * messages in write-side put procedure:
 *
 *	if (dcl->dcl_flags & F_DCL_M_PROTO_REQUESTED) {
 *		'raw mode' --> need to insert dle_xtra_hdr_len bytes
 *		in front of b_rptr (start of Ethernet frame).
 *		...
 *	} else {
 *		This is a Fast Path packet with dle_xtra_hdr_len bytes
 *		at b_rptr.
 *		...
 *	}
 */
#define	F_DCL_M_PROTO_REQUESTED	0x80

/* Conversion macros between the different co-allocated structures. */
#define	dcl_to_dle(dcl)	((dle_t *)(dcl)->dcl_hw)
#define	dle_to_hw(dle)	((void *)(dle))

/* Routines in the common code available to board-specific routines. */

#ifdef __cplusplus
extern "C" {
#endif

extern	int		dle_close(queue_t * q);
extern	void	dle_inbound(dle_t * dle, mblk_t * mp);
extern	void	dle_inbound_error(dle_t * dle, mblk_t * mp, UInt32 flags);
extern	void	dle_init(dle_t * dle, size_t xtra_hdr_len);
extern	int		dle_open(dle_t * dle, queue_t * q, dev_t * devp, int flag, int sflag, cred_t * credp, size_t dcl_len);
extern	void	dle_rsrv_ctl(queue_t * q, mblk_t * mp);
extern	void	dle_terminate(dle_t * dle);
extern	mblk_t	* dle_wput(queue_t * q, mblk_t * mp);
extern	mblk_t	* dle_wput_ud_error(mblk_t * mp, int dlpi_err, int unix_err);

#ifdef __cplusplus
}
#endif

#endif
