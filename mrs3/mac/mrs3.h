#ifndef __MRS3_H__
#define __MRS3_H__

// MRS3

#include <stdint.h>
#if !defined(WIN32)
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef MRS_UE4_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

#define MRS_PLACEMENT_NEW_RETURN(t,...)  {void *p=mrs_malloc(sizeof(t)); if(!p)return nullptr; else return new(p) t(__VA_ARGS__); }
#define MRS_PLACEMENT_DELETE(obj,t) { obj->~t(); mrs_free(obj); }


enum class MrsPollingType : int32_t {
  POLL=1,  // poll/WSAPoll
  EPOLL=2, // linux only
  KQUEUE=3 // apple only
};


typedef struct {
    uint32_t mruPeerNumNotUsed;
    uint32_t mruPeerNumConnecting;
    uint32_t mruPeerNumEstablished;
    uint32_t mruPeerNumClosing;
    uint32_t mruPeerNumCloseVerified;
    uint32_t mruPeerNumClosed;    
    uint64_t mruTotalDatagramsSent;
    uint64_t mruTotalDatagramsReceived;
    uint64_t mruTotalBytesReceived;
    uint64_t mruTotalBytesSent;
    uint64_t mruTotalInvalidCommandCount;
    uint64_t mruTotalRetransmitCount;
    uint64_t mruTotalMsgTrunc;
    uint32_t mruMaxRecvmmsgResult;
    uint32_t mruMaxSendmmsgResult;
    uint32_t mruMmsgBufferFullCount;
} MrsStats;

typedef int64_t MrsContextId; // 8bits
typedef int64_t MrsServerId; // 8bits (contextid<<8)|svid
typedef int64_t MrsConnectionId; // 48bits (contextid<<56)|(svid<<48)|connid






enum class MrsErrorCode : int32_t {
    NONE=0,

    INVALID_ARGUMENT=-1001,
    NO_CONNECTION_FOUND = -1002,
    NO_MEMORY = -1003,
    BUFFER_FULL = -1004,
    NO_RECORD_ENCODER = -1005,
    RECORD_TOO_LONG = -1006,
    INVALID_CONTEXT_ID=-1007,
    INVALID_SERVER_ID=-1008,
    INVALID_CONNECTION_ID=-1009,
    OPENSSL_ERROR=-1010,    
    PEM_TOO_LONG=-1011,
    PATH_TOO_LONG=-1012,
    CANT_OPEN_FILE=-1013,
    FILE_READ_ERROR=-1014,
    INTERNAL_ERROR=-1015,
    TLS_HANDSHAKE_NOT_FINISHED=-1016,
    DTLS_HANDSHAKE_NOT_FINISHED=-1017,
    MRU_NOT_INITIALIZED=-1018,
    CONNECT_FAILED=-1019,
    INVALID_ADDRESS=-1020,
    SOCKET_CREATE_ERROR=-1021,
    BIND_ERROR=-1022,
    TCP_LISTEN_ERROR=-1023,
    TOO_MANY_SERVER=-1024,
    INVALID_CONNECTION_TYPE=-1025,
    WS_HANDSHAKE_NOT_FINISHED=-1026,
    CANT_SEND_RAW_ON_WS=-1027,
    TCP_ACCEPT_ERROR=-1028,
    INVALID_RECORD_OPTION=-1029,    
    WINSOCK_INIT_FAILED=-1030,
    WINSOCK_VERSION_NOT_SUPPORTED=-1031,
    CONNECTION_TIMEOUT=-1032,
    INVALID_POLLING_TYPE=-1033,
    KEEPALIVE_TIMEOUT=-1034,
    CANT_SEND_TO_CLOSED_CONNECTION=-1035,
    RECVBUF_FULL=-1036,
    BROKEN_PIPE=-1037,
};

enum class MrsConnectionType : int32_t {
    TCP     = 1, 
    //    ENET    = 2, 
    WS      = 3,
    WSS     = 4,
    // 5 TCP_SSL
    MRU     = 6, 
};

