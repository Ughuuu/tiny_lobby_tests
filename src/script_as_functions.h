#pragma once
#include <scriptany/scriptany.h>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptgrid/scriptgrid.h>
#include <scripthandle/scripthandle.h>
#include <scriptmath/scriptmath.h>
#include <scriptstdstring/scriptstdstring.h>
#include <weakref/weakref.h>

#include <iostream>

#include "any_type.h"

CScriptArray *ConvertToArray(asIScriptEngine *engine,
                             const boost::container::vector<AnyElement> &elements);
CScriptDictionary *ConvertToDictionary(
    asIScriptEngine *engine, const boost::container::flat_map<std::string, AnyElement> &map);
CScriptAny *ConvertToAny(asIScriptEngine *engine, const AnyElement &element);
void as_print_int64(int64_t value);
void as_print_double(double value);
void as_print_bool(bool value);
void as_print_string(const std::string &value);
AnyElement ConvertFromScriptType(asIScriptEngine *engine, void *value, int typeId);
AnyElement ConvertFromAny(asIScriptEngine *engine, CScriptAny *any);
boost::container::vector<AnyElement> ConvertFromGrid(asIScriptEngine *engine, CScriptGrid *grid);
boost::container::flat_map<std::string, AnyElement> ConvertFromDictionary(asIScriptEngine *engine,
                                                                          CScriptDictionary *dict);
boost::container::vector<AnyElement> ConvertFromArray(asIScriptEngine *engine, CScriptArray *arr);
struct IncludeUserData {
    std::string base_path;
};
bool as_IsPathAllowed(const std::string &path);
int as_SecureIncludeCallback(const char *includePath, const char *fromPath, CScriptBuilder *builder,
                             void *userParam);
