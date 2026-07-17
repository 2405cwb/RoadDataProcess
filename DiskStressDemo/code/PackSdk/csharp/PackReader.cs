using System;
using System.Runtime.InteropServices;

namespace PackSdk
{
    public sealed class PackReader : IDisposable
    {
        private const string DllName = "PackSdk.dll";
        private IntPtr _handle;

        /// <summary>
        /// 打开一个图片包目录，目录下应包含 Pack_*.idx / Pack_*.dat 文件。
        /// verifyOnOpen=true 时会在打开后立即执行一次完整校验，数据量大时会更慢。
        /// </summary>
        public PackReader(string rootDir, bool verifyOnOpen = false)
        {
            var options = new PackOpenOptions
            {
                structSize = (uint)Marshal.SizeOf(typeof(PackOpenOptions)),
                flags = verifyOnOpen ? 1u : 0u
            };

            int ret = Native.Pack_Open(rootDir, ref options, out _handle);
            if (ret != 0)
            {
                throw new PackSdkException(ret, GetLastError(IntPtr.Zero));
            }
        }

        /// <summary>
        /// 当前数据集可读取图片总数，等价于旧单图模式里的图片文件数量。
        /// </summary>
        public ulong Count
        {
            get
            {
                EnsureOpen();
                ulong count;
                Check(Native.Pack_GetFrameCount(_handle, out count));
                return count;
            }
        }

        /// <summary>
        /// 获取数据集摘要，包括 pack 数量、总帧数、时间范围和警告数量。
        /// </summary>
        public PackDatasetInfo GetDatasetInfo()
        {
            EnsureOpen();
            PackDatasetInfo info;
            Check(Native.Pack_GetDatasetInfo(_handle, out info));
            return info;
        }

        /// <summary>
        /// 获取某张图片的索引信息，不读取 JPEG 数据。
        /// </summary>
        public PackFrameInfo GetFrameInfo(ulong globalIndex)
        {
            EnsureOpen();
            PackFrameInfo info;
            Check(Native.Pack_GetFrameInfo(_handle, globalIndex, out info));
            return info;
        }

        /// <summary>
        /// 按采集端 sourceIndex 查找全局序号。
        /// </summary>
        public ulong FindBySourceIndex(ulong sourceIndex)
        {
            EnsureOpen();
            ulong globalIndex;
            Check(Native.Pack_FindBySourceIndex(_handle, sourceIndex, out globalIndex));
            return globalIndex;
        }

        /// <summary>
        /// 读取某张图片的 JPEG 原始字节。调用方可继续用 System.Drawing、OpenCV、Qt 等库解码。
        /// </summary>
        public byte[] ReadJpegBytes(ulong globalIndex)
        {
            EnsureOpen();
            uint required;
            Check(Native.Pack_ReadJpeg(_handle, globalIndex, IntPtr.Zero, 0, out required));
            byte[] data = new byte[required];
            GCHandle pinned = GCHandle.Alloc(data, GCHandleType.Pinned);
            try
            {
                Check(Native.Pack_ReadJpeg(_handle, globalIndex, pinned.AddrOfPinnedObject(), required, out required));
            }
            finally
            {
                pinned.Free();
            }
            return data;
        }

        /// <summary>
        /// 把某张图片保存成单独 jpg 文件，适合兼容仍然依赖文件路径的旧代码。
        /// </summary>
        public void SaveJpeg(ulong globalIndex, string outputPath)
        {
            EnsureOpen();
            Check(Native.Pack_SaveJpeg(_handle, globalIndex, outputPath));
        }

        /// <summary>
        /// 批量导出全部图片。默认导出成旧 Image_0000/000_time.jpg 目录结构。
        /// </summary>
        public void ExportAll(string outputDir, PackNamingMode namingMode = PackNamingMode.LegacyDirs)
        {
            EnsureOpen();
            Check(Native.Pack_ExportAll(_handle, outputDir, (uint)namingMode, IntPtr.Zero, IntPtr.Zero));
        }

