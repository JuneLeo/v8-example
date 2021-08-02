// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <include/v8.h>

#include <include/libplatform/libplatform.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <map>

/**
 * This sample program shows how to implement a simple javascript shell
 * based on V8.  This includes initializing V8 with command line options,
 * creating global functions, compiling and executing strings.
 *
 * For a more sophisticated shell, consider using the debug shell D8.
 */


v8::Local<v8::Context> CreateShellContext(v8::Isolate *isolate);

void RunShell(v8::Local<v8::Context> context, v8::Platform *platform);

int RunMain(v8::Isolate *isolate, v8::Platform *platform, int argc,
            char *argv[]);

bool ExecuteString(v8::Isolate *isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions);

void Print(const v8::FunctionCallbackInfo<v8::Value> &args);

void Read(const v8::FunctionCallbackInfo<v8::Value> &args);

void Load(const v8::FunctionCallbackInfo<v8::Value> &args);

void Quit(const v8::FunctionCallbackInfo<v8::Value> &args);

void Version(const v8::FunctionCallbackInfo<v8::Value> &args);

void Call(const v8::FunctionCallbackInfo<v8::Value> &args);

v8::MaybeLocal<v8::String> ReadFile(v8::Isolate *isolate, const char *name);

void ReportException(v8::Isolate *isolate, v8::TryCatch *handler);

void GetPointX(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> &info);

void SetPointX(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info);

void GetPointY(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> &info);

void SetPointY(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info);

void PointMulti(const v8::FunctionCallbackInfo<v8::Value> &args);


static bool run_shell;

class Point {
public:
    Point(int x, int y) : x_(x), y_(y) {}

    int x_, y_;


    int multi() {
        return this->x_ * this->y_;
    }
};

// Utility function that extracts the C++ map pointer from a wrapper
// object.
std::map<std::string, std::string>* UnwrapMap(v8::Local<v8::Object> obj) {
    v8::Local<v8::External> field = v8::Local<v8::External>::Cast(obj->GetInternalField(0));
    void* ptr = field->Value();
    return static_cast<std::map<std::string, std::string>*>(ptr);
}

std::string ObjectToString(v8::Isolate* isolate, v8::Local<v8::Value> value) {
    v8::String::Utf8Value utf8_value(isolate, value);
    return std::string(*utf8_value);
}

void constructPoint(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = v8::Isolate::GetCurrent();

    //get an x and y
    double x = args[0]->NumberValue(isolate->GetCurrentContext()).ToChecked();
    double y = args[1]->NumberValue(isolate->GetCurrentContext()).ToChecked();

    //generate a new point
    Point *point = new Point(x, y);

    args.This()->SetInternalField(0, v8::External::New(isolate, point));
}

void PointGet(v8::Local<v8::Name> name, const v8::PropertyCallbackInfo<v8::Value> &info) {
    // Fetch the map wrapped by this object.
    std::map<std::string, std::string> *obj = UnwrapMap(info.Holder());

    // Convert the JavaScript string to a std::string.
    std::string key = ObjectToString(info.GetIsolate(),v8::Local<v8::String>::Cast(name));



    printf("interceptor Getting for Point property has called, name[%s]\n", key.c_str());
}

void PointSet(v8::Local<v8::Name> name, v8::Local<v8::Value> value_obj, const v8::PropertyCallbackInfo<v8::Value> &info) {
    if (name->IsSymbol()) return;
    // Fetch the map wrapped by this object.
    std::map<std::string, std::string>* obj = UnwrapMap(info.Holder());


    // Convert the key and value to std::strings.
    std::string key = ObjectToString(info.GetIsolate(), v8::Local<v8::String>::Cast(name));
    std::string value = ObjectToString(info.GetIsolate(), value_obj);

    printf("interceptor Setting for Point property has called, name[%s] = value[%s]\n", key.c_str(), value.c_str());
}

int main(int argc, char *argv[]) {

    v8::V8::InitializeICUDefaultLocation(argv[0]);
    v8::V8::InitializeExternalStartupData(argv[0]);
    std::unique_ptr<v8::Platform> platform = v8::platform::NewDefaultPlatform();
    v8::V8::InitializePlatform(platform.get());
    v8::V8::Initialize();
    v8::V8::SetFlagsFromCommandLine(&argc, argv, true);
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator =
            v8::ArrayBuffer::Allocator::NewDefaultAllocator();
    v8::Isolate *isolate = v8::Isolate::New(create_params);
    run_shell = (argc == 1);
    int result;
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = CreateShellContext(isolate);
        if (context.IsEmpty()) {
            fprintf(stderr, "Error creating context\n");
            return 1;
        }
        v8::Context::Scope context_scope(context);
        result = RunMain(isolate, platform.get(), argc, argv);
        if (run_shell) RunShell(context, platform.get());
    }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete create_params.array_buffer_allocator;
    return result;
}


