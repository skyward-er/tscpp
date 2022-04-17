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

/**
 * \file stream.h
 *
 * @brief TSCPP stream API.
 *
 * This file contains classes to serialize types to std
 * streams. These classes provide a high level API compatible with the C++ stl.
 * Error reporting is performed through exceptions.
 *
 * NOTE: The serialization format between the buffer and stream API is
 * interchangeable, so you can for example serialize using the buffer API and
 * unserialize using the stream API.
 */

#pragma once

#include <cstring>
#include <functional>
#include <istream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tscpp
{

/**
 * Exception class thrown by the input archives.
 */
class TscppException : public std::runtime_error
{
public:
    TscppException(const std::string& what) : runtime_error(what) {}

    TscppException(const std::string& what, const std::string& n)
        : runtime_error(what), n(n)
    {
    }

    /**
     * If the exception is thrown because an unknown/unexpected type has been
     * found in the input stream, this member function allows to access the
     * mangled type name.
     * It is useful to print an error message with the name of the type found in
     * the stream.
     *
     * \code
     * InputArchive ia(is);
     * Foo f;
     * try {
     *     ia >> f;
     * } catch(TscppException& ex) {
     *     if(ex.name().empty()==false)
     *         cerr << "While deserializing Foo, " << demangle(ex.name()) << "
     * was found\n";
     *     else
     *         cerr << ex.what() << endl;
     * }
     * \endcode
     *
     * \return The serialized type name or "" for other errors was found.
     */
    std::string name() const { return n; }

private:
    std::string n;
};

/**
 * @brief Type pool for the TSCPP stream API.
 *
 * A type pool is a class where you can register types and associate callbacks
 * to them. It is used to unserialize types when you don't know the exact type
 * or order in which types have been serialized.
 */
class TypePoolStream
{
public:
    /**
     * @brief Register a type and the associated callback.
     *
     * The type to register must have a default constructor defined.
     *
     * \tparam T Type to be registered.
     * \param callback Callback used when the given type is unserialized.
     */
    template <typename T>
    void registerType(std::function<void(T& t)> callback);

    void unserializeUnknownImpl(const std::string& name, std::istream& is,
                                std::streampos pos) const;

private:
    ///< Registered types serialized name and callback function
    std::map<std::string, std::function<void(std::istream&)>> types;
};

template <typename T>
void TypePoolStream::registerType(std::function<void(T& t)> callback)
{
#ifndef _MIOSIX
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type is not trivially copyable");
#endif
    types[typeid(T).name()] = [=](std::istream& is)
    {
        // NOTE: We copy the buffer to respect alignment requirements.
        // The buffer may not be suitably aligned for the unserialized type
        // TODO: support classes without default constructor
        // NOTE: we are writing on top of a constructed type without calling
        // its destructor. However, since it is trivially copyable, we at
        // least aren't overwriting pointers to allocated memory.
        T t;
        is.read(reinterpret_cast<char*>(&t), sizeof(T));
        if (is.eof())
            throw TscppException("eof");
        callback(t);
    };
}

/**
 * @brief The output archive.
 *
 * This class allows to serialize objects to any ostream using the << operator.
 */
class OutputArchive
{
public:
    /**
     * \param os Output stream where serialized types will be written.
     */
    OutputArchive(std::ostream& os) : os(os) {}

    /**
     * @brief Actual implementation of the serialization.
     *
     * Saves on the output stream the name followed by the data.
     *
     * @param name Type name saved before the data.
     * @param data Type data
     * @param size Size of the data
     */
    void serializeImpl(const char* name, const void* data, int size);

private:
    OutputArchive(const OutputArchive&) = delete;
    OutputArchive& operator=(const OutputArchive&) = delete;

    std::ostream& os;
};

/**
 * @brief Serialize a type.
 *
 * \param oa Archive where the type will be serialized.
 * \param t Type to serialize.
 */
template <typename T>
OutputArchive& operator<<(OutputArchive& oa, const T& t)
{
#ifndef _MIOSIX
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type is not trivially copyable");
#endif
    oa.serializeImpl(typeid(t).name(), &t, sizeof(t));
    return oa;
}

/**
 * @brief The input archive.
 *
 * This class allows to unserialize types from a stream, as long as you know
 * what types have been serialized in which order. Otherwise have a look at
 * UnknownInputArchive.
 *
 * To unserialize, use the >> operator.
 */
class InputArchive
{
public:
    /**
     * \param is Input stream where serialized types will be read.
     */
    InputArchive(std::istream& is) : is(is) {}

    /**
     * @brief Actual implementation of the deserialization.
     *
     * Reads, from the input stream, the name and the data.
     *
     * @param name Type name to read before the data.
     * @param data Pointer where to save the type data.
     * @param size Size of the data.
     */
    void unserializeImpl(const char* name, void* data, int size);

private:
    InputArchive(const InputArchive&) = delete;
    InputArchive& operator=(const InputArchive&) = delete;

    void wrongType(std::streampos pos);

    std::istream& is;
};

/**
 * @brief Unserialize a type.
 *
 * \param ia Archive where the type has been serialized.
 * \param t Type to unserialize.
 * \throws Throws a TscppException if the type found in the stream is not the
 * one expected, or if the stream eof is found.
 */
template <typename T>
InputArchive& operator>>(InputArchive& ia, T& t)
{
#ifndef _MIOSIX
    static_assert(std::is_trivially_copyable<T>::value,
                  "Type is not trivially copyable");
#endif
    ia.unserializeImpl(typeid(t).name(), &t, sizeof(t));
    return ia;
}

/**
 * @brief The unknown input archive.
 *
 * This class allows to unserialize types from a stream which have been
 * serialized in an unknown order.
 */
class UnknownInputArchive
{
public:
    /**
     * \param is Input stream where serialized types will be read.
     * \param tp TypePool containing the registered types and callbacks that
     * will be called as types are unserialized.
     */
    UnknownInputArchive(std::istream& is, const TypePoolStream& tp)
        : is(is), tp(tp)
    {
    }

    /**
     * @brief Unserialize one type from the input stream, calling the
     * corresponding callback registered in the TypePool.
     *
     * \throws Throws a TscppException if the type found in the stream has not
     * been registred in the TypePool or if the stream eof is found.
     */
    void unserialize();

private:
    UnknownInputArchive(const UnknownInputArchive&) = delete;
    UnknownInputArchive& operator=(const UnknownInputArchive&) = delete;

    std::istream& is;
    const TypePoolStream& tp;
};

/**
 * @brief Demangle a C++ name. Useful for printing type names in error logs.
 *
 * This function may not be supported on all platforms, in this case it returns
 * the the same string passed as a parameter.
 *
 * \param name Name to demangle.
 * \return The demangled name.
 */
std::string demangle(const std::string& name);

}  // namespace tscpp
