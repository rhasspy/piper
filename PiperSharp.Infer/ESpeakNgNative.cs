using System;
using System.Runtime.InteropServices;

namespace PiperSharp.Infer
{
    public static class ESpeakNgNative
    {
        private const string LibName = "libespeak-ng.dll"; // Or appropriate name based on target platform

        // From espeak_lib.h:
        // enum espeak_AUDIO_OUTPUT
        // Values are typically 0-indexed unless specified otherwise.
        // piper.cpp uses AUDIO_OUTPUT_SYNCHRONOUS.
        public enum espeak_AUDIO_OUTPUT : int
        {
            /// <summary>
            /// Play the audio, also return events
            /// </summary>
            AUDIO_OUTPUT_PLAYBACK = 0,

            /// <summary>
            /// Return the audio data in the event.buflen field, also return events
            /// </summary>
            AUDIO_OUTPUT_RETRIEVAL = 1,

            /// <summary>
            /// Return audio data + events, also play the audio
            /// </summary>
            AUDIO_OUTPUT_SYNCHRONOUS = 2,

            /// <summary>
            /// play the audio, also return events, but input is stopped while waiting for playback to catch up
            /// (equivalent to espeak_Synchronize() every 200mS)
            /// </summary>
            AUDIO_OUTPUT_SYNCH_PLAYBACK = 3
        }

        // Error codes from espeak_lib.h (not exhaustive, but EE_OK is crucial)
        // #define EE_OK           0       // operation was successful
        // #define EE_INTERNAL_ERROR       -1      // internal error.
        // #define EE_BUFFER_FULL          1       // The command could not be buffered.
        // #define EE_NOT_FOUND            2       // The command could not be found.

        /// <summary>
        /// Operation was successful.
        /// </summary>
        public const int EE_OK = 0;
        /// <summary>
        /// Internal error.
        /// </summary>
        public const int EE_INTERNAL_ERROR = -1;
        /// <summary>
        /// The command could not be buffered.
        /// </summary>
        public const int EE_BUFFER_FULL = 1;
        /// <summary>
        /// The command could not be found.
        /// </summary>
        public const int EE_NOT_FOUND = 2;


        /// <summary>
        /// Must be called before any other eSpeak function can be used.
        /// </summary>
        /// <param name="output">Specifies whether audio is played, or returned by callback, or both.</param>
        /// <param name="buflength">The length in mS of sound buffers passed to the SynthCallback function.
        /// Value of 0 gives a default of 200mS.</param>
        /// <param name="path">The directory which contains the espeak-data directory, or NULL for default.
        /// If path is NULL, then an attempt is made to use the environment variable ESPEAK_DATA_PATH.
        /// If this is not defined, then the data is searched for in the following places:
        ///   ./espeak-data
        ///   ../espeak-data
        ///   /usr/share/espeak-data        (or other prefix)
        /// The full path of the data directory is returned by espeak_Info()
        /// </param>
        /// <param name="options">Various options. Set to 0 for default.
        ///   ESPEAK_NO_EVENT_VALUES    Don't pass event values in espeak_EVENT type parameter of t_espeak_callback.
        ///   ESPEAK_PATH_APPDATA       Search for espeak_data in "%APPDATA%\espeak-data" instead of the program directory.
        ///                             This is for Windows and only applies if "path" is NULL.
        /// </param>
        /// <returns>Sample rate in Hz, or -1 (EE_INTERNAL_ERROR).</returns>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi, BestFitMapping = false, ThrowOnUnmappableChar = true)]
        public static extern int espeak_Initialize(espeak_AUDIO_OUTPUT output, int buflength, [MarshalAs(UnmanagedType.LPStr)] string? path, int options);

        /// <summary>
        /// This function should be called at the end of the program.
        /// It tidies up and frees memory.
        /// </summary>
        /// <returns>EE_OK on success.</returns>
        [DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
        public static extern int espeak_Terminate();

        // Other eSpeak P/Invoke methods will be added here as needed.
    }
}
