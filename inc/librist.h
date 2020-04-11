/* librist. Copyright 2019-2020 SipRadius LLC. All right reserved.
 * Author: Kuldeep Singh Dhaka <kuldeep@madresistor.com>
 * Author: Sergio Ammirata, Ph.D. <sergio@ammirata.net>
 */

#ifndef LIBRIST_H
#define LIBRIST_H

/* Track PROTOCOL and API changes */
#define RIST_PROTOCOL_VERSION (2)
#define RIST_API_VERSION (5)
#define RIST_SUBVERSION (1)
#define RIST_PEER_CONFIG_VERSION (0)

#include <stdint.h>
#include <stdlib.h>

/* __BEGIN_DECLS should be used at the beginning of your declarations,
   so that C++ compilers don't mangle their names.  Use __END_DECLS at
   the end of C declarations. */
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS /* empty */
# define __END_DECLS /* empty */
#endif

/* Reference: http://gcc.gnu.org/wiki/Visibility */
#if defined(_WIN32) || defined(__CYGWIN__)
# if defined(rist_EXPORTS)
#  if defined(__GNUC__)
#   define RIST_API __attribute__ ((dllexport))
#  else /* defined(__GNUC__) */
			/* Note: actually gcc seems to also supports this syntax. */
#   define RIST_API __declspec(dllexport)
#  endif /* defined(__GNUC__) */
# else /* defined(rist_EXPORTS) */
#  if defined(__GNUC__)
#   define RIST_API __attribute__ ((dllimport))
#  else
			/* Note: actually gcc seems to also supports this syntax. */
#   define RIST_API __declspec(dllimport)
#  endif
# endif /* defined(rist_EXPORTS) */
#else /* defined(_WIN32) || defined(__CYGWIN__) */
	#if __GNUC__ >= 4
		#define RIST_API __attribute__ ((visibility ("default")))
# else /* __GNUC__ >= 4 */
		#define RIST_API
# endif /* __GNUC__ >= 4 */
#endif /* defined(_WIN32) || defined(__CYGWIN__) */

__BEGIN_DECLS

enum rist_nack_type {
	RIST_NACK_RANGE = 0,
	RIST_NACK_BITMASK = 1,
};

enum rist_profile {
	RIST_PROFILE_SIMPLE = 0,
	RIST_PROFILE_MAIN = 1,
	RIST_PROFILE_ADVANCED = 2,
};

enum rist_log_level {
	RIST_LOG_QUIET = -1,
	RIST_LOG_INFO = 0,
	RIST_LOG_ERROR = 1,
	RIST_LOG_WARN = 2,
	RIST_LOG_DEBUG = 3,
	RIST_LOG_SIMULATE = 4,
};

enum rist_recovery_mode {
	RIST_RECOVERY_MODE_UNCONFIGURED = 0,
	RIST_RECOVERY_MODE_DISABLED = 1,
	RIST_RECOVERY_MODE_BYTES = 2,
	RIST_RECOVERY_MODE_TIME = 3,
};

enum rist_buffer_bloat_mode {
	RIST_BUFFER_BLOAT_MODE_OFF = 0,
	RIST_BUFFER_BLOAT_MODE_NORMAL = 1,
	RIST_BUFFER_BLOAT_MODE_AGGRESSIVE = 2
};

struct rist_receiver;
struct rist_sender;
struct rist_peer;

struct rist_data_block {
	const void *payload;
	size_t payload_len;
	uint64_t ts_ntp;
	/* These virtual ports are not used for simple profile */
	uint16_t virt_src_port;
	uint16_t virt_dst_port;
	/* These next fields are not needed/used by rist_sender_data_write */
	struct rist_peer *peer;
	uint32_t flow_id;
	uint64_t seq;
	uint32_t flags;
};

struct rist_oob_block {
	struct rist_peer *peer;
	const void *payload;
	size_t payload_len;
	uint64_t ts_ntp;
};

struct rist_peer_config {
	int version;
	const char *address;
	/* The gre_dst_port is not used for simple profile */
	uint16_t gre_dst_port;

