# Device OS 6.4.1 acknowledgment and session investigation

Read-only source investigation. All outputs are in the scratch directory. Commands were executed by zsh; no build, firmware change, infrastructure write, or shell test was run. Model inherited from dispatch: gpt-6-astra; reasoning: ultra. The source tree is `/Users/chipmc/.particle/toolchains/deviceOS/6.4.1`; repository is `/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter`.

## Conclusions and confidence

**High confidence, directly proven by source:** Passing PRIVATE alone does not request ACK-gated Future completion. On Boron/DTLS the event is nevertheless a CoAP confirmable request, with internal ACK/retransmission machinery. It is incorrect to equate PRIVATE-only with an explicit NO_ACK transport packet. Device OS immediately resolves the PRIVATE-only Future successfully after `channel.send()` succeeds; it registers an ACK completion handler only for explicit WITH_ACK. BackgroundPublishRK waits the Future properly, but without WITH_ACK that wait does not wait for the cloud.

**High confidence, directly proven by source:** An application queue can drain before cloud ACK, and the repository's default `Particle.disconnect()` can terminate the protocol, clearing its remaining retransmission copies. This establishes a concrete delivery-guarantee defect. It does not establish that the missing September 25 events followed this path; there are no ACK-correlated attempt logs for them.

**Unproven:** An unsafe fixed N-second window after connected, a Device OS session-resume defect, the specific transport location where the first transmission disappeared, and a causal claim that all observed losses came from early publication. `Particle.connected()` is deliberately delayed until handshake messages are processed and an additional protocol iteration succeeds. The implementation supports application publishes at that point. A post-connect delay is therefore an experiment/mitigation, not a proven requirement.

**Recommended first candidate:** Explicit WITH_ACK for every queued publish, preserving the queue until success/error becomes known. A delay alone does not close the demonstrated removal-before-ACK gap. Expect at-least-once behavior: lost ACKs can cause retransmitted or requeued duplicates, so bench accounting must identify each logical event, not merely count rows. WITH_ACK confirms Particle protocol acceptance, not Ubidots/integration delivery.

## Flag semantics and Future plumbing

`PRIVATE=0x1`, `NO_ACK=0x2`, `WITH_ACK=0x8`; the flags are separate. The wiring overload ORs supplied flags and does not inject WITH_ACK. System code strips PRIVATE because visibility no longer changes behavior. PublishQueuePosix preserves its caller's flags and BackgroundPublish passes them to Particle.publish (the separate library investigation should cite those queue lines).

The critical distinction is the code comment “Register completion handler only if acknowledgement was requested explicitly.” Default UDP confirmability controls lower transport retries; explicit WITH_ACK controls the completion callback visible to the queue.

### Flag constants

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/inc/system_cloud.h`

```cpp
178: const uint32_t PUBLISH_EVENT_FLAG_PUBLIC = 0x0;
179: const uint32_t PUBLISH_EVENT_FLAG_PRIVATE = 0x1;
180: const uint32_t PUBLISH_EVENT_FLAG_NO_ACK = 0x2;
181: const uint32_t PUBLISH_EVENT_FLAG_WITH_ACK = 0x8;
```

### Wiring overload flags, connection precondition and Future callback

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/wiring/inc/spark_wiring_cloud.h`

```cpp
288:     inline particle::Future<bool> publish(const char *eventName, const char *eventData, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
289:     {
290:         return publish(eventName, eventData, DEFAULT_CLOUD_EVENT_TTL, flags1, flags2);
291:     }
292: 
293:     inline particle::Future<bool> publish(const char *eventName, const String& eventData, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
294:     {
295:         return publish(eventName, eventData.c_str(), DEFAULT_CLOUD_EVENT_TTL, flags1, flags2);
296:     }
297: 
298:     inline particle::Future<bool> publish(const char *eventName, const char *eventData, int ttl, PublishFlags flags1, PublishFlags flags2 = PublishFlags())
299:     {
300:         return publish_event(eventName, eventData, eventData ? std::strlen(eventData) : 0, static_cast<int>(particle::ContentType::TEXT), ttl, flags1 | flags2);
301:     }
```