enum class MrsOption : int32_t {
    ENET_MTU = 1000,
    ENET_PEER_LIMIT = 1001,
    MRU_MTU = 2001,    
    MRU_PEER_LIMIT = 2002,
    MRU_DEFAULT_SENDQ_MAX = 2003,
    MRU_DEFAULT_FAST_RETRANSMIT_COUNT = 2004,
    MRU_ENABLE_CRC32 = 2005,
};

enum class MrsTlsVerifyCertResult : int32_t {
    TRUSTED = 1,
    UNTRUSTED = 0,
};

// mrs_send_record optbits
const int8_t MRS_RECORD_OPTION_ENCRYPT = 0x01;
const int8_t MRS_RECORD_OPTION_UNRELIABLE = 0x02;
const int8_t MRS_RECORD_OPTION_INTERNAL = (int8_t)0x80;

typedef void (*MrsAcceptCallback)( MrsServerId svid, MrsConnectionId cl_connid);
typedef void (*MrsConnectCallback)( MrsConnectionId connid);
typedef void (*MrsKeyExchangeCallback)( MrsConnectionId connid);
typedef void (*MrsDisconnectCallback)( MrsConnectionId connid);
typedef void (*MrsErrorCallback)( MrsConnectionId connid, MrsErrorCode errcode );
typedef void (*MrsServerErrorCallback)( MrsServerId svid, MrsErrorCode errcode );
typedef void (*MrsReadRecordCallback)( MrsConnectionId connid, uint8_t optbits, uint16_t payload_type, const char* payload, uint32_t payload_len );
typedef void (*MrsReadRawCallback)( MrsConnectionId connid, const char* data, uint32_t data_len );
typedef void (*MrsReadWsRawCallback)( MrsConnectionId connid, const char* data, uint32_t data_len );
typedef void (*MrsTlsVerifyCertResultCallback)(MrsTlsVerifyCertResult result );


