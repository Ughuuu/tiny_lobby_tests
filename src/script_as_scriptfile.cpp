#include "script_as_scriptfile.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <new>
#include <string>

#include "script_as.h"

#ifdef _WIN32_WCE
#include <windows.h>  // For GetModuleFileName
#ifdef GetObject
#undef GetObject
#endif
#endif

using namespace std;

BEGIN_AS_NAMESPACE

CScriptFileCustom *ScriptFile_Factory() { return new CScriptFileCustom(); }

void ScriptFile_Factory_Generic(asIScriptGeneric *gen) {
    *(CScriptFileCustom **)gen->GetAddressOfReturnLocation() = ScriptFile_Factory();
}

void ScriptFile_AddRef_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    file->AddRef();
}

void ScriptFile_Release_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    file->Release();
}

void ScriptFile_Open_Generic(asIScriptGeneric *gen) {
    asIScriptEngine *engine = gen->GetEngine();
    ScriptAS *self = static_cast<ScriptAS *>(engine->GetUserData());

    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    std::string *f = (std::string *)gen->GetArgAddress(0);
    std::string *m = (std::string *)gen->GetArgAddress(1);

    std::string base_path = self->scripts_folder + "/" + self->folder_name;

    int r = file->Open(base_path + "/" + *f, *m);
    gen->SetReturnDWord(r);
}

void ScriptFile_Close_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    int r = file->Close();
    gen->SetReturnDWord(r);
}

void ScriptFile_GetSize_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    int r = file->GetSize();
    gen->SetReturnDWord(r);
}

void ScriptFile_ReadString_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    int len = gen->GetArgDWord(0);
    string str = file->ReadString(len);
    gen->SetReturnObject(&str);
}

void ScriptFile_ReadLine_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    std::string str = file->ReadLine();
    gen->SetReturnObject(&str);
}

void ScriptFile_ReadInt_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    asUINT bytes = *(asUINT *)gen->GetAddressOfArg(0);
    *(asINT64 *)gen->GetAddressOfReturnLocation() = file->ReadInt(bytes);
}

void ScriptFile_ReadUInt_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    asUINT bytes = *(asUINT *)gen->GetAddressOfArg(0);
    *(asQWORD *)gen->GetAddressOfReturnLocation() = file->ReadUInt(bytes);
}

void ScriptFile_ReadFloat_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    *(float *)gen->GetAddressOfReturnLocation() = file->ReadFloat();
}

void ScriptFile_ReadDouble_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    *(double *)gen->GetAddressOfReturnLocation() = file->ReadDouble();
}

#if AS_WRITE_OPS == 1
void ScriptFile_WriteString_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    std::string *str = (std::string *)gen->GetArgAddress(0);
    gen->SetReturnDWord(file->WriteString(*str));
}

void ScriptFile_WriteInt_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    asINT64 val = *(asINT64 *)gen->GetAddressOfArg(0);
    asUINT bytes = *(asUINT *)gen->GetAddressOfArg(1);
    *(int *)gen->GetAddressOfReturnLocation() = file->WriteInt(val, bytes);
}

void ScriptFile_WriteUInt_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    asQWORD val = *(asQWORD *)gen->GetAddressOfArg(0);
    asUINT bytes = *(asUINT *)gen->GetAddressOfArg(1);
    *(int *)gen->GetAddressOfReturnLocation() = file->WriteUInt(val, bytes);
}

void ScriptFile_WriteFloat_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    float val = *(float *)gen->GetAddressOfArg(0);
    *(int *)gen->GetAddressOfReturnLocation() = file->WriteFloat(val);
}

void ScriptFile_WriteDouble_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    double val = *(double *)gen->GetAddressOfArg(0);
    *(int *)gen->GetAddressOfReturnLocation() = file->WriteDouble(val);
}
#endif

void ScriptFile_IsEOF_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    bool r = file->IsEOF();
    gen->SetReturnByte(r);
}

void ScriptFile_GetPos_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    gen->SetReturnDWord(file->GetPos());
}

