#pragma once
//
// CScriptFile
//
// This class encapsulates a FILE pointer in a reference counted class for
// use within AngelScript.
//

//---------------------------
// Declaration
//

#ifndef ANGELSCRIPT_H
// Avoid having to inform include path if header is already include before
#include <angelscript.h>
#endif

#include <stdio.h>

#include <string>

BEGIN_AS_NAMESPACE

class CScriptFileCustom {
   public:
    CScriptFileCustom();

    void AddRef() const;
    void Release() const;

    // TODO: Implement the "r+", "w+" and "a+" modes
    // mode = "r" -> open the file for reading
    //        "w" -> open the file for writing (overwrites existing file)
    //        "a" -> open the file for appending
    int Open(const std::string &filename, const std::string &mode);
    int Close();
    int GetSize() const;
    bool IsEOF() const;

    // Reading
    std::string ReadString(unsigned int length);
    std::string ReadLine();
    asINT64 ReadInt(asUINT bytes);
    asQWORD ReadUInt(asUINT bytes);
    float ReadFloat();
    double ReadDouble();

#if AS_WRITE_OPS == 1
    // Writing
    int WriteString(const std::string &str);
    int WriteInt(asINT64 v, asUINT bytes);
    int WriteUInt(asQWORD v, asUINT bytes);
    int WriteFloat(float v);
    int WriteDouble(double v);
#endif

    // Cursor
    int GetPos() const;
    int SetPos(int pos);
    int MovePos(int delta);

    // Big-endian = most significant byte first
    bool mostSignificantByteFirst;

   protected:
    ~CScriptFileCustom();

    mutable int refCount;
    FILE *file;
};

// This function will determine the configuration of the engine
// and use one of the two functions below to register the file type
void RegisterScriptFileCustom(asIScriptEngine *engine);

// Call this function to register the file type
// using native calling conventions
void RegisterScriptFileCustom_Native(asIScriptEngine *engine);

// Use this one instead if native calling conventions
// are not supported on the target platform
void RegisterScriptFileCustom_Generic(asIScriptEngine *engine);

END_AS_NAMESPACE