extern "C" {
    // 全体
    int32_t mrs_get_version();
    void mrs_enable_debug_allocation();
    void mrs_set_mem_funcs( void* (*m)(size_t), void (*f)(void*));
    void mrs_memleak_check();
    MrsErrorCode mrs_initialize();
    void mrs_update();
    void mrs_finalize();
    uint32_t mrs_get_connection_num();
    MrsContextId mrs_context_create(int32_t maxcliconn);
    MrsPollingType mrs_get_default_polling_type();
    MrsPollingType mrs_context_get_polling_type(MrsContextId ctxid);    
    MrsErrorCode mrs_context_set_polling_type(MrsContextId ctxid, MrsPollingType poll_type);
    void mrs_context_update(MrsContextId ctxid);
    void mrs_context_delete(MrsContextId ctxid);
    MrsErrorCode mrs_context_get_statistics( MrsContextId ctxid, MrsStats *output);    
    MrsErrorCode mrs_get_last_error();
    const char* mrs_get_error_string( MrsErrorCode errorcode);
    void mrs_sleep( uint32_t sleep_msec );
    void mrs_usleep( uint32_t sleep_usec );
    uint64_t mrs_now_msec();    
    void mrs_print( const char *fmt, ... );
    void *mrs_malloc(size_t sz) ;
    void mrs_free(void*ptr);
    MrsErrorCode mrs_set_ssl_certificate_chain_file( const char* pempath );
    MrsErrorCode mrs_set_ssl_private_key_file( const char* pempath );
    MrsErrorCode mrs_set_ssl_root_ca_file(const char *pempath);
    void mrs_set_tls_verify_cert_result_callback( MrsTlsVerifyCertResultCallback callback );
    MrsErrorCode mrs_context_set_mru_loss_rate(MrsContextId ctxid, float rate);
    
    // サーバー
    MrsServerId mrs_server_create( MrsContextId ctxid, MrsConnectionType type, const char* addr, uint16_t port, uint32_t maxconn );
    MrsErrorCode mrs_server_close( MrsServerId svid);
    uint32_t mrs_server_get_connection_num( MrsServerId server_id );
    int32_t mrs_server_get_all_connections( MrsServerId server_id, MrsConnectionId *outarray, int32_t outmax );    
    void mrs_server_set_transport_accept_callback( MrsServerId svid, MrsAcceptCallback cbfunc ) ;    
    void mrs_server_set_accept_callback( MrsServerId svid, MrsAcceptCallback callback );
    void mrs_server_set_error_callback( MrsServerId svid, MrsServerErrorCallback callback );    
    MrsErrorCode mrs_server_set_data( MrsServerId svid, void* dataptr );
    void* mrs_server_get_data( MrsServerId svid);
    MrsErrorCode mrs_server_set_ws_path( MrsServerId svid, const char* value );
    
    MrsErrorCode mrs_server_enable_encryption(MrsConnectionId connid);
    bool mrs_server_is_valid(MrsServerId svid);
    // クライアント
    MrsConnectionId mrs_connect( MrsContextId ctxid, MrsConnectionType type, const char* addr, uint16_t port, uint32_t timeout_msec );
    MrsErrorCode mrs_connection_enable_encryption(MrsConnectionId connid);
    void mrs_set_connect_callback( MrsConnectionId connid, MrsConnectCallback callback );
    void mrs_set_transport_connect_callback( MrsConnectionId connid, MrsConnectCallback callback );

    // sv/cl共通
    void mrs_set_disconnect_callback( MrsConnectionId connid, MrsDisconnectCallback callback );
    void mrs_set_error_callback( MrsConnectionId connid, MrsErrorCallback callback );
    void mrs_set_keyex_callback( MrsConnectionId connid, MrsKeyExchangeCallback callback );
    void mrs_set_read_record_callback( MrsConnectionId connid, MrsReadRecordCallback callback );
    void mrs_set_read_raw_callback( MrsConnectionId connid, MrsReadRawCallback callback );
    void mrs_set_read_ws_raw_callback( MrsConnectionId connid, MrsReadWsRawCallback callback);
    MrsErrorCode mrs_connection_set_data( MrsConnectionId connid, void* dataptr );
    void* mrs_connection_get_data( MrsConnectionId connid );
    int32_t mrs_connection_is_connected( MrsConnectionId connid );
    int32_t mrs_connection_set_recvbuf_max_size( MrsConnectionId connid, uint32_t size );
    int32_t mrs_connection_get_recvbuf_max_size( MrsConnectionId connid );
    int32_t mrs_connection_set_senddbuf_max_size( MrsConnectionId connid, uint32_t size );
    int32_t mrs_connection_get_sendbuf_max_size( MrsConnectionId connid );
    int32_t mrs_connection_get_recvbuf_used( MrsConnectionId connid);
    bool mrs_connection_is_valid(MrsConnectionId connid);
    int32_t mrs_connection_get_last_rtt(MrsConnectionId connid);

    MrsConnectionType mrs_connection_get_type( MrsConnectionId connid );
    const char* mrs_connection_get_ws_path( MrsConnectionId connid);

    uint32_t mrs_connection_get_remote_version( MrsConnectionId connid );
    
    int32_t mrs_send_record( MrsConnectionId connid, uint8_t optbits, uint16_t payload_type, const char* payload, uint32_t payload_len );
    int32_t mrs_send_raw( MrsConnectionId connid, const char* data, uint32_t data_len );
    int32_t mrs_send_ws_raw( MrsConnectionId connid, const char* data, uint32_t data_len );

    int32_t mrs_connection_is_writable( MrsConnectionId connid, uint32_t size );
    // 不要    MrsConnectionId mrs_connection_get_connection_id( MrsConnectionId connid );    

    MrsErrorCode mrs_connection_get_remote_address( MrsConnectionId connid, char *outaddr, uint32_t outaddrmax, uint16_t *outport);
    MrsErrorCode mrs_connection_close( MrsConnectionId connid );

    // オプション設定
    int32_t mrs_server_set_option( MrsServerId svid, MrsOption opt, int32_t value ); // mrs_set_keep_alive_update_msec etc..
    int32_t mrs_setver_get_option( MrsServerId svid, MrsOption opt );
    int32_t mrs_connection_set_option( MrsConnectionId connid, MrsOption opt, int32_t value ); // mrs_set_keep_alive_update_msec etc..
    int32_t mrs_connection_get_option( MrsConnectionId connid, MrsOption opt );    

    // debug
    void mrs_dumpbin(const char*s, uint32_t l) ;
    void mru_enable_debug_print_command_types();
    void mrs_debug_dump_peers(MrsContextId ctxid);
}

#ifdef MRS_UE4_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif


#endif
