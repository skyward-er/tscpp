# Copyright (C) 2024 by Skyward
#
# This program is free software; you can redistribute it and/or 
# it under the terms of the GNU General Public License as published 
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# As a special exception, if other files instantiate templates or use
# macros or inline functions from this file, or you compile this file
# and link it with other works to produce a work based on this file,
# this file does not by itself cause the resulting work to be covered
# by the GNU General Public License. However the source code for this
# file must still be made available in accordance with the GNU 
# Public License. This exception does not invalidate any other 
# why a work based on this file might be covered by the GNU General
# Public License.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>

cmake_minimum_required(VERSION 3.16)

project(TsCpp
    DESCRIPTION "Trivial serialization for C++"
    LANGUAGES CXX
)

add_library(tscpp INTERFACE)
target_sources(tscpp INTERFACE src/tscpp/buffer.cpp src/tscpp/stream.cpp)
target_include_directories(tscpp INTERFACE include)

add_executable(1_stream_known examples/1_stream_known.cpp)
target_link_libraries(1_stream_known tscpp)

add_executable(2_stream_unknown examples/2_stream_unknown.cpp)
target_link_libraries(2_stream_unknown tscpp)

add_executable(3_buffer_known examples/3_buffer_known.cpp)
target_link_libraries(3_buffer_known tscpp)

add_executable(4_buffer_unknown examples/4_buffer_unknown.cpp)
target_link_libraries(4_buffer_unknown tscpp)

add_executable(5_stream_failtest examples/5_stream_failtest.cpp)
target_link_libraries(5_stream_failtest tscpp)

add_executable(6_buffer_failtest examples/6_buffer_failtest.cpp)
target_link_libraries(6_buffer_failtest tscpp)