### Promise result and publish Future creation

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/wiring/src/spark_wiring_cloud.cpp`

```cpp
27: void publishCompletionCallback(int error, const void* data, void* callbackData, void* reserved) {
28:     auto p = Promise<bool>::fromDataPtr(callbackData);
29:     if (error != Error::NONE) {
30:         p.setError(Error((Error::Type)error, (const char*)data));
31:     } else {
32:         p.setResult(true);
33:     }
34: }
133: Future<bool> CloudClass::publish_event(const char* name, const char* data, size_t size, int type, int ttl,
134:         PublishFlags flags) {
135:     if (!connected()) {
136:         return Future<bool>(Error::INVALID_STATE);
137:     }
138:     spark_send_event_data d = {};
139:     d.size = sizeof(spark_send_event_data);
140:     d.data_size = size;
141:     d.content_type = static_cast<int>(type);
142: 
143:     // Completion handler
144:     Promise<bool> p;
145:     d.handler_callback = publishCompletionCallback;
146:     d.handler_data = p.dataPtr();
147: 
148:     if (!spark_send_event(name, data, ttl, flags.value(), &d) && !p.isDone()) {
149:         // Set generic error code in case completion callback wasn't invoked for some reason
150:         p.setError(Error::UNKNOWN);
151:         p.fromDataPtr(d.handler_data); // Free wrapper object
152:     }
153: 
154:     return p.future();
```

### No implicit acknowledgment flag

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_cloud.cpp`

```cpp
164: bool spark_send_event(const char* name, const char* data, int ttl, uint32_t flags, void* reserved)
165: {
166:     if (flags & PUBLISH_EVENT_FLAG_ASYNC) {
167:         SYSTEM_THREAD_CONTEXT_ASYNC_RESULT(spark_send_event(name, data, ttl, flags, reserved), true);
168:     } else {
169:         SYSTEM_THREAD_CONTEXT_SYNC(spark_send_event(name, data, ttl, flags, reserved));
170:     }
193:     // Visibility flags no longer have effect
194:     flags &= ~PUBLISH_EVENT_FLAG_PRIVATE;
195: 
196:     return spark_protocol_send_event(sp, name, data, ttl, flags, &d);
234: bool spark_cloud_flag_connected(void)
235: {
236:     return (SPARK_CLOUD_SOCKETED && SPARK_CLOUD_CONNECTED);
```

### Default confirmability versus explicit Future acknowledgment

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.cpp`

```cpp
30: void Publisher::add_ack_handler(message_id_t msg_id, CompletionHandler handler) {
31:     protocol->add_ack_handler(msg_id, std::move(handler), SEND_EVENT_ACK_TIMEOUT);
32: }
49:     bool confirmable = channel.is_unreliable();
50:     if (flags & EventType::NO_ACK) {
51:         confirmable = false;
52:     } else if (flags & EventType::WITH_ACK) {
53:         confirmable = true;
54:     }
55: 
56:     CoapMessageEncoder e((char*)msg.buf(), msg.capacity());
57:     e.type(confirmable ? CoapType::CON : CoapType::NON);
88:     err = channel.send(msg);
89:     if (err != ProtocolError::NO_ERROR) {
90:         return err;
91:     }
92: 
93:     // Register completion handler only if acknowledgement was requested explicitly
94:     if ((flags & EventType::WITH_ACK) && msg.has_id()) {
95:         add_ack_handler(msg.get_id(), std::move(handler));
96:     } else {
97:         handler.setResult();
98:     }
99: 
100:     return ProtocolError::NO_ERROR;
```

### DTLS is the unreliable transport under CoAP

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_message_channel.cpp`

```cpp
546: 	int ret = mbedtls_ssl_write(&ssl_context, message.buf(), message.length());
547: 	if (ret < 0 && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
548: 		LOG(ERROR, "mbedtls_ssl_write() failed: -0x%x", -ret);
549: 		if (ret == MBEDTLS_ERR_NET_SEND_FAILED) {
550: 			// Do not invalidate the session on network errors
551: 			return IO_ERROR_SOCKET_SEND_FAILED;
552: 		}
553: 		reset_session();
554: 		return IO_ERROR_GENERIC_MBEDTLS_SSL_WRITE;
555: 	}
556: 	sessionPersist.update(&ssl_context, callbacks.save, coap_state ? *coap_state : 0);
557: 	return NO_ERROR;
558: }
559: 
560: bool DTLSMessageChannel::is_unreliable()
561: {
562: 	return true;
563: }
```

### Boron DTLS stack includes CoAP reliability

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_protocol.h`

```cpp
42: class DTLSProtocol : public Protocol
43: {
44: 	CoAPChannel<CoAPReliableChannel<DTLSMessageChannel, decltype(SparkCallbacks::millis)>> channel;
```

### BackgroundPublish waits the Future, not just a boolean conversion

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/BackgroundPublishRK/src/BackgroundPublishRK.cpp`

```cpp
78:         // kick off the publish
79:         // WITH_ACK does not work as expected from a background thread
80:         // use the Future<bool> object directly as its default wait
81:         // (used by WITH_ACK) short-circuits when not called from the
82:         // main application thread
83:         auto ok = Particle.publish(event_name, event_data, event_flags);
84: 
85:         // then wait for publish to complete
86:         while(!ok.isDone() && state != BACKGROUND_PUBLISH_STOP)
87:         {
88:             // yield to rest of system while we wait
89:             delay(1);
90:         }
91: 
92:         if(completed_cb)
93:         {
94:             completed_cb(ok.isSucceeded(),
95:                 event_name,
96:                 event_data,
97:                 event_context);
```

## WITH_ACK completion, rejection and timeout

The explicit handler resolves success when an incoming ACK has a successful response code; errors include CoAP 4xx/5xx, timeout and abort. Timeout is 20 seconds; a protocol reset aborts handlers. The queue can therefore distinguish “sent locally” from “acknowledged” only by supplying WITH_ACK. This is not an integration/webhook acknowledgment.

### ACK routing and outcome mapping

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/protocol.cpp`

```cpp
94: 	// The passthrough flag is set for messages that are not intended for the old protocol
95: 	// implementation
96: 	if (!message.passthrough()) {
97: 		message_type = Messages::decodeType(queue, message.length());
98: 		if (type == CoAPType::ACK || type == CoAPType::RESET) {
99: 			// todo - this is a little too simple in the case of an empty ACK for a separate response
100: 			// the message should then be bound to the token. see CH19037
101: 			if (type == CoAPType::RESET) { // RST is sent with an empty code. It's like an unspecified error
102: 				LOG(TRACE, "Reset received, setting error code to internal server error.");
103: 				code = CoAPCode::INTERNAL_SERVER_ERROR;
104: 			}
105: 			notify_message_complete(msg_id, code);
260: void Protocol::notify_message_complete(message_id_t msg_id, CoAPCode::Enum responseCode) {
261: 	const auto codeClass = (int)responseCode >> 5;
262: 	if (CoAPCode::is_success(responseCode)) {
263: 		ack_handlers.setResult(msg_id);
264: 	} else {
265: 		int error = SYSTEM_ERROR_COAP;
266: 		switch (codeClass) {
267: 		case 4:
268: 			error = SYSTEM_ERROR_COAP_4XX;
269: 			break;
270: 		case 5:
271: 			error = SYSTEM_ERROR_COAP_5XX;
272: 			break;
273: 		}
274: 		ack_handlers.setError(msg_id, error);
275: 	}
566: void Protocol::reset() {
567: #if HAL_PLATFORM_OTA_PROTOCOL_V3
568: 	firmwareUpdate.reset();
569: #else
570: 	chunkedTransfer.reset();
571: #endif
572: 	pinger.reset();
573: 	timesync_.reset();
574: 	description.reset();
575: 	ack_handlers.clear();
576: 	channel.reset();
577: 	subscription_msg_ids.clear();
578: 	v2::CoapChannel::instance()->close();
655: ProtocolError Protocol::event_loop(CoAPMessageType::Enum& message_type)
656: {
657: 	// Process expired completion handlers
658: 	const system_tick_t t = callbacks.millis();
659: 	ack_handlers.update(t - last_ack_handlers_update);
660: 	last_ack_handlers_update = t;
```

### ACK timeout

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/inc/protocol_defs.h`

```cpp
99: const unsigned SEND_EVENT_ACK_TIMEOUT = 20000;
```

### Timeout/abort resolve Future failure

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/services/inc/completion_handler.h`

```cpp
299:     void clear() {
300:         for (Handler& h: handlers_) {
301:             h.handler.setError(SYSTEM_ERROR_ABORTED);
302:         }
303:         handlers_.clear();
304:         timeoutTicks_ = MAX_TIMEOUT;
305:         ticks_ = 0;
331:     int update(system_tick_t ticks) {
332:         if (!handlers_.isEmpty()) {
333:             ticks_ += ticks;
334:             if (ticks_ >= timeoutTicks_) {
335:                 timeoutTicks_ = MAX_TIMEOUT;
336:                 int count = 0; // Number of expired handlers
337:                 int i = 0;
338:                 do {
339:                     Handler& h = handlers_.at(i);
340:                     if (ticks_ >= h.ticks) {
341:                         // Remove expired handler
342:                         CompletionHandler handler = handlers_.takeAt(i).handler;
343:                         handler.setError(SYSTEM_ERROR_TIMEOUT);
344:                         ++count;
```

## Session restore and connected readiness

`Failed to load session data from persistent storage` means the saved session record is absent/invalid; Device OS discards it and uses normal connection/handshake. It does not indicate acceptance or loss of a publish, and is not a resume log. `restoreStatus=COMPLETE` is the restored-DTLS path; it returns SESSION_RESUMED. A restored session with matching application/system state skips HELLO but sends a CoAP ping. With changed state it sends HELLO and relevant description/subscriptions. The system does not set connected immediately upon loading local session data: it waits for pending client handshake messages and checks another communication-loop iteration.

The ping itself is asynchronous, but it is tracked in the pending-client-message store that gates connected. No further general settling interval appears after connected. There is a conditional old-R410 modem TX delay during handshake, described below; that is hardware/firmware-specific, not a universal N-second session policy.

### Missing saved session is a normal fallback

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_cloud_connection.cpp`

```cpp
84: int SessionConnection::load(const ServerAddress& addr)
85: {
86:     using particle::protocol::SessionPersistOpaque;
87: 
88:     SessionPersistOpaque persist;
89: 
90:     int r = Spark_Restore(&persist, sizeof(persist), SparkCallbacks::PERSIST_SESSION, nullptr);
91: 
92:     if (r == sizeof(persist) && persist.is_valid()) {
93:         SessionConnection* connection = (SessionConnection*)persist.connection_data();
94:         if (connection->server_address_checksum == compute_session_checksum(addr) &&
95:             connection->address.ss_family != AF_UNSPEC) {
96:             /* Assume valid */
97:             this->address = connection->address;
98:             LOG(INFO, "Loaded cloud server address and port from session data");
99:         } else {
100:             /* Invalidate */
101:             LOG(ERROR, "Address checksum %08x, expected %08x", connection->server_address_checksum, compute_session_checksum(addr));
102:             LOG(ERROR, "Address family %lu", connection->address.ss_family);
103:             discard();
104:             return -1;
105:         }
106:     } else {
107:         LOG(WARN, "Failed to load session data from persistent storage");
108:         discard();
109:         return -1;
110:     }
111:     return -1;
167: #if HAL_PLATFORM_CLOUD_UDP
168:     g_system_cloud_session_data.load(server_addr);
169: #endif /* HAL_PLATFORM_CLOUD_UDP */
170: 
171:     int r = system_cloud_connect(udp ? IPPROTO_UDP : IPPROTO_TCP, &server_addr,
172: #if HAL_PLATFORM_CLOUD_UDP
173:                                  (sockaddr*)&g_system_cloud_session_data.address);
```

### Complete restore versus handshake

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_message_channel.cpp`

```cpp
405: 	SessionPersist::RestoreStatus restoreStatus = sessionPersist.restore(&ssl_context, renegotiate, keys_checksum, coap_state, callbacks.restore, callbacks.save);
406: 	LOG(INFO,"(CMPL,RENEG,NO_SESS,ERR) restoreStatus=%d", restoreStatus);
407: 	if (restoreStatus==SessionPersist::COMPLETE)
408: 	{
409: 		LOG(INFO,"out_ctr %d,%d,%d,%d,%d,%d,%d,%d, next_coap_id=%x", sessionPersist.out_ctr[0],
410: 				sessionPersist.out_ctr[1],sessionPersist.out_ctr[2],sessionPersist.out_ctr[3],
411: 				sessionPersist.out_ctr[4],sessionPersist.out_ctr[5],sessionPersist.out_ctr[6],
412: 				sessionPersist.out_ctr[7], sessionPersist.next_coap_id);
413: 		sessionPersist.make_persistent();
414: 		LOG(INFO,"restored session from persisted session data. next_msg_id=%d", *coap_state);
415: 		return SESSION_RESUMED;
416: 	}
417: 	else if (restoreStatus==SessionPersist::RENEGOTIATE)
418: 	{
419: 		// session partially restored, fully restored via handshake
420: 	}
421: 	else // no session or clear
422: 	{
423: 		reset_session();
424: 		ProtocolError error = setup_context();
425: 		if (error)
426: 			return error;
427: 	}
428: 	uint8_t random[64];
429: 
430: 	do
431: 	{
432: 		while (ssl_context.state != MBEDTLS_SSL_HANDSHAKE_OVER)
433: 		{
434: 			ret = mbedtls_ssl_handshake_step(&ssl_context);
```

### Session resume sends ping; changed session sends HELLO

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/protocol.cpp`

```cpp
496: 	if (session_resumed) {
497: 		// for now, unconditionally move the session on resumption
498: 		channel.command(MessageChannel::MOVE_SESSION, nullptr);
499: 		uint32_t stateFlags = 0xffffffffu; // Check all flags, not just recognized ones
500: 		if (protocol_flags & ProtocolFlag::DEVICE_INITIATED_DESCRIBE) {
501: 			// The system controls when to send application Describe and subscriptions
502: 			stateFlags &= ~(AppStateDescriptor::APP_DESCRIBE_CRC | AppStateDescriptor::SUBSCRIPTIONS_CRC);
503: 		}
504: 		const auto currentState = app_state_descriptor(stateFlags);
505: 		const auto cachedState = channel.cached_app_state_descriptor();
506: 		if (currentState.equalsTo(cachedState, stateFlags)) {
507: 			LOG(INFO, "Skipping HELLO message");
508: 			error = ping(true);
509: 			if (error != ProtocolError::NO_ERROR) {
510: 				return error;
511: 			}
512: 			return ProtocolError::SESSION_RESUMED; // Not an error
513: 		} else {
514: 			// TODO: For now, make sure application Describe and subscriptions will be sent if the
515: 			// system state has changed
516: 			channel.command(Channel::SAVE_SESSION);
517: 			descriptor.app_state_selector_info(SparkAppStateSelector::ALL, SparkAppStateUpdate::RESET, 0, nullptr);
518: 			channel.command(Channel::LOAD_SESSION);
519: 		}
520: 	}
521: 
522: 	LOG(INFO, "Sending HELLO message");
523: 	error = hello(descriptor.was_ota_upgrade_successful());
524: 	if (error) {
525: 		LOG(ERROR,"Could not send HELLO message: %d", error);
526: 		return error;
527: 	}
528: 
529: 	if (protocol_flags & ProtocolFlag::REQUIRE_HELLO_RESPONSE) {
530: 		LOG(INFO, "Receiving HELLO response");
531: 		error = hello_response();
532: 		if (error) {
533: 			return error;
534: 		}
535: 	}
537: 	LOG(INFO, "Handshake completed");
538: 	channel.notify_established();
539: 
540: 	// An ACK or a response for the Hello message has already been received at this point, so we can
541: 	// update the cached session parameters
542: 	if (descriptor.app_state_selector_info) {
600: 	}
601: #if HAL_PLATFORM_OTA_PROTOCOL_V3
602: 	flags |= HELLO_FLAG_OTA_PROTOCOL_V3;
603: #endif
604: 	size_t len = build_hello(message, flags);
605: 	message.set_length(len);
606: 	message.set_confirm_received(true); // Send synchronously
607: 	last_message_millis = callbacks.millis();
```

### Ping is sent through the CoAP channel

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/inc/protocol.h`

```cpp
228: 	ProtocolError ping(bool forceCoAP=false)
229: 	{
230: 		Message message;
231: 		channel.create(message);
232: 		size_t len = 0;
233: 		if (!forceCoAP && (protocol_flags & ProtocolFlag::PING_AS_EMPTY_MESSAGE)) {
234: 			len = Messages::keep_alive(message.buf());
235: 		}
236: 		else {
237: 			len = Messages::ping(message.buf(), 0);
238: 		}
239: 		last_message_millis = callbacks.millis();
240: 		message.set_length(len);
241: 		return channel.send(message);
242: 	}
```

### Handshake waits on protocol pending messages

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_cloud_internal.cpp`

```cpp
1279:     if (err == protocol::SESSION_RESUMED) {
1280:         // XXX: ideally this event should be generated before we perform the handshake
1281:         // but the current semantic of indicating after the handshake/session-resumption are done
1282:         // also deserves a chance.
1283:         system_notify_event(cloud_status, cloud_status_session_resume);
1284:         session_resumed = true;
1285:     } else if (err != 0) {
1286:         return spark_protocol_to_system_error(err);
1287:     }
1288:     if (!session_resumed) {
1289:         // XXX: ideally this event should be generated before we perform the handshake
1290:         // but the current semantic of indicating after the handshake/session-resumption are done
1291:         // also deserves a chance.
1292:         system_notify_event(cloud_status, cloud_status_handshake);
1302:         spark_protocol_send_time_request(sp);
1303:     } else {
1304:         LOG(INFO,"cloud connected from existing session.");
1305: 
1306:         if (!hal_rtc_time_is_valid(nullptr) && spark_sync_time_last(nullptr, nullptr) == 0) {
1307:             spark_protocol_send_time_request(sp);
1308:         }
1334:     protocol_status status = {};
1335:     status.size = sizeof(status);
1336:     err = spark_protocol_get_status(sp, &status, nullptr);
1337:     if (err != 0) {
1338:         return spark_protocol_to_system_error(err);
1339:     }
1340:     if (status.flags & PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES) {
1341:         SPARK_CLOUD_HANDSHAKE_PENDING = 1;
1342:         LOG(TRACE, "Waiting until all handshake messages are processed by the protocol layer");
1343:     } else {
1344:         SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 1;
1345:     }
1346:     return 0;
690: void clientMessagesProcessed(void* reserved) {
691:     if (SPARK_CLOUD_HANDSHAKE_PENDING) {
692:         SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 1;
693:         SPARK_CLOUD_HANDSHAKE_PENDING = 0;
694:         LOG(INFO, "All handshake messages have been processed");
695: 
696:         // Send vitals with last panic data only once
697:         panic_set_last_panic_data_handled(nullptr);
698:     }
```

### Pending status tracks client requests

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_protocol.h`

```cpp
123: 	int get_status(protocol_status* status) const override {
124: 		SPARK_ASSERT(status);
125: 		status->flags = 0;
126: 		if (channel.has_unacknowledged_client_requests()) {
127: 			status->flags |= PROTOCOL_STATUS_HAS_PENDING_CLIENT_MESSAGES;
128: 		}
129: 		return NO_ERROR;
```

### Connected becomes true after acknowledgment readiness

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_task.cpp`

```cpp
360: void handle_cloud_connection(bool force_events)
361: {
362:     if (SPARK_CLOUD_SOCKETED)
363:     {
364:         if (!SPARK_CLOUD_CONNECTED && !SPARK_CLOUD_HANDSHAKE_PENDING)
365:         {
366:             int err = 0;
367:             if (SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE) {
368:                 // TODO: There's no protocol API to get the current session state, so we're running
369:                 // one more iteration of the communication loop to make sure all handshake messages
370:                 // have been acknowledged successfully
371:                 if (!Spark_Communication_Loop()) {
372:                     err = protocol::MESSAGE_TIMEOUT;
373:                 } else {
374:                     INFO("Cloud connected");
375:                     SPARK_CLOUD_CONNECTED = 1;
376:                     SPARK_CLOUD_HANDSHAKE_NOTIFY_DONE = 0;
377:                     cloud_failed_connection_attempts = 0;
378:                     protocol::v2::CoapChannel::instance()->open();
379:                     CloudDiagnostics::instance()->status(CloudDiagnostics::CONNECTED);
380:                     system_notify_event(cloud_status, cloud_status_connected);
```

## Lower-layer retries and disconnect destruction

Default PRIVATE-only DTLS events still have one original transmission plus up to three internal retries. Retry delays are approximately 4–6 s, then 8–12 s, then 16–24 s; the earliest third retry is around 28 seconds from first send. The interval is computed with `rand()%256`, so the strict upper edge is just below each stated bound. The application queue's “drained” condition does not inspect this store.

The default disconnect setting is non-graceful. The repo uses `Particle.disconnect()` without an explicit graceful option and has no `setDefaultDisconnectOptions`/`graceful` override in `src` (read-only rg search). The non-graceful TERMINATE command resets the protocol and clears the client retransmission store. A graceful disconnect instead waits for confirmable messages; it remains inferior as the sole fix because an application queue already removed an event before a later CoAP rejection/timeout cannot recover it.

Application serial timing supports possibility, not proof: on 15:00, queue-drained/teardown starts about 4.85 seconds after ConnSummary, potentially before the first resend. On 15:22, ledger gating holds teardown until about 21.66 seconds after ConnSummary, still before the earliest third resend for an event first sent at connect. Exact attempt times are not logged. These bounds cannot explain whether an ACK or rejection was received before teardown.

### CoAP retry schedule

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/coap_channel.h`

```cpp
35: const uint16_t ACK_TIMEOUT = 4000;
36: const uint16_t ACK_RANDOM_FACTOR = 1500;
37: const uint16_t ACK_RANDOM_DIVISOR = 1000;
38: const uint8_t MAX_RETRANSMIT = 3;
39: const uint16_t MAX_TRANSMIT_SPAN = 45*1000;
40: 
41: /**
42:  * The number of outstanding messages allowed.
43:  */
44: const uint8_t NSTART = 1;
45: 
46: /**
47:  * Determines the transmit timeout for the given transmission count.
48:  */
49: inline system_tick_t transmit_timeout(uint8_t transmit_count)
50: {
51: 	system_tick_t timeout = (ACK_TIMEOUT << transmit_count);
52: 	timeout += (timeout * (rand() % 256)) >> 9;
53: 	return timeout;
235: 	bool prepare_retransmit(system_tick_t now)
236: 	{
237: 		CoAPType::Enum coapType = CoAP::type(get_data());
238: 		if (coapType==CoAPType::CON) {
239: 			timeout = now + transmit_timeout(transmit_count);
240: 			if (transmit_count == 0) {
241: 				g_trasmittedMessageCounter++;
242: 			}
243: 			else {
244: 				g_retransmittedMessageCounter++;
245: 			}
246: 			transmit_count++;
247: 			return transmit_count <= MAX_RETRANSMIT+1;
248: 		}
249: 		// other message types are not resent on timeout
```

### CoAP confirmable messages get cached

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/coap_channel.cpp`

```cpp
105: ProtocolError CoAPMessageStore::send(Message& msg, system_tick_t time)
106: {
107: 	if (!msg.has_id())
108: 		return MISSING_MESSAGE_ID;
109: 
110: 	CoAPType::Enum coapType = CoAP::type(msg.buf());
111: 	if (coapType==CoAPType::CON || coapType==CoAPType::ACK || coapType==CoAPType::RESET)
112: 	{
113: 		// confirmable message, create a CoAPMessage for this
114: 		CoAPMessage* coapmsg = CoAPMessage::create(msg);
115: 		if (coapmsg==nullptr)
116: 		{
117: 			return INSUFFICIENT_STORAGE;
118: 		}
119: 		if (coapType==CoAPType::CON)
120: 		{
121: 			coapmsg->set_send_time(time);
122: 			coapmsg->prepare_retransmit(time);
123: 		}
124: 		else
125: 		{
126: 			coapmsg->set_expiration(time + MAX_TRANSMIT_SPAN);
127: 		}
128: 		add(*coapmsg);
129: 	}
130: 	return NO_ERROR;
51: bool CoAPMessageStore::retransmit(CoAPMessage* msg, Channel& channel, system_tick_t now)
52: {
53: 	bool retransmit = (msg->prepare_retransmit(now));
54: 	if (retransmit)
55: 	{
56: 		LOG(TRACE, "Retransmitting CoAP message; ID: %d; attempt %d of %d", (int)msg->get_id(),
57: 				(int)msg->get_transmit_count() - 1, (int)MAX_RETRANSMIT);
58: 		send_message(msg, channel);
59: 	}
60: 	return retransmit;
61: }
62: 
63: void CoAPMessageStore::message_timeout(CoAPMessage& msg, Channel& channel)
64: {
65: 	msg.notify_timeout();
66: 	if (msg.is_request()) {
67: 		LOG(ERROR, "CoAP message timeout; ID: %d", (int)msg.get_id());
68: 		g_unacknowledgedMessageCounter++;
69: 		// XXX: This will cancel _all_ messages with a timeout error, not just the timed out one.
70: 		// That's not ideal but should be okay while we're transitioning to the new CoAP API
71: 		v2::CoapChannel::instance()->close(SYSTEM_ERROR_COAP_TIMEOUT);
72: 		channel.command(MessageChannel::CLOSE);
73: 	}
```

### Reliable channel reset discards copies; send stores a copy

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/coap_channel.h`

```cpp
581: 	/**
582: 	 * Clear the message stores.
583: 	 */
584: 	void reset() override
585: 	{
586: 		server.clear();
587: 		client.clear();
588: 		channel::reset();
589: 	}
590: 
591: 	/**
592: 	 * Sends the message reliably. A non-confirmable message
593: 	 * it is sent once. A confirmable message is sent and resent
594: 	 * until an ack is received or the message times out.
595: 	 */
596: 	ProtocolError send(Message& msg) override
597: 	{
598: 		if (msg.send_direct() || msg.passthrough())
599: 			return delegateChannel.send(msg);
600: 
601: 		if (msg.is_request() && msg.get_confirm_received())
602: 			return send_synchronous(msg);
603: 
604: 		// determine the type of message.
605: 		CoAPMessageStore& store = msg.is_request() ? client : server;
606: 		ProtocolError error = store.send(msg, millis());
607: 		if (!error)
608: 			error = channel::send(msg);
609: 		return error;
```

### Non-graceful disconnect is the default

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_cloud_internal.h`

```cpp
135: class CloudConnectionSettings {
136: public:
137:     // Default disconnection settings
138:     static const bool DEFAULT_DISCONNECT_GRACEFULLY = false;
139:     static const unsigned DEFAULT_DISCONNECT_TIMEOUT = 30000;
140:     static const bool DEFAULT_DISCONNECT_CLEAR_SESSION = false;
141:     static const bool DEFAULT_RECONNECT = false;
142: 
143:     CloudConnectionSettings() :
144:             defaultDisconnectTimeout_(DEFAULT_DISCONNECT_TIMEOUT),
145:             defaultDisconnectGracefully_(DEFAULT_DISCONNECT_GRACEFULLY),
146:             defaultDisconnectClearSession_(DEFAULT_DISCONNECT_CLEAR_SESSION),
147:             defaultDisconnectReconnect_(DEFAULT_RECONNECT) {
181:         CloudDisconnectOptions result;
182:         if (pending.isGracefulSet()) {
183:             result.graceful(pending.graceful());
184:         } else {
185:             result.graceful(defaultDisconnectGracefully_);
186:         }
187:         if (pending.isTimeoutSet()) {
188:             result.timeout(pending.timeout());
189:         } else {
190:             result.timeout(defaultDisconnectTimeout_);
```

### Actual repository teardown calls

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/power/Connectivity.h`

```cpp
35: /**
36:  * @brief Requests both a Particle cloud disconnect and radio power-down.
37:  */
38: inline void requestFullDisconnectAndRadioOff() {
39:   Particle.disconnect();
40:   requestRadioPowerOff();
41: }
42: 
43: /**
44:  * @brief Requests a Particle cloud disconnect while leaving radio teardown to the caller.
45:  */
46: inline void requestCloudDisconnectOnly() {
47:   Particle.disconnect();
48: }
```

### Graceful option chooses DISCONNECT versus TERMINATE

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_task.cpp`

```cpp
733:         // Get disconnection options
734:         const auto opts = CloudConnectionSettings::instance()->takePendingDisconnectOptions();
735:         const bool graceful = (flags & CLOUD_DISCONNECT_GRACEFULLY) && opts.graceful();
736:         if (SPARK_CLOUD_CONNECTED) {
737:             if (graceful) {
738:                 // Notify the cloud that we're about to disconnect
739:                 spark_disconnect_command cmd = {};
740:                 cmd.size = sizeof(cmd);
741:                 cmd.cloud_reason = cloudReason;
742:                 cmd.network_reason = networkReason;
743:                 cmd.reset_reason = resetReason;
744:                 cmd.sleep_duration = sleepDuration;
745:                 cmd.timeout = opts.timeout();
746:                 const int r = spark_protocol_command(spark_protocol_instance(), ProtocolCommands::DISCONNECT, 0, &cmd);
747:                 if (r != protocol::NO_ERROR) {
748:                     LOG(WARN, "cloud_disconnect(): DISCONNECT command failed: %d", r);
749:                 }
750:             } else {
751:                 spark_protocol_command(spark_protocol_instance(), ProtocolCommands::TERMINATE, 0, nullptr);
752:             }
```

### DISCONNECT waits; TERMINATE resets immediately

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_protocol.h`

```cpp
84: 		case ProtocolCommands::DISCONNECT: {
85: 			int r = ProtocolError::NO_ERROR;
86: 			unsigned timeout = DEFAULT_DISCONNECT_COMMAND_TIMEOUT;
87: 			if (data) {
88: 				const auto d = (const spark_disconnect_command*)data;
89: 				if (d->timeout != 0) {
90: 					timeout = d->timeout;
91: 				}
92: 				if (d->cloud_reason != CLOUD_DISCONNECT_REASON_NONE) {
93: 					r = send_goodbye((cloud_disconnect_reason)d->cloud_reason, (network_disconnect_reason)d->network_reason,
94: 							(System_Reset_Reason)d->reset_reason, d->sleep_duration);
95: 				}
96: 			}
97: 			if (r == ProtocolError::NO_ERROR) {
98: 				r = wait_confirmable(timeout);
99: 			}
100: 			reset();
101: 			return r;
102: 		}
103: 		case ProtocolCommands::TERMINATE: {
104: 			reset();
105: 			return ProtocolError::NO_ERROR;
```

## Conditional old-modem drop mechanism — excluded for Dev-14 by inventory model

The installed sources contain an explicit silent-success packet-drop path for **SARA-R410 modem firmware <=203**. It drops within a 500-byte/50 ms TX window and returns SYSTEM_ERROR_NONE. Device OS comments describe protecting old R410 firmware against crashes, and handshake inserts 100 ms delay where the modem reports that requirement. The parent investigation subsequently supplied only non-secret inventory fields: Dev-14 has serial prefix P044 and modem firmware 03.15. [Particle serial-prefix documentation](https://docs.particle.io/hardware/best-practices/serial-number/) maps P044 to BRN404X; the [BRN404X datasheet](https://docs.particle.io/reference/datasheets/b-series/brn404x-datasheet/) specifies u-blox SARA-R510S-01B. Thus the old-R410-specific branch does not apply to this board, assuming that inventory association is correct, and must not be advanced as its root cause. Firmware 03.15 is consistent with that R510 model in the [vendor AT manual hosted by Particle](https://docs.particle.io/assets/datasheets/SARA-R5_ATCommands_UBX-19047455.pdf). Generic flow-control handling also converts a GSM0710 flow-control condition to success; no evidence shows that branch executed in these wakes. The exact model distinction matters: BRN404X is R510; BRN404 without X is R410.

### Modem drop window constants

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/network/ncp_client/sara/sara_ncp_client.cpp`

```cpp
103: const auto UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX = 203;
145: const size_t UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD = 500;
146: const system_tick_t UBLOX_NCP_R4_WINDOW_SIZE_MS = 50;
```

### Silent-success drops and flow control

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/hal/network/ncp_client/sara/sara_ncp_client.cpp`

```cpp
522: /*
523: * This is a callback that writes data into muxer channel 2 (data PPP channel)
524: * Whenever we encounter a large packet, we enforce a certain number of ms to pass before
525: * transmitting anything else on this channel. After we send large packet, we drop messages(bytes)
526: * for a certain amount of time defined by UBLOX_NCP_R4_WINDOW_SIZE_MS
527: */
528: int SaraNcpClient::dataChannelWrite(int id, const uint8_t* data, size_t size) {
529:     // Just in case perform some state checks to ensure that LwIP PPP implementation
530:     // does not write into the data channel when it's not supposed do
531:     CHECK_TRUE(connState_ == NcpConnectionState::CONNECTED, SYSTEM_ERROR_INVALID_STATE);
532:     CHECK_FALSE(muxerDataStream_->enabled(), SYSTEM_ERROR_INVALID_STATE);
533: 
534:     if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
535:         if ((millis() - lastWindow_) >= UBLOX_NCP_R4_WINDOW_SIZE_MS) {
536:             const int windowCount = ((millis() - lastWindow_) / UBLOX_NCP_R4_WINDOW_SIZE_MS);
537:             lastWindow_ += UBLOX_NCP_R4_WINDOW_SIZE_MS * windowCount;
538:             bytesInWindow_ = std::max(0, (int)bytesInWindow_ - (int)UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD * windowCount);
539:             if (bytesInWindow_ == 0) {
540:                 lastWindow_ = millis();
541:             }
542:         }
543: 
544:         if (bytesInWindow_ > 0 && (bytesInWindow_ + size) >= UBLOX_NCP_R4_BYTES_PER_WINDOW_THRESHOLD) {
545:             LOG_DEBUG(WARN, "Dropping");
546:             // Not an error
547:             return SYSTEM_ERROR_NONE;
548:         }
549:     }
550: 
551:     int err = gsm0710::GSM0710_ERROR_NONE;
552:     if (!sleepNoPPPWrite_) {
553:         err = muxer_.writeChannel(UBLOX_NCP_PPP_CHANNEL, data, size);
554:     }
555:     if (err == gsm0710::GSM0710_ERROR_FLOW_CONTROL) {
556:         // Not an error
557:         LOG_DEBUG(WARN, "Remote side flow control");
558:         err = 0;
661: int SaraNcpClient::getTxDelayInDataChannel() {
662:     if (ncpId() == PLATFORM_NCP_SARA_R410 && fwVersion_ <= UBLOX_NCP_R4_APP_FW_VERSION_NO_HW_FLOW_CONTROL_MAX) {
663:         return UBLOX_NCP_R4_WINDOW_SIZE_MS * 2;
664:     }
665:     return SYSTEM_ERROR_NONE;
666: }
```

### Conditional handshake TX delay

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_cloud_internal.cpp`

```cpp
1267: #if HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX
1268:     // XXX: Adding a delay only for platforms Boron and BSoM, because older cell versions of
1269:     // Boron R410M modems crash when handshake messages are sent without gap.
1270:     // We have a workaround for this issue in the NCP client, so the modem should no longer crash
1271:     // in any case, but we want to avoid dropping a set of publishes which are generated below,
1272:     // hence a delay here as a workaround.
1273:     auto timeout = cellularNetworkManager()->ncpClient()->getTxDelayInDataChannel();
1274:     if (timeout > 0) {
1275:         HAL_Delay_Milliseconds(timeout);
1276:     }
1277: #endif // HAL_PLATFORM_MUXER_MAY_NEED_DELAY_IN_TX
```

## Diagnostic implications

A diagnostics-only firmware that preserves PRIVATE-only publishes cannot truthfully fill an “ACK succeeded” field from the existing Future. That field must say `unobserved/no-WITH_ACK`, with Future success reported separately. For observing real ACKs without changing publish semantics, enable Device OS `comm.coap` TRACE as well as library TRACE, and correlate send IDs and incoming ACK/code records. Full transport trace can be voluminous and change timing, so retain USB serial and capture a paired lower-verbosity run. The library's one-per-attempt log should record monotonic elapsed since observed connected, original flags, attempt identity/name, Future result/error, and honest ACK visibility.

## Primary documentation cross-check

Particle's current [classic publish API reference](https://docs.particle.io/reference/device-os/api/publish/particle-publish-classic-api-publish/) agrees with this local-source distinction: default UDP messages retain internal retries, NO_ACK disables them, and WITH_ACK makes completion wait for acknowledgment. The installed 6.4.1 source excerpts above are the version-specific evidence; current docs are only a cross-check. No direct quote from the web documentation is reproduced here.

## Follow-up: sleep persistence, rate limiting, and the current webhook gate

The queue registers reset and cloud-status handlers. Both `reset` and `cloud_status_disconnecting` call `writeQueueToFiles()`, which moves the remaining `ramQueue` entries to file storage. This is an attempted persistence path, not blanket RAM-loss-on-disconnect. It cannot resurrect events already deleted after a premature Future success. It also does not persist a RAM `curEvent` that has been popped out of `ramQueue` and is still in flight; exact reset/sleep timing matters. A plain hardware reset, flash/reset, or HIBERNATE loss must not be conflated with ordinary software reset/disconnect notification. The file writer has weak error handling, separately documented by the library reviewer.

Device OS System.sleep's `CLOUD_DISCONNECT_GRACEFULLY` flag only permits graceful disconnection; its source explicitly says the actual option controls whether it happens. `WAIT_CLOUD` can force that option while connected. The repository manually requests default Particle.disconnect before its hardware sleep, so the already-terminated protocol is not repaired by a later sleep call.

WITH_ACK does not impose a new local publisher-rate algorithm. The classic publisher checks rate before constructing/sending an event and keeps five recent call timestamps, allowing the stated burst of four events per second. A failed or ultimately unacknowledged publish still occupies that call's slot; later failure does not undo its timestamp. CoAP's internal retransmissions call the message channel directly and do not re-enter Publisher::send_event, so they do not consume this local application-publish rate limit again. A queue-level retry is a new Particle.publish call and does count, but this queue waits 30 seconds after failure and 1 second after success, plus serializes its in-flight event. B's explicit WITH_ACK is therefore not expected to newly trigger this local rate limiter; it may lower throughput and extend awake time during bad connectivity. It does not alter the 800-file cap; sustained outages can still overflow that cap. Cloud account limits were not evaluated here.

**Important causal limitation:** In the literal current working tree, report enqueue arms global webhook supervision; the sleep gate requires `!session.awaitingWebhookResponse`. The supplied 15:00 capture disconnects about 4.85 seconds after ConnSummary with no displayed webhook timeout. A nonempty subscription response clears this single global boolean without correlating a specific report identity. A delayed/other response, missing logs/state changes, or a binary/source mismatch could reconcile this behavior, but none is proven. The premature-delete defect remains real; claiming the recorded disconnect definitively cut off those particular unacknowledged reports would exceed the evidence. There are two separate questions: why initial event delivery failed, and why the application stopped retaining/retrying it.

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.cpp`

```cpp
50:     // Register a system reset handler
51:     System.on(reset | cloud_status, systemEventHandler);
121: void PublishQueuePosix::writeQueueToFiles() {
122: 
123:     WITH_LOCK(*this) {
124:         while(!ramQueue.empty()) {
125:             PublishQueueEvent *event = ramQueue.front();
126:             ramQueue.pop_front();
127: 
128:             int fileNum = fileQueue.reserveFile();
402: void PublishQueuePosix::systemEventHandler(system_event_t event, int param) {
403:     if ((event == reset) || ((event == cloud_status) && (param == cloud_status_disconnecting))) {
404:         _log.trace("reset or disconnect event, save files to queue");
405:         PublishQueuePosix::instance().writeQueueToFiles();
406:     }
```

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/lib/PublishQueuePosixRK/src/PublishQueuePosixRK.h`

```cpp
414:     unsigned long waitAfterConnect = 2000; //!< time to wait after Particle.connected() before publishing
415:     unsigned long waitBetweenPublish = 1000; //!< how long to wait in milliseconds between publishes
416:     unsigned long waitAfterFailure = 30000; //!< how long to wait after failing to publish before trying again
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/system/src/system_sleep.cpp`

```cpp
135:         // This flag seems to be redundant in the presence of Particle.setDisconnectOptions() and
136:         // Particle.disconnect()
137:         if (configHelper.sleepFlags().isSet(SystemSleepFlag::WAIT_CLOUD) && spark_cloud_flag_connected()) {
138:             auto opts = CloudConnectionSettings::instance()->takePendingDisconnectOptions();
139:             opts.graceful(true);
140:             CloudConnectionSettings::instance()->setPendingDisconnectOptions(std::move(opts));
141:         }
142:         // The CLOUD_DISCONNECT_GRACEFULLY flag doesn't really enable graceful disconnection mode,
143:         // it merely indicates that it is ok to disconnect gracefully in this specific case.
144:         // Whether the system will actually disconnect from the cloud gracefully or not depends
145:         // on the disconnection options that can be set via Particle.setDisconnectOptions() or
146:         // Particle.disconnect().
147:         //
148:         // TODO: Rename CLOUD_DISCONNECT_GRACEFULLY to CLOUD_DISCONNECT_IMMEDIATELY and invert its
149:         // meaning to avoid confusion
150:         cloud_disconnect(CLOUD_DISCONNECT_GRACEFULLY, CLOUD_DISCONNECT_REASON_SLEEP, NETWORK_DISCONNECT_REASON_NONE,
151:                 RESET_REASON_NONE, duration);
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.cpp`

```cpp
34: ProtocolError Publisher::send_event(MessageChannel& channel, const char* event_name, const char* data, size_t data_size,
35:         int content_type, int ttl, int flags, system_tick_t time, CompletionHandler handler) {
36:     bool is_system_event = is_system(event_name);
37:     bool rate_limited = is_rate_limited(is_system_event, time);
38:     if (rate_limited) {
39:         g_rateLimitedEventsCounter++;
40:         return BANDWIDTH_EXCEEDED;
41:     }
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/publisher.h`

```cpp
70: 		else
71: 		{
72: 			static system_tick_t recent_event_ticks[5] =
73: 			{ (system_tick_t) -1000, (system_tick_t) -1000,
74: 					(system_tick_t) -1000, (system_tick_t) -1000,
75: 					(system_tick_t) -1000 };
76: 			static int evt_tick_idx = 0;
77: 
78: 			system_tick_t now = recent_event_ticks[evt_tick_idx] = millis;
79: 			evt_tick_idx++;
80: 			evt_tick_idx %= 5;
81: 			if (now - recent_event_ticks[evt_tick_idx] < 1000)
82: 			{
83: 				// exceeded allowable burst of 4 events per second
84: 				return true;
85: 			}
86: 		}
87: 		return false;
```

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/Generalized-Core-Counter.cpp`

```cpp
2091:   // Arm short-term webhook supervision only if publish succeeded.
2092:   // If we're already connected, start the 20s window immediately.
2093:   // Otherwise start it when CONNECTING_STATE reports a successful cloud connect.
2094:   if (queued) {
2095:     session.webhookExpectedOnConnect = true;
2096:     if (Particle.connected()) {
2097:       session.webhookExpectedOnConnect = false;
2098:       session.awaitingWebhookResponse = true;
2099:       session.webhookAwaitStartMs = millis();
2100:     }
2101:   } else {
2102:     Log.info("Webhook supervision not armed: publish queue rejected");
2103:   }
2384:   // Handle response from Ubidots webhook (legacy integration)
2385:   char responseString[64];
2386:   bool responseOk = true;
2387:   // Response is expected to be a single numeric code from the Particle
2388:   // integration response template (e.g. "200" or "201").
2389:   if (!data || !strlen(data)) {
2390:     snprintf(responseString, sizeof(responseString), "No Data");
2391:     responseOk = false;
2392:     Log.warn("Webhook response empty");
2393:   } else {
2394:     // Any webhook response indicates the integration path is alive.
2395:     // Update lastHookResponse even if it arrived after our short-term window.
2409:     if (session.awaitingWebhookResponse) {
2410:       session.awaitingWebhookResponse = false;
2411:       session.webhookAwaitStartMs = 0;
2412:     }
```

`/Users/chipmc/Documents/Maker/Particle/Projects/Generalized-Core-Counter/src/state/State_Sleep.cpp`

```cpp
483:     bool outputLedgersSynced = !Cloud::instance().hasPendingOutputLedgerSync();
484:     Cloud::LedgerSyncDiagnostics ledgerDiagnostics = Cloud::instance().ledgerSyncDiagnostics();
485:   #if !ENABLE_GATE_TRACE
486:     (void)ledgerDiagnostics;
487:   #endif
488:     bool ledgersSynced = configLedgersSynced && outputLedgersSynced;
489:     bool updatesChecked = !System.updatesPending();
490:     bool webhookConfirmed = !session.awaitingWebhookResponse;
511:     bool allComplete = queueEmpty && ledgersSynced && updatesChecked && webhookConfirmed;
```

## Runtime CoAP trace sufficiency

A `{"comm.coap", LOG_LEVEL_TRACE}` serial filter present from boot is sufficient in this Device OS source; Protocol::begin reads the runtime filter and enables channel debug. Protocol forces compile level ALL, DTLS undefines its compile level and logging.h defaults to ALL. CoAP trace prints message type, code, URI, encoded size, token and message ID; for events URI is `/E/<event name>`, enabling send-to-ACK correlation by ID without payload logging. The default logPayload argument is false. Library attempt sequence and payload length/hash should provide logical-event identity when repeated event names occur. An A-patch summary field should retain `ACK unobserved` for PRIVATE-only Futures and let external analysis derive actual ACK status from CoAP trace.

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/protocol.cpp`

```cpp
20: #undef LOG_COMPILE_TIME_LEVEL
21: #define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL
486: 	bool debug_enabled = LOG_ENABLED_C(TRACE, COAP_LOG_CATEGORY);
487: 	channel.set_debug_enabled(debug_enabled);
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/dtls_message_channel.cpp`

```cpp
20: #undef LOG_COMPILE_TIME_LEVEL
21: 
22: #include "logging.h"
23: 
24: LOG_SOURCE_CATEGORY("comm.dtls")
507: 		if (debug_enabled) {
508: 			LOG_C(TRACE, COAP_LOG_CATEGORY, "Received CoAP message");
509: 			logCoapMessage(LOG_LEVEL_TRACE, COAP_LOG_CATEGORY, (const char*)message.buf(), message.length());
541: 	if (debug_enabled) {
542: 		LOG_C(TRACE, COAP_LOG_CATEGORY, "Sending CoAP message");
543: 		logCoapMessage(LOG_LEVEL_TRACE, COAP_LOG_CATEGORY, (const char*)message.buf(), message.length());
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/services/inc/logging.h`

```cpp
231: #ifndef LOG_COMPILE_TIME_LEVEL
232: #define LOG_COMPILE_TIME_LEVEL LOG_LEVEL_ALL
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/src/coap_util.cpp`

```cpp
212:     _LOG_ATTR_INIT(attr);
213:     log_message(level, category, &attr, nullptr /* reserved */, "%s %s %s size=%u token=%s id=%u", type, code, uri,
214:             (unsigned)size, token, (unsigned)d.id());
215:     if (logPayload && d.hasPayload()) {
216:         log_printf(level, category, nullptr, "Payload (%u bytes): ", (unsigned)d.payloadSize());
217:         log_dump(level, category, d.payload(), d.payloadSize(), 0 /* flags */, nullptr /* reserved */);
218:         log_write(level, category, "\r\n", 2 /* size */, nullptr /* reserved */);
```

`/Users/chipmc/.particle/toolchains/deviceOS/6.4.1/communication/inc/coap_util.h`

```cpp
284: void logCoapMessage(LogLevel level, const char* category, const char* data, size_t size, bool logPayload = false);
```

## Independent source-only review of drafted bench diffs

Reviewed `BENCH-A-publish-diagnostics.diff` and `BENCH-B-explicit-ack.diff` without applying or building them. No source-level API incompatibility found in the versions reviewed: `PublishFlags(NO_ACK)` is the public flag constructor; `~`, `&`, and `|` are supported and preserve the uint8 flag representation; both `Flag` and `Flags` expose `.value()`; Future supports `isDone`, `isSucceeded`, and `error().type()`. B correctly clears NO_ACK before ORing WITH_ACK, and normalizing at dispatch covers already-persisted PRIVATE-only events. See Device OS `wiring/inc/spark_wiring_flags.h:53-70,122-124,143-167` and `wiring/inc/spark_wiring_async.h:480-497`.

A's `ack=not-observed` for PRIVATE-only Future results is honest. B's successful explicit-ACK Future supports `ack=confirmed`, referring only to successful Particle CoAP reply. Diagnostics add timing/output overhead; “no delivery behavior change” describes flags/state logic, not zero perturbation. Result logs execute before the original completion callback, slightly delaying queue advancement. Neither patch corrects the library's pre-existing dispatch-rejection stall; it now logs it. This preserves A's diagnostic-only intent.

**Timing qualification:** `System.on(cloud_status, ...)` callbacks execute asynchronously on the application thread, while connected is set in the system thread. The logged connection age is elapsed since *observed callback time*, which may be later than the true transition. It is not an exact device-system timestamp of SPARK_CLOUD_CONNECTED. The setup fallback for an already-connected worker is likewise approximate. For N-second threshold analysis, use observed connection time consistently and retain ConnSummary/CoAP chronology; callback delivery latency is an uncertainty. The draft author was advised to label this explicitly (`cObs` or equivalent).

**Registration:** setup registration is thread-safe; the returned SystemEventSubscription is a handle with no destructor-unsubscribe, so ignoring it does not immediately cancel the handler. The callback and its state are static and atomic between application/publisher threads. Current repository starts this worker once. Repeated stop/start would register duplicate subscriptions because the new registration is inside `if(!thread)`; retaining a single subscription/registration guard would improve robustness but is not a failure in the present boot path. System.on allocation failure is currently unchecked; a zero observed-time marker prevents pretending an event callback was seen, except for the explicit already-connected fallback. The draft author was alerted.

No compiler, linker or on-device execution validation was performed under this read-only/draft authorization. C++ source review does not certify a bench build.
