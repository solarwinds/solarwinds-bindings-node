#include "bindings.h"

//
// Create an event using oboe's context.
//
Event::Event() {
    oboe_status = oboe_event_init(&event, oboe_context_get(), NULL);
}

//
// Create an event using the specified metadata instead of
// oboe's context. Optionally add an edge to the metadata's
// op ID.
//
Event::Event(const oboe_metadata_t* md, bool addEdge) {
  if (addEdge) {
    // all this does is call oboe_event_init() followed by
    // oboe_event_add_edge().
    oboe_status = oboe_metadata_create_event(md, &event);
  } else {
    // this copies md to the event metadata except for the opId.
    // It creates a new random opId for the event.
    oboe_status = oboe_event_init(&event, md, NULL);
  }
}

Event::~Event() {
    oboe_event_destroy(&event);
}

Nan::Persistent<v8::FunctionTemplate> Event::constructor;

/**
 * Run at module initialization. Make the constructor and export JavaScript
 * properties and function.
 */
NAN_MODULE_INIT(Event::Init) {
    v8::Local<v8::FunctionTemplate> ctor = Nan::New<v8::FunctionTemplate>(Event::New);
    constructor.Reset(ctor);
    ctor->InstanceTemplate()->SetInternalFieldCount(1);
    ctor->SetClassName(Nan::New("Event").ToLocalChecked());

    Nan::SetPrototypeMethod(ctor, "addInfo", Event::addInfo);
    Nan::SetPrototypeMethod(ctor, "addEdge", Event::addEdge);
    Nan::SetPrototypeMethod(ctor, "getMetadata", Event::getMetadata);
    Nan::SetPrototypeMethod(ctor, "toString", Event::toString);
    Nan::SetPrototypeMethod(ctor, "getSampleFlag", Event::getSampleFlag);
    Nan::SetPrototypeMethod(ctor, "setSampleFlagTo", Event::setSampleFlagTo);

    target->Set(Nan::New("Event").ToLocalChecked(), ctor->GetFunction());
}

/**
 * JavaScript constructor
 *
 * new Event()
 * new Event(xtrace, addEdge)
 *
 * @param {Metadata|Event|string} xtrace - X-Trace ID to use for creating event
 * @param boolean [addEdge]
 *
 */
NAN_METHOD(Event::New) {
    // throw an error if constructor is called as a function without "new"
    if(!info.IsConstructCall()) {
        return Nan::ThrowError(Nan::New("Event::New - called without new keyword").ToLocalChecked());
    }

    Event* event;
    oboe_metadata_t* mdp;
    oboe_metadata_t converted_md;

    // Handle the no argument signature.
    if (info.Length() == 0) {
        event = new Event();
        event->Wrap(info.This());
        info.GetReturnValue().Set(info.This());
        return;
    }
    // Now handle the Metadata, Event, or String signatures. All have an optional
    // boolean (default true) to add an edge to the event (points to op ID in metadata).
    if (Metadata::isMetadata(info[0])) {
       Metadata* md = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());
       mdp = &md->metadata;
    } else if (info[0]->IsExternal()) {
        // this is only used by other C++ methods, not JavaScript. They must pass
        // a pointer to an oboe_metadata_t structure.
        mdp = static_cast<oboe_metadata_t*>(info[0].As<v8::External>()->Value());
    } else if (Event::isEvent(info[0])) {
        Event* e = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());
        mdp = &e->event.metadata;
    } else if (info[0]->IsString()) {
        Nan::Utf8String xtrace(info[0]);
        int status = oboe_metadata_fromstr(&converted_md, *xtrace, xtrace.length());
        if (status != 0) {
            Nan::ThrowError(Nan::New("Event::New - invalid X-Trace ID string").ToLocalChecked());
            return;
        }
        mdp = &converted_md;
    } else {
        Nan::ThrowTypeError(Nan::New("Event::New - invalid argument").ToLocalChecked());
        return;
    }

    // handle adding an edge default or specified explicitly?
    bool add_edge = true;
    if (info.Length() >= 2 && info[1]->IsBoolean()) {
        add_edge = info[1]->BooleanValue();
    }

    // now make the event using the metadata specified. in no case is
    // metadata allocated so there is no need to delete it.
    event = new Event(mdp, add_edge);
    event->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
}

/**
 * C++ callable function to create a JavaScript Metadata object.
 *
 * This signature includes a boolean for whether an edge is set or not.
 */
v8::Local<v8::Object> Event::NewInstance(Metadata* md, bool edge) {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 2;
    v8::Local<v8::Value> argv[argc] = {
        Nan::New<v8::External>(&md->metadata),
        Nan::New(edge)
    };
    v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
    // Now invoke the JavaScript callable constructor (Event::New).
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

    return scope.Escape(instance);
}


/**
 * C++ callable function to create a JavaScript Event object.
 */
v8::Local<v8::Object> Event::NewInstance(Metadata* md) {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 1;
    v8::Local<v8::Value> argv[argc] = { Nan::New<v8::External>(&md->metadata) };
    v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
    // Now invoke the JavaScript callable constructor (Event::New).
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

    return scope.Escape(instance);
}

/**
 * C++ callable function to create a JavaScript Event object.
 */
v8::Local<v8::Object> Event::NewInstance() {
    Nan::EscapableHandleScope scope;

    const unsigned argc = 0;
    v8::Local<v8::Value> argv[argc] = {};
    v8::Local<v8::Function> cons = Nan::New<v8::FunctionTemplate>(constructor)->GetFunction();
    v8::Local<v8::Object> instance = cons->NewInstance(argc, argv);

    return scope.Escape(instance);
}