// Extracts a C string from a V8 Utf8Value.
const char *ToCString(const v8::String::Utf8Value &value) {
    return *value ? *value : "<string conversion failed>";
}




// Creates a new execution environment containing the built-in
// functions.
v8::Local<v8::Context> CreateShellContext(v8::Isolate *isolate) {
    // Create a template for the global object.
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    // Bind the global 'print' function to the C++ Print callback.
    global->Set(
            v8::String::NewFromUtf8(isolate, "print", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Print));
    // Bind the global 'read' function to the C++ Read callback.
    global->Set(
            v8::String::NewFromUtf8(isolate, "read", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Read));
    // Bind the global 'load' function to the C++ Load callback.
    global->Set(
            v8::String::NewFromUtf8(isolate, "load", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Load));
    // Bind the 'quit' function
    global->Set(
            v8::String::NewFromUtf8(isolate, "quit", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Quit));
    // Bind the 'version' function
    global->Set(
            v8::String::NewFromUtf8(isolate, "version", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Version));

    v8::Local<v8::ObjectTemplate> my = v8::ObjectTemplate::New(isolate);
    my->Set(
            v8::String::NewFromUtf8(isolate, "call", v8::NewStringType::kNormal).ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Call));

    global->Set(
            v8::String::NewFromUtf8(isolate, "my", v8::NewStringType::kNormal).ToLocalChecked(),
            my
    );


    //创建动态变量
    v8::Local<v8::FunctionTemplate> point_templ = v8::FunctionTemplate::New(isolate, constructPoint);

    global->Set(v8::String::NewFromUtf8(
            isolate, "Point", v8::NewStringType::kNormal).ToLocalChecked(), point_templ);

    //初始化原型模板
    v8::Local<v8::ObjectTemplate> point_proto = point_templ->PrototypeTemplate();

    // 原型模板上挂载multi方法
    point_proto->Set(v8::String::NewFromUtf8(
            isolate, "multi", v8::NewStringType::kNormal).ToLocalChecked(),
                     v8::FunctionTemplate::New(isolate, PointMulti));

    // 初始化实例模板
    v8::Local<v8::ObjectTemplate> point_inst = point_templ->InstanceTemplate();

    //set the internal fields of the class as we have the Point class internally
    point_inst->SetInternalFieldCount(1);

    //associates the name "x"/"y" with its Get/Set functions
    point_inst->SetAccessor(v8::String::NewFromUtf8(isolate, "x").ToLocalChecked(), GetPointX, SetPointX);
    point_inst->SetAccessor(v8::String::NewFromUtf8(isolate, "y").ToLocalChecked(), GetPointY, SetPointY);

    point_inst->SetHandler(v8::NamedPropertyHandlerConfiguration(PointGet,PointSet));


    const v8::Local<v8::Context> context = v8::Context::New(isolate, NULL, global);

    return context;
}


void PointMulti(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::Isolate *isolate = args.GetIsolate();
    //start a handle scope
    v8::HandleScope handle_scope(isolate);


    v8::Local<v8::Object> self = args.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();

    int value = static_cast<Point *>(ptr)->multi();

    args.GetReturnValue().Set(value);
}


// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
void Print(const v8::FunctionCallbackInfo<v8::Value> &args) {
    bool first = true;
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        if (first) {
            first = false;
        } else {
            printf(" ");
        }
        v8::String::Utf8Value str(args.GetIsolate(), args[i]);
        const char *cstr = ToCString(str);
        printf("%s", cstr);
    }
    printf("\n");
    fflush(stdout);
}

void Call(const v8::FunctionCallbackInfo<v8::Value> &args) {
    Print(args);

    for (int i = 0; i < args.Length(); ++i) {
        v8::String::Utf8Value str(args.GetIsolate(), args[i]);
        const char *cstr = ToCString(str);
        printf("%s", cstr);
    }
}


// The callback that is invoked by v8 whenever the JavaScript 'read'
// function is called.  This function loads the content of the file named in
// the argument into a JavaScript string.
void Read(const v8::FunctionCallbackInfo<v8::Value> &args) {
    if (args.Length() != 1) {
        args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Bad parameters",
                                        v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }
    v8::String::Utf8Value file(args.GetIsolate(), args[0]);
    if (*file == NULL) {
        args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                        v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }
    v8::Local<v8::String> source;
    if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
        args.GetIsolate()->ThrowException(
                v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                        v8::NewStringType::kNormal).ToLocalChecked());
        return;
    }

    args.GetReturnValue().Set(source);
}

