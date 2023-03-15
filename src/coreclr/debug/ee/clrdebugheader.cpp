// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"
#include <daccess.h>

struct DebugTable
{
    const char* TableName;
    const void* TableElements;
    uint32_t RowCount;
};

struct DebugGlobalRow
{
    const char *GlobalName;
    const void *Address;
};

struct DebugTypeRow
{
    const char *TypeName;
    uint32_t Size;
};

struct DebugMemberOffsetRow
{
    const char *MemberName;
    uint32_t TypeId;
    uint32_t Offset;
};

struct DebugDefineRow
{
    const char *DefineName;
    uint32_t DefineValue;
};

static constexpr size_t DebugTablesArraySize = 5;
struct DebugTable s_DebugTables[DebugTablesArraySize];

static constexpr size_t DebugGlobalsArraySize = 50;
struct DebugGlobalRow s_DebugGlobals[DebugGlobalsArraySize];

static constexpr size_t DebugTypesArraySize = 100;
struct DebugTypeRow s_DebugTypes[DebugTypesArraySize];

static constexpr size_t DebugFieldsArraySize = 200;
struct DebugMemberOffsetRow s_DebugFields[DebugFieldsArraySize];

static constexpr size_t DebugBasesArraySize = 100;
struct DebugMemberOffsetRow s_DebugBases[DebugBasesArraySize];

static constexpr size_t DebugDefinesArraySize = 50;
struct DebugDefineRow s_DebugDefines[DebugDefinesArraySize];

// This structure is part of a in-memory serialization format that is used by diagnostic tools to
// reason about the runtime. As a contract with our diagnostic tools it must be kept up-to-date
// by changing the MajorVersion when breaking changes occur. If you are changing the runtime then
// you are responsible for understanding what changes are breaking changes.
//
// If you do want to make a breaking change please coordinate with diagnostics team as breaking changes
// require debugger side components to be updated, and then the new versions will need to be distributed
// to customers. Ideally you will check in updates to the runtime components, the debugger parser
// components, and the format specification at the same time.
//
// Although not guaranteed to be exhaustive, at a glance these are some potential breaking changes:
//   - Removing a field from this structure
//   - Reordering fields in the structure
//   - Changing the data type of a field in this structure
//   - Changing the data type of a field in another structure that is being referred to here with
//       the offsetof() operator
//   - Changing the data type of a global whose address is recorded in this structure
//   - Changing the meaning of a field or global referred to in this structure so that it can no longer
//     be used in the manner the format specification describes.
struct ClrDebugHeader
{
    // The cookie serves as a sanity check against process corruption or being requested
    // to treat some other non-.Net module as though it did contain the .Net runtime.
    // It can also be changed if we want to make a breaking change so drastic that
    // earlier debuggers should treat the module as if it had no .Net runtime at all.
    // If the cookie is valid a debugger is safe to assume the Major/Minor version fields
    // will follow, but any contents beyond that depends on the version values.
    // The cookie value is currently set to 0x20 0x43 0x44 0x48 ( CDH in ascii)
    const uint8_t Cookie[4] = { 0x20, 0x43, 0x44, 0x48 };

    // This counter can be incremented to indicate breaking changes
    // This field must be encoded little endian, regardless of the typical endianness of
    // the machine
    const uint16_t MajorVersion = 2;

    // This counter can be incremented to indicate back-compatible changes
    // This field must be encoded little endian, regardless of the typical endianness of
    // the machine
    const uint16_t MinorVersion = 0;

    // Pointer to the debug tables
    const DebugTable* DebugTables = (const DebugTable*)&s_DebugTables;

    // Number of debug tables
    uint32_t DebugTableCount = 0;
};

#ifdef TARGET_WINDOWS
#pragma comment (linker, "/EXPORT:ClrDebugHeader,DATA")
#endif
#ifdef _MSC_VER
extern "C" struct ClrDebugHeader ClrDebugHeader;
struct ClrDebugHeader ClrDebugHeader = {};
#else
DLLEXPORT ClrDebugHeader ClrDebugHeader = {};
#endif

