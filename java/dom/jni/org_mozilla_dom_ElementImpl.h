/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_mozilla_dom_ElementImpl */

#ifndef _Included_org_mozilla_dom_ElementImpl
#define _Included_org_mozilla_dom_ElementImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_ElementImpl_finalize
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    getAttribute
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_dom_ElementImpl_getAttribute
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    getAttributeNode
 * Signature: (Ljava/lang/String;)Lorg/w3c/dom/Attr;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_ElementImpl_getAttributeNode
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    getElementsByTagName
 * Signature: (Ljava/lang/String;)Lorg/w3c/dom/NodeList;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_ElementImpl_getElementsByTagName
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    getTagName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_org_mozilla_dom_ElementImpl_getTagName
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    normalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_ElementImpl_normalize
  (JNIEnv *, jobject);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    removeAttribute
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_ElementImpl_removeAttribute
  (JNIEnv *, jobject, jstring);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    removeAttributeNode
 * Signature: (Lorg/w3c/dom/Attr;)Lorg/w3c/dom/Attr;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_ElementImpl_removeAttributeNode
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    setAttribute
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_mozilla_dom_ElementImpl_setAttribute
  (JNIEnv *, jobject, jstring, jstring);

/*
 * Class:     org_mozilla_dom_ElementImpl
 * Method:    setAttributeNode
 * Signature: (Lorg/w3c/dom/Attr;)Lorg/w3c/dom/Attr;
 */
JNIEXPORT jobject JNICALL Java_org_mozilla_dom_ElementImpl_setAttributeNode
  (JNIEnv *, jobject, jobject);

#ifdef __cplusplus
}
#endif
#endif