	/* Recovery options */
	enum rist_recovery_mode recovery_mode;
	uint32_t recovery_maxbitrate;
	uint32_t recovery_maxbitrate_return;
	uint32_t recovery_length_min;
	uint32_t recovery_length_max;
	uint32_t recovery_reorder_buffer;
	uint32_t recovery_rtt_min;
	uint32_t recovery_rtt_max;
	uint32_t weight;

	enum rist_buffer_bloat_mode buffer_bloat_mode;
	uint32_t buffer_bloat_limit;
	uint32_t buffer_bloat_hard_limit;
};

/**
 * @brief Create Sender
 *
 * Create a RIST sender instance
 *
 * @param[out] ctx a context representing the sender instance
 * @param profile RIST profile
 * @param flow_id Flow ID, use 0 to delegate creation of flow_id to lib
 * @param log_level Level of log messages to display
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_sender_create(struct rist_sender **ctx, enum rist_profile profile,
				uint32_t flow_id, enum rist_log_level log_level);

 /**
 * @brief Assign dynamic authentiation handler
 *
 * Whenever a new peer is connected, @a connect_cb is called.
 * Whenever a new peer is disconnected, @a disconn_cb is called.
 *
 * @param ctx RIST sender context
 * @param connect_cb A pointer to the function that will be called when a new peer
 * connects. Return 1 or 0 to authorize or decline (NULL function pointer is valid)
 * @param disconn_cb A pointer to the function that will be called when a new peer
 * is marked as dead (NULL function pointer is valid)
 * @param arg is an the extra argument passed to the `conn_cb` and `disconn_cb`
 */
RIST_API int rist_sender_auth_handler_set(struct rist_sender *ctx,
		int (*connect_cb)(void *arg, const char* conn_ip, uint16_t conn_port, const char* local_ip, uint16_t local_port, struct rist_peer *peer),
		void (*disconn_cb)(void *arg, struct rist_peer *peer),
		void *arg);

/**
 * @brief Configure the SDES CName
 *
 * This allows you to override the auto-generated SDES CName
 *
 * @param ctx RIST sender context
 * @param cname cstring to be sent through librist (null terminated, 128 bytes max)
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_cname_set(struct rist_sender *ctx, const char *cname);

/**
 * @brief Add a peer connector to the existing sender.
 *
 * One sender can send data to multiple peers.
 *
 * @param ctx RIST sender context
 * @param[out] peer Store the new peer pointer
 * @param config a pointer to the struct rist_peer_config, which contains
 *        the configuration parameters for the peer endpoint.
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_sender_peer_create(struct rist_sender *ctx,
		struct rist_peer **peer, const struct rist_peer_config *config);

/**
 * @brief Remove a peer connector to the existing sender.
 *
 * @param ctx RIST sender context
 * @param peer a pointer to the struct rist_peer, which
 *        points to the peer endpoint.
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_sender_peer_destroy(struct rist_sender *ctx,
		struct rist_peer *peer);

/**
 * @brief Enable encryption
 *
 * Call after sender initialization to enable encryption.
 *
 * @param ctx RIST sender context
 * @param secret Pre-shared passphrase
 * @param key_size size of the key used for the encryption (128 or 256)
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_encrypt_aes_set(struct rist_sender *ctx,
		const char *secret, int key_size);

/**
 * @brief Set RIST retry timeout
 *
 * Set time interleaving retries during the protocol handshake
 *
 * @param ctx RIST sender context
 * @param t timeout in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_session_timeout_set(struct rist_sender *ctx, int t);

/**
 * @brief Set RIST keep-alive timeout
 *
 * Set keep-alive timeout
 *
 * @param ctx RIST sender context
 * @param t timeout in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_keepalive_timeout_set(struct rist_sender *ctx, int t);

/**
 * @brief Set RIST max jitter
 *
 * Set max jitter
 *
 * @param ctx RIST sender context
 * @param t max jitter in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_jitter_max_set(struct rist_sender *ctx, int t);

/**
 * @brief Enable out-of-band data channel
 *
 * Call after receiver initialization to enable out-of-band data.
 *
 * @param ctx RIST sender context
 * @param oob_callback A pointer to the function that will be called when out-of-band data
 * comes in (NULL function pointer is valid)
 * @param arg is an the extra argument passed to the `oob_callback`
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_oob_set(struct rist_sender *ctx, 
		void (*oob_callback)(void *arg, const struct rist_oob_block *oob_block),
		void *arg);

/**
 * @brief Enable compression
 *
 * Call after sender initialization to enable compression.
 *
 * @param ctx RIST sender context
 * @param compression, 0 to disable, 1-10 for compression level
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_compression_lz4_set(struct rist_sender *ctx, int compression);

/**
 * @brief Kickstart a pre-configured sender
 *
 * After all the peers have been added, this function triggers
 * the sender to start
 *
 * @param ctx RIST sender context
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_sender_start(struct rist_sender *ctx);

/**
 * @brief Write data directly to a remote receiver peer.
 *
 * This API is used to transmit out-of-band data to a remote receiver peer
 *
 * @param ctx RIST sender context
 * @param oob_block a pointer to the struct rist_oob_block
 * @return number of written bytes on success, -1 in case of error.
 */
