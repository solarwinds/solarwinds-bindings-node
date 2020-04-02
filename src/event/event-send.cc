#include "bindings.h"

//
// Send an event to the reporter
//
Napi::Value Event::sendReport(const Napi::CallbackInfo& info) {
  int status = send_event_x(OBOE_SEND_EVENT);
  return Napi::Number::New(info.Env(), status);
}

//
// send status. only used for init message.
//
Napi::Value Event::sendStatus(const Napi::CallbackInfo& info) {
  int status = send_event_x(OBOE_SEND_STATUS);
  return Napi::Number::New(info.Env(), status);
}

//
// Common code for sendReport and sendStatus.
//
int Event::send_event_x(int channel) {
  // fake up metadata so oboe can check it. change the op_id so it doesn't
  // match the event's in oboe's check.
  oboe_metadata_t omd = this->event.metadata;
  omd.ids.op_id[0] += 1;

  int status;

  status = oboe_event_add_timestamp(&this->event);
  if (status < 0) {
    return -1000;
  }
  status = oboe_event_add_hostname(&this->event);
  if (status < 0) {
    return -1001;
  }

  // finalize the bson buffer and send the message
  this->event.bb_str = oboe_bson_buffer_finish(&this->event.bbuf);
  if (!this->event.bb_str) {
    return -1002;
  }
  size_t len = this->event.bbuf.cur - this->event.bbuf.buf;

  status = oboe_raw_send(channel, this->event.bb_str, len);

  return status;
}
