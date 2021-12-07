#include "bindings.h"

// local function definition.
static int format(oboe_metadata_t* md, size_t len, char* buffer, uint flags);

//
// Convert an event's metadata to a string representation.
//
Napi::Value Event::toString(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  char buf[Event::fmtBufferSize];

  int rc;
  // no args is non-human-readable form - no delimiters, uppercase
  // if arg == 1 it's the original human readable form.
  // otherwise arg is a bitmask of options.
  if (info.Length() == 0) {
    rc = oboe_metadata_tostr(&this->event.metadata, buf, sizeof(buf) - 1);
  } else {
    int style = info[0].ToNumber().Int64Value();
    int flags;
    // make style 1 the previous default because ff_header alone is not very
    // useful.
    if (style == 1) {
      flags = Event::ff_header | Event::ff_task | Event::ff_op |
              Event::ff_flags | Event::ff_separators | Event::ff_lowercase;
    } else {
      // the style are the flags and the separator is a dash.
      flags = style;
    }

    rc = format(&this->event.metadata, sizeof(buf), buf, flags) ? 0 : -1;
  }

  return Napi::String::New(env, rc == 0 ? buf : "");
}

//
// core formatting code
//
//
// function to format an x-trace with components split by sep.
//
int format(oboe_metadata_t* md, size_t len, char* buffer, uint flags) {
  char* b = buffer;
  // when in type is 1 - everything is lowercase (w3c)
  char base = (md->type == 1 || flags & Event::ff_lowercase ? 'a' : 'A') - 10;
  const char sep = '-';

  auto puthex = [&b, base](uint8_t byte) {
    int digit = (byte >> 4);
    digit += digit <= 9 ? '0' : base;
    *b++ = (char)digit;
    digit = byte & 0xF;
    digit += digit <= 9 ? '0' : base;
    *b++ = (char)digit;

    return b;
  };

  // make sure there is enough room in the buffer.
  // prefix + task + op + flags + 3 colons + null terminator.
  if (2 + md->task_len + md->op_len + 2 + 4 > len)
    return false;

  const bool separators = flags & Event::ff_separators;

  if (flags & Event::ff_header) {
    if (md->type == 1) {
      b = puthex(0x00);
    } else {
      b = puthex(0x2b);
    }
    // only add a separator if more fields will be output.
    const uint more =
        Event::ff_task | Event::ff_op | Event::ff_flags | Event::ff_sample;
    if (flags & more && separators) {
      *b++ = sep;
    }
  }

  // place to hold pointer to each byte of task and/or op ids.
  uint8_t* mdp;

  // put the task ID
  if (flags & Event::ff_task) {
    mdp = (uint8_t*)&md->ids.task_id;
    for (unsigned int i = 0; i < md->task_len; i++) {
      b = puthex(*mdp++);
    }
    if (flags & (Event::ff_op | Event::ff_flags | Event::ff_sample) &&
        separators) {
      *b++ = sep;
    }
  }

  // put the op ID
  if (flags & Event::ff_op) {
    mdp = (uint8_t*)&md->ids.op_id;
    for (unsigned int i = 0; i < md->op_len; i++) {
      b = puthex(*mdp++);
    }
    if (flags & (Event::ff_flags | Event::ff_sample) && separators) {
      *b++ = sep;
    }
  }

  // put the flags byte or sample flag only. flags, if specified, takes
  // precedence over sample.
  if (flags & Event::ff_flags) {
    b = puthex(md->flags);
  } else if (flags & Event::ff_sample) {
    *b++ = (md->flags & 1) + '0';
  }

  // null terminate the string
  *b++ = '\0';

  // return bytes written
  return b - buffer;
}