RIST_API int rist_sender_oob_write(struct rist_sender *ctx, const struct rist_oob_block *oob_block);

/**
 * @brief Reads out-of-band data
 *
 * Use this API to read out-of-band data from an internal fifo queue instead of the callback
 *
 * @param ctx RIST sender context
 * @param[out] oob_block pointer to the rist_oob_block structure
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_sender_oob_read(struct rist_sender *ctx, const struct rist_oob_block **oob_block);

/**
 * @brief Write data into a librist packet.
 *
 * One sender can send write data into a librist packet.
 *
 * @param ctx RIST sender context
 * @param data_block pointer to the rist_data_block structure
 * the ts_ntp will be populated by the lib if a value of 0 is passed
 * @return number of written bytes on success, -1 in case of error.
 */
RIST_API int rist_sender_data_write(struct rist_sender *ctx, const struct rist_data_block *data_block);

/**
 * @brief Destroy RIST sender
 *
 * Destroy the RIST instance
 *
 * @param ctx RIST sender context
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_destroy(struct rist_sender *ctx);

/**
 * @brief Retrieve the current flow_id value
 *
 * Retrieve the current flow_id value
 *
 * @param ctx RIST sender context
 * @param flow_id pointer to your flow_id variable
 * @return 0 on success, -1 on error
 */
RIST_API int rist_sender_flow_id_get(struct rist_sender *ctx, uint32_t *flow_id);

/**
 * Create a RIST receiver instance
 *
 * @param[out] ctx a context representing the receiver instance
 * @param profile RIST profile
 * @param loglevel Level of log messages to display
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_create(struct rist_receiver **ctx, enum rist_profile profile,
			enum rist_log_level log_level);

/**
 * @brief Assign dynamic authentiation handler
 *
 * Whenever a new peer is connected, @a conn_cb is called.
 * Whenever a new peer is disconnected, @a disconn_cb is called.
 *
 * @param ctx RIST sender context
 * @param connect_cb A pointer to the function that will be called when a new peer
 * connects. Return 1 or 0 to authorize or decline (NULL function pointer is valid)
 * @param disconn_cb A pointer to the function that will be called when a new peer
 * is marked as dead (NULL function pointer is valid)
 * @param arg is an the extra argument passed to the `conn_cb` and `disconn_cb`
 */
RIST_API int rist_receiver_auth_handler_set(struct rist_receiver *ctx,
		int (*connect_cb)(void *arg, const char* conn_ip, uint16_t conn_port, const char* local_ip, uint16_t local_port, struct rist_peer *peer),
		void (*disconn_cb)(void *arg, struct rist_peer *peer),
		void *arg);

/**
 * @brief Configure the SDES CName
 *
 * This allows you to override the auto-generated SDES CName
 *
 * @param ctx RIST receiver context
 * @param cname cstring to be sent through librist (128 bytes max including null terminator)
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_cname_set(struct rist_receiver *ctx, const char *cname);

/**
 * @brief Add a peer connector to the existing receiver.
 *
 * One receiver can receive data from multiple peers.
 *
 * @param ctx RIST sender context
 * @param[out] peer Store the new peer pointer
 * @param config a pointer to the struct rist_peer_config, which contains
 *        the configuration parameters for the peer endpoint.
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_peer_create(struct rist_receiver *ctx, 
		struct rist_peer **peer, const struct rist_peer_config *config);

/**
 * @brief Remove a peer connector to the existing receiver.
 *
 * @param ctx RIST receiver context
 * @param peer a pointer to the struct rist_peer, which
 *        points to the peer endpoint.
 * @return 0 on success, -1 in case of error.
 */
