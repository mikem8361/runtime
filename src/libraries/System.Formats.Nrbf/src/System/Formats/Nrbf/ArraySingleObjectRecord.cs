﻿// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Collections.Generic;
using System.IO;
using System.Reflection.Metadata;
using System.Formats.Nrbf.Utils;
using System.Diagnostics;

namespace System.Formats.Nrbf;

/// <summary>
/// Represents a single-dimensional array of <see cref="object" />.
/// </summary>
/// <remarks>
/// ArraySingleObject records are described in <see href="https://learn.microsoft.com/openspecs/windows_protocols/ms-nrbf/982b2f50-6367-402a-aaf2-44ee96e2a5e0">[MS-NRBF] 2.4.3.2</see>.
/// </remarks>
internal sealed class ArraySingleObjectRecord : SZArrayRecord<SerializationRecord>
{
    internal ArraySingleObjectRecord(ArrayInfo arrayInfo) : base(arrayInfo) => Records = [];

    public override SerializationRecordType RecordType => SerializationRecordType.ArraySingleObject;

    public override TypeName TypeName
        => TypeNameHelpers.GetPrimitiveSZArrayTypeName(TypeNameHelpers.ObjectPrimitiveType);

    private List<SerializationRecord> Records { get; }

    /// <inheritdoc/>
    public override SerializationRecord?[] GetArray(bool allowNulls = true)
        => (SerializationRecord?[])(allowNulls ? _arrayNullsAllowed ??= ToArray(true) : _arrayNullsNotAllowed ??= ToArray(false));

    private SerializationRecord?[] ToArray(bool allowNulls)
    {
        SerializationRecord?[] values = new SerializationRecord?[Length];

        int valueIndex = 0;
        for (int recordIndex = 0; recordIndex < Records.Count; recordIndex++)
        {
            SerializationRecord record = Records[recordIndex];

            if (record is MemberReferenceRecord referenceRecord)
            {
                record = referenceRecord.GetReferencedRecord();
            }

            if (record is not NullsRecord nullsRecord)
            {
                values[valueIndex++] = record;
                continue;
            }

            if (!allowNulls)
            {
                ThrowHelper.ThrowArrayContainedNulls();
            }

            int nullCount = nullsRecord.NullCount;
            do
            {
                values[valueIndex++] = null;
                nullCount--;
            }
            while (nullCount > 0);
        }

        Debug.Assert(valueIndex == values.Length, "We should have traversed the entirety of the newly created array.");

        return values;
    }

    internal static ArraySingleObjectRecord Decode(BinaryReader reader)
        => new ArraySingleObjectRecord(ArrayInfo.Decode(reader));

    internal override (AllowedRecordTypes allowed, PrimitiveType primitiveType) GetAllowedRecordType()
    {
        // An array of objects can contain any Object or multiple nulls.
        const AllowedRecordTypes Allowed = AllowedRecordTypes.AnyObject | AllowedRecordTypes.Nulls;

        return (Allowed, default);
    }

    private protected override void AddValue(object value) => Records.Add((SerializationRecord)value);
}