        /// <summary>
        /// 校验 idx/dat 一致性，返回坏帧数、缺失 dat 数等摘要。
        /// </summary>
        public PackVerifyReport Verify()
        {
            EnsureOpen();
            PackVerifyReport report;
            Check(Native.Pack_Verify(_handle, out report));
            return report;
        }

        /// <summary>
        /// 关闭原生 SDK handle。Dispose 后不能再调用读取方法。
        /// </summary>
        public void Dispose()
        {
            if (_handle != IntPtr.Zero)
            {
                Native.Pack_Close(_handle);
                _handle = IntPtr.Zero;
            }
            GC.SuppressFinalize(this);
        }

        ~PackReader()
        {
            Dispose();
        }

        /// <summary>
        /// 确认当前对象仍持有有效原生句柄。
        /// </summary>
        private void EnsureOpen()
        {
            if (_handle == IntPtr.Zero)
            {
                throw new ObjectDisposedException("PackReader");
            }
        }

        /// <summary>
        /// 把 SDK 返回码转换成 C# 异常，异常信息来自 Pack_GetLastError。
        /// </summary>
        private void Check(int result)
        {
            if (result != 0)
            {
                throw new PackSdkException(result, GetLastError(_handle));
            }
        }

        /// <summary>
        /// 读取原生 SDK 最近一次错误文本。
        /// </summary>
        private static string GetLastError(IntPtr handle)
        {
            IntPtr p = Native.Pack_GetLastError(handle);
            return p == IntPtr.Zero ? string.Empty : Marshal.PtrToStringUni(p) ?? string.Empty;
        }

        private static class Native
        {
            [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            internal static extern int Pack_Open(string rootDir, ref PackOpenOptions options, out IntPtr handle);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern void Pack_Close(IntPtr handle);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_GetDatasetInfo(IntPtr handle, out PackDatasetInfo info);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_GetFrameCount(IntPtr handle, out ulong count);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_GetFrameInfo(IntPtr handle, ulong globalIndex, out PackFrameInfo frameInfo);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_FindBySourceIndex(IntPtr handle, ulong sourceIndex, out ulong globalIndex);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_ReadJpeg(IntPtr handle, ulong globalIndex, IntPtr buffer, uint bufferSize, out uint requiredSize);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            internal static extern int Pack_SaveJpeg(IntPtr handle, ulong globalIndex, string outputPath);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
            internal static extern int Pack_ExportAll(IntPtr handle, string outputDir, uint namingMode, IntPtr callback, IntPtr userData);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern int Pack_Verify(IntPtr handle, out PackVerifyReport report);

            [DllImport(DllName, CallingConvention = CallingConvention.StdCall)]
            internal static extern IntPtr Pack_GetLastError(IntPtr handle);
        }
    }

    public enum PackNamingMode : uint
    {
        LegacyDirs = 0,
        FlatSourceIndex = 1
    }

    public sealed class PackSdkException : Exception
    {
        public int ErrorCode { get; private set; }

        public PackSdkException(int errorCode, string message)
            : base(string.IsNullOrEmpty(message) ? string.Format("PackSdk failed: {0}", errorCode) : message)
        {
            ErrorCode = errorCode;
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct PackOpenOptions
    {
        public uint structSize;
        public uint flags;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PackDatasetInfo
    {
        public uint structSize;
        public uint formatVersion;
        public uint packCount;
        public uint hasWarning;
        public ulong frameCount;
        public ulong firstTimeValue;
        public ulong lastTimeValue;
        public ulong warningCount;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PackFrameInfo
    {
        public uint structSize;
        public uint packNo;
        public uint packFrameIndex;
        public uint statusFlags;
        public ulong globalIndex;
        public ulong sourceIndex;
        public ulong timeValue;
        public ulong datOffset;
        public uint jpgSize;
        public int width;
        public int height;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct PackVerifyReport
    {
        public uint structSize;
        public uint packCount;
        public uint missingDatCount;
        public uint badIdxCount;
        public ulong checkedFrameCount;
        public ulong badFrameCount;
        public ulong warningCount;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 512)]
        public string summary;
    }
}
