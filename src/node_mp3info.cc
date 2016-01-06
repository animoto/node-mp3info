/*
 * Copyright (c) 2013, Marek Będkowski <marek@bedkowski.pl>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include <v8.h>
#include <uv.h>
#include <node.h>
#include <node_buffer.h>
#include "node_mp3info.h"
//#include "debug.h"
#include "nan.h"

using namespace v8;
using namespace node;

namespace ns_mp3info {

char* stringArgToStr(const v8::Local<v8::Value> arg) {
  v8::String::Utf8Value v8Str(arg);
  char *cStr = (char*) malloc(strlen(*v8Str) + 1);
  strcpy(cStr, *v8Str);
  return cStr;
}

NAN_METHOD(node_get_mp3_info) {

    char *filename = stringArgToStr(info[0]);

    mp3info_loop_data* data = new mp3info_loop_data;
    data->filename = filename;

    data->callback.Reset(info[1].As<Function>());
    data->scantype = SCAN_QUICK;
    data->fullscan_vbr = VBR_AVERAGE;

    data->req.data = data;

    uv_queue_work(uv_default_loop(), &data->req,
        node_get_mp3info_async,
        (uv_after_work_cb)node_get_mp3info_after);


    return;
}

void node_get_mp3info_async (uv_work_t *req) {
    mp3info_loop_data* data = (mp3info_loop_data*) req->data;

    FILE  *fp;

    data->error = NULL;
    if ( !( fp=fopen(data->filename, "rb") ) ) {
        data->error = new std::string("File not found.");
    } else {
        mp3info* mp3 = new mp3info;

        mp3->filename=data->filename;
        mp3->file=fp;

        get_mp3_info(mp3, data->scantype, data->fullscan_vbr);
        fclose(fp);
        data->mp3 = mp3;
    }
}

void node_get_mp3info_after (uv_work_t *req) {
    mp3info_loop_data* data = (mp3info_loop_data*) req->data;

    Nan::TryCatch try_catch;
    Local<Object> o = Nan::New<Object>();
    Handle<Value> argv[2];
    if (data->error) {
        o->Set(Nan::New<String>("msg").ToLocalChecked(), Nan::New<String>(data->error->c_str(), data->error->length()).ToLocalChecked());

        argv[0] = o;
        argv[1] = Nan::Undefined();
    } else {
        mp3info* mp3 = data->mp3;
        o->Set(Nan::New<String>("length").ToLocalChecked(), Nan::New<Integer>(mp3->seconds));
        {
            Local<Object> id3 = Nan::New<Object>();
            id3->Set(Nan::New<String>("title").ToLocalChecked(), Nan::New<String>(mp3->id3.title).ToLocalChecked());
            id3->Set(Nan::New<String>("artist").ToLocalChecked(), Nan::New<String>(mp3->id3.artist).ToLocalChecked());
            id3->Set(Nan::New<String>("album").ToLocalChecked(), Nan::New<String>(mp3->id3.album).ToLocalChecked());
            id3->Set(Nan::New<String>("year").ToLocalChecked(), Nan::New<String>(mp3->id3.year).ToLocalChecked());
            id3->Set(Nan::New<String>("comment").ToLocalChecked(), Nan::New<String>(mp3->id3.comment).ToLocalChecked());
            o->Set(Nan::New<String>("id3").ToLocalChecked(), id3);
        }
        {
            Local<Object> header = Nan::New<Object>();

            header->Set(Nan::New<String>("sync").ToLocalChecked(), Nan::New<Number>(mp3->header.sync));
            header->Set(Nan::New<String>("version").ToLocalChecked(), Nan::New<Integer>(mp3->header.version));
            header->Set(Nan::New<String>("layer").ToLocalChecked(), Nan::New<Integer>(mp3->header.layer));
            header->Set(Nan::New<String>("crc").ToLocalChecked(), Nan::New<Integer>(mp3->header.crc));
            header->Set(Nan::New<String>("bitrate").ToLocalChecked(), Nan::New<Integer>(mp3->header.bitrate));
            header->Set(Nan::New<String>("freq").ToLocalChecked(), Nan::New<Integer>(mp3->header.freq));
            header->Set(Nan::New<String>("padding").ToLocalChecked(), Nan::New<Integer>(mp3->header.padding));
            header->Set(Nan::New<String>("extension").ToLocalChecked(), Nan::New<Integer>(mp3->header.extension));
            header->Set(Nan::New<String>("mode").ToLocalChecked(), Nan::New<Integer>(mp3->header.mode));
            header->Set(Nan::New<String>("mode_extension").ToLocalChecked(), Nan::New<Integer>(mp3->header.mode_extension));
            header->Set(Nan::New<String>("copyright").ToLocalChecked(), Nan::New<Integer>(mp3->header.copyright));
            header->Set(Nan::New<String>("original").ToLocalChecked(), Nan::New<Integer>(mp3->header.original));
            header->Set(Nan::New<String>("emphasis").ToLocalChecked(), Nan::New<Integer>(mp3->header.emphasis));

            o->Set(Nan::New<String>("header").ToLocalChecked(), header);
        }
        argv[0] = Nan::Undefined();
        argv[1] = o;
    }

    Nan::New(data->callback)->Call(Nan::GetCurrentContext()->Global(), 2, argv);
    data->callback.Reset();
    delete data;

    if (try_catch.HasCaught()) {
        FatalException(try_catch);
    }
}


void InitMP3INFO(Handle<Object> target) {
#define CONST_INT(value) \
  target->ForceSet(Nan::New<String>(#value).ToLocalChecked(), Nan::New<Integer>(value), \
      static_cast<PropertyAttribute>(ReadOnly|DontDelete));

    CONST_INT(SCAN_NONE);
    CONST_INT(SCAN_QUICK);
    CONST_INT(SCAN_FULL);

    CONST_INT(VBR_VARIABLE);
    CONST_INT(VBR_AVERAGE);
    CONST_INT(VBR_MEDIAN);

  SetMethod(target, "get_mp3_info",         node_get_mp3_info);
}

} // mp3info namespace
