using System;
using System.Runtime.InteropServices;

namespace PiperSharp.Infer
{
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
    public struct ESpeakPhonemeConfigNative
    {
        [MarshalAs(UnmanagedType.LPStr)]
        public string voice;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CodepointsPhonemeConfigNative
    {
        // This structure is used as a placeholder if no specific members
        // are known to be required by the native function.
        // If the C++ function piper::CodepointsPhonemeConfig is an empty struct
        // or takes no arguments relevant to C#, this empty struct is appropriate.
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct PhonemeIdConfigNative
    {
        // PhonemeId is represented as long in C# (PhonemeId.Value)
        public long idPad;
        public long idBos;
        public long idEos;

        // Marshal bool as a 1-byte integer (C++ bool)
        [MarshalAs(UnmanagedType.I1)]
        public bool interspersePad;

        // The phonemeIdMap is handled by the C++ side or through
        // other means, not directly part of this struct for P/Invoke simplification.
        // If it were needed, it might be passed as a JSON string if the native
        // side supports it, or through more complex interop techniques.
    }

    public static class PiperPhonemizeNative
    {
        // The name of the native library.
        // .NET will handle platform-specific extensions like .dll, .so, or .dylib.
        private const string LibName = "piper_phonemize";

        /// <summary>
        /// Phonemizes text using eSpeak-ng.
        /// </summary>
        /// <param name="text">The input text to phonemize.</param>
        /// <param name="config">Configuration for eSpeak-ng phonemization.</param>
        /// <param name="phonemesJsonOutput">Output JSON string representing List&lt;List&lt;uint&gt;&gt; of phoneme codepoints.</param>
        /// <returns>An integer status code (0 for success, non-zero for errors).</returns>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        public static extern int phonemize_eSpeak(
            [MarshalAs(UnmanagedType.LPStr)] string text,
            ref ESpeakPhonemeConfigNative config,
            out IntPtr phonemesJsonOutput);

        /// <summary>
        /// Phonemizes text into codepoints.
        /// </summary>
        /// <param name="text">The input text to phonemize.</param>
        /// <param name="config">Configuration for codepoints phonemization.</param>
        /// <param name="phonemesJsonOutput">Output JSON string representing List&lt;List&lt;uint&gt;&gt; of phoneme codepoints.</param>
        /// <returns>An integer status code (0 for success, non-zero for errors).</returns>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        public static extern int phonemize_codepoints(
            [MarshalAs(UnmanagedType.LPStr)] string text,
            ref CodepointsPhonemeConfigNative config, // Assuming this might be an empty struct if not specified
            out IntPtr phonemesJsonOutput);

        /// <summary>
        /// Converts a sentence of phonemes (codepoints) to phoneme IDs.
        /// </summary>
        /// <param name="phonemesInSentenceJson">Input JSON string representing List&lt;uint&gt; for a single sentence's phonemes.</param>
        /// <param name="config">Configuration for phoneme ID mapping.</param>
        /// <param name="phonemeIdsJsonOutput">Output JSON string representing List&lt;long&gt; of phoneme IDs.</param>
        /// <param name="missingPhonemesJsonOutput">Output JSON string representing Dictionary&lt;uint, ulong&gt; of missing phonemes and their counts.</param>
        /// <returns>An integer status code (0 for success, non-zero for errors).</returns>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        public static extern int phonemes_to_ids(
            [MarshalAs(UnmanagedType.LPStr)] string phonemesInSentenceJson,
            ref PhonemeIdConfigNative config,
            out IntPtr phonemeIdsJsonOutput,
            out IntPtr missingPhonemesJsonOutput);

        /// <summary>
        /// Frees a string allocated by the piper_phonemize native library.
        /// This should be called for any IntPtr returned as an output string (e.g., JSON outputs).
        /// </summary>
        /// <param name="strPtr">Pointer to the C string to be freed.</param>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern void free_piper_phonemize_string(IntPtr strPtr);
    }
}
