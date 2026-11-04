# === 配置 CMake === #
cmake_minimum_required(VERSION 3.20)


# === 配置项目 === #
project(Catelier)


# === 配置 C++ === #
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# === 配置通用编译器选项 === #
option(DEBUG_MODE "Build with debug info (for easier debugging)" OFF)
if(MSVC)
	if(DEBUG_MODE)
		# 禁用优化，完整调试
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /O0")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG:FULL /OPT:REF /OPT:ICF")
	else()
		# 禁用调试，完整优化
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /Oi /Ot /Oy /fp:fast")
		set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
		set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /OPT:REF /OPT:ICF")
	endif()
	add_compile_options(/utf-8)
endif()


# === 引入第三方库 === #
include(FetchContent)

# nanobench #
FetchContent_Declare(
	nanobench
	GIT_REPOSITORY https://github.com/martinus/nanobench.git
	GIT_TAG v4.3.11
	GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(nanobench)


# === 配置项目文件 === #
add_executable(Catelier
	test/catelier/Main.cpp
	src/catelier/CommonMacro.hpp
	src/catelier/CommonPrimitives.hpp
	src/catelier/foundation/structure/linear/sequence/ArrayList.hpp
	src/catelier/foundation/structure/linear/sequence/ArrayList.cpp
	include/catelier/foundation/structure/linear/sequence/ArrayList.hpp
	src/catelier/foundation/structure/associative/map/OpenHashMap.hpp
	src/catelier/foundation/structure/associative/map/OpenHashMap.cpp
	include/catelier/foundation/structure/associative/map/OpenHashMap.hpp
	src/catelier/util/HashUtils.hpp
	src/catelier/util/HashUtils.cpp
	src/catelier/foundation/structure/linear/sequence/SinglyLinkedList.hpp
	src/catelier/foundation/structure/linear/sequence/SinglyLinkedList.cpp
	include/catelier/foundation/structure/linear/sequence/SinglyLinkedList.hpp
	src/catelier/foundation/structure/linear/sequence/DoublyLinkedList.hpp
	src/catelier/foundation/structure/linear/sequence/DoublyLinkedList.cpp
	include/catelier/foundation/structure/linear/sequence/DoublyLinkedList.hpp
	src/catelier/foundation/structure/associative/map/ChainHashMap.hpp
	src/catelier/foundation/structure/associative/map/ChainHashMap.cpp
	include/catelier/foundation/structure/associative/map/ChainHashMap.hpp
	src/catelier/foundation/structure/linear/sequence/BitArray.hpp
	src/catelier/foundation/structure/linear/sequence/BitArray.cpp
	include/catelier/foundation/structure/linear/sequence/BitArray.hpp
	src/catelier/foundation/structure/linear/sequence/BitSet.hpp
	src/catelier/foundation/structure/linear/sequence/BitSet.cpp
	include/catelier/foundation/structure/linear/sequence/BitSet.hpp
	src/catelier/foundation/structure/linear/queue/RingBuffer.hpp
	src/catelier/foundation/structure/linear/queue/RingBuffer.cpp
	include/catelier/foundation/structure/linear/queue/RingBuffer.hpp
	src/catelier/foundation/structure/linear/stack/Stack.hpp
	src/catelier/foundation/structure/linear/stack/Stack.cpp
	include/catelier/foundation/structure/linear/stack/Stack.hpp
	src/catelier/foundation/structure/linear/queue/BoundedQueue.hpp
	src/catelier/foundation/structure/linear/queue/BoundedQueue.cpp
	include/catelier/foundation/structure/linear/queue/BoundedQueue.hpp
	src/catelier/foundation/structure/linear/queue/BoundedDeque.hpp
	src/catelier/foundation/structure/linear/queue/BoundedDeque.cpp
	include/catelier/foundation/structure/linear/queue/BoundedDeque.hpp
	src/catelier/foundation/structure/linear/queue/UnboundedQueue.hpp
	src/catelier/foundation/structure/linear/queue/UnboundedQueue.cpp
	include/catelier/foundation/structure/linear/queue/UnboundedQueue.hpp
	src/catelier/foundation/structure/linear/queue/UnboundedDeque.hpp
	src/catelier/foundation/structure/linear/queue/UnboundedDeque.cpp
	include/catelier/foundation/structure/linear/queue/UnboundedDeque.hpp
	src/catelier/foundation/structure/linear/priority/BinaryHeap.hpp
	src/catelier/foundation/structure/linear/priority/BinaryHeap.cpp
	include/catelier/foundation/structure/linear/priority/BinaryHeap.hpp
	src/catelier/foundation/structure/tree/binary/BinarySearchTree.hpp
	src/catelier/foundation/structure/tree/binary/BinarySearchTree.cpp
	include/catelier/foundation/structure/tree/binary/BinarySearchTree.hpp
	src/catelier/foundation/structure/tree/binary/AVLTree.hpp
	src/catelier/foundation/structure/tree/binary/AVLTree.cpp
	include/catelier/foundation/structure/tree/binary/AVLTree.hpp
	src/catelier/foundation/structure/tree/binary/BinaryTree.hpp
	src/catelier/foundation/structure/tree/binary/BinaryTree.cpp
	include/catelier/foundation/structure/tree/binary/BinaryTree.hpp
	src/catelier/foundation/structure/tree/binary/HuffmanTree.hpp
	src/catelier/foundation/structure/tree/binary/HuffmanTree.cpp
	include/catelier/foundation/structure/tree/binary/HuffmanTree.hpp
	src/catelier/foundation/structure/tree/disjoint/UnionFind.hpp
	src/catelier/foundation/structure/tree/disjoint/UnionFind.cpp
	include/catelier/foundation/structure/tree/disjoint/UnionFind.hpp
	src/catelier/foundation/structure/graph/AdjacencyMatrix.hpp
	src/catelier/foundation/structure/graph/AdjacencyMatrix.cpp
	include/catelier/foundation/structure/graph/AdjacencyMatrix.hpp
	src/catelier/foundation/structure/graph/AdjacencyList.hpp
	src/catelier/foundation/structure/graph/AdjacencyList.cpp
	include/catelier/foundation/structure/graph/AdjacencyList.hpp
	src/catelier/foundation/structure/graph/EdgeList.hpp
	src/catelier/foundation/structure/graph/EdgeList.cpp
	include/catelier/foundation/structure/graph/EdgeList.hpp
	src/catelier/foundation/structure/linear/sequence/HeapString.hpp
	src/catelier/foundation/structure/linear/sequence/HeapString.cpp
	include/catelier/foundation/structure/linear/sequence/HeapString.hpp
	src/catelier/foundation/structure/linear/sequence/CircularLinkedList.hpp
	src/catelier/foundation/structure/linear/sequence/CircularLinkedList.cpp
	include/catelier/foundation/structure/linear/sequence/CircularLinkedList.hpp
	src/catelier/foundation/sync/lock/BlockingLock.hpp
	src/catelier/foundation/sync/lock/BlockingLock.cpp
	include/catelier/foundation/sync/lock/BlockingLock.hpp
	src/catelier/foundation/sync/OnceFlag.hpp
	src/catelier/foundation/sync/OnceFlag.cpp
	include/catelier/foundation/sync/OnceFlag.hpp
	src/catelier/foundation/primitive/Atomic.hpp
	src/catelier/foundation/primitive/Atomic.cpp
	include/catelier/foundation/primitive/Atomic.hpp
	src/catelier/foundation/primitive/Thread.hpp
	src/catelier/foundation/primitive/Thread.cpp
	include/catelier/foundation/primitive/Thread.hpp
	src/catelier/foundation/sync/ConditionalVar.hpp
	src/catelier/foundation/sync/ConditionalVar.cpp
	include/catelier/foundation/sync/ConditionalVar.hpp
	src/catelier/foundation/sync/ConflatedSignal.hpp
	src/catelier/foundation/sync/ConflatedSignal.cpp
	include/catelier/foundation/sync/ConflatedSignal.hpp
	src/catelier/foundation/structure/linear/queue/concurrent/ConcurrentUnboundedQueue.hpp
	src/catelier/foundation/structure/linear/queue/concurrent/ConcurrentUnboundedQueue.cpp
	include/catelier/foundation/structure/linear/queue/concurrent/ConcurrentUnboundedQueue.hpp
	src/catelier/foundation/structure/associative/map/concurrent/ConcurrentChainHashMap.hpp
	src/catelier/foundation/structure/associative/map/concurrent/ConcurrentChainHashMap.cpp
	include/catelier/foundation/structure/associative/map/concurrent/ConcurrentChainHashMap.hpp
	src/catelier/architecture/imp/IMPList.hpp
	src/catelier/architecture/imp/IMPList.cpp
	include/catelier/architecture/imp/IMPList.hpp
	src/catelier/foundation/primitive/Time.hpp
	src/catelier/foundation/primitive/Time.cpp
	include/catelier/foundation/primitive/Time.hpp
	test/catelier/architecture/imp/IMPList.cpp
	"src/catelier/foundation/concurrency/ThreadLocalTable.hpp"
	include/catelier/foundation/concurrency/ThreadLocalTable.hpp
	src/catelier/foundation/concurrency/ThreadLocalTable.cpp
	src/catelier/foundation/sync/lock/SpinLock.hpp
	src/catelier/foundation/sync/lock/SpinLock.cpp
	include/catelier/foundation/sync/lock/SpinLock.hpp
)


# === 链接第三方库 === #
target_link_libraries(Catelier PRIVATE
	nanobench
)


# === 配置 LTO === #
if(NOT DEBUG_MODE)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_ERROR)
	if(IPO_SUPPORTED)
		set_target_properties(Catelier PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	else()
		message(WARNING "IPO/LTO not supported: ${IPO_ERROR}")
	endif()
endif()


# === 定义编译器宏 === #
target_compile_definitions(Catelier PRIVATE
	_CRT_SECURE_NO_WARNINGS
)


# === 定义项目路径宏 === #
target_compile_definitions(Catelier PRIVATE
	PROJECT_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
)