void ScriptFile_SetPos_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    int pos = (int)gen->GetArgDWord(0);
    gen->SetReturnDWord(file->SetPos(pos));
}

void ScriptFile_MovePos_Generic(asIScriptGeneric *gen) {
    CScriptFileCustom *file = (CScriptFileCustom *)gen->GetObject();
    int delta = (int)gen->GetArgDWord(0);
    gen->SetReturnDWord(file->MovePos(delta));
}

void RegisterScriptFileCustom_Native(asIScriptEngine *engine) {
    int r;

    r = engine->RegisterObjectType("file", 0, asOBJ_REF);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_FACTORY, "file @f()",
                                        asFUNCTION(ScriptFile_Factory), asCALL_CDECL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_ADDREF, "void f()",
                                        asMETHOD(CScriptFileCustom, AddRef), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_RELEASE, "void f()",
                                        asMETHOD(CScriptFileCustom, Release), asCALL_THISCALL);
    assert(r >= 0);

    r = engine->RegisterObjectMethod("file", "int open(const string &in, const string &in)",
                                     asMETHOD(CScriptFileCustom, Open), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int close()", asMETHOD(CScriptFileCustom, Close),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int getSize() const",
                                     asMETHOD(CScriptFileCustom, GetSize), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "bool isEndOfFile() const",
                                     asMETHOD(CScriptFileCustom, IsEOF), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "string readString(uint)",
                                     asMETHOD(CScriptFileCustom, ReadString), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "string readLine()",
                                     asMETHOD(CScriptFileCustom, ReadLine), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int64 readInt(uint)",
                                     asMETHOD(CScriptFileCustom, ReadInt), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "uint64 readUInt(uint)",
                                     asMETHOD(CScriptFileCustom, ReadUInt), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "float readFloat()",
                                     asMETHOD(CScriptFileCustom, ReadFloat), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "double readDouble()",
                                     asMETHOD(CScriptFileCustom, ReadDouble), asCALL_THISCALL);
    assert(r >= 0);
#if AS_WRITE_OPS == 1
    r = engine->RegisterObjectMethod("file", "int writeString(const string &in)",
                                     asMETHOD(CScriptFileCustom, WriteString), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeInt(int64, uint)",
                                     asMETHOD(CScriptFileCustom, WriteInt), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeUInt(uint64, uint)",
                                     asMETHOD(CScriptFileCustom, WriteUInt), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeFloat(float)",
                                     asMETHOD(CScriptFileCustom, WriteFloat), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeDouble(double)",
                                     asMETHOD(CScriptFileCustom, WriteDouble), asCALL_THISCALL);
    assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("file", "int getPos() const",
                                     asMETHOD(CScriptFileCustom, GetPos), asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int setPos(int)", asMETHOD(CScriptFileCustom, SetPos),
                                     asCALL_THISCALL);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int movePos(int)",
                                     asMETHOD(CScriptFileCustom, MovePos), asCALL_THISCALL);
    assert(r >= 0);

    r = engine->RegisterObjectProperty("file", "bool mostSignificantByteFirst",
                                       asOFFSET(CScriptFileCustom, mostSignificantByteFirst));
    assert(r >= 0);
}

