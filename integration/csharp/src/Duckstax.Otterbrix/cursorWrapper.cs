namespace Duckstax.EntityFramework.Otterbrix
{
    using System;
    using System.Runtime.InteropServices;

    public class CursorWrapper {
        const string libotterbrix = "../../../libotterbrix.so";

        [StructLayout(LayoutKind.Sequential)]
        private struct TransferErrorMessage {
            public int type;
            public IntPtr what;
        }

        [DllImport(libotterbrix, EntryPoint="ReleaseCursor", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern void ReleaseCursor(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorSize", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern int CursorSize(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorHasNext", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern bool CursorHasNext(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorNext", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern IntPtr CursorNext(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorGet", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern IntPtr CursorGet(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorGetByIndex", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern IntPtr CursorGet(IntPtr ptr, int index);
        [DllImport(libotterbrix, EntryPoint="CursorIsSuccess", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern bool CursorIsSuccess(IntPtr ptr);
        [DllImport(libotterbrix, EntryPoint="CursorIsError", ExactSpelling=false, CallingConvention=CallingConvention.Cdecl)]
        private static extern bool CursorIsError(IntPtr ptr);
        [DllImport(libotterbrix,
                   EntryPoint = "CursorGetError",
                   ExactSpelling = false,
                   CallingConvention = CallingConvention.Cdecl)]
        private static extern TransferErrorMessage CursorGetError(IntPtr ptr);

        public CursorWrapper(IntPtr cursorStoragePtr) { this.cursorStoragePtr = cursorStoragePtr; }
        ~CursorWrapper() { ReleaseCursor(cursorStoragePtr); }
        public int Size() { return CursorSize(cursorStoragePtr); }
        public bool HasNext() { return CursorHasNext(cursorStoragePtr); }
        public DocumentWrapper Next() { return new DocumentWrapper(CursorNext(cursorStoragePtr)); }
        public DocumentWrapper Get() { return new DocumentWrapper(CursorGet(cursorStoragePtr)); }
        public DocumentWrapper Get(int index) { return new DocumentWrapper(CursorGet(cursorStoragePtr, index)); }
        public bool IsSuccess() { return CursorIsSuccess(cursorStoragePtr); }
        public bool IsError() { return CursorIsError(cursorStoragePtr); }
        public ErrorMessage GetError() {
            TransferErrorMessage transfer = CursorGetError(cursorStoragePtr);
            ErrorMessage message = new ErrorMessage();
            message.type = (ErrorCode) transfer.type;
            message.what = Marshal.PtrToStringAnsi(transfer.what);
            Marshal.FreeHGlobal(transfer.what);
            return message;
        }

        private readonly IntPtr cursorStoragePtr;
    }
}