RIST_API int rist_receiver_peer_destroy(struct rist_receiver *ctx,
		struct rist_peer *peer);

/**
 * @brief Enable encryption
 *
 * Call after receiver initialization to enable encryption.
 *
 * @param ctx RIST receiver context
 * @param secret Pre-shared passphrase
 * @param key_size size of the key used for the encryption (128 or 256)
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_encrypt_aes_set(struct rist_receiver *ctx,
		const char *secret, int key_size);

/**
 * @brief Set RIST retry timeout
 *
 * Set time interleaving retries during the protocol handshake
 *
 * @param ctx RIST receiver context
 * @param t timeout in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_session_timeout_set(struct rist_receiver *ctx, int t);

/**
 * @brief Set RIST keep-alive timeout
 *
 * Set keep-alive timeout
 *
 * @param ctx RIST receiver context
 * @param t timeout in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_keepalive_timeout_set(struct rist_receiver *ctx, int t);

/**
 * @brief Set RIST max jitter
 *
 * Set max jittter
 *
 * @param ctx RIST receiver context
 * @param t max jitter in ms
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_max_jitter_set(struct rist_receiver *ctx, int t);

/**
 * @brief Enable out-of-band data channel
 *
 * Call after receiver initialization to enable out-of-band data.
 *
 * @param ctx RIST receiver context
 * @param oob_callback A pointer to the function that will be called when out-of-band data
 * comes in (NULL function pointer is valid)
 * @param arg is an the extra argument passed to the `oob_callback`
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_oob_set(struct rist_receiver *ctx, 
		void (*oob_callback)(void *arg, const struct rist_oob_block *oob_block),
		void *arg);

/**
 * @brief Configure nack type
 *
 * Choose the nack type used by the receiver.
 *
 * @param ctx RIST receiver context
 * @param nack_type 0 for range (default), 1 for bitmask
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_nack_type_set(struct rist_receiver *ctx, enum rist_nack_type nacks_type);

/**
 * @brief Enable data callback channel
 *
 * Call to enable data callback channel.
 *
 * @param ctx RIST receiver context
 * @param data_callback The function that will be called when a data frame is
 * received from a sender.
 * @param arg the extra argument passed to the `data_callback`
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_data_callback_set(struct rist_receiver *ctx,
	void (*data_callback)(void *arg, const struct rist_data_block *data_block),
	void *arg);

/**
 * @brief Setup receiver start
 *
 * Start receiver data output thread.
 *
 * @param ctx RIST receiver context
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_start(struct rist_receiver *ctx);

/**
 * @brief Write data directly to a remote sender peer.
 *
 * This API is used to transmit out-of-band data to a remote sender peer
 *
 * @param ctx RIST receiver context
 * @param oob_block a pointer to the struct rist_oob_block
 * @return number of written bytes on success, -1 on error
 */
RIST_API int rist_receiver_oob_write(struct rist_receiver *ctx, const struct rist_oob_block *oob_block);

/**
 * @brief Reads out-of-band data
 *
 * Use this API to read out-of-band data from an internal fifo queue instead of the callback
 *
 * @param ctx RIST receiver context
 * @param[out] oob_block a pointer to the rist_oob_block structure
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_oob_read(struct rist_receiver *ctx, const struct rist_oob_block **oob_block);

/**
 * @brief Reads rist data
 *
 * Use this API to read data from an internal fifo queue instead of the callback
 *
 * @param ctx RIST receiver context
 * @param[out] data_block a pointer to the rist_data_block structure
 * @param timeout How long to wait for queue data (ms), 0 for no wait
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_data_read(struct rist_receiver *ctx, const struct rist_data_block **data_block, int timeout);

/**
 * @brief Destroy RIST receiver
 *
 * Destroy RIST receiver instance
 *
 * @param ctx RIST receiver context
 * @return 0 on success, -1 on error
 */
RIST_API int rist_receiver_destroy(struct rist_receiver *ctx);

__END_DECLS

#endif
