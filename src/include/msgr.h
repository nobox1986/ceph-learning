#ifndef CEPH_MSGR_H
#define CEPH_MSGR_H

#ifndef __KERNEL__
#include <sys/socket.h> // for struct sockaddr_storage
#endif

#include "include/int_types.h"

/*
 * Data types for message passing layer used by Ceph.
 */

#define CEPH_MON_PORT    6789  /* default monitor port */

/*
 * client-side processes will try to bind to ports in this
 * range, simply for the benefit of tools like nmap or wireshark
 * that would like to identify the protocol.
 */
#define CEPH_PORT_FIRST  6789

/*
 * tcp connection banner.  include a protocol version. and adjust
 * whenever the wire protocol changes.  try to keep this string length
 * constant.
 */
#define CEPH_BANNER "ceph v027"

/*
 * Rollover-safe type and comparator for 32-bit sequence numbers.
 * Comparator returns -1, 0, or 1.
 */
typedef __u32 ceph_seq_t;

static inline __s32 ceph_seq_cmp(__u32 a, __u32 b)
{
       return (__s32)a - (__s32)b;
}


/*
 * entity_name -- logical name for a process participating in the
 * network, e.g. 'mds0' or 'osd3'.
 */
struct ceph_entity_name {
	__u8 type;      /* CEPH_ENTITY_TYPE_* */
	__le64 num;
} __attribute__ ((packed));

#define CEPH_ENTITY_TYPE_MON    0x01
#define CEPH_ENTITY_TYPE_MDS    0x02
#define CEPH_ENTITY_TYPE_OSD    0x04
#define CEPH_ENTITY_TYPE_CLIENT 0x08
#define CEPH_ENTITY_TYPE_MGR    0x10
#define CEPH_ENTITY_TYPE_AUTH   0x20

#define CEPH_ENTITY_TYPE_ANY    0xFF

extern const char *ceph_entity_type_name(int type);

/*
 * entity_addr -- network address
 */
struct ceph_entity_addr {
	__le32 type;
	__le32 nonce;  /* unique id for process (e.g. pid) */
	struct sockaddr_storage in_addr;
} __attribute__ ((packed));

struct ceph_entity_inst {
	struct ceph_entity_name name;
	struct ceph_entity_addr addr;
} __attribute__ ((packed));


/* used by message exchange protocol */
#define CEPH_MSGR_TAG_READY         1  /* server->client: ready for messages */
#define CEPH_MSGR_TAG_RESETSESSION  2  /* server->client: reset, try again */
#define CEPH_MSGR_TAG_WAIT          3  /* server->client: wait for racing
					  incoming connection */
#define CEPH_MSGR_TAG_RETRY_SESSION 4  /* server->client + cseq: try again
					  with higher cseq */
#define CEPH_MSGR_TAG_RETRY_GLOBAL  5  /* server->client + gseq: try again
					  with higher gseq */
#define CEPH_MSGR_TAG_CLOSE         6  /* closing pipe */
#define CEPH_MSGR_TAG_MSG           7  /* message */
#define CEPH_MSGR_TAG_ACK           8  /* message ack */
#define CEPH_MSGR_TAG_KEEPALIVE     9  /* just a keepalive byte! */
#define CEPH_MSGR_TAG_BADPROTOVER  10  /* bad protocol version */
#define CEPH_MSGR_TAG_BADAUTHORIZER 11 /* bad authorizer */
#define CEPH_MSGR_TAG_FEATURES      12 /* insufficient features */
#define CEPH_MSGR_TAG_SEQ           13 /* 64-bit int follows with seen seq number */
#define CEPH_MSGR_TAG_KEEPALIVE2     14
#define CEPH_MSGR_TAG_KEEPALIVE2_ACK 15  /* keepalive reply */


/*
 * connection negotiation
 */
struct ceph_msg_connect {
	__le64 features;     /* supported feature bits */
	__le32 host_type;    /* CEPH_ENTITY_TYPE_* */
	__le32 global_seq;   /* count connections initiated by this host */
	__le32 connect_seq;  /* count connections initiated in this session */
	__le32 protocol_version;
	__le32 authorizer_protocol;
	__le32 authorizer_len;
	__u8  flags;         /* CEPH_MSG_CONNECT_* */
} __attribute__ ((packed));

struct ceph_msg_connect_reply {
	__u8 tag;
	__le64 features;     /* feature bits for this session */
	__le32 global_seq;
	__le32 connect_seq;
	__le32 protocol_version;
	__le32 authorizer_len;
	__u8 flags;
} __attribute__ ((packed));

#define CEPH_MSG_CONNECT_LOSSY  1  /* messages i send may be safely dropped */


/*
 * message header
 */
struct ceph_msg_header_old {
	__le64 seq;       /* message seq# for this session */
	__le64 tid;       /* transaction id */
	__le16 type;      /* message type */
	__le16 priority;  /* priority.  higher value == higher priority */
	__le16 version;   /* version of message encoding */

	__le32 front_len; /* bytes in main payload */
	__le32 middle_len;/* bytes in middle payload */
	__le32 data_len;  /* bytes of data payload */
	__le16 data_off;  /* sender: include full offset;
			     receiver: mask against ~PAGE_MASK */

	struct ceph_entity_inst src, orig_src;
	__le32 reserved;
	__le32 crc;       /* header crc32c */
} __attribute__ ((packed));

struct ceph_msg_header {
	__le64 seq;       /* message seq# for this session 当前session内消息的唯一序号*/
	__le64 tid;       /* transaction id 消息全局唯一的id*/
	__le16 type;      /* message type 消息类型*/
	__le16 priority;  /* priority.  higher value == higher priority 优先级*/
	__le16 version;   /* version of message encoding 消息编码的版本*/

	__le32 front_len; /* bytes in main payload --payload的长度*/
	__le32 middle_len;/* bytes in middle payload --middle的长度*/
	__le32 data_len;  /* bytes of data payload --data的长度*/
	__le16 data_off;  /* sender: include full offset; 对象的数据偏移量
			     receiver: mask against ~PAGE_MASK */

	struct ceph_entity_name src;  //消息源

	/* oldest code we think can decode this.  unknown if zero. 一些旧的代码，用于兼容，如果为零则忽略*/
	__le16 compat_version;
	__le16 reserved;
	__le32 crc;       /* header crc32c 消息头的crc校验信息*/
} __attribute__ ((packed));

#define CEPH_MSG_PRIO_LOW     64
#define CEPH_MSG_PRIO_DEFAULT 127
#define CEPH_MSG_PRIO_HIGH    196
#define CEPH_MSG_PRIO_HIGHEST 255

/*
 * follows data payload
 * ceph_msg_footer_old does not support digital signatures on messages PLR
 */

struct ceph_msg_footer_old {
	__le32 front_crc, middle_crc, data_crc;
	__u8 flags;
} __attribute__ ((packed));

struct ceph_msg_footer {
	__le32 front_crc, middle_crc, data_crc;
	// sig holds the 64 bits of the digital signature for the message PLR
	__le64  sig;
	__u8 flags;
} __attribute__ ((packed));

#define CEPH_MSG_FOOTER_COMPLETE  (1<<0)   /* msg wasn't aborted */
#define CEPH_MSG_FOOTER_NOCRC     (1<<1)   /* no data crc */
#define CEPH_MSG_FOOTER_SIGNED	  (1<<2)   /* msg was signed */


#endif