NAN_METHOD(Event::toString) {
    // Unwrap the Event instance from V8 and get the metadata reference.
    oboe_metadata_t* md = &Nan::ObjectWrap::Unwrap<Event>(info.This())->event.metadata;
    char buf[OBOE_MAX_METADATA_PACK_LEN];

    int rc;
    // TODO consider accepting an argument to select upper or lower case.
    if (info.Length() == 1 && info[0]->ToBoolean()->BooleanValue()) {
      rc = Metadata::format(md, sizeof(buf), buf) ? 0 : -1;
    } else {
      rc = oboe_metadata_tostr(md, buf, sizeof(buf) - 1);
    }

    info.GetReturnValue().Set(Nan::New(rc == 0 ? buf : "").ToLocalChecked());
}

/**
 * JavaScript callable method to set the event's sample flag to the boolean
 * argument.
 *
 * returns the previous value of the flag.
 */
NAN_METHOD(Event::setSampleFlagTo) {
    if (info.Length() != 1) {
        return Nan::ThrowError("setSampleFlagTo requires one argument");
    }

    Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());
    bool previous = self->event.metadata.flags & XTR_FLAGS_SAMPLED;

    // truthy to set it, falsey to clear it
    if (info[0]->ToBoolean()->BooleanValue()) {
        self->event.metadata.flags |= XTR_FLAGS_SAMPLED;
    } else {
        self->event.metadata.flags &= ~XTR_FLAGS_SAMPLED;
    }
    info.GetReturnValue().Set(Nan::New(previous));
}

/**
 * JavaScript callable method to get the sample flag from
 * the event metadata.
 *
 * returns boolean
 */
NAN_METHOD(Event::getSampleFlag) {
    Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

    bool sampleFlag = self->event.metadata.flags & XTR_FLAGS_SAMPLED;
    info.GetReturnValue().Set(Nan::New(sampleFlag));
}

/**
 * JavaScript callable method to get a new metadata object with
 * the same metadata as this event.
 *
 * Event.getMetadata()
 *
 * Return a Metadata object with the metadata from this event.
 */
NAN_METHOD(Event::getMetadata) {
    Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());

    oboe_event_t* event = &self->event;
    Metadata* metadata = new Metadata(&event->metadata);

    info.GetReturnValue().Set(Metadata::NewInstance(metadata));
    delete metadata;
}

/**
 * JavaScript callable method to add an edge to the event.
 *
 * event.addEdge(edge)
 *
 * @param {Metadata | string} X-Trace ID to edge back to
 */
NAN_METHOD(Event::addEdge) {
    // Validate arguments
    if (info.Length() != 1) {
        return Nan::ThrowError("Invalid signature for event.addEdge");
    }

    // Unwrap event instance from V8
    Event* self = Nan::ObjectWrap::Unwrap<Event>(info.This());
    int status;

    if (Event::isEvent(info[0])) {
        Event* e = Nan::ObjectWrap::Unwrap<Event>(info[0]->ToObject());
        status = oboe_event_add_edge(&self->event, &e->event.metadata);
    } else if (Metadata::isMetadata(info[0])) {
        Metadata* md = Nan::ObjectWrap::Unwrap<Metadata>(info[0]->ToObject());
        status = oboe_event_add_edge(&self->event, &md->metadata);
    } else if (info[0]->IsString()) {
        Nan::Utf8String md_string(info[0]);
        status = oboe_event_add_edge_fromstr(&self->event, *md_string, md_string.length());
    } else {
        return Nan::ThrowTypeError("event.addEdge requires a metadata or event instance or string");
    }

    if (status < 0) {
        return Nan::ThrowError("Failed to add edge");
    }
}

/**
 * JavaScript method to add info to the event.
 *
 * event.addInfo(key, value)
 *
 * @param {string} key
 * @param {string | number | boolean} value
 */
NAN_METHOD(Event::addInfo) {
    // Validate arguments
    if (info.Length() != 2) {
        return Nan::ThrowError("Wrong number of arguments");
    }
    if (!info[0]->IsString()) {
        return Nan::ThrowTypeError("Key must be a string");
    }
    if (!info[1]->IsString() && !info[1]->IsNumber() && !info[1]->IsBoolean()) {
        return Nan::ThrowTypeError("Value must be a boolean, string or number");
    }

    // Unwrap event instance from V8
    Event* self = ObjectWrap::Unwrap<Event>(info.This());
    oboe_event_t* event = &self->event;

    // Get key string from arguments and prepare a status variable
    Nan::Utf8String key(info[0]);
    int status;

    // Handle integer values
    if (info[1]->IsBoolean()) {
        bool val = info[1]->BooleanValue();
        status = oboe_event_add_info_bool(event, *key, val);

    // Handle double values
    } else if (info[1]->IsInt32()) {
        int64_t val = info[1]->Int32Value();
        status = oboe_event_add_info_int64(event, *key, val);

    // Handle double values
    } else if (info[1]->IsNumber()) {
        const double val = info[1]->NumberValue();
        status = oboe_event_add_info_double(event, *key, val);

    // Handle string values
    } else {
        // Get value string from arguments
        Nan::Utf8String value(info[1]);

        // Detect if we should add as binary or a string
        // TODO evaluate using buffers for binary data...
        if (memchr(*value, '\0', value.length())) {
            status = oboe_event_add_info_binary(event, *key, *value, value.length());
        } else {
            status = oboe_event_add_info(event, *key, *value);
        }
    }

    if (status < 0) {
        return Nan::ThrowError("Failed to add info");
    }
}

/**
 * C++ callable method to determine if object is a JavaScript Event
 * instance.
 *
 */
bool Event::isEvent(v8::Local<v8::Value> object) {
    return Nan::New(Event::constructor)->HasInstance(object);
}
