/* The bridge between Kotlin (class audivra.wma.WmaNative) and wma_core.c. */
#include <jni.h>
#include <stdio.h>

#include "wma_core.h"

JNIEXPORT jint JNICALL Java_audivra_wma_WmaNative_decodeToWav(JNIEnv *env, jclass cls, jint fd, jstring out) {
    const char *outPath = (*env)->GetStringUTFChars(env, out, NULL);
    if (!outPath) return -100;
    char inPath[64];
    snprintf(inPath, sizeof inPath, "/proc/self/fd/%d", (int)fd);
    int r = wma_decode_to_wav(inPath, outPath);
    (*env)->ReleaseStringUTFChars(env, out, outPath);
    return r;
}

/* Returns the length and tags of the WMA file open at "fd" as UTF-8 bytes (or null). */
JNIEXPORT jbyteArray JNICALL Java_audivra_wma_WmaNative_probe(JNIEnv *env, jclass cls, jint fd) {
    char inPath[64];
    snprintf(inPath, sizeof inPath, "/proc/self/fd/%d", (int)fd);
    char buf[8192];
    int n = wma_probe(inPath, buf, (int)sizeof buf);
    if (n < 0) return NULL;
    jbyteArray arr = (*env)->NewByteArray(env, n);
    if (!arr) return NULL;
    (*env)->SetByteArrayRegion(env, arr, 0, n, (const jbyte *)buf);
    return arr;
}