// The callback that is invoked by v8 whenever the JavaScript 'load'
// function is called.  Loads, compiles and executes its argument
// JavaScript file.
void Load(const v8::FunctionCallbackInfo<v8::Value> &args) {
    for (int i = 0; i < args.Length(); i++) {
        v8::HandleScope handle_scope(args.GetIsolate());
        v8::String::Utf8Value file(args.GetIsolate(), args[i]);
        if (*file == NULL) {
            args.GetIsolate()->ThrowException(
                    v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                            v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }
        v8::Local<v8::String> source;
        if (!ReadFile(args.GetIsolate(), *file).ToLocal(&source)) {
            args.GetIsolate()->ThrowException(
                    v8::String::NewFromUtf8(args.GetIsolate(), "Error loading file",
                                            v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }
        if (!ExecuteString(args.GetIsolate(), source, args[i], false, false)) {
            args.GetIsolate()->ThrowException(
                    v8::String::NewFromUtf8(args.GetIsolate(), "Error executing file",
                                            v8::NewStringType::kNormal).ToLocalChecked());
            return;
        }
    }
}


// The callback that is invoked by v8 whenever the JavaScript 'quit'
// function is called.  Quits.
void Quit(const v8::FunctionCallbackInfo<v8::Value> &args) {
    // If not arguments are given args[0] will yield undefined which
    // converts to the integer value 0.
    int exit_code =
            args[0]->Int32Value(args.GetIsolate()->GetCurrentContext()).FromMaybe(0);
    fflush(stdout);
    fflush(stderr);
    exit(exit_code);
}


void Version(const v8::FunctionCallbackInfo<v8::Value> &args) {

    args.GetReturnValue().Set(
            v8::String::NewFromUtf8(args.GetIsolate(), v8::V8::GetVersion(),
                                    v8::NewStringType::kNormal).ToLocalChecked());
}


// Reads a file into a v8 string.
v8::MaybeLocal<v8::String> ReadFile(v8::Isolate *isolate, const char *name) {
    FILE *file = fopen(name, "rb");
    if (file == NULL) return v8::MaybeLocal<v8::String>();

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    rewind(file);

    char *chars = new char[size + 1];
    chars[size] = '\0';
    for (size_t i = 0; i < size;) {
        i += fread(&chars[i], 1, size - i, file);
        if (ferror(file)) {
            fclose(file);
            return v8::MaybeLocal<v8::String>();
        }
    }
    fclose(file);
    v8::MaybeLocal<v8::String> result = v8::String::NewFromUtf8(
            isolate, chars, v8::NewStringType::kNormal, static_cast<int>(size));
    delete[] chars;
    return result;
}


// Process remaining command line arguments and execute files
int RunMain(v8::Isolate *isolate, v8::Platform *platform, int argc,
            char *argv[]) {
    for (int i = 1; i < argc; i++) {
        const char *str = argv[i];
        if (strcmp(str, "--shell") == 0) {
            run_shell = true;
        } else if (strcmp(str, "-f") == 0) {
            // Ignore any -f flags for compatibility with the other stand-
            // alone JavaScript engines.
            continue;
        } else if (strncmp(str, "--", 2) == 0) {
            fprintf(stderr,
                    "Warning: unknown flag %s.\nTry --help for options\n", str);
        } else if (strcmp(str, "-e") == 0 && i + 1 < argc) {
            // Execute argument given to -e option directly.
            v8::Local<v8::String> file_name =
                    v8::String::NewFromUtf8(isolate, "unnamed",
                                            v8::NewStringType::kNormal).ToLocalChecked();
            v8::Local<v8::String> source;
            if (!v8::String::NewFromUtf8(isolate, argv[++i],
                                         v8::NewStringType::kNormal)
                    .ToLocal(&source)) {
                return 1;
            }
            bool success = ExecuteString(isolate, source, file_name, false, true);
            while (v8::platform::PumpMessageLoop(platform, isolate)) continue;
            if (!success) return 1;
        } else {

            fprintf(stdout, "read file '%s'\n", str);
            // Use all other arguments as names of files to load and run.
            v8::Local<v8::String> file_name =
                    v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal)
                            .ToLocalChecked();
            v8::Local<v8::String> source;
            if (!ReadFile(isolate, str).ToLocal(&source)) {
                fprintf(stderr, "Error reading '%s'\n", str);
                continue;
            }
            bool success = ExecuteString(isolate, source, file_name, false, true);
            while (v8::platform::PumpMessageLoop(platform, isolate)) continue;

            if (!success) return 1;
        }
    }
    return 0;
}


// The read-eval-execute loop of the shell.
void RunShell(v8::Local<v8::Context> context, v8::Platform *platform) {
    fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());
    static const int kBufferSize = 256;
    // Enter the execution environment before evaluating any code.
    v8::Context::Scope context_scope(context);
    v8::Local<v8::String> name(
            v8::String::NewFromUtf8(context->GetIsolate(), "(shell)", v8::NewStringType::kNormal).ToLocalChecked());
    while (true) {
        char buffer[kBufferSize];
        fprintf(stderr, "> ");
        char *str = fgets(buffer, kBufferSize, stdin);
        if (str == NULL) break;
        v8::HandleScope handle_scope(context->GetIsolate());
        ExecuteString(
                context->GetIsolate(), //isolate
                v8::String::NewFromUtf8(context->GetIsolate(), str,
                                        v8::NewStringType::kNormal).ToLocalChecked(), //source
                name, //name
                true,
                true);
        while (v8::platform::PumpMessageLoop(platform, context->GetIsolate()))
            continue;
    }
    fprintf(stderr, "\n");
}


// Executes a string within the current v8 context.
bool ExecuteString(v8::Isolate *isolate, v8::Local<v8::String> source,
                   v8::Local<v8::Value> name, bool print_result,
                   bool report_exceptions) {
    v8::HandleScope handle_scope(isolate);
    v8::TryCatch try_catch(isolate);
    v8::ScriptOrigin origin(name);
    v8::Local<v8::Context> context(isolate->GetCurrentContext());
    v8::Local<v8::Script> script;
    if (!v8::Script::Compile(context, source, &origin).ToLocal(&script)) {
        // Print errors that happened during compilation.
        if (report_exceptions)
            ReportException(isolate, &try_catch);
        return false;
    } else {
        v8::Local<v8::Value> result;
        if (!script->Run(context).ToLocal(&result)) {
            assert(try_catch.HasCaught());
            // Print errors that happened during execution.
            if (report_exceptions)
                ReportException(isolate, &try_catch);
            return false;
        } else {
            assert(!try_catch.HasCaught());
            if (print_result && !result->IsUndefined()) {
                // If all went well and the result wasn't undefined then print
                // the returned value.
                v8::String::Utf8Value str(isolate, result);
                const char *cstr = ToCString(str);
                printf("%s\n", cstr);
            }
            return true;
        }
    }
}


void ReportException(v8::Isolate *isolate, v8::TryCatch *try_catch) {
    v8::HandleScope handle_scope(isolate);
    v8::String::Utf8Value exception(isolate, try_catch->Exception());
    const char *exception_string = ToCString(exception);
    v8::Local<v8::Message> message = try_catch->Message();
    if (message.IsEmpty()) {
        // V8 didn't provide any extra information about this error; just
        // print the exception.
        fprintf(stderr, "%s\n", exception_string);
    } else {
        // Print (filename):(line number): (message).
        v8::String::Utf8Value filename(isolate,
                                       message->GetScriptOrigin().ResourceName());
        v8::Local<v8::Context> context(isolate->GetCurrentContext());
        const char *filename_string = ToCString(filename);
        int linenum = message->GetLineNumber(context).FromJust();
        fprintf(stderr, "%s:%i: %s\n", filename_string, linenum, exception_string);
        // Print line of source code.
        v8::String::Utf8Value sourceline(
                isolate, message->GetSourceLine(context).ToLocalChecked());
        const char *sourceline_string = ToCString(sourceline);
        fprintf(stderr, "%s\n", sourceline_string);
        // Print wavy underline (GetUnderline is deprecated).
        int start = message->GetStartColumn(context).FromJust();
        for (int i = 0; i < start; i++) {
            fprintf(stderr, " ");
        }
        int end = message->GetEndColumn(context).FromJust();
        for (int i = start; i < end; i++) {
            fprintf(stderr, "^");
        }
        fprintf(stderr, "\n");
        v8::Local<v8::Value> stack_trace_string;
        if (try_catch->StackTrace(context).ToLocal(&stack_trace_string) &&
            stack_trace_string->IsString() &&
            v8::Local<v8::String>::Cast(stack_trace_string)->Length() > 0) {
            v8::String::Utf8Value stack_trace(isolate, stack_trace_string);
            const char *stack_trace_string = ToCString(stack_trace);
            fprintf(stderr, "%s\n", stack_trace_string);
        }
    }
}


void GetPointX(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
    printf("GetPointX is calling\n");

    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();
    int value = static_cast<Point *>(ptr)->x_;
    info.GetReturnValue().Set(value);
}

void SetPointX(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
    printf("SetPointX is calling\n");

    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();
    static_cast<Point *>(ptr)->x_ = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
}


void GetPointY(v8::Local<v8::String> property, const v8::PropertyCallbackInfo<v8::Value> &info) {
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();
    int value = static_cast<Point *>(ptr)->y_;
    info.GetReturnValue().Set(value);
}

void SetPointY(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void> &info) {
    v8::Local<v8::Object> self = info.Holder();
    v8::Local<v8::External> wrap = v8::Local<v8::External>::Cast(self->GetInternalField(0));
    void *ptr = wrap->Value();
    static_cast<Point *>(ptr)->y_ = value->Int32Value(info.GetIsolate()->GetCurrentContext()).ToChecked();
}