#define baseoffset(type, base, field) (offsetof(type, field) - offsetof(base, field))

#define MAKE_GLOBAL_ENTRY(Name)                                                     \
    do                                                                              \
    {                                                                               \
        s_DebugGlobals[currentGlobalPos] = { #Name, &Name };                        \
        ++currentGlobalPos;                                                         \
        ASSERT(currentGlobalPos <= DebugGlobalsArraySize);                          \
    } while(0)                                                                      \

#define MAKE_TYPE_ENTRY_WITH_ID(TypeName, TypeIdName)                               \
    uint32_t TypeIdName = currentTypePos;                                           \
    do                                                                              \
    {                                                                               \
        s_DebugTypes[currentTypePos] = { #TypeName, sizeof(TypeName) };             \
        ++currentTypePos;                                                           \
        ASSERT(currentTypePos <= DebugTypesArraySize);                              \
    } while(0)                                                                      \

#define MAKE_TYPE_ENTRY(TypeName) MAKE_TYPE_ENTRY_WITH_ID(TypeName, TypeName##TypeId)

#define MAKE_BASE_TYPE_ENTRY_WITH_ID(TypeName, TypeIdName, BaseType, FieldName)     \
    do                                                                              \
    {                                                                               \
        s_DebugBases[currentBasePos] = { #BaseType, TypeIdName, baseoffset(TypeName, BaseType, FieldName)}; \
        ++currentBasePos;                                                           \
        ASSERT(currentBasePos <= DebugBasesArraySize);                              \
    } while(0)                                                                      \

#define MAKE_BASE_TYPE_ENTRY(TypeName, BaseType, FieldName) MAKE_BASE_TYPE_ENTRY_WITH_ID(TypeName, TypeName##TypeId, BaseType, FieldName)

#define MAKE_FIELD_ENTRY_WITH_ID(TypeName, TypeIdName, FieldName)                   \
    do                                                                              \
    {                                                                               \
        s_DebugFields[currentFieldPos] = { #FieldName, TypeIdName, offsetof(TypeName, FieldName) }; \
        ++currentFieldPos;                                                          \
        ASSERT(currentFieldPos <= DebugFieldsArraySize);                            \
    } while(0)                                                                      \

#define MAKE_FIELD_ENTRY(TypeName, FieldName) MAKE_FIELD_ENTRY_WITH_ID(TypeName, TypeName##TypeId, FieldName)

#define MAKE_DEFINE_ENTRY(Name, Value)                                              \
    do                                                                              \
    {                                                                               \
        s_DebugDefines[currentDefinePos] = { #Name, Value };                        \
        ++currentDefinePos;                                                         \
        ASSERT(currentDefinePos <= DebugDefinesArraySize);                          \
    } while(0)                                                                      \

#define MAKE_TABLE(TableName, TableAddress, TableCount)                             \
    do                                                                              \
    {                                                                               \
        s_DebugTables[currentTablePos] = { TableName, TableAddress,  TableCount };  \
        ++currentTablePos;                                                          \
        ASSERT(currentTablePos <= DebugTablesArraySize);                            \
    } while(0)                                                                      \

extern "C" void PopulateClrDebugHeaders()
{
    uint32_t currentTablePos = 0;
    uint32_t currentGlobalPos = 0;
    uint32_t currentTypePos = 0;
    uint32_t currentBasePos = 0;
    uint32_t currentFieldPos = 0;
    uint32_t currentDefinePos = 0;

    MAKE_GLOBAL_ENTRY(ThreadStore::s_pThreadStore);
    MAKE_TYPE_ENTRY(ThreadStore);
    MAKE_FIELD_ENTRY(ThreadStore, m_ThreadList);

    MAKE_TYPE_ENTRY(Thread);
    MAKE_FIELD_ENTRY(Thread, m_Link);
    MAKE_FIELD_ENTRY(Thread, m_ThreadId);
    MAKE_FIELD_ENTRY(Thread, m_OSThreadId);
    MAKE_FIELD_ENTRY(Thread, m_LastThrownObjectHandle);
    MAKE_FIELD_ENTRY(Thread, m_alloc_context);

    MAKE_TYPE_ENTRY(MethodTable);
    MAKE_FIELD_ENTRY(MethodTable, m_dwFlags);
    MAKE_FIELD_ENTRY(MethodTable, m_wFlags2);
    MAKE_FIELD_ENTRY(MethodTable, m_BaseSize);
    MAKE_FIELD_ENTRY(MethodTable, m_wToken);
    MAKE_FIELD_ENTRY(MethodTable, m_wNumVirtuals);
    MAKE_FIELD_ENTRY(MethodTable, m_wNumInterfaces);
    MAKE_FIELD_ENTRY(MethodTable, m_pParentMethodTable);
    MAKE_FIELD_ENTRY(MethodTable, m_pLoaderModule);
    MAKE_FIELD_ENTRY(MethodTable, m_pWriteableData);
    MAKE_FIELD_ENTRY(MethodTable, m_pCanonMT);
    MAKE_FIELD_ENTRY(MethodTable, m_pEEClass);
    MAKE_FIELD_ENTRY(MethodTable, m_pPerInstInfo);
    MAKE_FIELD_ENTRY(MethodTable, m_ElementTypeHnd);
    MAKE_FIELD_ENTRY(MethodTable, m_pMultipurposeSlot1);
    MAKE_FIELD_ENTRY(MethodTable, m_pInterfaceMap);
    MAKE_FIELD_ENTRY(MethodTable, m_pMultipurposeSlot2);

    MAKE_TYPE_ENTRY(MethodDesc);
    MAKE_FIELD_ENTRY(MethodDesc, m_chunkIndex);
    MAKE_FIELD_ENTRY(MethodDesc, m_wFlags);
    MAKE_FIELD_ENTRY(MethodDesc, m_bFlags2);
    MAKE_FIELD_ENTRY(MethodDesc, m_wFlags3AndTokenRemainder);
    MAKE_FIELD_ENTRY(MethodDesc, m_wSlotNumber);

    MAKE_TYPE_ENTRY(FCallMethodDesc);
    MAKE_BASE_TYPE_ENTRY(FCallMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(NDirectMethodDesc);
    MAKE_BASE_TYPE_ENTRY(NDirectMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(EEImplMethodDesc);
    MAKE_BASE_TYPE_ENTRY(EEImplMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(ArrayMethodDesc);
    MAKE_BASE_TYPE_ENTRY(ArrayMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(InstantiatedMethodDesc);
    MAKE_BASE_TYPE_ENTRY(InstantiatedMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(DynamicMethodDesc);
    MAKE_BASE_TYPE_ENTRY(DynamicMethodDesc, MethodDesc, m_chunkIndex);

    MAKE_TYPE_ENTRY(MethodDescChunk);
    MAKE_FIELD_ENTRY(MethodDescChunk, m_methodTable);
    MAKE_FIELD_ENTRY(MethodDescChunk, m_next);
    MAKE_FIELD_ENTRY(MethodDescChunk, m_size);
    MAKE_FIELD_ENTRY(MethodDescChunk, m_count);
    MAKE_FIELD_ENTRY(MethodDescChunk, m_flagsAndTokenRange);

    MAKE_TYPE_ENTRY(MethodImpl);

    MAKE_TYPE_ENTRY(TypeDesc);
    MAKE_FIELD_ENTRY(TypeDesc, m_typeAndFlags);

    MAKE_TYPE_ENTRY(ParamTypeDesc);
    MAKE_BASE_TYPE_ENTRY(ParamTypeDesc, TypeDesc, m_typeAndFlags);
    MAKE_FIELD_ENTRY(ParamTypeDesc, m_Arg);

    MAKE_TYPE_ENTRY(GenericsDictInfo);
    MAKE_FIELD_ENTRY(GenericsDictInfo, m_wNumDicts);
    MAKE_FIELD_ENTRY(GenericsDictInfo, m_wNumTyPars);

    MAKE_TYPE_ENTRY(MethodTableWriteableData);
    MAKE_FIELD_ENTRY(MethodTableWriteableData, m_dwFlags);

    MAKE_TYPE_ENTRY(Module);
    MAKE_FIELD_ENTRY(Module, m_pAssembly);
    MAKE_FIELD_ENTRY(Module, m_pSimpleName);
    MAKE_FIELD_ENTRY(Module, m_pPEAssembly);
    MAKE_FIELD_ENTRY(Module, m_pReadyToRunInfo);
    MAKE_FIELD_ENTRY(Module, m_TypeDefToMethodTableMap);
    MAKE_FIELD_ENTRY(Module, m_MethodDefToDescMap);

    MAKE_TYPE_ENTRY(PEAssembly);
    MAKE_FIELD_ENTRY(PEAssembly, m_pMDImport);
    MAKE_FIELD_ENTRY(PEAssembly, m_PEImage);

    MAKE_TYPE_ENTRY(PEImage);
    MAKE_FIELD_ENTRY(PEImage, m_path);
    MAKE_FIELD_ENTRY(PEImage, m_pLayouts);
    MAKE_FIELD_ENTRY(PEImage, m_sModuleFileNameHintUsedByDac);

    MAKE_DEFINE_ENTRY(IMAGE_FLAT, PEImage::IMAGE_FLAT);
    MAKE_DEFINE_ENTRY(IMAGE_LOADED, PEImage::IMAGE_LOADED);
    MAKE_DEFINE_ENTRY(IMAGE_COUNT, PEImage::IMAGE_COUNT);

    MAKE_TYPE_ENTRY(PEDecoder);
    MAKE_FIELD_ENTRY(PEDecoder, m_base);
    MAKE_FIELD_ENTRY(PEDecoder, m_size);
    MAKE_FIELD_ENTRY(PEDecoder, m_flags);

    MAKE_TYPE_ENTRY(PEImageLayout);
    MAKE_BASE_TYPE_ENTRY(PEImageLayout, PEDecoder, m_base);

    MAKE_TYPE_ENTRY(EEClass);
    MAKE_FIELD_ENTRY(EEClass, m_fFieldsArePacked);
    MAKE_FIELD_ENTRY(EEClass, m_cbFixedEEClassFields);

    MAKE_TYPE_ENTRY(ArrayClass);
    MAKE_FIELD_ENTRY(ArrayClass, m_rank);
    MAKE_FIELD_ENTRY(ArrayClass, m_ElementType);

    MAKE_TYPE_ENTRY(LoaderHeap);

    MAKE_TYPE_ENTRY(LoaderAllocator);
    MAKE_FIELD_ENTRY(LoaderAllocator, m_pLowFrequencyHeap);
    MAKE_FIELD_ENTRY(LoaderAllocator, m_pHighFrequencyHeap);
    MAKE_FIELD_ENTRY(LoaderAllocator, m_pStubHeap);

    MAKE_TYPE_ENTRY(GlobalLoaderAllocator);
    MAKE_BASE_TYPE_ENTRY(GlobalLoaderAllocator, LoaderAllocator, m_pLowFrequencyHeap);

    MAKE_TYPE_ENTRY(LookupMapBase);
    MAKE_FIELD_ENTRY(LookupMapBase, pNext);
    MAKE_FIELD_ENTRY(LookupMapBase, pTable);
    MAKE_FIELD_ENTRY(LookupMapBase, dwCount);
    MAKE_FIELD_ENTRY(LookupMapBase, supportedFlags);

    MAKE_TYPE_ENTRY_WITH_ID(LookupMap<MethodDesc *>, LookupMap_MethodDesc_Ptr);
    MAKE_BASE_TYPE_ENTRY_WITH_ID(LookupMap<MethodDesc *>, LookupMap_MethodDesc_Ptr, LookupMapBase, pNext);

    MAKE_TYPE_ENTRY(Bucket);
    MAKE_FIELD_ENTRY(Bucket, m_rgKeys);
    MAKE_FIELD_ENTRY(Bucket, m_rgValues);

    MAKE_TYPE_ENTRY(HashMap);
    MAKE_FIELD_ENTRY(HashMap, m_rgBuckets);

    MAKE_TYPE_ENTRY(PtrHashMap);
    MAKE_FIELD_ENTRY(PtrHashMap, m_HashMap);

    MAKE_GLOBAL_ENTRY(AppDomain::m_pTheAppDomain);
    MAKE_TYPE_ENTRY(AppDomain);
    MAKE_FIELD_ENTRY(AppDomain, m_Assemblies);
    MAKE_FIELD_ENTRY(AppDomain, m_Stage.m_val);
    MAKE_FIELD_ENTRY(AppDomain, m_friendlyName);

    MAKE_GLOBAL_ENTRY(SystemDomain::m_pSystemDomain);
    MAKE_TYPE_ENTRY(SystemDomain);
    MAKE_FIELD_ENTRY(SystemDomain, m_pSystemAssembly);
    MAKE_FIELD_ENTRY(SystemDomain, m_GlobalAllocator);

    MAKE_TYPE_ENTRY_WITH_ID(AppDomain::DomainAssemblyList, AppDomain_DomainAssemblyList);
    MAKE_FIELD_ENTRY_WITH_ID(AppDomain::DomainAssemblyList, AppDomain_DomainAssemblyList, m_array);

    MAKE_TYPE_ENTRY(DomainAssembly);
    MAKE_FIELD_ENTRY(DomainAssembly, m_pAssembly);

    MAKE_TYPE_ENTRY(ArrayListBase);
    MAKE_FIELD_ENTRY(ArrayListBase, m_count);
    MAKE_FIELD_ENTRY(ArrayListBase, m_firstBlock);

    MAKE_TYPE_ENTRY(ArrayList);
    MAKE_BASE_TYPE_ENTRY(ArrayList, ArrayListBase, m_count);

    MAKE_TYPE_ENTRY(Dictionary);
    MAKE_FIELD_ENTRY(Dictionary, m_pEntries);

    MAKE_TYPE_ENTRY_WITH_ID(ArrayListBase::ArrayListBlock, ArrayListBase_ArrayListBlock);
    MAKE_FIELD_ENTRY_WITH_ID(ArrayListBase::ArrayListBlock, ArrayListBase_ArrayListBlock, m_next);
    MAKE_FIELD_ENTRY_WITH_ID(ArrayListBase::ArrayListBlock, ArrayListBase_ArrayListBlock, m_blockSize);
    MAKE_FIELD_ENTRY_WITH_ID(ArrayListBase::ArrayListBlock, ArrayListBase_ArrayListBlock, m_array);

    MAKE_TYPE_ENTRY(Assembly);
    MAKE_FIELD_ENTRY(Assembly, m_pPEAssembly);
    MAKE_FIELD_ENTRY(Assembly, m_pModule);
    MAKE_FIELD_ENTRY(Assembly, m_pClassLoader);

    MAKE_TYPE_ENTRY(ClassLoader);

    MAKE_TYPE_ENTRY(ReadyToRunInfo);
    MAKE_FIELD_ENTRY(ReadyToRunInfo, m_nRuntimeFunctions);
    MAKE_FIELD_ENTRY(ReadyToRunInfo, m_pRuntimeFunctions);
    MAKE_FIELD_ENTRY(ReadyToRunInfo, m_pCompositeInfo);
    MAKE_FIELD_ENTRY(ReadyToRunInfo, m_entryPointToMethodDescMap);

    MAKE_TYPE_ENTRY(RUNTIME_FUNCTION);
    MAKE_FIELD_ENTRY(RUNTIME_FUNCTION, BeginAddress);
    MAKE_FIELD_ENTRY(RUNTIME_FUNCTION, EndAddress);
    MAKE_FIELD_ENTRY(RUNTIME_FUNCTION, UnwindData);

    MAKE_GLOBAL_ENTRY(g_gcDacGlobals);
    MAKE_GLOBAL_ENTRY(g_pFreeObjectMethodTable);

    MAKE_TYPE_ENTRY(GcDacVars);
    MAKE_FIELD_ENTRY(GcDacVars, major_version_number);
    MAKE_FIELD_ENTRY(GcDacVars, minor_version_number);
    MAKE_FIELD_ENTRY(GcDacVars, generation_size);
    MAKE_FIELD_ENTRY(GcDacVars, total_generation_count);
    MAKE_FIELD_ENTRY(GcDacVars, built_with_svr);
    MAKE_FIELD_ENTRY(GcDacVars, finalize_queue);
    MAKE_FIELD_ENTRY(GcDacVars, generation_table);
    MAKE_FIELD_ENTRY(GcDacVars, ephemeral_heap_segment);
    MAKE_FIELD_ENTRY(GcDacVars, alloc_allocated);
    MAKE_FIELD_ENTRY(GcDacVars, n_heaps);
    MAKE_FIELD_ENTRY(GcDacVars, g_heaps);

    MAKE_TYPE_ENTRY(dac_gc_heap);
    MAKE_FIELD_ENTRY(dac_gc_heap, alloc_allocated);
    MAKE_FIELD_ENTRY(dac_gc_heap, ephemeral_heap_segment);
    MAKE_FIELD_ENTRY(dac_gc_heap, finalize_queue);
    MAKE_FIELD_ENTRY(dac_gc_heap, generation_table);

    MAKE_TYPE_ENTRY(gc_alloc_context);
    MAKE_FIELD_ENTRY(gc_alloc_context, alloc_ptr);
    MAKE_FIELD_ENTRY(gc_alloc_context, alloc_limit);
    MAKE_FIELD_ENTRY(gc_alloc_context, alloc_bytes);
    MAKE_FIELD_ENTRY(gc_alloc_context, alloc_bytes_uoh);
    MAKE_FIELD_ENTRY(gc_alloc_context, alloc_count);

    MAKE_TYPE_ENTRY(dac_generation);
    MAKE_FIELD_ENTRY(dac_generation, allocation_context);
    MAKE_FIELD_ENTRY(dac_generation, start_segment);
    MAKE_FIELD_ENTRY(dac_generation, allocation_start); 

    MAKE_TYPE_ENTRY(dac_heap_segment);
    MAKE_FIELD_ENTRY(dac_heap_segment, allocated);
    MAKE_FIELD_ENTRY(dac_heap_segment, committed);
    MAKE_FIELD_ENTRY(dac_heap_segment, reserved);
    MAKE_FIELD_ENTRY(dac_heap_segment, used);
    MAKE_FIELD_ENTRY(dac_heap_segment, mem);
    MAKE_FIELD_ENTRY(dac_heap_segment, flags);
    MAKE_FIELD_ENTRY(dac_heap_segment, next);
    MAKE_FIELD_ENTRY(dac_heap_segment, background_allocated);
    MAKE_FIELD_ENTRY(dac_heap_segment, heap);

    MAKE_TYPE_ENTRY(SBuffer);
    MAKE_FIELD_ENTRY(SBuffer, m_size);
    MAKE_FIELD_ENTRY(SBuffer, m_flags);
    MAKE_FIELD_ENTRY(SBuffer, m_buffer);

    MAKE_TYPE_ENTRY(SString);
    MAKE_BASE_TYPE_ENTRY(SString, SBuffer, m_size);

    MAKE_TYPE_ENTRY(HeapList);
    MAKE_FIELD_ENTRY(HeapList, hpNext);
    MAKE_FIELD_ENTRY(HeapList, startAddress);
    MAKE_FIELD_ENTRY(HeapList, endAddress);
    MAKE_FIELD_ENTRY(HeapList, mapBase);
    MAKE_FIELD_ENTRY(HeapList, pHdrMap);

    MAKE_TYPE_ENTRY(Object);
    MAKE_FIELD_ENTRY(Object, m_pMethTab);

    MAKE_TYPE_ENTRY(ExceptionObject);
    MAKE_BASE_TYPE_ENTRY(ExceptionObject, Object, m_pMethTab);
    MAKE_FIELD_ENTRY(ExceptionObject, _message);
    MAKE_FIELD_ENTRY(ExceptionObject, _innerException);
    MAKE_FIELD_ENTRY(ExceptionObject, _stackTrace);

    MAKE_TYPE_ENTRY(StringObject);
    MAKE_BASE_TYPE_ENTRY(StringObject, Object, m_pMethTab);
    MAKE_FIELD_ENTRY(StringObject, m_StringLength);

    MAKE_TYPE_ENTRY(ArrayBase);
    MAKE_BASE_TYPE_ENTRY(ArrayBase, Object, m_pMethTab);
    MAKE_FIELD_ENTRY(ArrayBase, m_NumComponents);

    MAKE_TYPE_ENTRY_WITH_ID(StackTraceArray::ArrayHeader, StackTraceArray_ArrayHeader);
    MAKE_FIELD_ENTRY_WITH_ID(StackTraceArray::ArrayHeader, StackTraceArray_ArrayHeader, m_size);
    MAKE_FIELD_ENTRY_WITH_ID(StackTraceArray::ArrayHeader, StackTraceArray_ArrayHeader, m_thread);

    MAKE_TYPE_ENTRY(StackTraceElement);
    MAKE_FIELD_ENTRY(StackTraceElement, ip);
    MAKE_FIELD_ENTRY(StackTraceElement, sp);
    MAKE_FIELD_ENTRY(StackTraceElement, pFunc);
    MAKE_FIELD_ENTRY(StackTraceElement, flags);

    MAKE_TYPE_ENTRY(_hpRealCodeHdr);
    MAKE_FIELD_ENTRY(_hpRealCodeHdr, phdrDebugInfo);
    MAKE_FIELD_ENTRY(_hpRealCodeHdr, phdrJitEHInfo);
    MAKE_FIELD_ENTRY(_hpRealCodeHdr, phdrJitGCInfo);
    MAKE_FIELD_ENTRY(_hpRealCodeHdr, phdrMDesc);
    MAKE_FIELD_ENTRY(_hpRealCodeHdr, unwindInfos);

    MAKE_GLOBAL_ENTRY(ExecutionManager::m_pEEJitManager);
    MAKE_TYPE_ENTRY(EEJitManager);

    MAKE_GLOBAL_ENTRY(ExecutionManager::g_codeRangeMap);
    MAKE_TYPE_ENTRY(RangeSectionMapData);
    MAKE_FIELD_ENTRY(RangeSectionMapData, Data);

    MAKE_GLOBAL_ENTRY(ExecutionManager::m_pReadyToRunJitManager);
    MAKE_TYPE_ENTRY(ReadyToRunJitManager);

    MAKE_DEFINE_ENTRY(MinObjectSize, MIN_OBJECT_SIZE);
#ifdef FEATURE_EH_FUNCLETS
    MAKE_DEFINE_ENTRY(FEATURE_EH_FUNCLETS, FEATURE_EH_FUNCLETS);
#endif
    MAKE_DEFINE_ENTRY(UNION_METHODTABLE, MethodTable::UNION_METHODTABLE);

    MAKE_TABLE("Global", &s_DebugGlobals, currentGlobalPos);
    MAKE_TABLE("Type", &s_DebugTypes, currentTypePos);
    MAKE_TABLE("Field", &s_DebugFields, currentFieldPos);
    MAKE_TABLE("Base", &s_DebugBases, currentBasePos);
    MAKE_TABLE("Define", &s_DebugDefines, currentDefinePos);

    ClrDebugHeader.DebugTableCount = currentTablePos;
}