void RegisterScriptFileCustom_Generic(asIScriptEngine *engine) {
    int r;

    r = engine->RegisterObjectType("file", 0, asOBJ_REF);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_FACTORY, "file @f()",
                                        asFUNCTION(ScriptFile_Factory_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_ADDREF, "void f()",
                                        asFUNCTION(ScriptFile_AddRef_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectBehaviour("file", asBEHAVE_RELEASE, "void f()",
                                        asFUNCTION(ScriptFile_Release_Generic), asCALL_GENERIC);
    assert(r >= 0);

    r = engine->RegisterObjectMethod("file", "int open(const string &in, const string &in)",
                                     asFUNCTION(ScriptFile_Open_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int close()", asFUNCTION(ScriptFile_Close_Generic),
                                     asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int getSize() const",
                                     asFUNCTION(ScriptFile_GetSize_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "bool isEndOfFile() const",
                                     asFUNCTION(ScriptFile_IsEOF_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "string readString(uint)",
                                     asFUNCTION(ScriptFile_ReadString_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "string readLine()",
                                     asFUNCTION(ScriptFile_ReadLine_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int64 readInt(uint)",
                                     asFUNCTION(ScriptFile_ReadInt_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "uint64 readUInt(uint)",
                                     asFUNCTION(ScriptFile_ReadUInt_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "float readFloat()",
                                     asFUNCTION(ScriptFile_ReadFloat_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "double readDouble()",
                                     asFUNCTION(ScriptFile_ReadDouble_Generic), asCALL_GENERIC);
    assert(r >= 0);
#if AS_WRITE_OPS == 1
    r = engine->RegisterObjectMethod("file", "int writeString(const string &in)",
                                     asFUNCTION(ScriptFile_WriteString_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeInt(int64, uint)",
                                     asFUNCTION(ScriptFile_WriteInt_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeUInt(uint64, uint)",
                                     asFUNCTION(ScriptFile_WriteUInt_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeFloat(float)",
                                     asFUNCTION(ScriptFile_WriteFloat_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int writeDouble(double)",
                                     asFUNCTION(ScriptFile_WriteDouble_Generic), asCALL_GENERIC);
    assert(r >= 0);
#endif
    r = engine->RegisterObjectMethod("file", "int getPos() const",
                                     asFUNCTION(ScriptFile_GetPos_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int setPos(int)",
                                     asFUNCTION(ScriptFile_SetPos_Generic), asCALL_GENERIC);
    assert(r >= 0);
    r = engine->RegisterObjectMethod("file", "int movePos(int)",
                                     asFUNCTION(ScriptFile_MovePos_Generic), asCALL_GENERIC);
    assert(r >= 0);

    r = engine->RegisterObjectProperty("file", "bool mostSignificantByteFirst",
                                       asOFFSET(CScriptFileCustom, mostSignificantByteFirst));
    assert(r >= 0);
}

void RegisterScriptFileCustom(asIScriptEngine *engine) {
    if (strstr(asGetLibraryOptions(), "AS_MAX_PORTABILITY"))
        RegisterScriptFileCustom_Generic(engine);
    else
        RegisterScriptFileCustom_Native(engine);
}

CScriptFileCustom::CScriptFileCustom() {
    refCount = 1;
    file = 0;
    mostSignificantByteFirst = false;
}

CScriptFileCustom::~CScriptFileCustom() { Close(); }

void CScriptFileCustom::AddRef() const { asAtomicInc(refCount); }

void CScriptFileCustom::Release() const {
    if (asAtomicDec(refCount) == 0) delete this;
}

int CScriptFileCustom::Open(const std::string &filename, const std::string &mode) {
    // if filename has ..
    if (filename.find("..") != std::string::npos) return -1;
    // Close the previously opened file handle
    if (file) Close();

    std::string myFilename = filename;

    // Validate the mode
    string m;
#if AS_WRITE_OPS == 1
    if (mode != "r" && mode != "w" && mode != "a")
#else
    if (mode != "r")
#endif
        return -1;
    else
        m = mode;

#ifdef _WIN32_WCE
    // no relative pathing on CE
    char buf[MAX_PATH];
    static TCHAR apppath[MAX_PATH] = TEXT("");
    if (!apppath[0]) {
        GetModuleFileName(NULL, apppath, MAX_PATH);

        int appLen = _tcslen(apppath);
        while (appLen > 1) {
            if (apppath[appLen - 1] == TEXT('\\')) break;
            appLen--;
        }

        // Terminate the string after the trailing backslash
        apppath[appLen] = TEXT('\0');
    }
#ifdef _UNICODE
    wcstombs(buf, apppath, wcslen(apppath) + 1);
#else
    memcpy(buf, apppath, strlen(apppath));
#endif
    myFilename = buf + myFilename;
#endif

    // By default windows translates "\r\n" to "\n", but we want to read the file as-is.
    m += "b";

    // Open the file
#if _MSC_VER >= 1400 && !defined(__S3E__)
    // MSVC 8.0 / 2005 introduced new functions
    // Marmalade doesn't use these, even though it uses the MSVC compiler
    fopen_s(&file, myFilename.c_str(), m.c_str());
#else
    file = fopen(myFilename.c_str(), m.c_str());
#endif
    if (file == 0) return -1;

    return 0;
}

int CScriptFileCustom::Close() {
    if (file == 0) return -1;

    fclose(file);
    file = 0;

    return 0;
}

int CScriptFileCustom::GetSize() const {
    if (file == 0) return -1;

    int pos = ftell(file);
    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    fseek(file, pos, SEEK_SET);

    return size;
}

int CScriptFileCustom::GetPos() const {
    if (file == 0) return -1;

    return ftell(file);
}

int CScriptFileCustom::SetPos(int pos) {
    if (file == 0) return -1;

    int r = fseek(file, pos, SEEK_SET);

    // Return -1 on error
    return r ? -1 : 0;
}

int CScriptFileCustom::MovePos(int delta) {
    if (file == 0) return -1;

    int r = fseek(file, delta, SEEK_CUR);

    // Return -1 on error
    return r ? -1 : 0;
}

string CScriptFileCustom::ReadString(unsigned int length) {
    if (file == 0) return "";

    // Read the string
    string str;
    str.resize(length);
    int size = (int)fread(&str[0], 1, length, file);
    str.resize(size);

    return str;
}

string CScriptFileCustom::ReadLine() {
    if (file == 0) return "";

    // Read until the first new-line character
    string str;
    char buf[256];

    do {
        // Get the current position so we can determine how many characters were read
        int start = ftell(file);

        // Set the last byte to something different that 0, so that we can check if the buffer was
        // filled up
        buf[255] = 1;

        // Read the line (or first 255 characters, which ever comes first)
        char *r = fgets(buf, 256, file);
        if (r == 0) break;

        // Get the position after the read
        int end = ftell(file);

        // Add the read characters to the output buffer
        str.append(buf, end - start);
    } while (!feof(file) && buf[255] == 0 && buf[254] != '\n');

    return str;
}

asINT64 CScriptFileCustom::ReadInt(asUINT bytes) {
    if (file == 0) return 0;

    if (bytes > 8) bytes = 8;
    if (bytes == 0) return 0;

    unsigned char buf[8];
    size_t r = fread(buf, bytes, 1, file);
    if (r == 0) return 0;

    asINT64 val = 0;
    if (mostSignificantByteFirst) {
        unsigned int n = 0;
        for (; n < bytes; n++) val |= asQWORD(buf[n]) << ((bytes - n - 1) * 8);

        // Check the most significant byte to determine if the rest
        // of the qword must be filled to give a negative value
        if (buf[0] & 0x80)
            for (; n < 8; n++) val |= asQWORD(0xFF) << (n * 8);
    } else {
        unsigned int n = 0;
        for (; n < bytes; n++) val |= asQWORD(buf[n]) << (n * 8);

        // Check the most significant byte to determine if the rest
        // of the qword must be filled to give a negative value
        if (buf[bytes - 1] & 0x80)
            for (; n < 8; n++) val |= asQWORD(0xFF) << (n * 8);
    }

    return val;
}

asQWORD CScriptFileCustom::ReadUInt(asUINT bytes) {
    if (file == 0) return 0;

    if (bytes > 8) bytes = 8;
    if (bytes == 0) return 0;

    unsigned char buf[8];
    size_t r = fread(buf, bytes, 1, file);
    if (r == 0) return 0;

    asQWORD val = 0;
    if (mostSignificantByteFirst) {
        unsigned int n = 0;
        for (; n < bytes; n++) val |= asQWORD(buf[n]) << ((bytes - n - 1) * 8);
    } else {
        unsigned int n = 0;
        for (; n < bytes; n++) val |= asQWORD(buf[n]) << (n * 8);
    }

    return val;
}

float CScriptFileCustom::ReadFloat() {
    if (file == 0) return 0;

    unsigned char buf[4];
    size_t r = fread(buf, 4, 1, file);
    if (r == 0) return 0;

    union conv {
        asUINT val;
        float fp;
    } value = {0};
    if (mostSignificantByteFirst) {
        unsigned int n = 0;
        for (; n < 4; n++) value.val |= asUINT(buf[n]) << ((3 - n) * 8);
    } else {
        unsigned int n = 0;
        for (; n < 4; n++) value.val |= asUINT(buf[n]) << (n * 8);
    }

    return value.fp;
}

double CScriptFileCustom::ReadDouble() {
    if (file == 0) return 0;

    unsigned char buf[8];
    size_t r = fread(buf, 8, 1, file);
    if (r == 0) return 0;

    union conv {
        asQWORD val;
        double fp;
    } value = {0};
    if (mostSignificantByteFirst) {
        unsigned int n = 0;
        for (; n < 8; n++) value.val |= asQWORD(buf[n]) << ((7 - n) * 8);
    } else {
        unsigned int n = 0;
        for (; n < 8; n++) value.val |= asQWORD(buf[n]) << (n * 8);
    }

    return value.fp;
}

bool CScriptFileCustom::IsEOF() const {
    if (file == 0) return true;

    return feof(file) ? true : false;
}

#if AS_WRITE_OPS == 1
int CScriptFileCustom::WriteString(const std::string &str) {
    if (file == 0) return -1;

    // Write the entire string
    size_t r = fwrite(&str[0], 1, str.length(), file);

    return int(r);
}

int CScriptFileCustom::WriteInt(asINT64 val, asUINT bytes) {
    if (file == 0) return 0;

    unsigned char buf[8];
    if (mostSignificantByteFirst) {
        for (unsigned int n = 0; n < bytes; n++) buf[n] = (val >> ((bytes - n - 1) * 8)) & 0xFF;
    } else {
        for (unsigned int n = 0; n < bytes; n++) buf[n] = (val >> (n * 8)) & 0xFF;
    }

    size_t r = fwrite(&buf, bytes, 1, file);
    return int(r);
}

int CScriptFileCustom::WriteUInt(asQWORD val, asUINT bytes) {
    if (file == 0) return 0;

    unsigned char buf[8];
    if (mostSignificantByteFirst) {
        for (unsigned int n = 0; n < bytes; n++) buf[n] = (val >> ((bytes - n - 1) * 8)) & 0xFF;
    } else {
        for (unsigned int n = 0; n < bytes; n++) buf[n] = (val >> (n * 8)) & 0xFF;
    }

    size_t r = fwrite(&buf, bytes, 1, file);
    return int(r);
}

int CScriptFileCustom::WriteFloat(float f) {
    if (file == 0) return 0;

    unsigned char buf[4];
    union conv {
        asUINT val;
        float fp;
    } value;
    value.fp = f;
    if (mostSignificantByteFirst) {
        for (unsigned int n = 0; n < 4; n++) buf[n] = (value.val >> ((3 - n) * 4)) & 0xFF;
    } else {
        for (unsigned int n = 0; n < 4; n++) buf[n] = (value.val >> (n * 8)) & 0xFF;
    }

    size_t r = fwrite(&buf, 4, 1, file);
    return int(r);
}

int CScriptFileCustom::WriteDouble(double d) {
    if (file == 0) return 0;

    unsigned char buf[8];
    union conv {
        asQWORD val;
        double fp;
    } value;
    value.fp = d;
    if (mostSignificantByteFirst) {
        for (unsigned int n = 0; n < 8; n++) buf[n] = (value.val >> ((7 - n) * 8)) & 0xFF;
    } else {
        for (unsigned int n = 0; n < 8; n++) buf[n] = (value.val >> (n * 8)) & 0xFF;
    }

    size_t r = fwrite(&buf, 8, 1, file);
    return int(r);
}
#endif

END_AS_NAMESPACE
