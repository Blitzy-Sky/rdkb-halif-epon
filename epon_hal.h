/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

/**
 * @file epon_hal.h
 * @brief Interface definition for the RDK-B EPON HAL: lifecycle, status, statistics
 *        and maintenance for an Ethernet PON (EPON) Optical Network Unit (ONU).
 *
 * This header is the complete EPON HAL contract. It declares the status codes an
 * implementation may return, the configuration and reporting data types a caller
 * constructs or reads, the three callbacks an implementation invokes, and fifteen
 * functions in five groups:
 *
 * - **lifecycle** - epon_hal_init() and epon_hal_deinit();
 * - **version** - epon_hal_get_version();
 * - **information** - epon_hal_get_link_info(), epon_hal_get_llid_info(),
 *   epon_hal_get_olt_info(), epon_hal_get_manufacturer_info() and
 *   epon_hal_get_interface_list();
 * - **statistics** - epon_hal_get_link_stats(), epon_hal_get_transceiver_stats() and
 *   epon_hal_clear_stats();
 * - **maintenance** - epon_hal_reset_onu(), epon_hal_factory_reset(),
 *   epon_hal_set_oam_log_mask() and dpoe_hal_get_cpe_mac_table().
 *
 * The interface has an explicit lifecycle: epon_hal_init() must be called before any
 * other function, epon_hal_deinit() tears the interface down and requires
 * epon_hal_init() to be called again before further use, and epon_hal_factory_reset()
 * likewise ends the initialized lifetime. Every function except
 * epon_hal_get_version() reports its outcome synchronously through an
 * epon_hal_return_t status code; there is no error callback, error queue or other
 * out-of-band error channel, which the repository specification states under
 * `Internal Error Handling` (`docs/pages/halSpec.md`).
 *
 * The interface declares no handle, instance pointer or context argument, so
 * initialization state is a property of the process rather than of an object the
 * caller holds and one initialized instance per process is what the interface
 * supports. It declares no setter other than epon_hal_set_oam_log_mask(), no
 * separate callback registration function - the three callbacks are installed only
 * through epon_hal_init() - and no persistence entry point.
 *
 * @note Two of the declarations here are extensions rather than part of the base
 *       contract. dpoe_hal_get_cpe_mac_table() belongs to the DPoE (DOCSIS
 *       Provisioning of EPON) extension, which a build enables with
 *       `-DEPON_HAL_DPOE_SUPPORT` and whose availability a caller states through
 *       epon_hal_config_t::dpoe_supported. The HAL_LOG() front end and its
 *       replaceable HAL_LOG_FUNCTION() backend are a logging facility this interface
 *       ships for an implementer rather than functions a caller invokes.
 *
 * @warning This repository ships an interface header only: no implementation, build
 *          manifest or library declaration accompanies it, so nothing here names a
 *          delivered artefact to link against. EPON is also not available on either
 *          RDK-B reference platform, so no statement in this header is presented as
 *          observed runtime behaviour.
 *
 * Behaviour stated in this header is derived from these declarations and from the
 * repository specification (`docs/pages/halSpec.md`). Where neither establishes a
 * fact - whether a particular call may block, or which thread a callback arrives on -
 * that is stated as unspecified rather than filled in by inference from a signature.
 */

/**
 * @defgroup EPON_HAL EPON HAL
 *
 * The EPON HAL (Hardware Abstraction Layer) is used in RDK-B (Reference Design
 * Kit-Broadband) to provide an abstraction layer for an Ethernet PON (EPON) Optical
 * Network Unit (ONU) on the WAN side of the gateway: its registration lifecycle with
 * the Optical Line Terminal (OLT), the link, LLID, transceiver, OLT and manufacturer
 * information a caller reports, the counters it reads and clears, and the reset and
 * OAM logging controls it applies.
 *
 * @defgroup EPON_HAL_TYPES  EPON HAL Data Types
 * @ingroup  EPON_HAL
 *
 * @defgroup EPON_HAL_APIS   EPON HAL APIs
 * @ingroup  EPON_HAL
 *
 **/

