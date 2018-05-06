/***************************************************************************
 *   Copyright (C) 2018 by Terraneo Federico                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "tscpp.h"
#include <memory>
#if defined(__GNUC__) && !defined(_MIOSIX)
#include <cxxabi.h>
#endif

using namespace std;

namespace tscpp {

int TypePool::unserializeUnknownImpl(const char *name, const void *buffer, int bufSize) const
{
    auto it=types.find(name);
    if(it==types.end()) return TypeNotFound;

    if(it->second.size>bufSize) return BufferTooSmall;

    it->second.usc(buffer);
    return it->second.size;
}

void TypePool::unserializeUnknownImpl(const string& name, istream& is, streampos pos) const
{
    auto it=types.find(name);
    if(it==types.end()) {
        is.seekg(pos);
        throw TscppException("unknown type",name);
    }

    unique_ptr<char[]> unserialized(new char[it->second.size]);
    is.read(reinterpret_cast<char*>(unserialized.get()),it->second.size);
    if(is.eof()) throw TscppException("eof");
    it->second.usc(unserialized.get());
}

int serializeImpl(void *buffer, int bufSize, const char *name, const void *data, int size)
{
    int nameSize=strlen(name);
    int serializedSize=nameSize+1+size;
    if(serializedSize>bufSize) return BufferTooSmall;

    char *buf=reinterpret_cast<char*>(buffer);
    memcpy(buf,name,nameSize+1); //Copy also the \0
    memcpy(buf+nameSize+1,data,size);
    return serializedSize;
}

int unserializeImpl(const char *name, void *data, int size, const void *buffer, int bufSize)
{
    int nameSize=strlen(name);
    int serializedSize=nameSize+1+size;
    if(serializedSize>bufSize) return BufferTooSmall;

    const char *buf=reinterpret_cast<const char*>(buffer);
    if(memcmp(buf,name,nameSize+1)) return TypeMismatch;

    //NOTE: we are writing on top of a constructed type without calling its
    //destructor. However, since it is trivially copyable, we at least aren't
    //overwriting pointers to allocated memory.
    memcpy(data,buf+nameSize+1,size);
    return serializedSize;
}

int unserializeUnknown(const TypePool& tp, const void *buffer, int bufSize)
{
    const char *buf=reinterpret_cast<const char*>(buffer);
    int nameSize=strnlen(buf,bufSize);
    if(nameSize>=bufSize) return BufferTooSmall;

    const char *name=buf;
    buf+=nameSize+1;
    bufSize-=nameSize+1;
    return tp.unserializeUnknownImpl(name,buf,bufSize)+nameSize+1;
}

string peekTypeName(const void *buffer, int bufSize)
{
    const char *buf=reinterpret_cast<const char*>(buffer);
    int nameSize=strnlen(buf,bufSize);
    if(nameSize>=bufSize) return "";
    return buf;
}

string demangle(const string& name)
{
    #if defined(__GNUC__) && !defined(_MIOSIX)
    string result=name;
    int status;
    char* demangled=abi::__cxa_demangle(name.c_str(),NULL,0,&status);
    if(status==0 && demangled) result=demangled;
    if(demangled) free(demangled);
    return result;
    #else
    return name; //Demangle not supported
    #endif
}

void OutputArchive::serializeImpl(const char *name, const void *data, int size)
{
    int nameSize=strlen(name);
    os.write(name,nameSize+1);
    os.write(reinterpret_cast<const char*>(data),size);
}

void InputArchive::unserializeImpl(const char *name, void *data, int size)
{
    auto pos=is.tellg();
    int nameSize=strlen(name);
    unique_ptr<char[]> unserializedName(new char[nameSize+1]);
    is.read(unserializedName.get(),nameSize+1);
    if(is.eof()) throw TscppException("eof");

    if(memcmp(unserializedName.get(),name,nameSize+1)) wrongType(pos);

    //NOTE: we are writing on top of a constructed type without calling its
    //destructor. However, since it is trivially copyable, we at least aren't
    //overwriting pointers to allocated memory.
    is.read(reinterpret_cast<char*>(data),size);
    if(is.eof()) throw TscppException("eof");
}

void InputArchive::wrongType(streampos pos)
{
    is.seekg(pos);
    string name;
    getline(is,name,'\0');
    is.seekg(pos);
    throw TscppException("wrong type",name);
}

void UnknownInputArchive::unserialize()
{
    auto pos=is.tellg();
    string name;
    getline(is,name,'\0');
    if(is.eof()) throw TscppException("eof");

    tp.unserializeUnknownImpl(name,is,pos);
}

} //namespace tscpp