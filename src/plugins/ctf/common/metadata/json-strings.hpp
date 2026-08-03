/*
 * Copyright (c) 2022-2024 Philippe Proulx <pproulx@efficios.com>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_PLUGINS_CTF_COMMON_METADATA_JSON_STRINGS_HPP
#define BABELTRACE_PLUGINS_CTF_COMMON_METADATA_JSON_STRINGS_HPP

namespace ctf::jsonstr {

inline constexpr auto accuracy = "accuracy";
inline constexpr auto align = "alignment";
inline constexpr auto attrs = "attributes";
inline constexpr auto bigEndian = "big-endian";
inline constexpr auto bitOrder = "bit-order";
inline constexpr auto btNs = "babeltrace.org,2020";
inline constexpr auto byteOrder = "byte-order";
inline constexpr auto clkCls = "clock-class";
inline constexpr auto cycles = "cycles";
inline constexpr auto dataStreamCls = "data-stream-class";
inline constexpr auto dataStreamClsId = "data-stream-class-id";
inline constexpr auto dataStreamId = "data-stream-id";
inline constexpr auto defClkClsId = "default-clock-class-id";
inline constexpr auto defClkTs = "default-clock-timestamp";
inline constexpr auto descr = "description";
inline constexpr auto discEventRecordCounterSnap = "discarded-event-record-counter-snapshot";
inline constexpr auto dynLenArray = "dynamic-length-array";
inline constexpr auto dynLenBlob = "dynamic-length-blob";
inline constexpr auto dynLenStr = "dynamic-length-string";
inline constexpr auto elemFc = "element-field-class";
inline constexpr auto emfUri = "emf-uri";
inline constexpr auto encoding = "encoding";
inline constexpr auto env = "environment";
inline constexpr auto eventRecordCls = "event-record-class";
inline constexpr auto eventRecordClsId = "event-record-class-id";
inline constexpr auto eventRecordCommonCtx = "event-record-common-context";
inline constexpr auto eventRecordCommonCtxFc = "event-record-common-context-field-class";
inline constexpr auto eventRecordHeader = "event-record-header";
inline constexpr auto eventRecordHeaderFc = "event-record-header-field-class";
inline constexpr auto eventRecordPayload = "event-record-payload";
inline constexpr auto eventRecordSpecCtx = "event-record-specific-context";
inline constexpr auto extensions = "extensions";
inline constexpr auto fc = "field-class";
inline constexpr auto fcAlias = "field-class-alias";
inline constexpr auto fixedLenBitArray = "fixed-length-bit-array";
inline constexpr auto fixedLenBitMap = "fixed-length-bit-map";
inline constexpr auto fixedLenBool = "fixed-length-boolean";
inline constexpr auto fixedLenFloat = "fixed-length-floating-point-number";
inline constexpr auto fixedLenSInt = "fixed-length-signed-integer";
inline constexpr auto fixedLenUInt = "fixed-length-unsigned-integer";
inline constexpr auto flags = "flags";
inline constexpr auto freq = "frequency";
inline constexpr auto ftl = "first-to-last";
inline constexpr auto id = "id";
inline constexpr auto len = "length";
inline constexpr auto lenFieldLoc = "length-field-location";
inline constexpr auto littleEndian = "little-endian";
inline constexpr auto logLevel = "log-level";
inline constexpr auto logLevelAlert = "alert";
inline constexpr auto logLevelCritical = "critical";
inline constexpr auto logLevelDebugFunction = "debug:function";
inline constexpr auto logLevelDebugLine = "debug:line";
inline constexpr auto logLevelDebugModule = "debug:module";
inline constexpr auto logLevelDebug = "debug";
inline constexpr auto logLevelDebugProcess = "debug:process";
inline constexpr auto logLevelDebugProgram = "debug:program";
inline constexpr auto logLevelDebugSystem = "debug:system";
inline constexpr auto logLevelDebugUnit = "debug:unit";
inline constexpr auto logLevelEmergency = "emergency";
inline constexpr auto logLevelError = "error";
inline constexpr auto logLevelInfo = "info";
inline constexpr auto logLevelNotice = "notice";
inline constexpr auto logLevelWarning = "warning";
inline constexpr auto ltf = "last-to-first";
inline constexpr auto mappings = "mappings";
inline constexpr auto mediaType = "media-type";
inline constexpr auto memberClasses = "member-classes";
inline constexpr auto metadataStreamUuid = "metadata-stream-uuid";
inline constexpr auto minAlign = "minimum-alignment";
inline constexpr auto name = "name";
inline constexpr auto ns = "namespace";
inline constexpr auto nullTerminatedStr = "null-terminated-string";
inline constexpr auto offsetFromOrigin = "offset-from-origin";
inline constexpr auto optional = "optional";
inline constexpr auto opts = "options";
inline constexpr auto origin = "origin";
inline constexpr auto path = "path";
inline constexpr auto payloadFc = "payload-field-class";
inline constexpr auto pktContentLen = "packet-content-length";
inline constexpr auto pktCtx = "packet-context";
inline constexpr auto pktCtxFc = "packet-context-field-class";
inline constexpr auto pktEndDefClkTs = "packet-end-default-clock-timestamp";
inline constexpr auto pktHeader = "packet-header";
inline constexpr auto pktHeaderFc = "packet-header-field-class";
inline constexpr auto pktMagicNumber = "packet-magic-number";
inline constexpr auto pktSeqNum = "packet-sequence-number";
inline constexpr auto pktTotalLen = "packet-total-length";
inline constexpr auto preamble = "preamble";
inline constexpr auto precision = "precision";
inline constexpr auto prefDispBase = "preferred-display-base";
inline constexpr auto roles = "roles";
inline constexpr auto seconds = "seconds";
inline constexpr auto selFieldLoc = "selector-field-location";
inline constexpr auto selFieldRanges = "selector-field-ranges";
inline constexpr auto specCtxFc = "specific-context-field-class";
inline constexpr auto staticLenArray = "static-length-array";
inline constexpr auto staticLenBlob = "static-length-blob";
inline constexpr auto staticLenStr = "static-length-string";
inline constexpr auto structure = "structure";
inline constexpr auto traceCls = "trace-class";
inline constexpr auto type = "type";
inline constexpr auto uid = "uid";
inline constexpr auto unixEpoch = "unix-epoch";
inline constexpr auto utf16Be = "utf-16be";
inline constexpr auto utf16Le = "utf-16le";
inline constexpr auto utf32Be = "utf-32be";
inline constexpr auto utf32Le = "utf-32le";
inline constexpr auto utf8 = "utf-8";
inline constexpr auto uuid = "uuid";
inline constexpr auto variant = "variant";
inline constexpr auto varLenSInt = "variable-length-signed-integer";
inline constexpr auto varLenUInt = "variable-length-unsigned-integer";
inline constexpr auto version = "version";

} /* namespace ctf::jsonstr */

#endif /* BABELTRACE_PLUGINS_CTF_COMMON_METADATA_JSON_STRINGS_HPP */