#ifndef _EPON_HAL_H_
#define _EPON_HAL_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup EPON_HAL_TYPES
 * @{
 */

/**
 * @brief EPON HAL API version, as three components and the packed value built from
 *        them.
 *
 * The version this header declares is the version of the *interface*, fixed at
 * compile time from the three components below. Its semantics are:
 *
 * - a change of major version means an incompatible API or ABI change, so a caller
 *   compiled against one major version cannot be assumed to work against another;
 * - a change of minor version means a backwards-compatible addition;
 * - a change of patch version means a backwards-compatible fix.
 *
 * epon_hal_get_version() reports the version of the *implementation* in the same
 * packed encoding, so a caller compares the two at start-up to establish
 * compatibility rather than assuming it.
 *
 * @see epon_hal_get_version
 */
#define EPON_HAL_VERSION_MAJOR 1 /*!< Major component of the interface version this header declares. A caller compares it against the top byte of epon_hal_get_version() and must treat a mismatch as incompatible. */
#define EPON_HAL_VERSION_MINOR 0 /*!< Minor component of the interface version this header declares. An implementation reporting a higher minor version than the caller compiled against remains compatible; the reverse is not stated by this interface. */
#define EPON_HAL_VERSION_PATCH 0 /*!< Patch component of the interface version this header declares. It carries no compatibility consequence, so a caller does not gate on it. */
#define EPON_HAL_MAKE_VERSION(major, minor, patch) ((major << 24) | (minor << 16) | (patch)) /*!< Packs three version components into one 32-bit value, placing major in bits 31-24, minor in bits 23-16 and patch in bits 15-0. It is the encoding epon_hal_get_version() returns, so a caller uses this macro to build a value to compare against rather than composing the shifts itself. Note that the patch component occupies sixteen bits here, not eight. */
#define EPON_HAL_API_VERSION EPON_HAL_MAKE_VERSION(EPON_HAL_VERSION_MAJOR, EPON_HAL_VERSION_MINOR, EPON_HAL_VERSION_PATCH) /*!< The interface version this header declares, packed by EPON_HAL_MAKE_VERSION(). This is the value a caller compares against what epon_hal_get_version() returns. */

/**
 * @brief Buffer length constants.
 *
 * Each constant below is the declared size of a fixed array in one of the reporting
 * structures, in bytes for a character or octet array and in elements for
 * EPON_HAL_MAX_INTERFACES. A caller bounds every read by the constant that applies to
 * the field, and does not rely on a NUL terminator where this interface does not
 * state one: the repository specification's `Memory Model` topic directs a caller to
 * treat each fixed-size character buffer as zero-terminated and bounded by its
 * declared length, so the length is the limit even where a terminator is present
 * (`docs/pages/halSpec.md`).
 */
#define EPON_HAL_MAC_ADDR_LEN 6              /*!< Length in bytes of every MAC address array in this interface - epon_llid_info_t::local_mac_address, epon_olt_info_t::mac_address and dpoe_cpe_mac_entry_t::mac_address. Six octets, in transmission order; this interface states no text formatting for them. */
#define EPON_HAL_VENDOR_OUI_LEN 3            /*!< Length in bytes of a vendor OUI (Organizationally Unique Identifier) array - epon_onu_manufacturer_info_t::vendor_oui and epon_olt_info_t::vendor_oui. Three octets, being the first three of the corresponding MAC address space. */
#define EPON_HAL_MANUFACTURER_LEN 32         /*!< Size in bytes of epon_onu_manufacturer_info_t::manufacturer. The text is vendor-supplied, so a caller reads it as opaque rather than matching it against a fixed vocabulary. */
#define EPON_HAL_MODEL_NUMBER_LEN 16         /*!< Size in bytes of epon_onu_manufacturer_info_t::model_number. */
#define EPON_HAL_HW_VERSION_LEN 16           /*!< Size in bytes of epon_onu_manufacturer_info_t::hardware_version. The format is vendor-specific, so a caller compares it as text rather than parsing it as a version number. */
#define EPON_HAL_SW_VERSION_LEN 16           /*!< Size in bytes of epon_onu_manufacturer_info_t::software_version. The format is vendor-specific; it is unrelated to the packed value epon_hal_get_version() returns, which reports the HAL interface version rather than the ONU firmware version. */
#define EPON_HAL_SERIAL_NUMBER_LEN 32        /*!< Size in bytes of epon_onu_manufacturer_info_t::serial_number. */
#define EPON_HAL_MODE_LEN 16                 /*!< Size in bytes of epon_hal_link_info_t::mode, which carries the negotiated operational mode as text such as "1G-EPON" or "10G-EPON". This interface does not enumerate the permitted values, so a caller matches against the strings its vendor documents. */
#define EPON_HAL_MAX_INTERFACES 16           /*!< Element count of epon_interface_list_t::interface. It is a fixed capacity embedded in the caller's structure rather than an allocation, so there is nothing to free; the populated prefix is bounded by epon_interface_list_t::interface_count. */
#define EPON_HAL_INTERFACE_NAME_LEN 32       /*!< Size in bytes of epon_onu_interface_info_t::name, which carries a Linux network interface name such as "veip0". */
#define EPON_HAL_OLT_VENDOR_INFO_LEN 64      /*!< Size in bytes reserved for OLT vendor information. No field declared in this header uses it: epon_olt_info_t carries only the OLT MAC address and vendor OUI, so the constant is declared without a corresponding member and a caller has no field to apply it to. */

/**
 * @brief LLID constants.
 */
#define EPON_LLID_NOT_APPLICABLE 0xFFFF      /*!< Sentinel written to epon_alarm_info_t::llid when the alarm concerns the device as a whole rather than one Logical Link Identifier. A caller must test the LLID field against this value before treating it as an LLID, because 0xFFFF is otherwise a syntactically valid 16-bit LLID. */

/** @} */  //END OF GROUP EPON_HAL_TYPES

/**
 * @defgroup HAL_LOGGER Generic HAL Logging APIs
 * @ingroup EPON_HAL
 * @brief Generic logging macros for HAL modules - implementer defines the backend.
 *
 * This group is a facility for the *implementer* of the HAL rather than an interface a
 * caller invokes: nothing declared here is called by RDK-B middleware. It supplies a
 * severity ladder, one logging entry point, and a compile-time replaceable backend, so
 * that log records from different vendor implementations carry the same shape.
 *
 * The default backend appends to `/rdklogs/logs/EPONMANAGERLog.txt.0`. If an
 * implementer defines HAL_LOG_FUNCTION() before including this header and that
 * definition discards its input, no output is produced at all - which the repository
 * specification calls out under `Logging and debugging requirements` as the first
 * thing to check when log records are missing, ahead of the OAM log mask
 * (`docs/pages/halSpec.md`).
 *
 * @see epon_hal_set_oam_log_mask
 * @{
 */

/**
 * @brief Severity levels for HAL_LOG(), in descending order of severity.
 *
 * HAL_LOG_LEVEL_OAM is specific to this interface: it exists so that OAM protocol
 * message logging, which epon_hal_set_oam_log_mask() controls, can be filtered
 * separately from every other kind of record. It sits between
 * HAL_LOG_LEVEL_INFO and HAL_LOG_LEVEL_DEBUG in the enumeration, but a backend is
 * free to route it independently rather than treating the ordinal as a threshold.
 *
 * @note The ordinals are contiguous from zero, so a backend may use the value to index
 *       a mapping table onto its own severity vocabulary - which is what the
 *       HAL_LOG_FUNCTION() examples below do.
 *
 * @see HAL_LOG
 * @see epon_hal_set_oam_log_mask
 */
typedef enum {
    HAL_LOG_LEVEL_FATAL = 0,    /*!< Fatal error: a failure the implementation cannot continue past. The most severe level, and the zero ordinal. */
    HAL_LOG_LEVEL_ERROR,        /*!< Error condition: an operation failed. This is the level at which a status code returned to the caller should also be recorded. */
    HAL_LOG_LEVEL_WARN,         /*!< Warning condition: something unexpected that did not fail the operation. */
    HAL_LOG_LEVEL_NOTICE,       /*!< Normal but significant condition, such as a registration state change worth recording on a quiet system. */
    HAL_LOG_LEVEL_INFO,         /*!< Informational message describing ordinary progress. */
    HAL_LOG_LEVEL_OAM,          /*!< OAM protocol message record. This is the level at which the messages selected by epon_hal_set_oam_log_mask() are emitted, and it exists so a backend can filter OAM traffic separately from every other record. */
    HAL_LOG_LEVEL_DEBUG,        /*!< Debug-level message, for diagnosis rather than operation. */
    HAL_LOG_LEVEL_TRACE         /*!< Trace-level detailed message. The least severe level, and the highest ordinal. */
} hal_log_level_t;

/**
 * @brief Emits one log record at the given severity, tagged with the enclosing
 *        function name and line number.
 *
 * This is the single logging entry point an implementer uses. It forwards to
 * HAL_LOG_FUNCTION(), adding `__FUNCTION__` and `__LINE__` so that every record
 * carries its origin without the caller having to supply it. Because the origin is
 * captured by the preprocessor at the point of expansion, the macro must be invoked
 * directly at the site being reported rather than wrapped in a helper function, which
 * would record the helper instead.
 *
 * @param level  Severity of the record, as a hal_log_level_t value. Passing a value
 *               outside the enumeration is not rejected by this interface: the value is
 *               forwarded to the backend, which the default definition prints as a
 *               number and a mapping backend would use as a table index.
 * @param format printf-style format string. Must be a string literal for the compiler
 *               to check it against the arguments, and must not be caller-controlled
 *               text, since a `%` in the data would otherwise be read as a conversion.
 * @param ...    Arguments matching the conversions in @p format. Zero arguments is
 *               permitted - the `##__VA_ARGS__` form removes the preceding comma - so a
 *               record with no substitutions is written the same way.
 *
 * @note Whether a record is emitted at all depends entirely on HAL_LOG_FUNCTION().
 *       With the default definition each record is appended to
 *       `/rdklogs/logs/EPONMANAGERLog.txt.0`; a build that redefines the backend to
 *       discard its input produces nothing.
 * @warning The default backend opens, writes and closes the log file on every record
 *          and applies no locking, so an implementation that logs from more than one
 *          thread or process must supply its own backend if it needs interleaving
 *          guarantees. This interface states none.
 *
 * @see hal_log_level_t
 * @see HAL_LOG_FUNCTION
 *
 * Example usage:
 * @code
 * HAL_LOG(HAL_LOG_LEVEL_ERROR, "Failed to initialize: %s", error_msg);
 * HAL_LOG(HAL_LOG_LEVEL_INFO, "Device registered successfully");
 * HAL_LOG(HAL_LOG_LEVEL_DEBUG, "Counter value: %d", counter);
 * @endcode
 */
#define HAL_LOG(level, format, ...)   HAL_LOG_FUNCTION(level, __FUNCTION__, __LINE__, format, ##__VA_ARGS__) /*!< Logging entry point: forwards @p level, the enclosing function name, the line number and the formatted message to HAL_LOG_FUNCTION(). */

/**
 * @brief Replaceable logging backend that receives every HAL_LOG() record.
 *
 * The definition below is guarded by `#ifndef`, so an implementer selects a backend by
 * defining HAL_LOG_FUNCTION() before including this header; the guard then yields and
 * the implementer's definition is used. It takes the severity, the originating function
 * name and line number, the format string and its arguments, in that order, and any
 * replacement must accept the same five-part parameter list.
 *
 * The default definition appends one line to `/rdklogs/logs/EPONMANAGERLog.txt.0` in
 * the form `[level][function:line] message`, opening and closing the file per record.
 * The numeric severity is written rather than a name, so a reader maps it back through
 * hal_log_level_t.
 *
 * @param level  Severity of the record, as a hal_log_level_t value.
 * @param func   Name of the function the record originated in, supplied by HAL_LOG().
 * @param line   Line number the record originated at, supplied by HAL_LOG().
 * @param format printf-style format string.
 * @param ...    Arguments matching the conversions in @p format.
 *
 * @note The default definition ignores a failed `fopen`, so a record is silently
 *       dropped where the log directory is absent or not writable. It reports no error
 *       to its caller and has no return value to check.
 * @warning A replacement definition is compiled into the implementation, so the
 *          destination cannot be changed at run time through this interface.
 *
 * @see HAL_LOG
 *
 * Example for RDK Logger:
 * @code
 * #define HAL_LOG_FUNCTION(level, func, line, format, ...) \
 *     RDK_LOG(rdk_log_map[level], "LOG.RDK.HAL", "[%s:%d] " format, func, line, ##__VA_ARGS__)
 * @endcode
 *
 * Example for printf:
 * @code
 * #define HAL_LOG_FUNCTION(level, func, line, format, ...) \
 *     printf("[HAL][%s:%d] " format "\n", func, line, ##__VA_ARGS__)
 * @endcode
 *
 * Example for syslog:
 * @code
 * #define HAL_LOG_FUNCTION(level, func, line, format, ...) \
 *     syslog(syslog_map[level], "[%s:%d] " format, func, line, ##__VA_ARGS__)
 * @endcode
 */
#ifndef HAL_LOG_FUNCTION //default definition 
#define HAL_LOG_FUNCTION(level, func, line, format, ...) \
do { \
    FILE *fp = fopen("/rdklogs/logs/EPONMANAGERLog.txt.0", "a+"); \
    if (fp) { \
        fprintf(fp, "[%d][%s:%d] " format "\n", level, func, line, ##__VA_ARGS__); \
        fclose(fp); \
    } \
} while(0)
#endif

/** @} */ /* End of HAL_LOGGER group */

/**
 * @addtogroup EPON_HAL_TYPES
 * @{
 */

/**
 * @brief Status codes returned by the EPON HAL functions.
 *
 * These twelve codes are the complete vocabulary this interface defines. Success is
 * zero and every failure is negative, so a caller may test for failure with a sign
 * test, but should switch on the specific code wherever the recovery differs - and it
 * does differ: EPON_HAL_ERROR_INVALID_PARAM and EPON_HAL_ERROR_WRONG_PON_MODE are not
 * retryable, while EPON_HAL_ERROR_RESOURCE and EPON_HAL_ERROR_TIMEOUT are.
 *
 * Every function except epon_hal_get_version() reports its outcome synchronously
 * through a value of this type. There is no error callback, error queue or other
 * out-of-band error channel, which the repository specification states under
 * `Internal Error Handling` (`docs/pages/halSpec.md`), so a caller learns of a failure
 * at the point of the call.
 *
 * @note Which subset of these codes a given function may return is per-function and is
 *       documented on each declaration; this enumeration is the union, not a set any
 *       one function draws on in full.
 *
 * @see epon_hal_get_version
 */
typedef enum {
    EPON_HAL_SUCCESS = 0,                   /*!< The operation completed and any output structure the caller supplied has been populated. The only non-negative code. */
    EPON_HAL_ERROR_INVALID_PARAM = -1,      /*!< An argument was rejected: a NULL pointer, an unset or wrong `struct_size`, or a value outside the range the declaration states. Nothing is written to an output structure. The caller corrects the call site; retrying with the same argument cannot succeed. */
    EPON_HAL_ERROR_NOT_INITIALIZED = -2,    /*!< epon_hal_init() has not completed successfully, or epon_hal_deinit() has since torn the interface down. The caller initializes and retries. */
    EPON_HAL_ERROR_HW_FAILURE = -3,         /*!< A hardware operation failed. The caller logs with context and surfaces the fault; a retry may succeed where the cause was transient. */
    EPON_HAL_ERROR_NOT_SUPPORTED = -4,      /*!< The implementation does not offer this operation or this optional data on this platform. The caller treats the capability as absent and stops calling it rather than retrying. */
    EPON_HAL_ERROR_TIMEOUT = -5,            /*!< The operation did not complete within the implementation's internal bound. Only epon_hal_init() states a numeric bound, of three minutes. A retry is reasonable; a repeat suggests a hardware or provisioning fault. */
    EPON_HAL_ERROR_MEMORY = -6,             /*!< An allocation inside the implementation failed. The caller treats this as a system-level resource failure, releases what it can, and avoids a tight retry loop. */
    EPON_HAL_ERROR_RESOURCE = -7,           /*!< A resource other than memory was unavailable. The caller retries after a backoff, and distinguishes this from EPON_HAL_ERROR_MEMORY when logging. */
    EPON_HAL_ERROR_CALLBACK_REG = -8,       /*!< Installing the callbacks supplied in epon_hal_config_t failed, so initialization failed. There is no partially initialized state to unwind: the caller treats the interface as uninitialized. */
    EPON_HAL_ERROR_CONFIG = -9,             /*!< A configuration operation failed - documented by epon_hal_factory_reset() for the case where default configuration cannot be restored. The caller escalates; the device may need attention. */
    EPON_HAL_ERROR_WRONG_PON_MODE = -10,    /*!< The optical hardware is provisioned for a different PON technology, for example an ITU PON rather than EPON. This is a provisioning mismatch rather than a transient fault, so retrying cannot clear it and the deployment has to be corrected. */
    EPON_HAL_ERROR = -11                    /*!< A failure the implementation does not attribute to any of the codes above. No specific recovery is implied, so the caller logs with full context. */
} epon_hal_return_t;

/**
 * @brief Registration status of the ONU, as reported to
 *        epon_hal_config_t::status_callback.
 *
 * The four values describe how far the ONU has progressed towards being registered
 * with the OLT: no signal, signal present but not yet registered, registered, and
 * deregistered. LLID-0 is the control channel over which MPCP and OAM run, so its
 * state is what distinguishes the last two.
 *
 * @note This interface exposes status values only. It does not state which transitions
 *       between them are legal or in what order they occur, so a caller must not infer
 *       a state machine from the ordinals below. What it does state is a reporting
 *       constraint: EPON_ONU_STATUS_REGISTRATION is delivered only once every
 *       configured interface is up, which epon_hal_get_interface_list() documents.
 *
 * @see epon_hal_config_t
 * @see epon_hal_get_interface_list
 */
typedef enum {
    EPON_ONU_STATUS_LOS =0 ,                    /*!< Loss of signal: the physical layer is down or not detected, so no optical signal is present. The zero value. */
    EPON_ONU_STATUS_DOWNSTREAM_SIGNAL_DETECTED, /*!< A downstream optical signal is present, so the fibre and the OLT are reachable, but the ONU is not yet registered. Link, LLID and OLT information cannot be relied on in this state. */
    EPON_ONU_STATUS_REGISTRATION,               /*!< LLID-0 is online, including MPCP and OAM, so the ONU is registered with the OLT. This is the state in which OLT and LLID information becomes available, and it is reported only after every configured interface has come up. */
    EPON_ONU_STATUS_DEREGISTRATION,             /*!< LLID-0 is offline: the ONU is no longer registered. A caller should treat previously read link, LLID and OLT information as stale. */
} epon_onu_status_t;

/**
 * @brief Link state of one EPON network interface.
 *
 * The two values report whether a single interface - a `veip` instance created by the
 * OLT - is carrying traffic. They describe one interface rather than the ONU as a
 * whole, which epon_onu_status_t reports.
 *
 * @note EPON_ONU_INTF_STATUS_LINK_DOWN is the zero value here because no explicit
 *       ordinals are assigned, so a zero-initialised epon_onu_interface_info_t reads
 *       as link-down rather than as an unset field. A caller therefore relies on
 *       epon_interface_list_t::interface_count to tell populated entries from unset
 *       ones, not on the status value.
 *
 * @see epon_onu_interface_info_t
 * @see epon_onu_status_t
 */
typedef enum {
    EPON_ONU_INTF_STATUS_LINK_DOWN,             /*!< The interface is down and is not carrying traffic. The zero value, so it is also what an unset field reads as. */
    EPON_ONU_INTF_STATUS_LINK_UP,               /*!< The ONU is registered and this interface is up and carrying traffic. */
} epon_interface_link_status_t;

/**
 * @brief One EPON network interface, as a name and its link state.
 *
 * This is the unit epon_hal_get_interface_list() reports and the value
 * epon_hal_config_t::interface_status_callback delivers. The name is what identifies
 * the interface across both surfaces: the callback carries a single record rather than
 * an index, so a caller that manages several interfaces matches on the name field.
 *
 * @see epon_hal_get_interface_list
 * @see epon_interface_list_t
 */
typedef struct {
    char name[EPON_HAL_INTERFACE_NAME_LEN];       /*!< Linux network interface name, such as "veip0", in up to EPON_HAL_INTERFACE_NAME_LEN (32) bytes. This is the field a caller correlates on, because the interface-status callback identifies its subject only by name. */
    epon_interface_link_status_t status;          /*!< Link state of this interface. Since EPON_ONU_INTF_STATUS_LINK_DOWN is the zero value, an unset record is indistinguishable from a down interface by this field alone. @see epon_interface_link_status_t */
} epon_onu_interface_info_t;

/**
 * @brief Standard IEEE 802.3ah EPON alarm identifiers.
 *
 * These are the standard EPON alarms defined by IEEE 802.3ah. Each is delivered
 * through epon_hal_config_t::alarm_callback inside an epon_alarm_info_t whose
 * `alarm_type` is EPON_ALARM_TYPE_STANDARD; vendor-specific alarms are enumerated
 * separately by epon_vendor_alarm_t.
 *
 * The five errored-event members report that a *threshold* the implementation applies
 * has been crossed, not the underlying counter value. This interface does not declare
 * where those thresholds are set or read, so a caller cannot establish from here what
 * error rate a given alarm corresponds to.
 *
 * @note EPON_HAL_ALARM_MAX is a bound rather than an alarm. It is one past the last
 *       valid identifier, so it is what a caller compares against when validating a
 *       received value and never a value the callback delivers.
 *
 * @see epon_alarm_info_t
 * @see epon_alarm_type_t
 */
typedef enum {
    EPON_HAL_ALARM_LOFI = 0,                /*!< Loss of frame or loss of lock: the receiver cannot delineate frames. The zero value. */
    EPON_HAL_ALARM_ERROR_SYMBOL_PERIOD,     /*!< The errored-symbol-period threshold was exceeded. The threshold itself is not exposed by this interface. */
    EPON_HAL_ALARM_ERROR_FRAME,             /*!< The errored-frame threshold was exceeded. */
    EPON_HAL_ALARM_ERROR_FRAME_PERIOD,      /*!< The errored-frame-period threshold was exceeded. */
    EPON_HAL_ALARM_ERROR_FRAME_SECONDS,     /*!< The errored-frame-seconds threshold was exceeded. */
    EPON_HAL_ALARM_OAM_SESSION_LOST,        /*!< The OAM session with the OLT was lost. Data forwarding may continue, but OAM-derived information such as epon_hal_get_olt_info() reports can no longer be refreshed. */
    EPON_HAL_ALARM_EQUIPMENT_FAILURE,       /*!< The ONU reports an equipment or hardware failure. */
    EPON_HAL_ALARM_MAX                      /*!< Upper bound of the enumeration, one past the last valid alarm. It is a sentinel for validation, never an alarm the callback delivers. */
} epon_hal_alarm_t;

/**
 * @brief Vendor-specific EPON alarm identifiers (DPoE).
 *
 * These alarms are vendor-specific rather than standardised by IEEE 802.3ah, and are
 * delivered through epon_hal_config_t::alarm_callback inside an epon_alarm_info_t whose
 * `alarm_type` is EPON_ALARM_TYPE_VENDOR_SPECIFIC. Because the two enumerations both
 * start at zero, the `alarm_type` discriminator is what tells a caller which
 * enumeration to read a received identifier against.
 *
 * @note EPON_VENDOR_ALARM_DYING_GASP reports imminent loss of power. It is a
 *       notification about a power event, not a power-management control: this
 *       interface declares no function that configures or influences a power state, as
 *       the repository specification records under `Power Management Requirements`
 *       (`docs/pages/halSpec.md`). A caller may react to it, but has very little time
 *       in which to do so and this interface states no bound on that time.
 * @note EPON_VENDOR_ALARM_MAX is a bound rather than an alarm, one past the last valid
 *       identifier.
 *
 * @see epon_alarm_info_t
 * @see epon_alarm_type_t
 */
typedef enum {
    EPON_VENDOR_ALARM_LOS = 0,              /*!< Loss of optical signal detected. The zero value, and distinct from the standard EPON_HAL_ALARM_LOFI, which reports loss of frame delineation rather than loss of light. */
    EPON_VENDOR_ALARM_DYING_GASP,           /*!< Imminent power loss detected. A caller has only whatever time the hardware's residual power allows; this interface states no bound on it. */
    EPON_VENDOR_ALARM_POWER_LOW,            /*!< Received optical power fell below the implementation's lower threshold. The corresponding measured value and threshold are readable through epon_hal_get_transceiver_stats(). */
    EPON_VENDOR_ALARM_POWER_HIGH,           /*!< Received optical power rose above the implementation's upper threshold. */
    EPON_VENDOR_ALARM_TEMPERATURE,          /*!< The transceiver temperature threshold was exceeded. The measured temperature is readable through epon_hal_get_transceiver_stats(). */
    EPON_VENDOR_ALARM_FEC_THRESHOLD,        /*!< The threshold for uncorrectable FEC (Forward Error Correction) errors was exceeded. The counter itself is epon_hal_link_stats_t::fec_uncorrectable. */
    EPON_VENDOR_ALARM_LASER_BIAS_CURRENT,   /*!< The laser bias current is outside its permitted range. The measured value is epon_hal_transceiver_stats_t::bias_current. */
    EPON_VENDOR_ALARM_SUPPLY_VOLTAGE,       /*!< The transceiver supply voltage is outside its permitted range. The measured value is epon_hal_transceiver_stats_t::supply_voltage. */
    EPON_VENDOR_ALARM_MAX                   /*!< Upper bound of the enumeration, one past the last valid vendor alarm. It is a sentinel for validation, never an alarm the callback delivers. */
} epon_vendor_alarm_t;

/**
 * @brief Discriminator selecting which alarm enumeration an epon_alarm_info_t carries.
 *
 * This is the tag for the anonymous union in epon_alarm_info_t. It has to be read
 * first, because epon_hal_alarm_t and epon_vendor_alarm_t both begin at zero and
 * overlap across their whole range, so an identifier is meaningless without knowing
 * which enumeration it belongs to.
 *
 * @see epon_alarm_info_t
 */
typedef enum {
    EPON_ALARM_TYPE_STANDARD = 0,           /*!< The union's `standard_alarm` member is the valid one, and its value is an epon_hal_alarm_t. */
    EPON_ALARM_TYPE_VENDOR_SPECIFIC = 1     /*!< The union's `vendor_alarm` member is the valid one, and its value is an epon_vendor_alarm_t. */
} epon_alarm_type_t;

/**
 * @brief One alarm notification: which alarm, on which LLID, and whether it is raised
 *        or cleared.
 *
 * This is the record epon_hal_config_t::alarm_callback receives, and it carries both
 * standard and vendor-specific alarms in one shape. Reading it correctly is a
 * three-step sequence: read `alarm_type` to learn which union member is valid, read
 * that member for the alarm identifier, then read `is_active` to learn whether the
 * alarm is being raised or cleared. Reading the union without first reading
 * `alarm_type` yields a value that is syntactically valid in both enumerations and
 * means different things in each.
 *
 * @warning The implementation passes this structure by pointer, and this interface does
 *          not state whether that pointer remains valid after the callback returns. A
 *          callback that needs to retain any of this data must copy it rather than
 *          store the pointer, which the repository specification states under `Memory
 *          Model` (`docs/pages/halSpec.md`).
 *
 * @see epon_alarm_type_t
 * @see epon_hal_alarm_t
 * @see epon_vendor_alarm_t
 * @see epon_hal_config_t
 */
typedef struct {
    epon_alarm_type_t alarm_type;           /*!< Which enumeration the union member below is to be read against. This field is read first; the union is meaningless without it. @see epon_alarm_type_t */
    union {
        epon_hal_alarm_t standard_alarm;    /*!< The standard IEEE 802.3ah alarm identifier. Valid only when `alarm_type` is EPON_ALARM_TYPE_STANDARD. */
        epon_vendor_alarm_t vendor_alarm;   /*!< The vendor-specific alarm identifier. Valid only when `alarm_type` is EPON_ALARM_TYPE_VENDOR_SPECIFIC. */
    };                                      /*!< Anonymous union holding the alarm identifier in exactly one of two forms, selected by `alarm_type`. Both members are enumerations that begin at zero, so the two forms are indistinguishable by value alone. */
    uint16_t llid;                          /*!< Logical Link Identifier the alarm concerns, or EPON_LLID_NOT_APPLICABLE (0xFFFF) where the alarm concerns the device as a whole. A caller tests against that sentinel before treating the value as an LLID, since 0xFFFF is otherwise a valid 16-bit LLID. */
    bool is_active;                         /*!< True when the alarm is being raised, false when it is being cleared. The same alarm identifier is therefore delivered twice over its lifetime, and a caller that counts deliveries without reading this field will double-count. */
} epon_alarm_info_t;

/**
 * @brief OAM message-type flags selecting which messages are logged (IEEE 802.3ah).
 *
 * The members are bit flags, each occupying a distinct bit, so a caller combines the
 * types it wants with a bitwise OR and passes the result to
 * epon_hal_set_oam_log_mask(). A mask of zero disables OAM logging entirely and
 * EPON_OAM_ALL enables every type. A caller must not compare a mask for equality
 * against a single member.
 *
 * @note The mask controls *logging*, not *processing*: all OAM messages are always
 *       processed regardless of it, as epon_hal_set_oam_log_mask() documents. It cannot
 *       be used to suppress protocol handling.
 * @note Organization-specific OAM messages, which IEEE 802.3ah identifies as type
 *       0xFE, have no flag of their own. They are covered by EPON_OAM_VAR_REQUEST and
 *       EPON_OAM_VAR_RESPONSE, so a caller wanting them enables those two.
 * @note MPCP GATE and REPORT messages are not available for logging at all. They are
 *       handled in hardware and are not visible to software, so no flag here reaches
 *       them and EPON_OAM_ALL does not either.
 *
 * @warning EPON_OAM_ALL is 0xFFFFFFFF, which sets every bit including the twenty-five
 *          this enumeration does not define. It is the documented way to enable
 *          everything, but it is not a mask a caller can meaningfully compare a
 *          narrower selection against.
 *
 * @see epon_hal_set_oam_log_mask
 * @see HAL_LOG_LEVEL_OAM
 */
typedef enum {
    EPON_OAM_INFO           = (1 << 0),  /*!< OAM Information PDU, IEEE 802.3ah code 0x00: the periodic keep-alive that maintains the OAM session. It is the highest-rate message class here, so enabling it produces the most log volume. */
    EPON_OAM_EVENT          = (1 << 1),  /*!< OAM Event Notification, code 0x01: the message class through which the ONU reports link events to the OLT. */
    EPON_OAM_VAR_REQUEST    = (1 << 2),  /*!< OAM Variable Request, code 0x02. Also covers organization-specific requests, code 0xFE, which have no flag of their own. */
    EPON_OAM_VAR_RESPONSE   = (1 << 3),  /*!< OAM Variable Response, code 0x03. Also covers organization-specific responses, code 0xFE. */
    EPON_OAM_LOOPBACK       = (1 << 4),  /*!< OAM Loopback Control, code 0x04: the message class that places the ONU into or out of loopback for testing. */
    EPON_OAM_MPCP_REGISTER  = (1 << 5),  /*!< MPCP REGISTER message, exchanged during discovery and registration with the OLT. Unlike GATE and REPORT, this MPCP message class is visible to software and can be logged. */
    EPON_OAM_MPCP_REGISTER_ACK = (1 << 6), /*!< MPCP REGISTER_ACK message, the acknowledgement that completes registration. */
    EPON_OAM_ALL            = 0xFFFFFFFF /*!< Every bit set, which enables logging for all loggable OAM message types. It does not reach MPCP GATE and REPORT, which are handled in hardware and are not visible to software at all. */
} epon_oam_log_type_t;

/**
 * @brief EPON link counters, in the TR-181 `Device.Optical.Interface.{i}.Stats`
 *        vocabulary plus EPON-specific extensions.
 *
 * This is the record epon_hal_get_link_stats() populates and epon_hal_clear_stats()
 * resets. The first ten members carry the TR-181 names given against each field, so a
 * caller mapping this HAL onto a TR-181 data model has a one-to-one correspondence for
 * them; the remainder are EPON extensions with no TR-181 counterpart.
 *
 * Every counter is monotonically increasing from the last reset, so a caller derives a
 * rate by differencing two reads and dividing by the interval. Two reads separated by
 * an epon_hal_clear_stats() call cannot be differenced, and this interface offers no
 * way to detect that a reset happened between them.
 *
 * @note The caller allocates this structure and must set `struct_size` to
 *       `sizeof(epon_hal_link_stats_t)` before every call that takes it. This is the
 *       interface's version-negotiation mechanism, and omitting it is not benign:
 *       epon_hal_get_link_stats() returns EPON_HAL_ERROR_INVALID_PARAM for an invalid
 *       `struct_size`.
 * @note This interface does not state whether the counters are cleared by
 *       epon_hal_deinit(), epon_hal_reset_onu() or a reboot, so a caller must not
 *       assume they survive - or that they do not survive - any of those.
 *
 * @see epon_hal_get_link_stats
 * @see epon_hal_clear_stats
 */
typedef struct {
    uint32_t struct_size;            /*!< Set by the caller to `sizeof(epon_hal_link_stats_t)` before the call. It is the only member the caller writes; every other member is written by the implementation. An unset or wrong value yields EPON_HAL_ERROR_INVALID_PARAM. */
    uint64_t packets_sent;           /*!< Total packets transmitted since the last reset. TR-181 `PacketsSent`. */
    uint64_t packets_received;       /*!< Total packets received since the last reset. TR-181 `PacketsReceived`. */
    uint64_t bytes_sent;             /*!< Total bytes transmitted since the last reset. TR-181 `BytesSent`. This interface does not state whether Ethernet framing overhead is included, so a caller must not derive line rate from it without a vendor statement. */
    uint64_t bytes_received;         /*!< Total bytes received since the last reset. TR-181 `BytesReceived`. */
    uint64_t errors_sent;            /*!< Packets that could not be transmitted because of an error. TR-181 `ErrorsSent`. Counted separately from `discard_packets_sent`, which counts deliberate drops. */
    uint64_t errors_received;        /*!< Packets received with an error. TR-181 `ErrorsReceived`. */
    uint64_t discard_packets_sent;   /*!< Packets discarded before transmission for a reason other than error - buffer exhaustion, for instance. TR-181 `DiscardPacketsSent`. */
    uint64_t discard_packets_received; /*!< Packets discarded on reception for a reason other than error. TR-181 `DiscardPacketsReceived`. */
    uint32_t max_bit_rate;           /*!< Maximum bit rate of the link in Mbps. TR-181 `MaxBitRate`. This is a capability rather than a counter, so epon_hal_clear_stats() has nothing to reset in it; the 32-bit width is sufficient for both 1G-EPON and 10G-EPON. */
    uint64_t fec_corrected;          /*!< FEC (Forward Error Correction) corrected bits since the last reset. An EPON extension with no TR-181 counterpart. A rising value indicates a degrading but still usable optical path. */
    uint64_t fec_uncorrectable;      /*!< FEC codewords that could not be corrected since the last reset. An EPON extension. This is the counter behind EPON_VENDOR_ALARM_FEC_THRESHOLD, and it counts codewords where `fec_corrected` counts bits, so the two are not directly comparable. */
    uint64_t broadcast_packets_sent; /*!< Broadcast packets transmitted since the last reset. An EPON extension. Included in `packets_sent` rather than counted separately from it. */
    uint64_t broadcast_packets_received; /*!< Broadcast packets received since the last reset. An EPON extension. */
    uint64_t multicast_packets_sent; /*!< Multicast packets transmitted since the last reset. An EPON extension. */
    uint64_t multicast_packets_received; /*!< Multicast packets received since the last reset. An EPON extension. */
    uint64_t ranging_resyncs;        /*!< Times the ONU has had to re-run MPCP ranging since the last reset. An EPON extension. A non-zero and rising value indicates instability in the optical path or in the OLT relationship. */
    uint64_t mac_resets;             /*!< Times the EPON MAC has been reset since the last reset of the counters. An EPON extension. This counts MAC-level resets, which are not the same event as an epon_hal_reset_onu() call. */
} epon_hal_link_stats_t;

/**
 * @brief Optical transceiver measurements and the thresholds they are judged against,
 *        in the TR-181 `Device.Optical.Interface.{i}` vocabulary plus vendor
 *        extensions.
 *
 * This is the record epon_hal_get_transceiver_stats() populates. Unlike
 * epon_hal_link_stats_t it holds instantaneous measurements rather than counters, so
 * epon_hal_clear_stats() does not affect it and two reads may legitimately differ in
 * either direction.
 *
 * Six of the nine members are TR-181 fields and pair up as measurement and threshold:
 * `optical_signal_level` against `lower_optical_threshold` and
 * `upper_optical_threshold`, and `transmit_optical_level` against
 * `lower_transmit_power_threshold` and `upper_transmit_power_threshold`. That pairing is
 * what lets a caller judge a reading without hard-coding a limit. The remaining three
 * are vendor extensions and are reported without thresholds, so a caller has nothing in
 * this interface to compare them against.
 *
 * @note The caller allocates this structure and must set `struct_size` to
 *       `sizeof(epon_hal_transceiver_stats_t)` before the call, or
 *       epon_hal_get_transceiver_stats() returns EPON_HAL_ERROR_INVALID_PARAM.
 * @note Every member is a `float`. This interface states no precision, no resolution
 *       and no sentinel for an unavailable reading, so a caller cannot distinguish an
 *       unsupported measurement from a genuine zero by value; the function returns
 *       EPON_HAL_ERROR_NOT_SUPPORTED where the whole record is unavailable.
 *
 * @see epon_hal_get_transceiver_stats
 */
typedef struct {
    uint32_t struct_size;            /*!< Set by the caller to `sizeof(epon_hal_transceiver_stats_t)` before the call. It is the only member the caller writes. An unset or wrong value yields EPON_HAL_ERROR_INVALID_PARAM. */
    float transmit_optical_level;    /*!< Transmit optical power in dBm. TR-181 `TransmitOpticalLevel`. Judged against `lower_transmit_power_threshold` and `upper_transmit_power_threshold`. Negative values are normal for an ONU transmitter. */
    float optical_signal_level;      /*!< Received optical power in dBm. TR-181 `OpticalSignalLevel`. Judged against `lower_optical_threshold` and `upper_optical_threshold`, and the measurement behind EPON_VENDOR_ALARM_POWER_LOW and EPON_VENDOR_ALARM_POWER_HIGH. */
    float lower_optical_threshold;   /*!< Lower limit in dBm for `optical_signal_level`, below which the received signal is considered too weak. TR-181 `LowerOpticalThreshold`. Reported by the implementation; this interface declares no way for a caller to set it. */
    float upper_optical_threshold;   /*!< Upper limit in dBm for `optical_signal_level`, above which the received signal is considered too strong and may saturate the receiver. TR-181 `UpperOpticalThreshold`. */
    float lower_transmit_power_threshold; /*!< Lower limit in dBm for `transmit_optical_level`. TR-181 `LowerTransmitPowerThreshold`. */
    float upper_transmit_power_threshold; /*!< Upper limit in dBm for `transmit_optical_level`. TR-181 `UpperTransmitPowerThreshold`. */
    float bias_current;           /*!< Laser bias current in mA. A vendor extension with no TR-181 counterpart and no threshold field in this structure, though EPON_VENDOR_ALARM_LASER_BIAS_CURRENT reports that the implementation's own limit was crossed. A drifting value is a recognised precursor of transmitter failure. */
    float temperature;             /*!< Transceiver module temperature in degrees Celsius. A vendor extension, reported without a threshold field; EPON_VENDOR_ALARM_TEMPERATURE reports a crossing of the implementation's limit. */
    float supply_voltage;          /*!< Transceiver supply voltage in volts. A vendor extension, reported without a threshold field; EPON_VENDOR_ALARM_SUPPLY_VOLTAGE reports a crossing of the implementation's limit. */
} epon_hal_transceiver_stats_t;


/**
 * @brief Emulation mode of one Logical Link Identifier (LLID).
 *
 * An EPON ONU carries several LLIDs over one fibre, and the mode says which emulation a
 * given LLID provides: a point-to-point link to the OLT, or the shared medium used for
 * broadcast and multicast traffic.
 *
 * @note This interface reports the mode and declares no function that sets it: the
 *       OLT assigns LLIDs during registration, so a caller reads this field rather than
 *       choosing it.
 *
 * @see epon_llid_info_t
 */
typedef enum {
    EPON_LLID_MODE_UNICAST = 0, /*!< Point-to-point emulation: the LLID carries unicast traffic between the OLT and this ONU alone. The zero value, and the mode of the great majority of LLIDs. */
    EPON_LLID_MODE_BROADCAST = 1 /*!< Shared-medium emulation: the LLID carries broadcast traffic to every ONU on the PON, or multicast traffic to a group. A caller must not assume traffic on such an LLID was addressed to this ONU alone. */
} epon_llid_mode_t;

/**
 * @brief Registration state of one Logical Link Identifier.
 *
 * The five values track an individual LLID through assignment by the OLT and release
 * again, which is a finer view than the device-wide epon_onu_status_t: LLIDs are
 * assigned individually, so two LLIDs on the same ONU may legitimately report different
 * states at the same instant.
 *
 * @note These are status values. This interface does not state which transitions
 *       between them are legal or in what order they occur, so a caller must not infer
 *       a state machine from the ordinals - in particular, EPON_LLID_STATE_FAILED is
 *       not stated to be terminal, and nothing here says whether recovery from it
 *       passes through EPON_LLID_STATE_UNREGISTERED.
 *
 * @see epon_llid_info_t
 * @see epon_onu_status_t
 */
typedef enum {
    EPON_LLID_STATE_UNREGISTERED = 0,   /*!< No LLID is assigned and the ONU is not registered for this link. The zero value, so it is also what an unset field reads as. */
    EPON_LLID_STATE_REGISTERING = 1,    /*!< LLID assignment is in progress. A transient state; this interface states no bound on how long it may last. */
    EPON_LLID_STATE_REGISTERED = 2,     /*!< The LLID is assigned and the ONU is registered for this link. Registration alone does not imply that traffic is flowing - `forwarding_state` decides that. */
    EPON_LLID_STATE_DEREGISTERING = 3,  /*!< The LLID is being released and deregistration is in progress. A transient state. */
    EPON_LLID_STATE_FAILED = 4          /*!< Registration failed, or the link failed after registration. This interface does not state whether the implementation retries on its own, so a caller must not assume recovery is automatic. */
} epon_llid_state_t;

/**
 * @brief Forwarding state of one Logical Link Identifier.
 *
 * Forwarding is reported separately from registration because the two are independent:
 * an LLID in EPON_LLID_STATE_REGISTERED may still have forwarding disabled, so a caller
 * that treats registration as "traffic is flowing" will be wrong. Read both fields.
 *
 * @note This interface reports the forwarding state and declares no function that sets
 *       it.
 *
 * @see epon_llid_info_t
 * @see epon_llid_state_t
 */
typedef enum {
    EPON_LLID_FORWARDING_DISABLED = 0,  /*!< Forwarding is disabled and traffic on this LLID is blocked. The zero value. */
    EPON_LLID_FORWARDING_ENABLED = 1,   /*!< Forwarding is enabled and traffic on this LLID is allowed. */
    EPON_LLID_FORWARDING_LEARNING = 2   /*!< The LLID is learning, so forwarding is limited. This interface does not state which traffic is admitted while learning, so a caller must not assume it is either all or none. */
} epon_llid_forwarding_state_t;

/**
 * @brief One Logical Link Identifier: its value, mode, registration state, forwarding
 *        state, encryption flag and local MAC address.
 *
 * These records are the elements of the array epon_hal_get_llid_info() returns through
 * epon_llid_list_t. Registration state and forwarding state are independent and both
 * have to be read: an LLID can be registered with forwarding disabled.
 *
 * @note The implementation allocates the array these records live in and the caller
 *       frees it, which epon_llid_list_t and epon_hal_get_llid_info() both state. An
 *       individual record is not separately allocated, so a caller frees the array as a
 *       whole and never a member of it.
 *
 * @see epon_llid_list_t
 * @see epon_hal_get_llid_info
 */
typedef struct {
    uint16_t llid_value;                              /*!< The 16-bit Logical Link Identifier the OLT assigned. LLID 0 is the control channel over which MPCP and OAM run, and it is the link whose state epon_onu_status_t reports. */
    epon_llid_mode_t mode;                            /*!< Whether this LLID provides point-to-point or shared-medium emulation. @see epon_llid_mode_t */
    epon_llid_state_t state;                          /*!< Registration state of this LLID. Independent of `forwarding_state`. @see epon_llid_state_t */
    epon_llid_forwarding_state_t forwarding_state;    /*!< Forwarding state of this LLID. A caller reads this rather than inferring traffic flow from `state`. @see epon_llid_forwarding_state_t */
    bool encryption_enabled;                          /*!< True when traffic on this LLID is encrypted. It reports only *whether* encryption is on; *which* cipher is in use is the link-wide epon_hal_link_info_t::encryption, so a caller reads both to describe an LLID's protection. */
    uint8_t local_mac_address[EPON_HAL_MAC_ADDR_LEN]; /*!< Local MAC address associated with this LLID, EPON_HAL_MAC_ADDR_LEN (6) octets in transmission order. This is the ONU-side address; the OLT's is epon_olt_info_t::mac_address. */
} epon_llid_info_t;

/**
 * @brief The set of LLIDs the ONU currently has, as a HAL-allocated array with its
 *        count and the platform capacity.
 *
 * This is one of exactly two places in this interface where the implementation
 * allocates memory that the caller must free; the other is dpoe_cpe_mac_table_t. The
 * caller allocates this outer structure, epon_hal_get_llid_info() fills in the counts
 * and allocates `llid_list`, and the caller releases `llid_list` when it has finished
 * with it.
 *
 * @warning `llid_count` bounds `llid_list`, not `max_llid_count`. Reading
 *          `max_llid_count` entries would read past the allocation, because
 *          `max_llid_count` is the platform's capacity rather than the number of
 *          elements allocated.
 * @warning This structure carries no `struct_size` member, unlike the five structures
 *          that do, so epon_hal_get_llid_info() performs no version negotiation on it.
 *
 * @see epon_hal_get_llid_info
 * @see epon_llid_info_t
 */
typedef struct {
    uint32_t max_llid_count;        /*!< Maximum number of LLIDs the platform supports. A capability, not a count of populated entries, and *not* the bound on `llid_list`. */
    uint32_t llid_count;            /*!< Number of LLIDs currently configured and active, and therefore the exact number of elements the implementation allocated in `llid_list`. This is the bound a caller iterates to. Zero means no LLID is active. */
    epon_llid_info_t *llid_list;    /*!< Array of `llid_count` LLID records, allocated by the implementation and freed by the caller. This interface does not state what the pointer holds when `llid_count` is zero, so a caller checks the count before dereferencing and before freeing. */
} epon_llid_list_t;


/**
 * @brief Manufacturer and version identity of the ONU, in the TR-181
 *        `Device.DeviceInfo` vocabulary.
 *
 * This is the record epon_hal_get_manufacturer_info() populates. Every text field is a
 * fixed-size array whose content is vendor-supplied, so a caller reads it as opaque
 * text bounded by the array size rather than parsing it against a format this interface
 * defines. The TR-181 name each field corresponds to is given against it, so a caller
 * mapping this HAL onto a TR-181 data model has a one-to-one correspondence.
 *
 * @note The caller allocates this structure and must set `struct_size` to
 *       `sizeof(epon_onu_manufacturer_info_t)` before the call, or
 *       epon_hal_get_manufacturer_info() returns EPON_HAL_ERROR_INVALID_PARAM.
 * @note `software_version` reports the ONU firmware version and is unrelated to the
 *       packed value epon_hal_get_version() returns, which reports the version of this
 *       HAL interface. Conflating the two is the easiest mistake to make here.
 *
 * @see epon_hal_get_manufacturer_info
 */
typedef struct {
    uint32_t struct_size;                                 /*!< Set by the caller to `sizeof(epon_onu_manufacturer_info_t)` before the call. It is the only member the caller writes. An unset or wrong value yields EPON_HAL_ERROR_INVALID_PARAM. */
    char manufacturer[EPON_HAL_MANUFACTURER_LEN];         /*!< Manufacturer name, in up to EPON_HAL_MANUFACTURER_LEN (32) bytes. TR-181 `Device.DeviceInfo.Manufacturer`. */
    char model_number[EPON_HAL_MODEL_NUMBER_LEN];         /*!< Model number, in up to EPON_HAL_MODEL_NUMBER_LEN (16) bytes. TR-181 `Device.DeviceInfo.ModelNumber`. */
    char hardware_version[EPON_HAL_HW_VERSION_LEN];     /*!< Hardware revision, in up to EPON_HAL_HW_VERSION_LEN (16) bytes. TR-181 `Device.DeviceInfo.HardwareVersion`. The format is vendor-specific, so a caller compares it as text rather than ordering it as a version. */
    char software_version[EPON_HAL_SW_VERSION_LEN];     /*!< ONU firmware revision, in up to EPON_HAL_SW_VERSION_LEN (16) bytes. TR-181 `Device.DeviceInfo.SoftwareVersion`. This is the firmware on the device, not the HAL interface version that epon_hal_get_version() reports. */
    char serial_number[EPON_HAL_SERIAL_NUMBER_LEN];        /*!< Device serial number, in up to EPON_HAL_SERIAL_NUMBER_LEN (32) bytes. TR-181 `Device.DeviceInfo.SerialNumber`. */
    uint8_t vendor_oui[EPON_HAL_VENDOR_OUI_LEN];         /*!< Vendor OUI (Organizationally Unique Identifier), EPON_HAL_VENDOR_OUI_LEN (3) octets. This is the ONU vendor's OUI; the OLT vendor's is epon_olt_info_t::vendor_oui. It is binary rather than text, so a caller formats it for display rather than printing it. */
} epon_onu_manufacturer_info_t;


/**
 * @brief Encryption mode negotiated for the EPON link.
 *
 * The mode is a property of the link as a whole, reported by
 * epon_hal_get_link_info(). Whether a given LLID's traffic is actually encrypted is the
 * per-LLID epon_llid_info_t::encryption_enabled flag, so a caller reads both to
 * describe one LLID's protection: this enumeration says *which* cipher, that flag says
 * *whether* it is applied.
 *
 * @note This interface reports the negotiated mode and declares no function that sets
 *       it, so a caller cannot choose or change the cipher through this HAL.
 *
 * @see epon_hal_link_info_t
 * @see epon_llid_info_t
 */
typedef enum {
    EPON_ENCRYPTION_MODE_DISABLED = 0,      /*!< No data encryption is applied on the link. The zero value, so it is also what an unset field reads as - which matters here, because reading an unpopulated structure would understate the protection in place rather than look obviously wrong. */
    EPON_ENCRYPTION_MODE_AES_128 = 1,       /*!< AES-128, as specified by IEEE 802.3ah. */
    EPON_ENCRYPTION_MODE_TRIPLE_CHURNING = 2, /*!< Triple churning, the lighter-weight scheme carried over from earlier PON deployments. */
    EPON_ENCRYPTION_MODE_AES_256 = 3,       /*!< AES-256. An extension beyond IEEE 802.3ah, so a caller must not assume every implementation or OLT offers it. */
} epon_encryption_mode_t;

/**
 * @brief Operational mode and encryption mode of the EPON link.
 *
 * This is the record epon_hal_get_link_info() populates. Both members describe what was
 * negotiated with the OLT rather than what the caller requested: this interface declares
 * no function that sets either.
 *
 * @note Unlike the five structures that carry `struct_size`, this one does not, so
 *       epon_hal_get_link_info() performs no version negotiation on it.
 *
 * @see epon_hal_get_link_info
 */
typedef struct {
    char mode[EPON_HAL_MODE_LEN];                       /*!< Negotiated operational mode as text, in up to EPON_HAL_MODE_LEN (16) bytes - for example "1G-EPON" or "10G-EPON". This interface does not enumerate the permitted strings, so a caller matches against the values its vendor documents rather than parsing the rate out of the text. */
    epon_encryption_mode_t encryption;   /*!< Negotiated link encryption mode. Whether a particular LLID's traffic is encrypted is epon_llid_info_t::encryption_enabled. @see epon_encryption_mode_t */
} epon_hal_link_info_t;

/**
 * @brief The ONU's network interfaces, as a fixed-capacity array with the count that is
 *        populated.
 *
 * This is the record epon_hal_get_interface_list() populates. The array is embedded in
 * the structure the caller allocated rather than allocated by the implementation, so
 * there is nothing to free here - which distinguishes it from epon_llid_list_t and
 * dpoe_cpe_mac_table_t, the two types that do transfer ownership.
 *
 * @warning `interface_count` bounds the populated prefix. Entries beyond it are not
 *          written by the implementation and must not be read: because
 *          EPON_ONU_INTF_STATUS_LINK_DOWN is the zero value of
 *          epon_interface_link_status_t, an unwritten entry in a zero-initialised
 *          structure is indistinguishable from a real down interface.
 *
 * @see epon_hal_get_interface_list
 * @see epon_onu_interface_info_t
 */
typedef struct {
    uint32_t interface_count;               /*!< Number of populated entries in `interface`. It never exceeds EPON_HAL_MAX_INTERFACES (16), and it is the bound a caller iterates to. */
    epon_onu_interface_info_t interface[EPON_HAL_MAX_INTERFACES]; /*!< Storage for up to EPON_HAL_MAX_INTERFACES (16) interface records, of which only the first `interface_count` are populated. Embedded in the caller's structure, so it is not an allocation and there is nothing to free. */
} epon_interface_list_t;

/**
 * @brief Identity of the OLT the ONU is registered with.
 *
 * This is the record epon_hal_get_olt_info() populates. Both members are learned from
 * the OLT during MPCP registration and OAM discovery as specified by IEEE 802.3ah, so
 * neither is available before the ONU has registered - the function returns
 * EPON_HAL_ERROR_NOT_SUPPORTED in that case rather than reporting zeros.
 *
 * @note The caller allocates this structure and must set `struct_size` to
 *       `sizeof(epon_olt_info_t)` before the call, or epon_hal_get_olt_info() returns
 *       EPON_HAL_ERROR_INVALID_PARAM.
 * @note EPON_HAL_OLT_VENDOR_INFO_LEN is declared as a buffer length for OLT vendor
 *       information, but this structure carries no such field. A caller looking for
 *       richer OLT identity than the MAC address and OUI below will not find it in this
 *       interface.
 *
 * @see epon_hal_get_olt_info
 */
typedef struct {
    uint32_t struct_size;                               /*!< Set by the caller to `sizeof(epon_olt_info_t)` before the call. It is the only member the caller writes. An unset or wrong value yields EPON_HAL_ERROR_INVALID_PARAM. */
    uint8_t mac_address[EPON_HAL_MAC_ADDR_LEN];         /*!< OLT OAM MAC address, EPON_HAL_MAC_ADDR_LEN (6) octets, taken from MPCP GATE messages as specified in IEEE 802.3ah Clause 64.3.3. Described by the interface as always available once the ONU is registered. */
    uint8_t vendor_oui[EPON_HAL_VENDOR_OUI_LEN];        /*!< OLT vendor OUI, EPON_HAL_VENDOR_OUI_LEN (3) octets, taken from the OAM Information OAMPDU as specified in IEEE 802.3ah Clause 57.4.2.2. This identifies the OLT vendor, not the ONU vendor, which is epon_onu_manufacturer_info_t::vendor_oui. */
} epon_olt_info_t;

/**
 * @brief Initialization parameters for the EPON HAL: the DPoE capability statement and
 *        the three notification callbacks.
 *
 * This is the only input structure in the interface, and the only route by which
 * callbacks are installed: there is no separate registration function, so a callback
 * cannot be added, replaced or removed except by tearing the interface down with
 * epon_hal_deinit() and calling epon_hal_init() again with a different configuration.
 * A caller that wants no notification of a particular kind leaves that member NULL.
 *
 * @note The caller allocates this structure and must set `struct_size` to
 *       `sizeof(epon_hal_config_t)` before calling epon_hal_init(), which returns
 *       EPON_HAL_ERROR_INVALID_PARAM for an unset or wrong value.
 * @note The structure is passed to epon_hal_init() through a `const` pointer, and
 *       nothing in this interface states that the implementation retains that pointer
 *       beyond the call. A caller therefore keeps ownership of the storage and may reuse
 *       or release it once epon_hal_init() has returned - but must not rely on the
 *       implementation having copied it either, so the conservative course is to keep
 *       the structure alive for the initialized lifetime.
 *
 * @warning This interface does not state which thread the three callbacks are invoked
 *          on, whether two of them may run concurrently, or whether calling back into
 *          the HAL from inside one is permitted. The repository specification's
 *          `Threading Model` topic records that no threading model is specified and that
 *          a caller must not assume one (`docs/pages/halSpec.md`), so a callback body
 *          should do as little as possible - copy what it needs and hand off - rather
 *          than assume it may block or re-enter.
 *
 * @see epon_hal_init
 * @see epon_hal_deinit
 */
typedef struct {
    uint32_t struct_size;        /*!< Set by the caller to `sizeof(epon_hal_config_t)` before calling epon_hal_init(). An unset or wrong value yields EPON_HAL_ERROR_INVALID_PARAM. */
    bool dpoe_supported; /*!< True where the caller requires DPoE (DOCSIS Provisioning of EPON) support, which epon_hal_init() configures. It is the caller's statement of intent, not a report of platform capability: where DPoE is unavailable, dpoe_hal_get_cpe_mac_table() returns EPON_HAL_ERROR_NOT_SUPPORTED rather than this field reading false. @see dpoe_hal_get_cpe_mac_table */

    /**
     * @brief Invoked when the ONU registration status changes.
     *
     * Receives the new epon_onu_status_t by value, so there is no pointer lifetime to
     * manage. EPON_ONU_STATUS_REGISTRATION is delivered only once every configured
     * interface is up, which epon_hal_get_interface_list() states, so this callback is
     * not a per-interface notification - `interface_status_callback` is.
     *
     * Leave NULL to receive no status notification.
     *
     * @see epon_onu_status_t
     */
    void (*status_callback)(epon_onu_status_t status);

    /**
     * @brief Invoked when an alarm is raised or cleared.
     *
     * Receives a pointer to an epon_alarm_info_t describing the alarm type, the specific
     * standard or vendor alarm, the LLID it concerns, and whether it is being raised or
     * cleared. Both a raise and a clear are delivered through this one callback, so the
     * body must read `is_active` rather than treating every delivery as a new fault.
     *
     * The pointer is `const` and this interface does not state that it remains valid
     * after the callback returns, so a body that needs to retain any of the data copies
     * it rather than storing the pointer.
     *
     * Leave NULL to receive no alarm notification.
     *
     * @see epon_alarm_info_t
     *
     * Example usage:
     * @code
     * void my_alarm_callback(const epon_alarm_info_t *alarm_info) {
     *     if (alarm_info->alarm_type == EPON_ALARM_TYPE_STANDARD) {
     *         printf("Standard alarm %d on LLID %u: %s\n",
     *                alarm_info->standard_alarm, alarm_info->llid,
     *                alarm_info->is_active ? "RAISED" : "CLEARED");
     *     } else {
     *         printf("Vendor alarm %d on LLID %u: %s\n",
     *                alarm_info->vendor_alarm, alarm_info->llid,
     *                alarm_info->is_active ? "RAISED" : "CLEARED");
     *     }
     * }
     * @endcode
     */
    void (*alarm_callback)(const epon_alarm_info_t *alarm_info);

    /**
     * @brief Invoked when the link state of one layer-2 interface changes.
     *
     * Receives an epon_onu_interface_info_t by value, so there is no pointer lifetime to
     * manage. Where the device has several WAN interfaces the callback fires once per
     * interface, and the record carries no index, so a caller distinguishes them by the
     * `name` field - "veip0", "veip1" and so on.
     *
     * Leave NULL to receive no interface notification.
     *
     * @see epon_onu_interface_info_t
     * @see epon_hal_get_interface_list
     */
    void (*interface_status_callback)(epon_onu_interface_info_t status);
}epon_hal_config_t;

/** @} */  //END OF GROUP EPON_HAL_TYPES

/**
 * @addtogroup EPON_HAL_APIS
 * @{
 */

/*
 * Contract shared by every function below, stated once here rather than repeated
 * fifteen times, and referenced from each declaration:
 *
 * - **Return values.** Every function except epon_hal_get_version() reports its outcome
 *   synchronously through an epon_hal_return_t status code. There is no error callback
 *   and no error queue, so a caller learns of a failure at the point of the call.
 *   epon_hal_get_version() returns a value rather than a status and cannot fail.
 *
 * - **Ordering.** epon_hal_init() must succeed before any other function is called.
 *   epon_hal_deinit() ends the initialized lifetime and epon_hal_init() must be called
 *   again afterwards; epon_hal_factory_reset() likewise requires re-initialization.
 *
 * - **Blocking.** Only epon_hal_init() carries a bound: it returns
 *   EPON_HAL_ERROR_TIMEOUT where initialization blocked for more than three minutes.
 *   For the other fourteen functions this interface does not specify whether a call may
 *   block, and a caller must not assume either behaviour; the repository specification
 *   records the same conclusion under `Blocking calls`
 *   (`docs/pages/halSpec.md`). Several of them plainly involve hardware transactions, so
 *   a caller that cannot tolerate an unbounded wait imposes its own bound.
 *
 * - **Thread safety.** This interface does not specify a threading model, and a caller
 *   must not assume one: whether two functions may be called concurrently, whether one
 *   may be re-entered, and which thread the callbacks arrive on are all unspecified,
 *   which the repository specification states under `Threading Model`. A caller
 *   therefore serialises its own calls unless a specific vendor documents otherwise.
 *
 * - **Memory.** The caller allocates every structure it passes in and sets `struct_size`
 *   on the five that carry one. The implementation allocates exactly two arrays -
 *   epon_llid_list_t::llid_list and dpoe_cpe_mac_table_t::cpe_list - and the caller frees
 *   both.
 */


/**
 * @brief Reports the EPON HAL interface version the implementation was built against,
 *        as one packed 32-bit value.
 *
 * A caller uses this to establish compatibility before it relies on anything else in
 * the interface: it compares the major component against the major component of
 * EPON_HAL_API_VERSION, the version this header declares, and treats a mismatch as
 * incompatible. The minor and patch components are informational for that decision.
 *
 * This is the one function in the interface that returns a value rather than a status
 * code, so it has nothing to fail with and no output parameter to check.
 *
 * @pre None stated by this interface. Every other function documents
 *      EPON_HAL_ERROR_NOT_INITIALIZED and so requires epon_hal_init() first, but this
 *      one returns no status and therefore has no way to report an
 *      uninitialized state - which is consistent with a version being a build-time
 *      property rather than a runtime one. Even so, this interface does not
 *      *state* that the function may be called before epon_hal_init(), so a caller
 *      that needs the version to decide whether to initialize at all is relying on
 *      something the interface leaves unspecified.
 *
 * @post No caller-visible state changes. Repeated calls return the same value for the
 *       lifetime of the loaded implementation.
 *
 * @return The interface version, packed as EPON_HAL_MAKE_VERSION() packs it: major in
 *         bits 31-24, minor in bits 23-16 and patch in bits 15-0. A caller extracts the
 *         major component with `value >> 24`. Every 32-bit value is a syntactically
 *         valid version, so there is no sentinel for "unknown" and a return of zero
 *         reads as version 0.0.0 rather than as an error.
 *
 * @note This interface does not specify whether this call may block; it reports a
 *       build-time constant, so a caller has no reason to expect it to, but must not
 *       assume it either.
 * @note The value reported here is the version of the *HAL interface*, not of the ONU
 *       firmware. The firmware revision is epon_onu_manufacturer_info_t::software_version,
 *       reported by epon_hal_get_manufacturer_info().
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL rather than assuming it is safe to read concurrently.
 *
 * @see EPON_HAL_API_VERSION
 * @see EPON_HAL_MAKE_VERSION
 * @see epon_hal_get_manufacturer_info
 *
 * Example usage:
 * @code
 * uint32_t version = epon_hal_get_version();
 * if ((version >> 24) != (EPON_HAL_API_VERSION >> 24)) {
 *     // Major version mismatch - incompatible!
 *     fprintf(stderr, "EPON HAL version mismatch!\n");
 *     exit(1);
 * }
 * @endcode
 */
uint32_t epon_hal_get_version(void);

/**
 * @brief Brings the EPON HAL up against the supplied configuration and installs the
 *        caller's callbacks.
 *
 * This is the entry point that starts the interface's initialized lifetime. It installs
 * the three callback handlers carried in @p config, configures DPoE support where the
 * caller has asked for it, and initializes the interface mapping. It is the only route
 * by which callbacks are installed - the interface declares no separate registration
 * function - so a caller that wants to change a callback later must tear the interface
 * down with epon_hal_deinit() and initialize again.
 *
 * @param[in] config Pointer to a caller-allocated epon_hal_config_t. Must not be NULL,
 *                   and `config->struct_size` must be set to
 *                   `sizeof(epon_hal_config_t)` - that field is the interface's
 *                   version-negotiation mechanism, and an unset or wrong value is
 *                   rejected rather than tolerated. The structure carries the
 *                   `dpoe_supported` flag and the three callback pointers, each of which
 *                   may be NULL where the caller wants no notification of that kind.
 *                   The pointer is `const`, so the implementation does not modify the
 *                   structure, and the caller retains ownership of the storage. This
 *                   interface does not state whether the implementation copies the
 *                   configuration or retains the pointer, so the caller must neither
 *                   rely on a copy having been taken nor release the storage on the
 *                   assumption that it was: keeping the structure alive for the
 *                   initialized lifetime is the only course consistent with what the
 *                   interface states.
 *
 * @pre No other function in this interface may be called before this one has returned
 *      EPON_HAL_SUCCESS. A call made first returns EPON_HAL_ERROR_NOT_INITIALIZED
 *      instead of doing its work - except epon_hal_get_version(), which returns no
 *      status, and epon_hal_factory_reset(), which does not document that code.
 * @pre The optical hardware must be present and provisioned for EPON. Where it is
 *      provisioned for another PON technology the call fails with
 *      EPON_HAL_ERROR_WRONG_PON_MODE, which no amount of retrying will clear.
 *
 * @post On EPON_HAL_SUCCESS the interface is initialized, the non-NULL callbacks in
 *       @p config are installed and may begin firing, and every other function may be
 *       called. On any failure code the interface is *not* initialized and there is no
 *       partially initialized state for the caller to unwind: in particular, a caller
 *       must not call epon_hal_deinit() to clean up after a failed initialization, since
 *       that function documents EPON_HAL_ERROR_NOT_INITIALIZED for exactly that
 *       situation. This interface does not state whether a failed call may be retried
 *       directly, so a caller that retries should treat each attempt as a fresh first
 *       call.
 *
 * @returns epon_hal_return_t - the outcome of initialization, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The interface is initialized and every other function may
 *         now be called.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p config is NULL, `struct_size` is unset or
 *         does not match, or the structure carries a value the implementation rejects.
 *         The client corrects the call site; retrying with the same configuration cannot
 *         succeed.
 * @retval EPON_HAL_ERROR_WRONG_PON_MODE - The hardware is provisioned for a different
 *         PON technology, an ITU PON for instance rather than EPON. This is a
 *         provisioning mismatch, not a transient fault: the client stops, reports it,
 *         and does not retry, because the deployment has to be corrected.
 * @retval EPON_HAL_ERROR_HW_FAILURE - Hardware initialization failed. The client logs
 *         with context and surfaces the fault; a retry may succeed where the cause was
 *         transient.
 * @retval EPON_HAL_ERROR_CALLBACK_REG - Installing the callbacks failed, so
 *         initialization failed as a whole. The client treats the interface as
 *         uninitialized; there is nothing to unwind.
 * @retval EPON_HAL_ERROR_TIMEOUT - Initialization blocked for more than three minutes
 *         and the implementation gave up. A retry is reasonable; a repeat suggests a
 *         hardware or provisioning fault rather than a slow start.
 *
 * @note This call may block, and it is the only function in the interface with a stated
 *       bound: three minutes, past which it returns EPON_HAL_ERROR_TIMEOUT rather than
 *       waiting indefinitely. A caller must size its own start-up budget accordingly and
 *       must not treat initialization as a fast call.
 * @note The three-minute figure is a limit on the implementation, not a target. This
 *       interface states nothing about the typical duration, so a caller cannot derive
 *       an expected initialization time from it.
 *
 * @warning This interface does not specify a threading model and does not state that this
 *          function is thread safe, nor which thread the callbacks it installs are
 *          invoked on. A caller serialises its own calls into the HAL, and in particular
 *          must not call epon_hal_init() and epon_hal_deinit() concurrently from
 *          different threads.
 * @warning Callbacks may begin firing as soon as they are installed, which can be before
 *          this function returns. A caller must therefore have its callback bodies ready
 *          to run before it calls this function, not after.
 *
 * @see epon_hal_config_t
 * @see epon_hal_deinit
 * @see epon_hal_return_t
 */
epon_hal_return_t epon_hal_init(const epon_hal_config_t *config);

/**
 * @brief Tears the EPON HAL down, unregistering the callbacks, releasing the
 *        implementation's own allocations and deregistering the ONU from the OLT.
 *
 * This ends the initialized lifetime that epon_hal_init() began. It is called when the
 * EPON application shuts down, and its effects reach beyond the calling process: the ONU
 * is deregistered from the OLT, so the optical link stops carrying traffic for every
 * consumer of it, not only for the caller.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS. Calling this after a failed
 *      initialization, or twice in succession, returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED rather than succeeding silently.
 *
 * @post On EPON_HAL_SUCCESS the callbacks are unregistered and will not fire again, the
 *       implementation's internal allocations are released, hardware cleanup has been
 *       performed and the ONU is deregistered from the OLT. epon_hal_init() must be
 *       called again, with a configuration, before any other function is used. The two
 *       arrays the implementation hands to the caller - epon_llid_list_t::llid_list and
 *       dpoe_cpe_mac_table_t::cpe_list - are the caller's to release and are *not*
 *       covered by this teardown, so a caller that has not freed them still must. On
 *       EPON_HAL_ERROR_HW_FAILURE this interface does not state how much of the teardown
 *       completed, so a caller must treat the interface as unusable and must not assume
 *       either that it is still initialized or that it has been cleanly released.
 *
 * @returns epon_hal_return_t - the outcome of teardown, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The interface was torn down; epon_hal_init() is required
 *         before further use.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - The interface was not initialized, so there
 *         was nothing to tear down. The client treats this as already-torn-down rather
 *         than as a fault, and does not retry.
 * @retval EPON_HAL_ERROR_HW_FAILURE - Hardware cleanup failed. The client logs with
 *         context and surfaces the fault. Because the completed extent of the teardown
 *         is unspecified, the client should not simply retry in a loop; a process restart
 *         is the safe recovery.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. Deregistering from the OLT is a protocol exchange
 *       rather than a local operation, so a caller on a shutdown path that cannot wait
 *       indefinitely imposes its own bound.
 * @note Deregistering the ONU disrupts service for the whole device, so this is not a
 *       call to make speculatively or as part of a recovery attempt for an unrelated
 *       failure.
 *
 * @warning This interface does not specify a threading model and does not state that this
 *          function is thread safe, so it does not state what happens if another thread
 *          or process is inside a HAL call while this one runs. A caller must quiesce its
 *          own use of the interface before tearing it down, and must not rely on the
 *          implementation to serialise a concurrent teardown.
 *
 * @see epon_hal_init
 * @see epon_hal_return_t
 */
epon_hal_return_t epon_hal_deinit(void);

/**
 * @brief Reads the EPON link counters into a caller-supplied structure.
 *
 * Populates the caller's epon_hal_link_stats_t with the packet, byte, error, discard,
 * FEC, broadcast, multicast, ranging and MAC-reset counters, together with the link's
 * maximum bit rate. The first ten members carry TR-181
 * `Device.Optical.Interface.{i}.Stats` names; the rest are EPON extensions.
 *
 * Every counter accumulates from the last epon_hal_clear_stats(), so a caller derives a
 * rate by differencing two reads and dividing by the interval between them. A pair of
 * reads that straddles an epon_hal_clear_stats() call cannot be differenced, and this
 * interface provides no way to detect that a reset happened in between.
 *
 * @param[in,out] stats Pointer to a caller-allocated epon_hal_link_stats_t. Must not be
 *                      NULL, and `stats->struct_size` must be set to
 *                      `sizeof(epon_hal_link_stats_t)` before the call - that field is
 *                      the reason the direction is `in,out` rather than `out`, since the
 *                      caller writes it and the implementation reads it. The caller both
 *                      allocates and releases the storage; nothing in this interface
 *                      states that the implementation retains the pointer beyond the
 *                      call, so the caller may reuse or release the storage once the
 *                      function has returned.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 * @pre `stats->struct_size` must hold the compiled size of the structure, or the call
 *      returns EPON_HAL_ERROR_INVALID_PARAM and writes nothing.
 *
 * @post On EPON_HAL_SUCCESS the counter members hold the values read at the time of the
 *       call. This interface does not state that members the implementation could not
 *       read are zeroed, so a caller must not treat an unset member as meaningful, and
 *       should zero-initialise the structure before the call if it wants a known
 *       starting state. On any failure code no member may be relied on.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The counters were read and @p stats is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p stats is NULL or its `struct_size` is unset
 *         or does not match. The client corrects the call site; retrying with the same
 *         argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. Counters are read from hardware, so a caller
 *       sampling them on a periodic timer should not assume the read is instantaneous.
 * @note The counters are unrelated to the optical measurements
 *       epon_hal_get_transceiver_stats() reports, which are instantaneous rather than
 *       cumulative and are not affected by epon_hal_clear_stats().
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. In particular, this interface does not state that the counter
 *          set is read atomically, so a caller must not assume that two members of one
 *          returned structure were sampled at the same instant.
 *
 * @see epon_hal_link_stats_t
 * @see epon_hal_clear_stats
 * @see epon_hal_get_transceiver_stats
 *
 * Example usage:
 * @code
 * epon_hal_link_stats_t stats = {0};
 * stats.struct_size = sizeof(stats);
 * if (epon_hal_get_link_stats(&stats) == EPON_HAL_SUCCESS) {
 *     printf("Packets sent: %lu\n", stats.packets_sent);
 * }
 * @endcode
 */
epon_hal_return_t epon_hal_get_link_stats(epon_hal_link_stats_t *stats);

/**
 * @brief Reads the optical transceiver measurements and their thresholds into a
 *        caller-supplied structure.
 *
 * Populates the caller's epon_hal_transceiver_stats_t with transmit and receive optical
 * power in dBm, the four threshold values those are judged against, and the laser bias
 * current, module temperature and supply voltage. Six members are TR-181
 * `Device.Optical.Interface.{i}` fields and three are vendor extensions.
 *
 * These are instantaneous measurements rather than counters: two reads may differ in
 * either direction, epon_hal_clear_stats() has no effect on them, and there is nothing
 * here to difference. The threshold members are what let a caller judge a reading
 * without hard-coding a limit, so a caller should compare against them rather than
 * against a constant of its own.
 *
 * @param[in,out] stats Pointer to a caller-allocated epon_hal_transceiver_stats_t. Must
 *                      not be NULL, and `stats->struct_size` must be set to
 *                      `sizeof(epon_hal_transceiver_stats_t)` before the call, which is
 *                      why the direction is `in,out`. The caller both allocates and
 *                      releases the storage; nothing in this interface states that the
 *                      implementation retains the pointer beyond the call.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 * @pre `stats->struct_size` must hold the compiled size of the structure, or the call
 *      returns EPON_HAL_ERROR_INVALID_PARAM and writes nothing.
 *
 * @post On EPON_HAL_SUCCESS the members hold the values read at the time of the call.
 *       This interface states no sentinel for an individual unavailable measurement and
 *       does not state that unreadable members are zeroed, so a caller cannot
 *       distinguish an unsupported measurement from a genuine zero by value alone; where
 *       the whole record is unavailable the function returns
 *       EPON_HAL_ERROR_NOT_SUPPORTED instead. On any failure code no member may be
 *       relied on.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The measurements were read and @p stats is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p stats is NULL or its `struct_size` is unset
 *         or does not match. The client corrects the call site; retrying with the same
 *         argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 * @retval EPON_HAL_ERROR_NOT_SUPPORTED - The implementation cannot report transceiver
 *         measurements on this platform - a module with no digital diagnostic
 *         monitoring, for instance. The client treats optical diagnostics as
 *         unavailable and stops calling this function rather than retrying.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. Reading a transceiver's diagnostics typically means
 *       an I2C transaction with the optical module, so a caller polling frequently should
 *       not assume the read is free.
 * @note The three vendor-extension members - bias current, temperature and supply
 *       voltage - carry no threshold field in this structure. A caller wanting to know
 *       whether one of them is out of range relies on the corresponding vendor alarm
 *       rather than on a comparison it makes itself.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. It also does not state that the nine members are sampled
 *          atomically, so a caller must not assume a measurement and its threshold were
 *          read at the same instant.
 *
 * @see epon_hal_transceiver_stats_t
 * @see epon_vendor_alarm_t
 * @see epon_hal_get_link_stats
 */
epon_hal_return_t epon_hal_get_transceiver_stats(epon_hal_transceiver_stats_t *stats);

/**
 * @brief Reads the ONU's Logical Link Identifiers into a caller-supplied structure,
 *        allocating the array of records the caller then frees.
 *
 * Populates the caller's epon_llid_list_t with the platform LLID capacity, the number of
 * LLIDs currently active, and an array of that many epon_llid_info_t records giving each
 * LLID's value, emulation mode, registration state, forwarding state, encryption flag
 * and local MAC address.
 *
 * This is one of exactly two functions in the interface that transfer ownership of
 * memory to the caller; dpoe_hal_get_cpe_mac_table() is the other. The caller allocates
 * the outer structure, the implementation allocates the inner array, and the caller
 * releases that array.
 *
 * @param[in,out] llid_list Pointer to a caller-allocated epon_llid_list_t. Must not be
 *                          NULL. The caller allocates and releases the outer structure;
 *                          the implementation allocates `llid_list->llid_list` with
 *                          exactly `llid_list->llid_count` elements and the caller must
 *                          free that array when it has finished with it, or the
 *                          allocation is leaked. The direction is `in,out` because the
 *                          caller supplies the structure the implementation writes
 *                          through; unlike the five structures that carry `struct_size`,
 *                          this one does not, so there is no size field for the caller to
 *                          set and no version negotiation performed.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and allocates nothing.
 * @pre The caller must have released any array obtained from a previous successful call
 *      through the same structure. This interface does not state that the implementation
 *      frees or reuses a pointer already present in the structure, so overwriting one
 *      without freeing it first leaks it.
 *
 * @post On EPON_HAL_SUCCESS `llid_count` holds the number of active LLIDs and
 *       `llid_list` addresses that many records, which the caller must free.
 *       `max_llid_count` holds the platform capacity and is *not* the bound on the
 *       array. Where `llid_count` is zero this interface does not state what
 *       `llid_list` holds, so a caller checks the count before dereferencing and before
 *       freeing. On any failure code no member may be relied on and, in particular, the
 *       caller must not free `llid_list`, because this interface does not state that it
 *       was set.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The LLID set was read; `llid_count` records are available
 *         and the array is the caller's to free.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p llid_list is NULL. The client corrects the
 *         call site; retrying with the same argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. It performs an allocation on the caller's behalf as
 *       well as reading hardware state, so a caller on a latency-sensitive path should
 *       not poll it.
 * @note This function does not document EPON_HAL_ERROR_MEMORY, even though it allocates.
 *       This interface therefore does not state which code is returned when the
 *       allocation fails, so a caller must handle a failure code outside the three listed
 *       above rather than assuming the set is exhaustive.
 *
 * @warning `llid_count` bounds the array, not `max_llid_count`. Iterating to
 *          `max_llid_count` reads past the allocation.
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. The returned array is the caller's, so its lifetime is not
 *          affected by later HAL calls - but epon_hal_deinit() does not free it either,
 *          so a caller must release it before shutting down.
 *
 * @see epon_llid_list_t
 * @see epon_llid_info_t
 * @see dpoe_hal_get_cpe_mac_table
 */
epon_hal_return_t epon_hal_get_llid_info(epon_llid_list_t *llid_list);


/**
 * @brief Reads the ONU's manufacturer and version identity into a caller-supplied
 *        structure.
 *
 * Populates the caller's epon_onu_manufacturer_info_t with the manufacturer name, model
 * number, hardware and firmware revisions, serial number and vendor OUI, as identified
 * by IEEE 802.3ah. Each text member corresponds to a TR-181 `Device.DeviceInfo` field,
 * named against it in the structure, so a caller mapping this HAL onto a TR-181 data
 * model has a one-to-one correspondence.
 *
 * The values are vendor-supplied and this interface defines no format for any of them,
 * so a caller reads each as opaque text bounded by its array size rather than parsing
 * it.
 *
 * @param[in,out] info Pointer to a caller-allocated epon_onu_manufacturer_info_t. Must
 *                     not be NULL, and `info->struct_size` must be set to
 *                     `sizeof(epon_onu_manufacturer_info_t)` before the call, which is
 *                     why the direction is `in,out`. The caller both allocates and
 *                     releases the storage. The member array sizes bound what may be
 *                     read from it: `manufacturer` and `serial_number` hold 32 bytes
 *                     each, `model_number`, `hardware_version` and `software_version`
 *                     16 bytes each, and `vendor_oui` 3 octets. Those are array sizes,
 *                     so a caller bounds every read by them rather than relying on a
 *                     terminator this interface does not promise for a value that fills
 *                     its array.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 * @pre `info->struct_size` must hold the compiled size of the structure, or the call
 *      returns EPON_HAL_ERROR_INVALID_PARAM and writes nothing.
 *
 * @post On EPON_HAL_SUCCESS the members the implementation was able to read are
 *       populated. This interface does not state that members it could not read are
 *       zeroed or emptied, so a caller must not treat an unset member as meaningful and
 *       should zero-initialise the structure before the call if it wants a known
 *       starting state. On any failure code no member may be relied on.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The identity was read and @p info is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p info is NULL or its `struct_size` is unset
 *         or does not match. The client corrects the call site; retrying with the same
 *         argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. The values it reports are properties of the device
 *       rather than of the link, so a caller normally reads them once at start-up rather
 *       than polling.
 * @note `software_version` is the ONU firmware revision. It is not the HAL interface
 *       version, which epon_hal_get_version() reports as a packed 32-bit value, and the
 *       two are not comparable.
 * @note Unlike epon_hal_get_olt_info(), this function documents no
 *       EPON_HAL_ERROR_NOT_SUPPORTED, so this interface does not state a case in which
 *       the ONU's own identity is unavailable.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL.
 *
 * @see epon_onu_manufacturer_info_t
 * @see epon_hal_get_olt_info
 * @see epon_hal_get_version
 */
epon_hal_return_t epon_hal_get_manufacturer_info(epon_onu_manufacturer_info_t *info);

/**
 * @brief Resets the EPON link counters to zero.
 *
 * Zeroes the counters epon_hal_get_link_stats() reports - the packet, byte, error,
 * discard, FEC, broadcast, multicast, ranging-resync and MAC-reset counts - so that
 * subsequent reads accumulate from this point.
 *
 * It does not affect the optical measurements epon_hal_get_transceiver_stats() reports,
 * which are instantaneous readings rather than accumulations, and it does not affect
 * epon_hal_link_stats_t::max_bit_rate, which is a link capability rather than a counter.
 * It takes no argument, so the reset is unconditional and covers the whole counter set:
 * this interface offers no way to clear one counter, or the counters of one LLID or one
 * interface, in isolation.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and clears nothing.
 *
 * @post On EPON_HAL_SUCCESS the counters read zero, or near zero where traffic continues
 *       during the call. On EPON_HAL_ERROR_HW_FAILURE this interface does not state
 *       whether some counters were cleared before the failure, so a caller must treat
 *       the counter set as being in an unknown state and must not difference a
 *       subsequent read against a value taken before the failed call.
 *
 * @returns epon_hal_return_t - the outcome of the reset, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The counters were reset.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 * @retval EPON_HAL_ERROR_HW_FAILURE - The reset failed in hardware. The client logs with
 *         context and discards any counter baseline it was holding; a retry may succeed
 *         where the cause was transient.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour.
 * @note Clearing the counters destroys the baseline of any other consumer that is
 *       differencing reads, and this interface offers no way to detect that a reset
 *       occurred. A caller in a system where more than one component samples these
 *       counters should not clear them on its own account.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL - and in particular must not run it concurrently with
 *          epon_hal_get_link_stats(), since this interface does not state what a read
 *          interleaved with a reset returns.
 *
 * @see epon_hal_get_link_stats
 * @see epon_hal_link_stats_t
 */
epon_hal_return_t epon_hal_clear_stats(void);

/**
 * @brief Restarts the ONU's registration with the OLT: deregister, soft reset,
 *        re-register.
 *
 * The ONU deregisters, performs a soft reset and then attempts to register again
 * following the IEEE 802.3ah MPCP discovery and registration procedure. Traffic stops
 * for the duration, so this is a recovery action rather than a routine one.
 *
 * Unlike epon_hal_deinit() and epon_hal_factory_reset(), this call does *not* end the
 * initialized lifetime: the interface stays initialized, the callbacks stay installed,
 * and a caller does not need to call epon_hal_init() again afterwards. What it resets is
 * the ONU's relationship with the OLT, not the HAL.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and does nothing.
 *
 * @post On EPON_HAL_SUCCESS the reset has been *initiated*. The success code reports
 *       that the request was accepted, not that registration has completed: this
 *       interface does not state that the function waits for the ONU to come back up.
 *       A caller therefore observes completion through
 *       epon_hal_config_t::status_callback reaching EPON_ONU_STATUS_REGISTRATION, or by
 *       polling epon_hal_get_interface_list(), rather than by treating the return as the
 *       end of the operation. Until then, link, LLID and OLT information must be treated
 *       as stale. On EPON_HAL_ERROR_HW_FAILURE this interface does not state whether the
 *       ONU was left deregistered, so a caller must not assume the previous state
 *       survived.
 *
 * @returns epon_hal_return_t - the outcome of the request, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The reset was initiated. Registration completes
 *         asynchronously.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 * @retval EPON_HAL_ERROR_HW_FAILURE - The reset failed. The client logs with context and
 *         surfaces the fault; because the resulting ONU state is unspecified, it should
 *         re-read status rather than retry blindly.
 *
 * @note This operation causes a temporary service disruption for the whole device, so it
 *       is not a call to make speculatively.
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. Nor does it state how long re-registration takes,
 *       so a caller that needs a bound imposes its own and watches the status callback.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. Status and interface callbacks will fire while the reset
 *          proceeds, and this interface does not state which thread they arrive on, so a
 *          caller must not hold a lock across this call that its own callback bodies also
 *          need.
 *
 * @see epon_onu_status_t
 * @see epon_hal_get_interface_list
 * @see epon_hal_factory_reset
 */
epon_hal_return_t epon_hal_reset_onu(void);

/**
 * @brief Restores the EPON HAL configuration to factory defaults, ending the initialized
 *        lifetime.
 *
 * Clears the custom settings and statistics and restores the default operational
 * parameters. It is the most destructive call in the interface: all configuration is
 * lost, service is disrupted, and epon_hal_init() must be called again before any other
 * function is used - so a caller that treats this as an ordinary maintenance operation
 * and carries on calling getters is relying on behaviour this interface does not grant.
 *
 * @pre This function does **not** document EPON_HAL_ERROR_NOT_INITIALIZED, unlike every
 *      other function that takes no argument. This interface therefore does not state
 *      whether it may be called before epon_hal_init(), or what happens if it is, so a
 *      caller should not rely on either outcome and should initialize first. That
 *      omission is a gap in the interface definition rather than a licence to skip
 *      initialization.
 *
 * @post On EPON_HAL_SUCCESS the configuration is at factory defaults and the initialized
 *       lifetime has ended: epon_hal_init() is required before any further use, and the
 *       previously installed callbacks must not be relied on. Statistics are cleared, so
 *       any counter baseline a caller was holding is invalid. On
 *       EPON_HAL_ERROR_HW_FAILURE or EPON_HAL_ERROR_CONFIG this interface does not state
 *       how much of the reset completed, so a caller must treat both the configuration
 *       and the initialization state as indeterminate. It must not treat
 *       re-initialization as a recovery from that state: epon_hal_init() is stated as a
 *       requirement only after a *successful* reset, so this interface establishes no
 *       defined route out of a failed one.
 *
 * @returns epon_hal_return_t - the outcome of the reset, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The factory reset completed. epon_hal_init() must be called
 *         again before any other function.
 * @retval EPON_HAL_ERROR_HW_FAILURE - The reset failed in hardware. The client logs with
 *         context and surfaces the fault; the device may be left in a partially reset
 *         state.
 * @retval EPON_HAL_ERROR_CONFIG - Default configuration could not be restored. The client
 *         escalates rather than retrying: this reports that the defaults themselves could
 *         not be applied, so the device may need attention.
 *
 * @note This operation causes service disruption and the loss of all configuration.
 *       Re-initialization with epon_hal_init() is required afterwards.
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. A factory reset in hardware is typically slow, and
 *       no bound is stated, so a caller that cannot wait indefinitely imposes its own.
 * @note The three codes above are the complete documented set, and it does not include
 *       EPON_HAL_ERROR_NOT_INITIALIZED. A caller should nonetheless handle an
 *       undocumented code defensively rather than assuming the set is exhaustive.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe. A caller must quiesce its own use of the
 *          interface before calling this, because the call invalidates the initialized
 *          state that every other function depends on.
 *
 * @see epon_hal_init
 * @see epon_hal_reset_onu
 * @see epon_hal_return_t
 */
epon_hal_return_t epon_hal_factory_reset(void);

/**
 * @brief Reads the negotiated operational mode and encryption mode of the EPON link into
 *        a caller-supplied structure.
 *
 * Populates the caller's epon_hal_link_info_t with the operational mode as text - "1G-EPON"
 * or "10G-EPON", for instance - and the encryption mode negotiated with the OLT. Both
 * describe what was negotiated rather than what the caller asked for: this interface
 * declares no function that sets either, so these are read-only properties of the link.
 *
 * @param[out] info Pointer to a caller-allocated epon_hal_link_info_t. Must not be NULL.
 *                  The caller both allocates and releases the storage; nothing in this
 *                  interface states that the implementation retains the pointer beyond
 *                  the call. `info->mode` is a EPON_HAL_MODE_LEN (16) byte array, and
 *                  that array size is what bounds a read of it. Unlike the five
 *                  structures that carry `struct_size`, this one does not, so the
 *                  direction is `out` - the caller sets nothing before the call - and no
 *                  version negotiation is performed.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 *
 * @post On EPON_HAL_SUCCESS the two members hold the negotiated values. This interface
 *       does not state what they hold before the ONU has registered, nor that unreadable
 *       members are zeroed - and EPON_ENCRYPTION_MODE_DISABLED is the zero value of its
 *       enumeration, so an unpopulated structure reads as "no encryption" rather than as
 *       obviously unset. A caller should therefore confirm registration through
 *       epon_onu_status_t before relying on either member, and should zero-initialise
 *       the structure if it wants a known starting state. On any failure code no member
 *       may be relied on.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The link information was read and @p info is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p info is NULL. The client corrects the call
 *         site; retrying with the same argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour.
 * @note `encryption` reports the cipher negotiated for the link. Whether a particular
 *       LLID's traffic is actually encrypted is epon_llid_info_t::encryption_enabled,
 *       from epon_hal_get_llid_info(), so a caller describing one LLID's protection reads
 *       both.
 * @note Unlike epon_hal_get_olt_info(), this function documents no
 *       EPON_HAL_ERROR_NOT_SUPPORTED and no failure specific to an unregistered ONU, so
 *       the return value alone does not tell a caller whether the link has come up.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL.
 *
 * @see epon_hal_link_info_t
 * @see epon_encryption_mode_t
 * @see epon_hal_get_llid_info
 */
epon_hal_return_t epon_hal_get_link_info(epon_hal_link_info_t *info);

/**
 * @brief Reads the ONU's network interfaces and their link states into a caller-supplied
 *        structure.
 *
 * Populates the caller's epon_interface_list_t with the interfaces the OLT has configured
 * - the S1 or IP interfaces, named "veip0", "veip1" and so on - each entry carrying the
 * interface name and its current link state. Several interfaces may exist, each
 * potentially associated with a different VLAN or service instance.
 *
 * Unlike epon_hal_get_llid_info() and dpoe_hal_get_cpe_mac_table(), this function
 * allocates nothing: the array is embedded in the caller's structure, so there is nothing
 * to free.
 *
 * @param[out] if_list Pointer to a caller-allocated epon_interface_list_t. Must not be
 *                     NULL. The caller both allocates and releases the storage; nothing in
 *                     this interface states that the implementation retains the pointer
 *                     beyond the call. The embedded array holds at most
 *                     EPON_HAL_MAX_INTERFACES (16) entries and the populated prefix is
 *                     given by `if_list->interface_count`; each entry's `name` is a
 *                     EPON_HAL_INTERFACE_NAME_LEN (32) byte array, which is what bounds a
 *                     read of it. The direction is `out` - this structure carries no
 *                     `struct_size`, so the caller sets nothing before the call and no
 *                     version negotiation is performed.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 *
 * @post On EPON_HAL_SUCCESS `interface_count` holds the number of populated entries, never
 *       more than EPON_HAL_MAX_INTERFACES (16), and the first `interface_count` entries
 *       are populated. Entries beyond that count are not written and must not be read. On
 *       any failure code no member may be relied on, including `interface_count`.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The interface list was read and @p if_list is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p if_list is NULL. The client corrects the call
 *         site; retrying with the same argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes and
 *         retries.
 *
 * @note EPON_ONU_STATUS_REGISTRATION is delivered only after every configured interface is
 *       operational, that is once each reports EPON_ONU_INTF_STATUS_LINK_UP. A caller may
 *       therefore treat that status as the point at which this list is fully up, and
 *       conversely must not expect the device-wide status to reflect a single interface
 *       going down - epon_hal_config_t::interface_status_callback reports that.
 * @note This interface does not specify whether this call may block, and a caller must not
 *       assume either behaviour.
 * @note This interface does not state whether an interface that has been removed by the
 *       OLT disappears from the list or remains in it reporting
 *       EPON_ONU_INTF_STATUS_LINK_DOWN, so a caller must not treat the set of names as
 *       stable across calls.
 *
 * @warning Iterate only to `interface_count`. Because EPON_ONU_INTF_STATUS_LINK_DOWN is the
 *          zero value of epon_interface_link_status_t, an unwritten entry in a
 *          zero-initialised structure is indistinguishable from a genuine down interface,
 *          so reading past the count silently invents interfaces.
 * @warning This interface does not specify a threading model and does not state that this
 *          function is thread safe, so a caller serialises it with its other calls into the
 *          HAL.
 *
 * @see epon_interface_list_t
 * @see epon_onu_interface_info_t
 * @see epon_onu_status_t
 */
epon_hal_return_t epon_hal_get_interface_list(epon_interface_list_t *if_list);

/**
 * @brief Reads the identity of the OLT the ONU is registered with into a caller-supplied
 *        structure.
 *
 * Populates the caller's epon_olt_info_t with the OLT's OAM MAC address, taken from MPCP
 * GATE messages as specified in IEEE 802.3ah Clause 64.3.3, and its vendor OUI, taken
 * from the OAM Information OAMPDU as specified in Clause 57.4.2.2. Both are learned
 * during registration and OAM discovery, so neither exists before the ONU has
 * registered - in which case the call reports EPON_HAL_ERROR_NOT_SUPPORTED rather than
 * returning zeros a caller might mistake for an answer.
 *
 * @param[in,out] olt_info Pointer to a caller-allocated epon_olt_info_t. Must not be
 *                         NULL, and `olt_info->struct_size` must be set to
 *                         `sizeof(epon_olt_info_t)` before the call, which is why the
 *                         direction is `in,out`. The caller both allocates and releases
 *                         the storage. `mac_address` holds EPON_HAL_MAC_ADDR_LEN (6)
 *                         octets and `vendor_oui` EPON_HAL_VENDOR_OUI_LEN (3); both are
 *                         binary rather than text, so a caller formats them for display
 *                         rather than printing them directly.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and writes nothing.
 * @pre `olt_info->struct_size` must hold the compiled size of the structure, or the call
 *      returns EPON_HAL_ERROR_INVALID_PARAM and writes nothing.
 * @pre The ONU must be registered with the OLT - EPON_ONU_STATUS_REGISTRATION - or the
 *      call returns EPON_HAL_ERROR_NOT_SUPPORTED.
 *
 * @post On EPON_HAL_SUCCESS both members hold the values learned from the OLT; the
 *       interface describes each as always available once the ONU is registered. On any
 *       failure code no member may be relied on. A caller should also treat a previously
 *       read value as stale once the status callback reports
 *       EPON_ONU_STATUS_DEREGISTRATION, because this interface does not state that the
 *       same OLT will be seen after re-registration.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The OLT identity was read and @p olt_info is populated.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p olt_info is NULL or its `struct_size` is
 *         unset or does not match. The client corrects the call site; retrying with the
 *         same argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes and
 *         retries.
 * @retval EPON_HAL_ERROR_NOT_SUPPORTED - The OLT identity is not available, which the
 *         interface attributes to the ONU not being registered. This is the one status in
 *         this interface that a caller should expect to clear on its own: the client waits
 *         for EPON_ONU_STATUS_REGISTRATION and reads again rather than treating the
 *         capability as absent.
 *
 * @note This interface does not specify whether this call may block, and a caller must not
 *       assume either behaviour. The values are learned through OAM discovery rather than
 *       read from local registers, so a caller should not poll this on a fast path.
 * @note Some OLT-reported fields are vendor-specific and are not available on every OLT
 *       implementation, so a caller should tolerate an OUI it does not recognise rather
 *       than treating it as an error.
 * @note `vendor_oui` here is the OLT vendor's. The ONU's own is
 *       epon_onu_manufacturer_info_t::vendor_oui, from
 *       epon_hal_get_manufacturer_info(), and the two are commonly different.
 *
 * @warning This interface does not specify a threading model and does not state that this
 *          function is thread safe, so a caller serialises it with its other calls into
 *          the HAL.
 *
 * @see epon_olt_info_t
 * @see epon_onu_status_t
 * @see epon_hal_get_manufacturer_info
 *
 * Example usage:
 * @code
 * epon_olt_info_t olt_info = {0};
 * olt_info.struct_size = sizeof(olt_info);
 * if (epon_hal_get_olt_info(&olt_info) == EPON_HAL_SUCCESS) {
 *     printf("OLT MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
 *            olt_info.mac_address[0], olt_info.mac_address[1],
 *            olt_info.mac_address[2], olt_info.mac_address[3],
 *            olt_info.mac_address[4], olt_info.mac_address[5]);
 * }
 * @endcode
 */
epon_hal_return_t epon_hal_get_olt_info(epon_olt_info_t *olt_info);

/**
 * @brief Selects which IEEE 802.3ah OAM message types the implementation writes log
 *        records for, replacing any selection previously in force.
 *
 * The mask is a set of epon_oam_log_type_t bit flags combined with a bitwise OR. Each
 * OAM message whose type is selected is emitted through HAL_LOG at
 * HAL_LOG_LEVEL_OAM, which is a level of its own precisely so a logging backend can
 * filter OAM traffic separately from every other record the HAL produces.
 *
 * This is the only setter in the interface. It is a whole-mask assignment rather than a
 * modification: the value supplied becomes the selection, so a caller that wants to add
 * a type to an existing selection ORs the two masks itself and passes the result. This
 * interface declares no corresponding getter, so a caller that needs to know the mask
 * currently in force must retain what it last set.
 *
 * @param[in] oam_log_mask Bitmask of epon_oam_log_type_t values, ORed together. Zero
 *                         disables OAM logging entirely and EPON_OAM_ALL - 0xFFFFFFFF -
 *                         enables every loggable type. The seven defined flags occupy
 *                         bits 0 to 6; the remaining twenty-five bits are undefined by
 *                         this interface, and it does not state whether setting one is
 *                         rejected, ignored or reserved, so a caller passes either
 *                         EPON_OAM_ALL or an OR of defined flags and nothing else. Note
 *                         that no value of this parameter is documented as returning
 *                         EPON_HAL_ERROR_INVALID_PARAM, so a caller cannot rely on an
 *                         out-of-range bit being reported.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and the selection in force is unchanged.
 *
 * @post On EPON_HAL_SUCCESS the supplied mask is the selection in force, and it applies
 *       to messages handled after the call. This interface does not state whether a
 *       message already in flight when the call is made is logged under the old
 *       selection or the new one. On any failure code the previous selection must be
 *       assumed to be unchanged, since this interface states no partial-application
 *       behaviour. The selection does not survive epon_hal_deinit(): nothing here states
 *       that it is retained, so a caller re-applies it after each initialization.
 *
 * @returns epon_hal_return_t - the outcome of the configuration change, reported
 *          synchronously.
 * @retval EPON_HAL_SUCCESS - The selection was applied.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 * @retval EPON_HAL_ERROR_NOT_SUPPORTED - The implementation does not offer OAM message
 *         logging on this platform. The client treats OAM logging as unavailable and
 *         stops calling this function rather than retrying; it is a statement about the
 *         platform, not about the mask supplied.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. It configures a logging selection rather than
 *       performing a hardware read, so it is the least likely call here to be
 *       expensive, but that is an expectation rather than a stated contract.
 * @note The mask controls *logging*, not *processing*. Every OAM message is processed
 *       regardless of it, so this function cannot be used to suppress protocol
 *       handling - only the log records that handling produces. A caller must not use a
 *       zero mask as a way of disabling OAM.
 * @note Whether a record reaches a destination depends on the HAL_LOG_FUNCTION backend
 *       in force. With the default backend each selected message appends a line to
 *       `/rdklogs/logs/EPONMANAGERLog.txt.0`; where an integrator has overridden the
 *       macro, the destination and the filtering are that backend's business and this
 *       mask only decides which messages are offered to it.
 * @note EPON_OAM_INFO selects the periodic OAM keep-alive, which is the highest-rate
 *       message class here. Enabling it, or EPON_OAM_ALL, on a production device
 *       produces sustained log volume for as long as the OAM session is up.
 * @note MPCP GATE and REPORT messages cannot be selected at all. They are handled in
 *       hardware and are not visible to software, so no flag reaches them and
 *       EPON_OAM_ALL does not either.
 *
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. Because the call replaces the whole mask rather than modifying
 *          it, two callers each applying their own selection will overwrite one another;
 *          the interface offers no read-modify-write primitive and no getter with which
 *          a caller could detect that having happened.
 *
 * @see epon_oam_log_type_t
 * @see HAL_LOG_LEVEL_OAM
 * @see HAL_LOG
 *
 * Example usage:
 * @code
 * // Enable logging for OAM Info and Event messages only
 * uint32_t mask = EPON_OAM_INFO | EPON_OAM_EVENT;
 * epon_hal_set_oam_log_mask(mask);
 *
 * // Enable all OAM message logging
 * epon_hal_set_oam_log_mask(EPON_OAM_ALL);
 *
 * // Disable all OAM message logging
 * epon_hal_set_oam_log_mask(0);
 * @endcode
 */
epon_hal_return_t epon_hal_set_oam_log_mask(uint32_t oam_log_mask);

/** @} */  //END OF GROUP EPON_HAL_APIS

/* ============================================================================
 * DPoE (DOCSIS Provisioning of EPON) Extension APIs
 * Enable with -DEPON_HAL_DPOE_SUPPORT at compile time
 * ============================================================================ */

/**
 * @addtogroup EPON_HAL_TYPES
 * @{
 */

/**
 * @brief How a CPE MAC address came to be in the DPoE table.
 *
 * The distinction decides whether an entry is expected to persist: a statically
 * configured entry is provisioned and does not age out, while a dynamically learned one
 * was observed on the subscriber side and is subject to ageing. It is also the field
 * that gives dpoe_cpe_mac_entry_t::age_time its meaning, since that member is documented
 * as zero for a static entry.
 *
 * @note Both enumerators are explicitly valued, so the numeric values are part of the
 *       contract rather than an artefact of declaration order.
 * @warning DPOE_CPE_MAC_STATIC is zero, so a zeroed dpoe_cpe_mac_entry_t is
 *          indistinguishable from a genuine static entry by this field alone. A caller
 *          reads only the entries the counts in dpoe_cpe_mac_table_t bound rather than
 *          scanning for a terminator.
 *
 * @see dpoe_cpe_mac_entry_t
 * @see dpoe_hal_get_cpe_mac_table
 */
typedef enum {
    DPOE_CPE_MAC_STATIC = 0,       /*!< Provisioned entry. It was configured rather than observed, and it does not age out, which is why `age_time` is documented as zero for it. */
    DPOE_CPE_MAC_DYNAMIC = 1       /*!< Learned entry. It was observed on the subscriber side and is subject to ageing, so `age_time` carries how long it has been held. */
} dpoe_cpe_mac_type_t;

/**
 * @brief One row of the DPoE CPE MAC address table.
 *
 * Each row identifies a single customer-premises device by MAC address, states whether
 * the entry was provisioned or learned, and - for a learned entry - how long it has been
 * held. Rows are returned as the array dpoe_hal_get_cpe_mac_table() allocates; this
 * structure is never populated by a caller.
 *
 * This structure does not carry a `struct_size` member, so no version negotiation is
 * performed on it. A caller must be built against the same header revision as the
 * implementation it links to.
 *
 * @see dpoe_cpe_mac_table_t
 * @see dpoe_cpe_mac_type_t
 * @see dpoe_hal_get_cpe_mac_table
 */
typedef struct {
    uint8_t mac_address[EPON_HAL_MAC_ADDR_LEN]; /*!< The CPE's MAC address, six bytes in network order, bounded by EPON_HAL_MAC_ADDR_LEN (6). It is a fixed-width field with no terminator, so all six bytes are always significant and a caller must not treat it as a string. */
    dpoe_cpe_mac_type_t type;      /*!< Whether this row was provisioned (DPOE_CPE_MAC_STATIC) or learned (DPOE_CPE_MAC_DYNAMIC). This field is what gives `age_time` its meaning and is read before it. @see dpoe_cpe_mac_type_t */
    uint32_t age_time;             /*!< Seconds this entry has been held, for a learned row. Documented as zero where `type` is DPOE_CPE_MAC_STATIC, so a caller reads `type` first: a zero here means "not applicable" for a static row and "just learned" for a dynamic one, and the two are indistinguishable by value alone. This interface states no ageing timeout, so a caller must not infer when a learned row will be removed. */
} dpoe_cpe_mac_entry_t;

/**
 * @brief The DPoE CPE MAC address table, with its rows in an array the implementation
 *        allocates and the caller frees.
 *
 * This is the record dpoe_hal_get_cpe_mac_table() populates. The caller allocates the
 * outer structure; the implementation allocates `cpe_list` and the caller releases it.
 * It is one of exactly two ownership transfers in this interface, epon_llid_list_t being
 * the other, and the two behave the same way.
 *
 * **The bound on `cpe_list` is the sum of two members, not one.** The array holds
 * `static_cpe_count + dynamic_cpe_count` elements; `max_cpe` is the platform's capacity
 * and is *not* the number of rows present. A caller that iterates to `max_cpe` reads
 * past the allocation.
 *
 * This structure does not carry a `struct_size` member, so no version negotiation is
 * performed on it.
 *
 * @note This interface does not state whether the array is ordered, whether static rows
 *       precede dynamic ones, or whether the order is stable between calls. A caller
 *       that needs a particular ordering imposes it itself, and reads
 *       dpoe_cpe_mac_entry_t::type per row rather than assuming a partition.
 *
 * @warning `cpe_list` must be freed by the caller when it has finished with it, or the
 *          allocation is leaked. It must be freed exactly once, and it must not be freed
 *          after a call that failed, because this interface does not state that the
 *          member was set in that case.
 *
 * @see dpoe_hal_get_cpe_mac_table
 * @see dpoe_cpe_mac_entry_t
 * @see epon_llid_list_t
 */
typedef struct {
    uint32_t max_cpe;        /*!< Number of CPE entries the platform can hold. This is a capacity, reported for a caller's information; it is **not** the bound on `cpe_list` and iterating to it reads past the allocation. */
    uint32_t static_cpe_count;     /*!< Number of provisioned rows in `cpe_list`. Together with `dynamic_cpe_count` it gives the array's element count. */
    uint32_t dynamic_cpe_count;    /*!< Number of learned rows in `cpe_list`. Together with `static_cpe_count` it gives the array's element count; neither member bounds the array on its own. */
    dpoe_cpe_mac_entry_t *cpe_list; /*!< Array of `static_cpe_count + dynamic_cpe_count` rows, allocated by the implementation and **freed by the caller**. Where both counts are zero this interface does not state what this member holds, so a caller checks the counts before dereferencing and before freeing. @see dpoe_hal_get_cpe_mac_table */
} dpoe_cpe_mac_table_t;

/** @} */  //END OF GROUP EPON_HAL_TYPES

/**
 * @addtogroup EPON_HAL_APIS
 * @{
 */

/**
 * @brief Reads the DPoE CPE MAC address table into a caller-supplied structure,
 *        allocating the array of rows the caller then frees.
 *
 * Populates the caller's dpoe_cpe_mac_table_t with the platform's CPE capacity, the
 * number of provisioned and learned entries, and an array holding both sets of rows.
 * Each row gives a CPE MAC address, whether it was provisioned or learned, and - for a
 * learned row - how long it has been held.
 *
 * This is the interface's one DPoE - DOCSIS Provisioning of EPON - function and the one
 * optional call in it. A platform that does not provision EPON through DOCSIS reports
 * that through EPON_HAL_ERROR_NOT_SUPPORTED rather than omitting the declaration: the
 * header carries the instruction to enable DPoE with `-DEPON_HAL_DPOE_SUPPORT`, but no
 * conditional compilation on that macro exists anywhere in this file, so the declaration
 * is always present and the runtime code is the only mechanism a caller can rely on.
 *
 * It is also one of exactly two functions here that transfer ownership of memory to the
 * caller; epon_hal_get_llid_info() is the other. The caller allocates the outer
 * structure, the implementation allocates the inner array, and the caller releases that
 * array.
 *
 * @param[in,out] cpe_table Pointer to a caller-allocated dpoe_cpe_mac_table_t. Must not
 *                          be NULL. The caller allocates and releases the outer
 *                          structure; the implementation allocates
 *                          `cpe_table->cpe_list` with
 *                          `static_cpe_count + dynamic_cpe_count` elements and the
 *                          caller must free that array when it has finished with it, or
 *                          the allocation is leaked. The direction is `in,out` because
 *                          the caller supplies the storage the implementation writes
 *                          through; this structure carries no `struct_size` member, so
 *                          there is nothing for the caller to set beforehand and no
 *                          version negotiation is performed.
 *
 * @pre epon_hal_init() must have returned EPON_HAL_SUCCESS, or the call returns
 *      EPON_HAL_ERROR_NOT_INITIALIZED and allocates nothing.
 * @pre The caller must have released any array obtained from a previous successful call
 *      through the same structure. This interface does not state that the implementation
 *      frees or reuses a pointer already present in the structure, so overwriting one
 *      without freeing it first leaks it.
 *
 * @post On EPON_HAL_SUCCESS `static_cpe_count` and `dynamic_cpe_count` hold the row
 *       counts, `cpe_list` addresses their sum of records, and that array is the
 *       caller's to free. `max_cpe` holds the platform capacity and is *not* the bound
 *       on the array. Where both counts are zero this interface does not state what
 *       `cpe_list` holds, so a caller checks the counts before dereferencing and before
 *       freeing. On any failure code no member may be relied on and, in particular, the
 *       caller must not free `cpe_list`, because this interface does not state that it
 *       was set.
 *
 * @returns epon_hal_return_t - the outcome of the read, reported synchronously.
 * @retval EPON_HAL_SUCCESS - The table was read; the two counts sum to the number of
 *         rows available and the array is the caller's to free.
 * @retval EPON_HAL_ERROR_INVALID_PARAM - @p cpe_table is NULL. The client corrects the
 *         call site; retrying with the same argument cannot succeed.
 * @retval EPON_HAL_ERROR_NOT_INITIALIZED - epon_hal_init() has not succeeded, or
 *         epon_hal_deinit() has since torn the interface down. The client initializes
 *         and retries.
 * @retval EPON_HAL_ERROR_NOT_SUPPORTED - DPoE is not supported on this platform. This is
 *         the authoritative indication of DPoE absence and it is a permanent condition,
 *         so the client stops calling this function rather than retrying. It is
 *         independent of what the caller passed in
 *         epon_hal_config_t::dpoe_supported, which states the caller's expectation
 *         rather than reporting platform capability.
 *
 * @note This interface does not specify whether this call may block, and a caller must
 *       not assume either behaviour. It performs an allocation on the caller's behalf as
 *       well as reading forwarding-database state whose size grows with the number of
 *       devices behind the ONU, so a caller on a latency-sensitive path should not poll
 *       it.
 * @note This function does not document EPON_HAL_ERROR_MEMORY, even though it allocates.
 *       This interface therefore does not state which code is returned when the
 *       allocation cannot be satisfied, and a caller must handle any documented failure
 *       code by not dereferencing and not freeing `cpe_list` rather than by testing for
 *       one specific code.
 * @note The table is a snapshot. This interface declares no notification for a CPE
 *       appearing or ageing out - epon_hal_config_t's three callbacks cover link state,
 *       alarms and OAM messages only - so a caller that needs to track changes re-reads
 *       the table and compares, and must expect rows to appear and disappear between
 *       calls.
 *
 * @warning `cpe_table->cpe_list` must be freed by the caller, exactly once, and only
 *          after a call that returned EPON_HAL_SUCCESS. Failing to free it leaks an
 *          allocation whose size grows with the number of devices behind the ONU, which
 *          is the more damaging of the two leaks this interface makes possible.
 * @warning This interface does not specify a threading model and does not state that
 *          this function is thread safe, so a caller serialises it with its other calls
 *          into the HAL. It also does not state that the counts and the array are
 *          consistent with one instant, so a caller must not assume the table was read
 *          atomically.
 *
 * @see dpoe_cpe_mac_table_t
 * @see dpoe_cpe_mac_entry_t
 * @see epon_hal_config_t
 * @see epon_hal_get_llid_info
 *
 * Example usage:
 * @code
 * dpoe_cpe_mac_table_t table = {0};
 * if (dpoe_hal_get_cpe_mac_table(&table) == EPON_HAL_SUCCESS) {
 *     uint32_t total = table.static_cpe_count + table.dynamic_cpe_count;
 *     for (uint32_t i = 0; i < total; i++) {
 *         printf("CPE %u: type %d age %u\n", i,
 *                table.cpe_list[i].type, table.cpe_list[i].age_time);
 *     }
 *     free(table.cpe_list);
 * }
 * @endcode
 */
epon_hal_return_t dpoe_hal_get_cpe_mac_table(dpoe_cpe_mac_table_t *cpe_table);

/** @} */  //END OF GROUP EPON_HAL_APIS

#ifdef __cplusplus
}
#endif

#endif /* _EPON_HAL_H_ */

