// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace System.IO.Compression
{
    public partial class ZipArchiveEntry
    {
        internal const ZipVersionMadeByPlatform CurrentZipPlatform = ZipVersionMadeByPlatform.Unix;

        /// <summary>
        /// To get the file name of a ZipArchiveEntry, we should be parsing the FullName based
        /// on the path specifications and requirements of the OS that ZipArchive was created on.
        /// This method takes in a FullName and the platform of the ZipArchiveEntry and returns
        /// the platform-correct file name.
        /// </summary>
        /// <remarks>This method ensures no validation on the paths. Invalid characters are allowed.</remarks>
        internal static string ParseFileName(string path, ZipVersionMadeByPlatform madeByPlatform) =>
            madeByPlatform == ZipVersionMadeByPlatform.Windows ? GetFileName_Windows(path) : GetFileName_Unix(path);
    }
}
