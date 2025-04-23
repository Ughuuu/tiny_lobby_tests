#pragma once

#ifndef ANGELSCRIPT_H
#include <angelscript.h>
#endif

#include <scriptany/scriptany.h>

#include <string>

BEGIN_AS_NAMESPACE

// JSON decoding/encoding exposed to AngelScript
CScriptAny *DecodeJSON_from_string(const std::string &json);
std::string EncodeJSON_to_string(CScriptAny *any);
CScriptAny *DecodeJSON_from_file(const std::string &filename);
void EncodeJSON_to_file(CScriptAny *any, const std::string &filename);

// Bind functions to AngelScript engine
void RegisterScriptJSON(asIScriptEngine *engine);

END_AS_NAMESPACE
