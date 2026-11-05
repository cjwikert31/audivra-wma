#ifndef WMA_CORE_H
#define WMA_CORE_H
/* Decodes a Windows Media Audio (ASF) file to a 16-bit PCM WAV file.
 * Returns 0 on success, a negative number on failure. */
int wma_decode_to_wav(const char *in_path, const char *out_path);

/* Reads the length and tags of a WMA file. Writes UTF-8 text into "out":
 * length in milliseconds, then title, artist, album, track, year, genre and album artist,
 * separated by the character 0x1F. Returns the number of bytes written, or a negative number. */
int wma_probe(const char *in_path, char *out, int out_size);

#endif
