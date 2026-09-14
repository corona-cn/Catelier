# === 配置 CMake === #
cmake_minimum_required(VERSION 3.20)


# === 配置项目 === #
project(Catelier)


# === 配置 C++ === #
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# === 配置通用编译器选项 === #
option(RELEASE_WITH_DEBUG "Build Release with debug info (for easier debugging)" OFF)
if(MSVC)
	if(RELEASE_WITH_DEBUG)
		# 调试专用 Release：禁用所有优化，保留完整调试信息
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O0")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
	else()
		# 原有优化 Release
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy /GL /fp:fast")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF /OPT:ICF")
	endif()
	add_compile_options(/utf-8)
endif()


# === 配置项目文件 === #
add_executable(Catelier
	test/Main.cpp
	src/catelier/CommonMacro.hpp
	src/catelier/CommonPrimitives.hpp
	src/catelier/foundation/ArrayList.hpp
	src/catelier/foundation/ArrayList.cpp
	include/catelier/foundation/ArrayList.hpp
	src/catelier/foundation/OpenHashMap.hpp
	src/catelier/foundation/OpenHashMap.cpp
	include/catelier/foundation/OpenHashMap.hpp
	src/catelier/util/HashUtils.hpp
	src/catelier/util/HashUtils.cpp
	src/catelier/foundation/SinglyLinkedList.hpp
	src/catelier/foundation/SinglyLinkedList.cpp
	include/catelier/foundation/SinglyLinkedList.hpp
	src/catelier/foundation/DoublyLinkedList.hpp
	src/catelier/foundation/DoublyLinkedList.cpp
	include/catelier/foundation/DoublyLinkedList.hpp
	src/catelier/foundation/ChainHashMap.hpp
	src/catelier/foundation/ChainHashMap.cpp
	include/catelier/foundation/ChainHashMap.hpp
	src/catelier/foundation/BitArray.hpp
	src/catelier/foundation/BitArray.cpp
	include/catelier/foundation/BitArray.hpp
	src/catelier/foundation/BitSet.hpp
	src/catelier/foundation/BitSet.cpp
	include/catelier/foundation/BitSet.hpp
	src/catelier/foundation/RingBuffer.hpp
	src/catelier/foundation/RingBuffer.cpp
	include/catelier/foundation/RingBuffer.hpp
	src/catelier/foundation/Stack.hpp
	src/catelier/foundation/Stack.cpp
	include/catelier/foundation/Stack.hpp
	src/catelier/foundation/BoundedQueue.hpp
	src/catelier/foundation/BoundedQueue.cpp
	include/catelier/foundation/BoundedQueue.hpp
	src/catelier/foundation/BoundedDeque.hpp
	src/catelier/foundation/BoundedDeque.cpp
	include/catelier/foundation/BoundedDeque.hpp
)


# === 定义编译器宏 === #
target_compile_definitions(Catelier PRIVATE
	_CRT_SECURE_NO_WARNINGS
)


# === 定义项目路径宏 === #
target_compile_definitions(Catelier PRIVATE
	PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)